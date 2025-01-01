/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include <QCoreApplication>
#include <QFile>
#include <QMutex>
#include <QQmlContext>
#include <QTemporaryDir>
#include <QThread>
#include <QVector>
#include <QWaitCondition>
#include <akaudiocaps.h>
#include <akcompressedaudiocaps.h>
#include <akcompressedaudiopacket.h>
#include <akcompressedvideocaps.h>
#include <akcompressedvideopacket.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <iak/akelement.h>
#include <mp4v2/mp4v2.h>

#include "videomuxermp4v2element.h"

struct AudioCodecsTable
{
    AkCompressedAudioCaps::AudioCodecID codecID;
    int mp4CodecID;

    inline static const AudioCodecsTable *table()
    {
        static const AudioCodecsTable mp4v2AudioCodecsTable[] {
            {AkCompressedAudioCaps::AudioCodecID_aac    , MP4_MPEG4_AUDIO_TYPE},
            {AkCompressedAudioCaps::AudioCodecID_mp3    , MP4_MPEG2_AUDIO_TYPE},
            {AkCompressedAudioCaps::AudioCodecID_unknown, 0                   },
        };

        return mp4v2AudioCodecsTable;
    }

    inline static const AudioCodecsTable *byCodecID(AkCompressedAudioCaps::AudioCodecID codecID)
    {
        auto item = table();

        for (; item->codecID; ++item)
            if (item->codecID == codecID)
                return item;

        return item;
    }

    inline static QList<AkCodecID> codecs()
    {
        QList<AkCodecID> codecs;
        auto item = table();

        for (; item->codecID; ++item)
            codecs << item->codecID;

        return codecs;
    }
};

struct VideoCodecsTable
{
    AkCompressedVideoCaps::VideoCodecID codecID;
    int mp4CodecID;

    inline static const VideoCodecsTable *table()
    {
        static const VideoCodecsTable mp4v2VideoCodecsTable[] {
            {AkCompressedVideoCaps::VideoCodecID_h264   , MP4_PRIVATE_VIDEO_TYPE},
            {AkCompressedVideoCaps::VideoCodecID_unknown, 0                     },
        };

        return mp4v2VideoCodecsTable;
    }

    inline static const VideoCodecsTable *byCodecID(AkCompressedVideoCaps::VideoCodecID codecID)
    {
        auto item = table();

        for (; item->codecID; ++item)
            if (item->codecID == codecID)
                return item;

        return item;
    }

    inline static QList<AkCodecID> codecs()
    {
        QList<AkCodecID> codecs;
        auto item = table();

        for (; item->codecID; ++item)
            codecs << item->codecID;

        return codecs;
    }
};

class VideoMuxerMp4V2ElementPrivate
{
    public:
        VideoMuxerMp4V2Element *self;
        bool m_optimize {false};
        MP4FileHandle m_file {nullptr};
        MP4TrackId m_audioTrack {MP4_INVALID_TRACK_ID};
        MP4TrackId m_videoTrack {MP4_INVALID_TRACK_ID};
        uint32_t m_globalTimeScale {90000};
        bool m_initialized {false};
        bool m_paused {false};
        AkElementPtr m_packetSync {akPluginManager->create<AkElement>("Utils/PacketSync")};

        explicit VideoMuxerMp4V2ElementPrivate(VideoMuxerMp4V2Element *self);
        ~VideoMuxerMp4V2ElementPrivate();
        bool init();
        void uninit();
        MP4TrackId addH264Track(MP4FileHandle file,
                                const AkCompressedVideoCaps &videoCaps,
                                QByteArray &privateData) const;
        void packetReady(const AkPacket &packet);
};

VideoMuxerMp4V2Element::VideoMuxerMp4V2Element():
    AkVideoMuxer()
{
    this->d = new VideoMuxerMp4V2ElementPrivate(this);
    this->setMuxer(this->muxers().value(0));
}

VideoMuxerMp4V2Element::~VideoMuxerMp4V2Element()
{
    this->d->uninit();
    delete this->d;
}

QStringList VideoMuxerMp4V2Element::muxers() const
{
    return {"mp4"};
}

AkVideoMuxer::FormatID VideoMuxerMp4V2Element::formatID(const QString &muxer) const
{
    Q_UNUSED(muxer)

    return FormatID_mp4;
}

QString VideoMuxerMp4V2Element::description(const QString &muxer) const
{
    Q_UNUSED(muxer)

    return {"MP4 (libmp4v2)"};
}

QString VideoMuxerMp4V2Element::extension(const QString &muxer) const
{
    Q_UNUSED(muxer)

    return {"mp4"};
}

bool VideoMuxerMp4V2Element::gapsAllowed(AkCodecType type) const
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return true;

    case AkCompressedCaps::CapsType_Video:
        return false;

    default:
        break;
    }

    return true;
}

QList<AkCodecID> VideoMuxerMp4V2Element::supportedCodecs(const QString &muxer,
                                                         AkCodecType type) const
{
    Q_UNUSED(muxer)

    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return AudioCodecsTable::codecs();

    case AkCompressedCaps::CapsType_Video:
        return VideoCodecsTable::codecs();

    case AkCompressedCaps::CapsType_Unknown:
        return AudioCodecsTable::codecs() + VideoCodecsTable::codecs();

    default:
        break;
    }

    return {};
}

AkCodecID VideoMuxerMp4V2Element::defaultCodec(const QString &muxer,
                                               AkCodecType type) const
{
    auto codecs = this->supportedCodecs(muxer, type);

    if (codecs.isEmpty())
        return 0;

    return codecs.first();
}

bool VideoMuxerMp4V2Element::optimize() const
{
    return this->d->m_optimize;
}

QString VideoMuxerMp4V2Element::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoMuxerMp4V2/share/qml/main.qml");
}

void VideoMuxerMp4V2Element::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoMuxerMp4V2", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void VideoMuxerMp4V2Element::setOptimize(bool optimize)
{
    if (optimize == this->d->m_optimize)
        return;

    this->d->m_optimize = optimize;
    emit this->optimizeChanged(optimize);
}

void VideoMuxerMp4V2Element::resetOptimize()
{
    this->setOptimize(false);
}

void VideoMuxerMp4V2Element::resetOptions()
{
    AkVideoMuxer::resetOptions();
    this->resetOptimize();
}

AkPacket VideoMuxerMp4V2Element::iStream(const AkPacket &packet)
{
    if (this->d->m_paused || !this->d->m_initialized || !this->d->m_packetSync)
        return {};

    return this->d->m_packetSync->iStream(packet);
}

bool VideoMuxerMp4V2Element::setState(ElementState state)
{
    auto curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            this->d->m_paused = state == AkElement::ElementStatePaused;
        case AkElement::ElementStatePlaying:
            if (!this->d->init()) {
                this->d->m_paused = false;

                return false;
            }

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->d->m_paused = false;

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            this->d->m_paused = true;

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    }

    return false;
}

VideoMuxerMp4V2ElementPrivate::VideoMuxerMp4V2ElementPrivate(VideoMuxerMp4V2Element *self):
    self(self)
{
    if (this->m_packetSync)
        QObject::connect(this->m_packetSync.data(),
                         &AkElement::oStream,
                         [this] (const AkPacket &packet) {
                             this->packetReady(packet);
                         });
}

VideoMuxerMp4V2ElementPrivate::~VideoMuxerMp4V2ElementPrivate()
{

}

bool VideoMuxerMp4V2ElementPrivate::init()
{
    this->uninit();

    if (!this->m_packetSync)
        return false;

    AkCompressedVideoCaps videoCaps =
            self->streamCaps(AkCompressedCaps::CapsType_Video);

    if (!videoCaps) {
        qCritical() << "No valid video format set";

        return false;
    }

    auto vcodec = VideoCodecsTable::byCodecID(videoCaps.codec());

    if (!vcodec->codecID) {
        qCritical() << "Video codec not supported by this muxer:" << videoCaps.codec();

        return false;
    }

    AkCompressedAudioCaps audioCaps =
            self->streamCaps(AkCompressedCaps::CapsType_Audio);

    const AudioCodecsTable *acodec = nullptr;

    if (audioCaps) {
        acodec = AudioCodecsTable::byCodecID(audioCaps.codec());

        if (!acodec->codecID) {
            qCritical() << "Audio codec not supported by this muxer:" << audioCaps.codec();

            return false;
        }
    }

    // Create the file

    auto location = self->location();
    this->m_file = MP4Create(location.toStdString().c_str(), 0);

    if (this->m_file == MP4_INVALID_FILE_HANDLE) {
        qCritical() << "Failed to create the file";

        return false;
    }

    if (!MP4SetTimeScale(this->m_file, this->m_globalTimeScale)) {
        qCritical() << "Failed to set the time scalr";

        return false;
    }

    MP4SetVideoProfileLevel(this->m_file, MPEG4_MP_L2);

    // Read private headers

    QByteArray privateData;

    for (auto &header: self->streamHeaders(AkCompressedCaps::CapsType_Video))
        privateData += QByteArray(header.constData(), header.size());

    // Add the video track

    qInfo() << "Adding video track with format:" << videoCaps;
    this->m_videoTrack = MP4_INVALID_TRACK_ID;

    if (vcodec->codecID == AkCompressedVideoCaps::VideoCodecID_h264) {
        this->m_videoTrack =
            this->addH264Track(this->m_file, videoCaps, privateData);

        if (this->m_videoTrack == MP4_INVALID_TRACK_ID) {
            qCritical() << "Failed to add H264 track";

            return false;
        }
    } else {
        this->m_videoTrack =
                MP4AddVideoTrack(this->m_file,
                                 this->m_globalTimeScale,
                                 MP4_INVALID_DURATION,
                                 videoCaps.rawCaps().width(),
                                 videoCaps.rawCaps().height(),
                                 VideoCodecsTable::byCodecID(vcodec->codecID)->mp4CodecID);

        if (this->m_videoTrack == MP4_INVALID_TRACK_ID) {
            qCritical() << "Error adding the video track";

            return false;
        }

        if (!privateData.isEmpty()) {
            if (!MP4SetTrackESConfiguration(this->m_file,
                                            this->m_videoTrack,
                                            reinterpret_cast<const uint8_t *>(privateData.constData()),
                                            privateData.size())) {
                qCritical() << "Failed setting the video extra data";

                return false;
            }
        }
    }

    // Add the audio track

    this->m_audioTrack = MP4_INVALID_TRACK_ID;

    if (audioCaps) {
        qInfo() << "Adding audio track with format:" << audioCaps;
        this->m_audioTrack =
            MP4AddAudioTrack(this->m_file,
                             audioCaps.rawCaps().rate(),
                             MP4_INVALID_DURATION,
                             AudioCodecsTable::byCodecID(acodec->codecID)->mp4CodecID);

        if (this->m_audioTrack == MP4_INVALID_TRACK_ID) {
            qCritical() << "Error adding the audio track";

            return false;
        }

        privateData.clear();

        for (auto &header: self->streamHeaders(AkCompressedCaps::CapsType_Audio))
            privateData += QByteArray(header.constData(), header.size());

        if (!privateData.isEmpty()) {
            if (!MP4SetTrackESConfiguration(this->m_file,
                                            this->m_audioTrack,
                                            reinterpret_cast<const uint8_t *>(privateData.constData()),
                                            privateData.size())) {
                qCritical() << "Failed setting the audio extra data";

                return false;
            }
        }
    }

    auto tags = MP4TagsAlloc();
    MP4TagsFetch(tags, this->m_file);
    MP4TagsSetEncodingTool(tags, qApp->applicationName().toStdString().c_str());
    MP4TagsStore(tags, this->m_file);
    MP4TagsFree(tags);

    this->m_packetSync->setProperty("audioEnabled",
                                    this->m_audioTrack != MP4_INVALID_TRACK_ID);
    this->m_packetSync->setProperty("discardLast", false);
    this->m_packetSync->setState(AkElement::ElementStatePlaying);

    qInfo() << "Starting MP4 muxing";
    this->m_initialized = true;

    return true;
}

void VideoMuxerMp4V2ElementPrivate::uninit()
{
    if (!this->m_initialized)
        return;

    this->m_initialized = false;
    this->m_packetSync->setState(AkElement::ElementStateNull);

    for (uint32_t i = 0; i < MP4GetNumberOfTracks(this->m_file); ++i) {
        auto id = MP4FindTrackId(this->m_file, i);
        auto duration = MP4GetTrackDuration(this->m_file, id);
        MP4AddTrackEdit(this->m_file,
                        id,
                        MP4_INVALID_EDIT_ID,
                        0,
                        duration,
                        false);
    }

    MP4Close(this->m_file);

    if (this->m_optimize) {
        QTemporaryDir tempDir;

        if (tempDir.isValid()) {
            QFileInfo fileInfo(self->location());
            auto tmp = tempDir.filePath(fileInfo.baseName()
                                        + "_tmp."
                                        + fileInfo.completeSuffix());
            QFile::remove(tmp);

            if (MP4Optimize(fileInfo.filePath().toStdString().c_str(),
                             tmp.toStdString().c_str())) {
                QFile::remove(self->location());
                QFile::rename(tmp, self->location());
            } else {
                qCritical() << "Filename is invalid or error while opening";
                QFile::remove(tmp);
            }
        } else {
            qCritical() << "Can't create the temporary directory";
        }
    }

    this->m_paused = false;
}

MP4TrackId VideoMuxerMp4V2ElementPrivate::addH264Track(MP4FileHandle file,
                                                       const AkCompressedVideoCaps &videoCaps,
                                                       QByteArray &privateData) const
{
    if (privateData.isEmpty())
        return MP4_INVALID_TRACK_ID;

    QDataStream ds(&privateData, QIODeviceBase::ReadOnly);
    quint64 nHeaders = 0;
    ds >> nHeaders;

    if (nHeaders < 2)
        return MP4_INVALID_TRACK_ID;

    QVector<QByteArray> headers;

    for (quint64 i = 0; i < nHeaders; ++i) {
        quint64 payloadSize = 0;
        ds >> payloadSize;
        QByteArray header(payloadSize, Qt::Uninitialized);
        ds.readRawData(header.data(), header.size());
        headers << header;
    }

    static const int h264NaluLengthSize = 4;

    auto sps = reinterpret_cast<const uint8_t *>(headers[0].constData())
               + h264NaluLengthSize;
    auto pps = reinterpret_cast<const uint8_t *>(headers[1].constData())
               + h264NaluLengthSize;

    qsizetype spsSize = headers[0].size() - h264NaluLengthSize;
    qsizetype ppsSize = headers[1].size() - h264NaluLengthSize;

    auto videoTrack =
        MP4AddH264VideoTrack(file,
                             this->m_globalTimeScale,
                             MP4_INVALID_DURATION,
                             videoCaps.rawCaps().width(),
                             videoCaps.rawCaps().height(),
                             static_cast<uint8_t>(sps[1]),
                             static_cast<uint8_t>(sps[2]),
                             static_cast<uint8_t>(sps[3]),
                             h264NaluLengthSize - 1);

    if (videoTrack == MP4_INVALID_TRACK_ID)
        return MP4_INVALID_TRACK_ID;

    MP4AddH264SequenceParameterSet(file,
                                   videoTrack,
                                   sps,
                                   uint16_t(spsSize));
    MP4AddH264PictureParameterSet(file,
                                  videoTrack,
                                  pps,
                                  uint16_t(ppsSize));

    return videoTrack;
}

void VideoMuxerMp4V2ElementPrivate::packetReady(const AkPacket &packet)
{
    bool isAudio = packet.type() == AkPacket::PacketAudio
                   || packet.type() == AkPacket::PacketAudioCompressed;
    MP4TrackId track = isAudio?
                           this->m_audioTrack:
                           this->m_videoTrack;
    bool isSyncSample = true;

    if (packet.type() == AkPacket::PacketVideoCompressed)
        isSyncSample =
                AkCompressedVideoPacket(packet).flags()
                & AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame;

    auto timeScale = MP4GetTrackTimeScale(this->m_file, track);

    if (!MP4WriteSample(this->m_file,
                        track,
                        reinterpret_cast<const uint8_t *>(packet.constData()),
                                                          packet.size(),
                        qRound64(packet.duration()
                                 * packet.timeBase().value()
                                 * timeScale),
                        0,
                        isSyncSample)) {
        if (isAudio)
            qCritical() << "Failed to write the audio packet";
        else
            qCritical() << "Failed to write the video packet";
    }
}

#include "moc_videomuxermp4v2element.cpp"

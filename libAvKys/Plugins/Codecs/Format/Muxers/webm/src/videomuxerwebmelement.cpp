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
#include <QWaitCondition>
#include <akpacket.h>
#include <akcompressedaudiocaps.h>
#include <akcompressedvideocaps.h>
#include <akcompressedaudiopacket.h>
#include <akcompressedvideopacket.h>
#include <akfrac.h>
#include <mkvparser/mkvreader.h>
#include <mkvmuxer/mkvmuxer.h>
#include <mkvmuxer/mkvmuxertypes.h>
#include <mkvmuxer/mkvwriter.h>

#include "videomuxerwebmelement.h"

struct AudioCodecsTable
{
    AkCompressedAudioCaps::AudioCodecID codecID;
    const char *str;

    inline static const AudioCodecsTable *table()
    {
        static const AudioCodecsTable audioCodecsTable[] {
            {AkCompressedAudioCaps::AudioCodecID_vorbis , mkvmuxer::Tracks::kVorbisCodecId},
            {AkCompressedAudioCaps::AudioCodecID_opus   , mkvmuxer::Tracks::kOpusCodecId  },
            {AkCompressedAudioCaps::AudioCodecID_unknown, ""                              },
        };

        return audioCodecsTable;
    }

    inline static const AudioCodecsTable *byCodecID(AkCompressedAudioCaps::AudioCodecID codecID)
    {
        auto item = table();

        for (; item->codecID; ++item)
            if (item->codecID == codecID)
                return item;

        return item;
    }
};

struct VideoCodecsTable
{
    AkCompressedVideoCaps::VideoCodecID codecID;
    const char *str;

    inline static const VideoCodecsTable *table()
    {
        static const VideoCodecsTable videoCodecsTable[] {
            {AkCompressedVideoCaps::VideoCodecID_vp8    , mkvmuxer::Tracks::kVp8CodecId},
            {AkCompressedVideoCaps::VideoCodecID_vp9    , mkvmuxer::Tracks::kVp9CodecId},
            {AkCompressedVideoCaps::VideoCodecID_av1    , mkvmuxer::Tracks::kAv1CodecId},
            {AkCompressedVideoCaps::VideoCodecID_unknown, ""                           },
        };

        return videoCodecsTable;
    }

    inline static const VideoCodecsTable *byCodecID(AkCompressedVideoCaps::VideoCodecID codecID)
    {
        auto item = table();

        for (; item->codecID; ++item)
            if (item->codecID == codecID)
                return item;

        return item;
    }
};

class VideoMuxerWebmElementPrivate
{
    public:
        VideoMuxerWebmElement *self;
        mkvmuxer::MkvWriter m_writer;
        mkvmuxer::Segment m_muxerSegment;
        uint64_t m_audioTrackIndex {0};
        uint64_t m_videoTrackIndex {0};
        bool m_accurateClusterDuration {false};
        bool m_fixedSizeClusterTimecode {false};
        bool m_liveMode {false};
        bool m_outputCues {true};
        uint64_t m_maxClusterSize {0};
        bool m_outputCuesBlockNumber {true};
        bool m_cuesBeforeClusters {false};
        uint64_t m_maxClusterDuration {0};
        uint64_t m_timeCodeScale {100000};
        qreal m_clock {0.0};
        bool m_isFirstAudioPackage {true};
        qreal m_audioPts {0.0};
        qreal m_lastAudioDuration {0.0};
        qreal m_audioDiff {0.0};
        bool m_isFirstVideoPackage {true};
        qreal m_videoPts {0.0};
        qreal m_lastVideoDuration {0.0};
        qreal m_videoDiff {0.0};
        QMutex m_mutex;
        QWaitCondition m_audioReady;
        QWaitCondition m_videoReady;
        bool m_initialized {false};
        explicit VideoMuxerWebmElementPrivate(VideoMuxerWebmElement *self);
        ~VideoMuxerWebmElementPrivate();
        bool init();
        void uninit();
};

VideoMuxerWebmElement::VideoMuxerWebmElement():
    AkVideoMuxer()
{
    this->d = new VideoMuxerWebmElementPrivate(this);
}

VideoMuxerWebmElement::~VideoMuxerWebmElement()
{
    this->d->uninit();
    delete this->d;
}

AkVideoMuxer::FormatID VideoMuxerWebmElement::formatID() const
{
    return FormatID_webm;
}

QString VideoMuxerWebmElement::extension() const
{
    return {"webm"};
}

QList<AkCodecID> VideoMuxerWebmElement::supportedCodecs(AkCodecType type) const
{
    static const QList<AkCodecID> audioCodecs {
        AkCompressedAudioCaps::AudioCodecID_vorbis,
        AkCompressedAudioCaps::AudioCodecID_opus
    };
    static const QList<AkCodecID> videoCodecs {
        AkCompressedVideoCaps::VideoCodecID_vp8,
        AkCompressedVideoCaps::VideoCodecID_vp9,
        AkCompressedVideoCaps::VideoCodecID_av1
    };

    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return audioCodecs;

    case AkCompressedCaps::CapsType_Video:
        return videoCodecs;

    case AkCompressedCaps::CapsType_Unknown:
        return audioCodecs + videoCodecs;

    default:
        break;
    }

    return {};
}

AkCodecID VideoMuxerWebmElement::defaultCodec(AkCodecType type) const
{
    auto codecs = this->supportedCodecs(type);

    if (codecs.isEmpty())
        return 0;

    return codecs.first();
}

QString VideoMuxerWebmElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoMuxerWebm/share/qml/main.qml");
}

void VideoMuxerWebmElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoMuxerWebm", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket VideoMuxerWebmElement::iCompressedAudioStream(const AkCompressedAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_initialized || this->d->m_audioTrackIndex < 1)
        return {};

    while (this->d->m_isFirstVideoPackage && this->d->m_initialized) {
        if (this->d->m_videoReady.wait(&this->d->m_mutex, 3000))
            break;

        if (!this->d->m_initialized)
            return {};
    }

    mkvmuxer::Frame frame;

    if (!frame.Init(reinterpret_cast<const uint8_t *>(packet.constData()),
                                                      packet.size())) {
        qCritical() << "Can't initialize the audio frame";

        return {};
    }

    qreal pts = packet.pts() * packet.timeBase().value();
    this->d->m_audioPts = pts + this->d->m_audioDiff;
    this->d->m_lastAudioDuration = packet.duration() * packet.timeBase().value();

    if (this->d->m_isFirstAudioPackage) {
        this->d->m_audioDiff = this->d->m_clock - pts;
        this->d->m_audioPts = this->d->m_clock;
        this->d->m_isFirstAudioPackage = false;
    } else {
        if (this->d->m_audioPts < this->d->m_clock)
            this->d->m_audioDiff = this->d->m_clock - pts;
        else
            this->d->m_clock = this->d->m_audioPts;
    }

    frame.set_track_number(this->d->m_audioTrackIndex);
    frame.set_timestamp(qRound64(1e9 * this->d->m_clock));
    frame.set_is_key(packet.flags() & AkCompressedAudioPacket::AudioPacketTypeFlag_KeyFrame);

    if (!this->d->m_muxerSegment.AddGenericFrame(&frame))
        qCritical() << "Could not add the audio frame";

    this->d->m_audioReady.wakeAll();

    return {};
}

AkPacket VideoMuxerWebmElement::iCompressedVideoStream(const AkCompressedVideoPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_initialized)
        return {};

    mkvmuxer::Frame frame;

    if (!frame.Init(reinterpret_cast<const uint8_t *>(packet.constData()),
                                                      packet.size())) {
        qCritical() << "Can't initialize the video frame";

        return {};
    }

    qreal pts = packet.pts() * packet.timeBase().value();
    this->d->m_videoPts = pts + this->d->m_videoDiff;
    this->d->m_lastVideoDuration = packet.duration() * packet.timeBase().value();

    if (this->d->m_isFirstVideoPackage) {
        this->d->m_videoDiff = this->d->m_clock - pts;
        this->d->m_videoPts = this->d->m_clock;
        this->d->m_isFirstVideoPackage = false;
    } else {
        if (this->d->m_videoPts < this->d->m_clock)
            this->d->m_videoDiff = this->d->m_clock - pts;
        else
            this->d->m_clock = this->d->m_videoPts;
    }

    frame.set_track_number(this->d->m_videoTrackIndex);
    frame.set_timestamp(qRound64(1e9 * this->d->m_clock));
    frame.set_is_key(packet.flags() & AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame);

    if (!this->d->m_muxerSegment.AddGenericFrame(&frame))
        qCritical() << "Could not add the video frame";

    this->d->m_videoReady.wakeAll();

    return {};
}

void VideoMuxerWebmElement::resetOptions()
{
    AkVideoMuxer::resetOptions();
}

bool VideoMuxerWebmElement::setState(ElementState state)
{
    auto curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            if (!this->d->init())
                return false;

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
            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    }

    return false;
}

VideoMuxerWebmElementPrivate::VideoMuxerWebmElementPrivate(VideoMuxerWebmElement *self):
    self(self)
{
}

VideoMuxerWebmElementPrivate::~VideoMuxerWebmElementPrivate()
{

}

bool VideoMuxerWebmElementPrivate::init()
{
    this->uninit();

    this->m_clock = 0.0;
    this->m_isFirstAudioPackage = true;
    this->m_audioPts = 0.0;
    this->m_lastAudioDuration = 0.0;
    this->m_audioDiff = 0.0;
    this->m_isFirstVideoPackage = true;
    this->m_videoPts = 0.0;
    this->m_lastVideoDuration = 0.0;
    this->m_videoDiff = 0.0;

    this->m_audioTrackIndex = 0;
    this->m_videoTrackIndex = 0;

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

    auto location = self->location();

    if (!this->m_writer.Open(location.toStdString().c_str())) {
        qCritical() << "Failed to open file for writting:" << location;

        return false;
    }

    // Set Segment element attributes

    if (!this->m_muxerSegment.Init(&this->m_writer)) {
        qCritical() << "Failed to initialize the muxer segment";
        this->m_writer.Close();
        QFile::remove(location);

        return false;
    }

    this->m_muxerSegment.AccurateClusterDuration(this->m_accurateClusterDuration);
    this->m_muxerSegment.UseFixedSizeClusterTimecode(this->m_fixedSizeClusterTimecode);

    this->m_muxerSegment.set_mode(this->m_liveMode?
                                      mkvmuxer::Segment::kLive:
                                      mkvmuxer::Segment::kFile);

    if (this->m_maxClusterDuration > 0)
        this->m_muxerSegment.set_max_cluster_duration(this->m_maxClusterDuration);

    if (this->m_maxClusterSize > 0)
        this->m_muxerSegment.set_max_cluster_size(this->m_maxClusterSize);

    this->m_muxerSegment.OutputCues(this->m_outputCues);

    // Set SegmentInfo element attributes

    auto info = this->m_muxerSegment.GetSegmentInfo();
    info->set_timecode_scale(this->m_timeCodeScale);
    info->set_muxing_app(qApp->applicationName().toStdString().c_str());
    info->set_writing_app(qApp->applicationName().toStdString().c_str());

    // Add the video track to the muxer

    qInfo() << "Adding video track with format:" << videoCaps;
    this->m_videoTrackIndex =
            this->m_muxerSegment.AddVideoTrack(videoCaps.width(),
                                               videoCaps.height(),
                                               0);

    if (this->m_videoTrackIndex < 1) {
        qCritical() << "Could not add video track";
        this->m_writer.Close();
        QFile::remove(location);

        return false;
    }

    auto videoTrack =
            static_cast<mkvmuxer::VideoTrack *>(this->m_muxerSegment.GetTrackByNumber(this->m_videoTrackIndex));

    if (!videoTrack) {
        qCritical() << "Could not get video track";
        this->m_writer.Close();
        QFile::remove(location);

        return false;
    }

    videoTrack->set_name("Video");
    videoTrack->set_language("und");
    videoTrack->set_codec_id(vcodec->str);
    videoTrack->set_width(videoCaps.width());
    videoTrack->set_height(videoCaps.height());
    videoTrack->set_frame_rate(videoCaps.fps().value());

    // Add the audio track to the muxer

    if (audioCaps) {
        qInfo() << "Adding audio track with format:" << audioCaps;
        this->m_audioTrackIndex =
                this->m_muxerSegment.AddAudioTrack(audioCaps.rate(),
                                                   audioCaps.channels(),
                                                   0);

        if (this->m_audioTrackIndex < 1) {
            qCritical() << "Could not add audio track";
            this->m_writer.Close();
            QFile::remove(location);

            return false;
        }

        auto audioTrack =
                static_cast<mkvmuxer::AudioTrack *>(this->m_muxerSegment.GetTrackByNumber(this->m_audioTrackIndex));

        if (!audioTrack) {
            qCritical() << "Could not get audio track";
            this->m_writer.Close();
            QFile::remove(location);

            return false;
        }

        audioTrack->set_name("Audio");
        audioTrack->set_language("und");
        audioTrack->set_codec_id(acodec->str);
        audioTrack->set_bit_depth(audioCaps.bps());
        audioTrack->set_channels(audioCaps.channels());
        audioTrack->set_sample_rate(audioCaps.rate());
    }

    // Write the codec headers

    QByteArray privateData;

    for (auto &header: self->streamHeaders(AkCompressedCaps::CapsType_Video))
        privateData += QByteArray(header.constData(), header.size());

    if (!privateData.isEmpty())
        videoTrack->SetCodecPrivate(reinterpret_cast<const uint8_t *>(privateData.constData()),
                                    privateData.size());

    if (audioCaps) {
        privateData.clear();

        for (auto &header: self->streamHeaders(AkCompressedCaps::CapsType_Audio))
            privateData += QByteArray(header.constData(), header.size());

        if (!privateData.isEmpty()) {
            auto audioTrack =
                    static_cast<mkvmuxer::AudioTrack *>(this->m_muxerSegment.GetTrackByNumber(this->m_audioTrackIndex));

            if (audioTrack) {
                audioTrack->SetCodecPrivate(reinterpret_cast<const uint8_t *>(privateData.constData()),
                                            privateData.size());
            }
        }
    }

    qInfo() << "Starting Webm muxing";
    this->m_initialized = true;

    return true;
}

void VideoMuxerWebmElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    qInfo() << "Stopping Webm muxing";
    this->m_initialized = false;

    auto audioDuration = this->m_audioPts + this->m_lastAudioDuration;
    auto videoDuration = this->m_videoPts + this->m_lastVideoDuration;
    qreal duration =
            this->m_audioTrackIndex < 1?
                videoDuration:
                qMax(audioDuration, videoDuration);

    qInfo() << QString("Video duration: %1 (a: %2, v: %3)")
               .arg(duration)
               .arg(audioDuration)
               .arg(videoDuration)
               .toStdString().c_str();
    this->m_muxerSegment.set_duration(qRound64(duration * 1e9 / this->m_timeCodeScale));

    if (!this->m_muxerSegment.Finalize())
        qCritical() << "Finalization of segment failed";

    this->m_writer.Close();

    if (this->m_cuesBeforeClusters) {
        mkvparser::MkvReader reader;

        if (reader.Open(self->location().toStdString().c_str())) {
            qCritical() << "Filename is invalid or error while opening";
            qInfo() << "Webm muxing stopped";

            return;
        }

        QTemporaryDir tempDir;

        if (!tempDir.isValid()) {
            qCritical() << "Can't create the temporary directory";
            qInfo() << "Webm muxing stopped";

            return;
        }

        QFileInfo fileInfo(self->location());
        auto tmp = tempDir.filePath(fileInfo.baseName()
                                    + "_tmp."
                                    + fileInfo.completeSuffix());
        QFile::remove(tmp);

        if (this->m_writer.Open(tmp.toStdString().c_str())) {
            if (this->m_muxerSegment.CopyAndMoveCuesBeforeClusters(&reader,
                                                                   &this->m_writer)) {
                reader.Close();
                this->m_writer.Close();
                QFile::remove(self->location());
                QFile::rename(tmp, self->location());
            } else {
                qCritical() << "Unable to copy and move cues before clusters";
                reader.Close();
                this->m_writer.Close();
                QFile::remove(tmp);
            }
        } else {
            qCritical() << "Filename is invalid or error while opening";
            reader.Close();
            this->m_writer.Close();
            QFile::remove(tmp);
        }
    }

    qInfo() << "Webm muxing stopped";
}

#include "moc_videomuxerwebmelement.cpp"

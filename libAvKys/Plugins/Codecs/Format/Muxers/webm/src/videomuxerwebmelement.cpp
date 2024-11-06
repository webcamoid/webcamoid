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
            {AkCompressedVideoCaps::VideoCodecID_vp8    , mkvmuxer::Tracks::kVp9CodecId},
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
        bool m_isFirstAudioPackage {true};
        qreal m_firstAudioTime {0.0};
        qreal m_lastAudioTime {0.0};
        qreal m_lastAudioDuration {0.0};
        bool m_isFirstVideoPackage {true};
        qreal m_firstVideoTime {0.0};
        qreal m_lastVideoTime {0.0};
        qreal m_lastVideoDuration {0.0};
        QMutex m_mutex;
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

QList<AkCodecID> VideoMuxerWebmElement::supportedCodecs(AkCompressedCaps::CapsType type) const
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return {AkCompressedAudioCaps::AudioCodecID_vorbis,
                AkCompressedAudioCaps::AudioCodecID_opus};

    case AkCompressedCaps::CapsType_Video:
        return {AkCompressedVideoCaps::VideoCodecID_vp8,
                AkCompressedVideoCaps::VideoCodecID_vp9,
                AkCompressedVideoCaps::VideoCodecID_av1};

    default:
        break;
    }

    return {};
}

AkCodecID VideoMuxerWebmElement::defaultCodec(AkCompressedCaps::CapsType type) const
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return AkCompressedAudioCaps::AudioCodecID_vorbis;

    case AkCompressedCaps::CapsType_Video:
        return AkCompressedVideoCaps::VideoCodecID_vp8;

    default:
        break;
    }

    return 0;
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

    mkvmuxer::Frame frame;

    if (!frame.Init(reinterpret_cast<const uint8_t *>(packet.constData()),
                                                      packet.size())) {
        qCritical() << "Can't initialize the audio frame";

        return {};
    }

    this->d->m_lastAudioTime = packet.pts() * packet.timeBase().value();
    this->d->m_lastAudioDuration = packet.duration() * packet.timeBase().value();

    if (this->d->m_isFirstAudioPackage) {
        this->d->m_firstAudioTime = this->d->m_lastAudioTime;
        this->d->m_isFirstAudioPackage = false;
    }

    frame.set_track_number(this->d->m_audioTrackIndex);
    frame.set_timestamp(qRound64(1e9 * this->d->m_lastAudioTime));
    frame.set_is_key(packet.flags() & AkCompressedAudioPacket::AudioPacketTypeFlag_KeyFrame);

    if (!this->d->m_muxerSegment.AddGenericFrame(&frame))
        qCritical() << "Could not add the audio frame";

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
    this->d->m_lastVideoTime = packet.pts() * packet.timeBase().value();
    this->d->m_lastVideoDuration = packet.duration() * packet.timeBase().value();

    if (this->d->m_isFirstVideoPackage) {
        this->d->m_firstVideoTime = this->d->m_lastVideoTime;
        this->d->m_isFirstVideoPackage = false;
    }

    frame.set_track_number(this->d->m_videoTrackIndex);
    frame.set_timestamp(qRound64(1e9 * this->d->m_lastVideoTime));
    frame.set_is_key(packet.flags() & AkCompressedAudioPacket::AudioPacketTypeFlag_KeyFrame);

    if (!this->d->m_muxerSegment.AddGenericFrame(&frame))
        qCritical() << "Could not add the video frame";

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

    this->m_isFirstAudioPackage = true;
    this->m_isFirstVideoPackage = true;
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

    if (!audioCaps) {
        acodec = AudioCodecsTable::byCodecID(audioCaps.codec());

        if (!acodec->codecID) {
            qCritical() << "Audio codec not supported by this muxer:" << audioCaps.codec();

            return false;
        }
    }

    if (!this->m_writer.Open(self->location().toStdString().c_str())) {
        qCritical() << "Failed to open file for writting:" << self->location();

        return false;
    }

    // Set Segment element attributes

    if (!this->m_muxerSegment.Init(&this->m_writer)) {
        qCritical() << "Failed to initialize the muxer segment";

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
    info->set_writing_app(QCoreApplication::applicationName().toStdString().c_str());

    // Add the video track to the muxer

    this->m_videoTrackIndex =
            this->m_muxerSegment.AddVideoTrack(videoCaps.width(),
                                               videoCaps.height(),
                                               0);

    if (this->m_videoTrackIndex < 1) {
        qCritical() << "Could not add video track";

        return false;
    }

    auto videoTrack =
            static_cast<mkvmuxer::VideoTrack *>(this->m_muxerSegment.GetTrackByNumber(this->m_videoTrackIndex));

    if (!videoTrack) {
        qCritical() << "Could not get video track";

        return false;
    }

    videoTrack->set_name("Video");
    videoTrack->set_codec_id(vcodec->str);
    videoTrack->set_frame_rate(videoCaps.fps().value());

    // Add the audio track to the muxer

    if (audioCaps) {
        this->m_audioTrackIndex =
                this->m_muxerSegment.AddAudioTrack(audioCaps.rate(),
                                                   audioCaps.channels(),
                                                   1);

        if (this->m_audioTrackIndex < 1) {
            qCritical() << "Could not add audio track";

            return false;
        }

        auto audioTrack =
                static_cast<mkvmuxer::AudioTrack *>(this->m_muxerSegment.GetTrackByNumber(this->m_audioTrackIndex));

        if (!audioTrack) {
            qCritical() << "Could not get audio track";

            return false;
        }

        audioTrack->set_name("Audio");
        audioTrack->set_codec_id(acodec->str);
        audioTrack->set_bit_depth(audioCaps.bps());
    }

    // Set Cues element attributes

    auto cues = this->m_muxerSegment.GetCues();
    cues->set_output_block_number(this->m_outputCuesBlockNumber);
    this->m_muxerSegment.CuesTrack(this->m_videoTrackIndex);

    if (audioCaps)
        this->m_muxerSegment.CuesTrack(this->m_audioTrackIndex);

    this->m_initialized = true;

    return true;
}

void VideoMuxerWebmElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    auto duration =
            this->m_audioTrackIndex < 1?
                this->m_lastVideoTime - this->m_firstVideoTime + this->m_lastVideoDuration:
                qMax(this->m_lastAudioTime - this->m_firstAudioTime + this->m_lastAudioDuration,
                     this->m_lastVideoTime - this->m_firstVideoTime + this->m_lastVideoDuration);

    this->m_muxerSegment.set_duration(qRound64(duration * 1e9 / this->m_timeCodeScale));

    if (!this->m_muxerSegment.Finalize())
        qCritical() << "Finalization of segment failed";

    this->m_writer.Close();
    this->m_isFirstAudioPackage = true;
    this->m_isFirstVideoPackage = true;
    this->m_audioTrackIndex = 0;
    this->m_videoTrackIndex = 0;

    if (this->m_cuesBeforeClusters) {
        mkvparser::MkvReader reader;

        if (reader.Open(self->location().toStdString().c_str())) {
            qCritical() << "Filename is invalid or error while opening";

            return;
        }

        QTemporaryDir tempDir;

        if (!tempDir.isValid()) {
            qCritical() << "Can't create the temporary directory";

            return;
        }

        QFileInfo fileInfo;
        auto tmp = tempDir.filePath(fileInfo.baseName()
                                    + "_tmp."
                                    + fileInfo.completeSuffix());
        QFile::remove(tmp);

        if (!this->m_writer.Open(tmp.toStdString().c_str())) {
            qCritical() << "Filename is invalid or error while opening";

            return;
        }

        if (!this->m_muxerSegment.CopyAndMoveCuesBeforeClusters(&reader,
                                                                &this->m_writer)) {
            qCritical() << "Unable to copy and move cues before clusters";

            return;
        }

        reader.Close();
        this->m_writer.Close();

        QFile::remove(self->location());
        QFile::rename(tmp, self->location());
    }
}

#include "moc_videomuxerwebmelement.cpp"

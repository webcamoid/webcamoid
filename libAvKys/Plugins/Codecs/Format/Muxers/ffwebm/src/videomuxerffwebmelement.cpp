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

#include <QMutex>
#include <QQmlContext>
#include <QThread>
#include <QWaitCondition>
#include <akaudiocaps.h>
#include <akcompressedaudiocaps.h>
#include <akcompressedaudiopacket.h>
#include <akcompressedvideocaps.h>
#include <akcompressedvideopacket.h>
#include <akvideocaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>

extern "C" {
    #include <libavformat/avformat.h>
}

#include "videomuxerffwebmelement.h"


struct AudioCodecsTable
{
    AkCompressedAudioCaps::AudioCodecID codecID;
    AVCodecID ffID;
    const char *name;

    inline static const AudioCodecsTable *table()
    {
        static const AudioCodecsTable ffwebmAudioCodecsTable[] {
            {AkCompressedAudioCaps::AudioCodecID_vorbis , AV_CODEC_ID_VORBIS, "vorbis"},
            {AkCompressedAudioCaps::AudioCodecID_opus   , AV_CODEC_ID_OPUS  , "opus"  },
            {AkCompressedAudioCaps::AudioCodecID_unknown, AV_CODEC_ID_NONE  , ""      },
        };

        return ffwebmAudioCodecsTable;
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
    AVCodecID ffID;
    const char *name;

    inline static const VideoCodecsTable *table()
    {
        static const VideoCodecsTable ffwebmVideoCodecsTable[] {
            {AkCompressedVideoCaps::VideoCodecID_vp8    , AV_CODEC_ID_VP8 , "vp8"},
            {AkCompressedVideoCaps::VideoCodecID_vp9    , AV_CODEC_ID_VP9 , "vp9"},
            {AkCompressedVideoCaps::VideoCodecID_av1    , AV_CODEC_ID_AV1 , "av1"},
            {AkCompressedVideoCaps::VideoCodecID_unknown, AV_CODEC_ID_NONE, ""   },
        };

        return ffwebmVideoCodecsTable;
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

struct PixelFormatsTable
{
    AkVideoCaps::PixelFormat format;
    AVPixelFormat ffFormat;

    inline static const PixelFormatsTable *table()
    {
        static const PixelFormatsTable ffwebmPixelFormatsTable[] {
            {AkVideoCaps::Format_bgr24    , AV_PIX_FMT_BGR24    },
            {AkVideoCaps::Format_bgr48    , AV_PIX_FMT_BGR48    },
            {AkVideoCaps::Format_bgra     , AV_PIX_FMT_BGRA     },
            {AkVideoCaps::Format_bgra64   , AV_PIX_FMT_BGRA64   },
            {AkVideoCaps::Format_nv12     , AV_PIX_FMT_NV12     },
            {AkVideoCaps::Format_nv16     , AV_PIX_FMT_NV16     },
            {AkVideoCaps::Format_nv21     , AV_PIX_FMT_NV21     },
            {AkVideoCaps::Format_rgb24    , AV_PIX_FMT_RGB24    },
            {AkVideoCaps::Format_uyvy422  , AV_PIX_FMT_UYVY422  },
            {AkVideoCaps::Format_y10      , AV_PIX_FMT_GRAY10   },
            {AkVideoCaps::Format_y12      , AV_PIX_FMT_GRAY12   },
            {AkVideoCaps::Format_y14      , AV_PIX_FMT_GRAY14   },
            {AkVideoCaps::Format_y8       , AV_PIX_FMT_GRAY8    },
            {AkVideoCaps::Format_yuv420p  , AV_PIX_FMT_YUV420P  },
            {AkVideoCaps::Format_yuv420p10, AV_PIX_FMT_YUV420P10},
            {AkVideoCaps::Format_yuv420p12, AV_PIX_FMT_YUV420P12},
            {AkVideoCaps::Format_yuv420p14, AV_PIX_FMT_YUV420P14},
            {AkVideoCaps::Format_yuv422p  , AV_PIX_FMT_YUV422P  },
            {AkVideoCaps::Format_yuv422p10, AV_PIX_FMT_YUV422P10},
            {AkVideoCaps::Format_yuv422p12, AV_PIX_FMT_YUV422P12},
            {AkVideoCaps::Format_yuv422p14, AV_PIX_FMT_YUV422P14},
            {AkVideoCaps::Format_yuv440p  , AV_PIX_FMT_YUV440P  },
            {AkVideoCaps::Format_yuv440p10, AV_PIX_FMT_YUV440P10},
            {AkVideoCaps::Format_yuv440p12, AV_PIX_FMT_YUV440P12},
            {AkVideoCaps::Format_yuv444p  , AV_PIX_FMT_YUV444P  },
            {AkVideoCaps::Format_yuv444p10, AV_PIX_FMT_YUV444P10},
            {AkVideoCaps::Format_yuv444p12, AV_PIX_FMT_YUV444P12},
            {AkVideoCaps::Format_yuv444p14, AV_PIX_FMT_YUV444P14},
            {AkVideoCaps::Format_yuyv422  , AV_PIX_FMT_YUYV422  },
            {AkVideoCaps::Format_yvu422p  , AV_PIX_FMT_YVYU422  },
            {AkVideoCaps::Format_none     , AV_PIX_FMT_NONE     },
        };

        return ffwebmPixelFormatsTable;
    }

    inline static const PixelFormatsTable *byFormat(AkVideoCaps::PixelFormat format)
    {
        auto item = table();

        for (; item->format; ++item)
            if (item->format == format)
                return item;

        return item;
    }
};

struct SampleFormatsTable
{
    AkAudioCaps::SampleFormat format;
    bool planar;
    AVSampleFormat ffFormat;

    inline static const SampleFormatsTable *table()
    {
        static const SampleFormatsTable ffwebmSampleFormatsTable[] {
            {AkAudioCaps::SampleFormat_u8  , false, AV_SAMPLE_FMT_U8  },
            {AkAudioCaps::SampleFormat_s16 , false, AV_SAMPLE_FMT_S16 },
            {AkAudioCaps::SampleFormat_s32 , false, AV_SAMPLE_FMT_S32 },
            {AkAudioCaps::SampleFormat_flt , false, AV_SAMPLE_FMT_FLT },
            {AkAudioCaps::SampleFormat_dbl , false, AV_SAMPLE_FMT_DBL },
            {AkAudioCaps::SampleFormat_u8  , true , AV_SAMPLE_FMT_U8P },
            {AkAudioCaps::SampleFormat_s16 , true , AV_SAMPLE_FMT_S16P},
            {AkAudioCaps::SampleFormat_s32 , true , AV_SAMPLE_FMT_S32P},
            {AkAudioCaps::SampleFormat_flt , true , AV_SAMPLE_FMT_FLTP},
            {AkAudioCaps::SampleFormat_dbl , true , AV_SAMPLE_FMT_DBLP},
            {AkAudioCaps::SampleFormat_s64 , false, AV_SAMPLE_FMT_S64 },
            {AkAudioCaps::SampleFormat_s64 , true , AV_SAMPLE_FMT_S64P},
            {AkAudioCaps::SampleFormat_none, false, AV_SAMPLE_FMT_NONE},
        };

        return ffwebmSampleFormatsTable;
    }

    inline static const SampleFormatsTable *byFormat(AkAudioCaps::SampleFormat format,
                                                     bool planar)
    {
        auto item = table();

        for (; item->format; ++item)
            if (item->format == format && item->planar == planar)
                return item;

        return item;
    }
};

class VideoMuxerFFWebmElementPrivate
{
    public:
        VideoMuxerFFWebmElement *self;
        AVFormatContext *m_context {nullptr};
        bool m_accurateClusterDuration {false};
        bool m_fixedSizeClusterTimecode {false};
        bool m_liveMode {true};
        bool m_outputCues {true};
        uint64_t m_maxClusterSize {0};
        bool m_outputCuesBlockNumber {true};
        bool m_cuesBeforeClusters {false};
        uint64_t m_maxClusterDuration {0};
        QMutex m_mutex;
        bool m_initialized {false};
        bool m_paused {false};
        int64_t m_packetPos {0};
        AkElementPtr m_packetSync {akPluginManager->create<AkElement>("Utils/PacketSync")};

        explicit VideoMuxerFFWebmElementPrivate(VideoMuxerFFWebmElement *self);
        ~VideoMuxerFFWebmElementPrivate();
        bool init();
        void uninit();
        void packetReady(const AkPacket &packet);
};

VideoMuxerFFWebmElement::VideoMuxerFFWebmElement():
    AkVideoMuxer()
{
    this->d = new VideoMuxerFFWebmElementPrivate(this);
}

VideoMuxerFFWebmElement::~VideoMuxerFFWebmElement()
{
    this->d->uninit();
    delete this->d;
}

AkVideoMuxer::FormatID VideoMuxerFFWebmElement::formatID() const
{
    return FormatID_webm;
}

QString VideoMuxerFFWebmElement::extension() const
{
    return {"webm"};
}

bool VideoMuxerFFWebmElement::gapsAllowed(AkCodecType type) const
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return false;

    case AkCompressedCaps::CapsType_Video:
        return true;

    default:
        break;
    }

    return true;
}

QList<AkCodecID> VideoMuxerFFWebmElement::supportedCodecs(AkCodecType type) const
{
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

AkCodecID VideoMuxerFFWebmElement::defaultCodec(AkCodecType type) const
{
    auto codecs = this->supportedCodecs(type);

    if (codecs.isEmpty())
        return 0;

    return codecs.first();
}

QString VideoMuxerFFWebmElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoMuxerFFWebm/share/qml/main.qml");
}

void VideoMuxerFFWebmElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoMuxerFFWebm", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void VideoMuxerFFWebmElement::resetOptions()
{
    AkVideoMuxer::resetOptions();
}

AkPacket VideoMuxerFFWebmElement::iStream(const AkPacket &packet)
{
    if (this->d->m_paused || !this->d->m_initialized || !this->d->m_packetSync)
        return {};

    return this->d->m_packetSync->iStream(packet);
}

bool VideoMuxerFFWebmElement::setState(ElementState state)
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

VideoMuxerFFWebmElementPrivate::VideoMuxerFFWebmElementPrivate(VideoMuxerFFWebmElement *self):
    self(self)
{
    if (this->m_packetSync)
        QObject::connect(this->m_packetSync.data(),
                         &AkElement::oStream,
                         [this] (const AkPacket &packet) {
                             this->packetReady(packet);
                         });
}

VideoMuxerFFWebmElementPrivate::~VideoMuxerFFWebmElementPrivate()
{

}

bool VideoMuxerFFWebmElementPrivate::init()
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

    auto format = av_guess_format("webm",
                                  nullptr,
                                  "video/webm");

    if (!format) {
        qCritical() << "Webm format not found";

        return false;
    }

    auto location = self->location();
    this->m_context = nullptr;

    if (avformat_alloc_output_context2(&this->m_context,
                                       format,
                                       "webm",
                                       location.toStdString().c_str()) < 0) {
        qCritical() << "Can't allocate the context";

        return false;
    }

    // Add the video track to the muxer

    AVCodec codec;
    memset(&codec, 0, sizeof(AVCodec));
    codec.name = vcodec->name;
    codec.long_name = vcodec->name;
    codec.wrapper_name = vcodec->name;
    codec.type = AVMEDIA_TYPE_VIDEO;
    codec.id = vcodec->ffID;
    codec.capabilities = AV_CODEC_CAP_DR1
                       | AV_CODEC_CAP_DELAY
                       | AV_CODEC_CAP_OTHER_THREADS
                       | AV_CODEC_CAP_ENCODER_REORDERED_OPAQUE;
    AVClass codecClass;
    memset(&codecClass, 0, sizeof(AVClass));
    codecClass.class_name = vcodec->name;
    codecClass.item_name = av_default_item_name;
    codecClass.version = LIBAVUTIL_VERSION_INT;
    codec.priv_class = &codecClass;

    auto videoStream = avformat_new_stream(this->m_context, &codec);

    if (!videoStream) {
        qCritical() << "Failed to add stream";
        avformat_free_context(this->m_context);
        this->m_context = nullptr;

        return false;
    }

    videoStream->index = 0;
    videoStream->id = 0;
    videoStream->codecpar->codec_type = codec.type;
    videoStream->codecpar->codec_id = codec.id;
    auto ffFormat = PixelFormatsTable::byFormat(videoCaps.rawCaps().format())->ffFormat;

    if (ffFormat == AV_PIX_FMT_NONE)
        ffFormat = AV_PIX_FMT_YUV420P;

    videoStream->codecpar->format = ffFormat;
    videoStream->codecpar->profile = 0;
    videoStream->codecpar->level = 0;
    videoStream->codecpar->bit_rate = videoCaps.bitrate();
    videoStream->codecpar->width = videoCaps.rawCaps().width();
    videoStream->codecpar->height = videoCaps.rawCaps().height();
    videoStream->codecpar->sample_aspect_ratio.num = 1;
    videoStream->codecpar->sample_aspect_ratio.den = 1;
    videoStream->codecpar->framerate.num = videoCaps.rawCaps().fps().num();
    videoStream->codecpar->framerate.den = videoCaps.rawCaps().fps().den();
    videoStream->codecpar->field_order = AV_FIELD_UNKNOWN;
    videoStream->codecpar->video_delay = 0;
    videoStream->time_base.num = videoStream->codecpar->framerate.den;
    videoStream->time_base.den = videoStream->codecpar->framerate.num;
    videoStream->duration = 0;
    videoStream->nb_frames = 0;
    videoStream->metadata = nullptr;
    videoStream->avg_frame_rate.num = videoStream->codecpar->framerate.num;
    videoStream->avg_frame_rate.den = videoStream->codecpar->framerate.den;
    videoStream->r_frame_rate.num = videoStream->codecpar->framerate.num;
    videoStream->r_frame_rate.den = videoStream->codecpar->framerate.den;
    videoStream->pts_wrap_bits = 64;

    QByteArray videoPrivateData;

    for (auto &header: self->streamHeaders(AkCompressedCaps::CapsType_Video))
        videoPrivateData += QByteArray(header.constData(), header.size());

    if (!videoPrivateData.isEmpty()) {
        videoStream->codecpar->extradata_size =
                videoPrivateData.size() + AV_INPUT_BUFFER_PADDING_SIZE;
        videoStream->codecpar->extradata =
                reinterpret_cast<uint8_t *>(av_malloc(videoStream->codecpar->extradata_size));
        memcpy(videoStream->codecpar->extradata,
               videoPrivateData.constData(),
               videoPrivateData.size());
        memset(videoStream->codecpar->extradata + videoPrivateData.size(),
               0,
               AV_INPUT_BUFFER_PADDING_SIZE);
    }

    // Add the audio track to the muxer

    if (audioCaps) {
        AVCodec codec;
        memset(&codec, 0, sizeof(AVCodec));
        codec.name = acodec->name;
        codec.long_name = acodec->name;
        codec.wrapper_name = acodec->name;
        codec.type = AVMEDIA_TYPE_AUDIO;
        codec.id = acodec->ffID;
        codec.capabilities = AV_CODEC_CAP_DR1
                           | AV_CODEC_CAP_DELAY
                           | AV_CODEC_CAP_SMALL_LAST_FRAME;
        AVClass codecClass;
        memset(&codecClass, 0, sizeof(AVClass));
        codecClass.class_name = acodec->name;
        codecClass.item_name = av_default_item_name;
        codecClass.version = LIBAVUTIL_VERSION_INT;
        codec.priv_class = &codecClass;

        auto audioStream = avformat_new_stream(this->m_context, &codec);

        if (!audioStream) {
            qCritical() << "Failed to add stream";
            avformat_free_context(this->m_context);
            this->m_context = nullptr;

            return false;
        }

        audioStream->index = 1;
        audioStream->id = 1;
        audioStream->codecpar->codec_type = codec.type;
        audioStream->codecpar->codec_id = codec.id;
        auto ffFormat = SampleFormatsTable::byFormat(audioCaps.rawCaps().format(),
                                                     audioCaps.rawCaps().planar())->ffFormat;

        if (ffFormat == AV_SAMPLE_FMT_NONE)
            ffFormat = AV_SAMPLE_FMT_S16;

        audioStream->codecpar->format = ffFormat;
        audioStream->codecpar->bit_rate = audioCaps.bitrate();
        audioStream->codecpar->bits_per_raw_sample =
                audioCaps.rawCaps().bps();
        audioStream->codecpar->bits_per_coded_sample =
                audioStream->codecpar->bits_per_raw_sample;
        int channels = qBound(1, audioCaps.rawCaps().channels(), 2);
        av_channel_layout_default(&audioStream->codecpar->ch_layout, channels);
        audioStream->codecpar->sample_rate =
                audioCaps.rawCaps().rate();

        audioStream->codecpar->field_order = AV_FIELD_UNKNOWN;
        audioStream->codecpar->video_delay = 0;
        audioStream->time_base.num = audioStream->codecpar->framerate.den;
        audioStream->time_base.den = audioStream->codecpar->framerate.num;
        audioStream->duration = 0;
        audioStream->nb_frames = 0;
        audioStream->metadata = nullptr;
        audioStream->pts_wrap_bits = 64;

        QByteArray audioPrivateData;

        for (auto &header: self->streamHeaders(AkCompressedCaps::CapsType_Audio))
            audioPrivateData += QByteArray(header.constData(), header.size());

        if (!audioPrivateData.isEmpty()) {
            audioStream->codecpar->extradata_size =
                    audioPrivateData.size() + AV_INPUT_BUFFER_PADDING_SIZE;
            audioStream->codecpar->extradata =
                    reinterpret_cast<uint8_t *>(av_malloc(audioStream->codecpar->extradata_size));
            memcpy(audioStream->codecpar->extradata,
                   audioPrivateData.constData(),
                   audioPrivateData.size());
            memset(audioStream->codecpar->extradata + audioPrivateData.size(),
                   0,
                   AV_INPUT_BUFFER_PADDING_SIZE);
        }
    }

    // Initialzethe muxer

    AVDictionary *options = nullptr;
    av_dict_set(&options, "live", "0", 0);
    av_dict_set(&options, "cues_to_front", "0", 0);
    av_dict_set(&options, "dash", "0", 0);

    if (avformat_init_output(this->m_context, &options) < 1) {
        qCritical() << "Failed initializing the context";
        avformat_free_context(this->m_context);
        this->m_context = nullptr;
        av_dict_free(&options);

        return false;
    }

    if (avio_open(&this->m_context->pb,
                  location.toStdString().c_str(),
                  AVIO_FLAG_READ_WRITE) < 0) {
        qCritical() << "Failed to open file";
        avformat_free_context(this->m_context);
        this->m_context = nullptr;
        av_dict_free(&options);

        return false;
    }

    // Write the format headers

    if (avformat_write_header(this->m_context, nullptr) < 0) {
        qCritical() << "Error writting the file header";
        avformat_free_context(this->m_context);
        this->m_context = nullptr;
        av_dict_free(&options);

        return false;
    }

    av_dict_free(&options);

    this->m_packetSync->setProperty("audioEnabled", bool(audioCaps));
    this->m_packetSync->setProperty("discardLast", false);
    this->m_packetSync->setState(AkElement::ElementStatePlaying);
    this->m_packetPos = 0;

    qInfo() << "Starting Webm muxing";
    this->m_initialized = true;

    return true;
}

void VideoMuxerFFWebmElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;
    this->m_packetSync->setState(AkElement::ElementStateNull);

    if (av_write_trailer(this->m_context) < 0)
        qCritical() << "Failed writting the trailer";

    avio_close(this->m_context->pb);
    avformat_free_context(this->m_context);
    this->m_context = nullptr;

    this->m_paused = false;
}

void VideoMuxerFFWebmElementPrivate::packetReady(const AkPacket &packet)
{
    bool isAudio = packet.type() == AkPacket::PacketAudio
                   || packet.type() == AkPacket::PacketAudioCompressed;
    uint64_t track = isAudio? 1: 0;

    if (track >= this->m_context->nb_streams)
        return;

    bool isKey = true;

    if (packet.type() == AkPacket::PacketVideoCompressed)
        isKey = AkCompressedVideoPacket(packet).flags()
                & AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame;

    auto stream = this->m_context->streams[track];

    auto avPacket = av_packet_alloc();
    avPacket->data = reinterpret_cast<uint8_t *>(packet.data());
    avPacket->size = packet.size();
    avPacket->stream_index = track;
    avPacket->pts = uint64_t(packet.pts()
                             * packet.timeBase().value()
                             * stream->time_base.den
                             / stream->time_base.num);
    avPacket->dts = avPacket->pts;
    avPacket->duration = uint64_t(packet.duration()
                                  * packet.timeBase().value()
                                  * stream->time_base.den
                                  / stream->time_base.num);
    avPacket->flags = isKey? AV_PKT_FLAG_KEY: 0;
    avPacket->pos = this->m_packetPos;
    avPacket->time_base.num = stream->time_base.num;
    avPacket->time_base.den = stream->time_base.den;

    if (av_interleaved_write_frame(this->m_context, avPacket) < 0) {
        if (isAudio)
            qCritical() << "Failed to write the audio packet";
        else
            qCritical() << "Failed to write the video packet";
    }

    av_packet_free(&avPacket);
    this->m_packetPos++;
}

#include "moc_videomuxerffwebmelement.cpp"

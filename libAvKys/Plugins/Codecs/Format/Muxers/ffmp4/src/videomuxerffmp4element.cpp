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
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

#include "videomuxerffmp4element.h"


struct AudioCodecsTable
{
    AkCompressedAudioCaps::AudioCodecID codecID;
    AVCodecID ffID;
    const char *name;

    inline static const AudioCodecsTable *table()
    {
        static const AudioCodecsTable ffmp4AudioCodecsTable[] {
            {AkCompressedAudioCaps::AudioCodecID_aac    , AV_CODEC_ID_AAC , "aac"},
            {AkCompressedAudioCaps::AudioCodecID_mpeg3  , AV_CODEC_ID_MP3 , "mp3"},
            {AkCompressedAudioCaps::AudioCodecID_unknown, AV_CODEC_ID_NONE, ""   },
        };

        return ffmp4AudioCodecsTable;
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
        static const VideoCodecsTable ffmp4VideoCodecsTable[] {
            {AkCompressedVideoCaps::VideoCodecID_ffh264 , AV_CODEC_ID_H264, "h264"},
            {AkCompressedVideoCaps::VideoCodecID_ffhevc , AV_CODEC_ID_HEVC, "hevc"},
            {AkCompressedVideoCaps::VideoCodecID_unknown, AV_CODEC_ID_NONE, ""    },
        };

        return ffmp4VideoCodecsTable;
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
        static const PixelFormatsTable ffmp4PixelFormatsTable[] {
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

        return ffmp4PixelFormatsTable;
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
        static const SampleFormatsTable ffmp4SampleFormatsTable[] {
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

        return ffmp4SampleFormatsTable;
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

class VideoMuxerFFMp4ElementPrivate
{
    public:
        VideoMuxerFFMp4Element *self;
        AVFormatContext *m_context {nullptr};
        bool m_liveMode {true};
        QMutex m_mutex;
        bool m_initialized {false};
        bool m_paused {false};
        int64_t m_packetPos {0};
        AkElementPtr m_packetSync {akPluginManager->create<AkElement>("Utils/PacketSync")};

        explicit VideoMuxerFFMp4ElementPrivate(VideoMuxerFFMp4Element *self);
        ~VideoMuxerFFMp4ElementPrivate();
        void readHeaders(AVCodecParameters *codecpar,
                             QByteArray &privateData);
        bool init();
        void uninit();
        void packetReady(const AkPacket &packet);
};

VideoMuxerFFMp4Element::VideoMuxerFFMp4Element():
    AkVideoMuxer()
{
    this->d = new VideoMuxerFFMp4ElementPrivate(this);
}

VideoMuxerFFMp4Element::~VideoMuxerFFMp4Element()
{
    this->d->uninit();
    delete this->d;
}

AkVideoMuxer::FormatID VideoMuxerFFMp4Element::formatID() const
{
    return FormatID_mp4;
}

QString VideoMuxerFFMp4Element::extension() const
{
    return {"mp4"};
}

bool VideoMuxerFFMp4Element::gapsAllowed(AkCodecType type) const
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

QList<AkCodecID> VideoMuxerFFMp4Element::supportedCodecs(AkCodecType type) const
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

AkCodecID VideoMuxerFFMp4Element::defaultCodec(AkCodecType type) const
{
    auto codecs = this->supportedCodecs(type);

    if (codecs.isEmpty())
        return 0;

    return codecs.first();
}

QString VideoMuxerFFMp4Element::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoMuxerFFMp4/share/qml/main.qml");
}

void VideoMuxerFFMp4Element::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoMuxerFFMp4", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void VideoMuxerFFMp4Element::resetOptions()
{
    AkVideoMuxer::resetOptions();
}

AkPacket VideoMuxerFFMp4Element::iStream(const AkPacket &packet)
{
    if (this->d->m_paused || !this->d->m_initialized || !this->d->m_packetSync)
        return {};

    return this->d->m_packetSync->iStream(packet);
}

bool VideoMuxerFFMp4Element::setState(ElementState state)
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

VideoMuxerFFMp4ElementPrivate::VideoMuxerFFMp4ElementPrivate(VideoMuxerFFMp4Element *self):
    self(self)
{
    if (this->m_packetSync)
        QObject::connect(this->m_packetSync.data(),
                         &AkElement::oStream,
                         [this] (const AkPacket &packet) {
                             this->packetReady(packet);
                         });
}

VideoMuxerFFMp4ElementPrivate::~VideoMuxerFFMp4ElementPrivate()
{

}

void VideoMuxerFFMp4ElementPrivate::readHeaders(AVCodecParameters *codecpar,
                                                    QByteArray &privateData)
{
    QDataStream ds(&privateData, QIODeviceBase::ReadOnly);
    int profile = 0;
    ds >> profile;
    int level = 0;
    ds >> level;
    int fieldOrder = 0;
    ds >> fieldOrder;
    quint64 extraDataSize = 0;
    ds >> extraDataSize;
    QByteArray extraData(extraDataSize, Qt::Uninitialized);
    ds.readRawData(extraData.data(), extraData.size());

    codecpar->profile = profile;
    codecpar->level = level;
    codecpar->field_order = AVFieldOrder(fieldOrder);
    codecpar->extradata_size = extraData.size();
    codecpar->extradata =
            reinterpret_cast<uint8_t *>(av_malloc(codecpar->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE));
    memcpy(codecpar->extradata,
           extraData.constData(),
           codecpar->extradata_size);
    memset(codecpar->extradata + codecpar->extradata_size,
           0,
           AV_INPUT_BUFFER_PADDING_SIZE);
}

bool VideoMuxerFFMp4ElementPrivate::init()
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

    auto format = av_guess_format("mp4",
                                  nullptr,
                                  "video/mp4");

    if (!format) {
        qCritical() << "Mp4 format not found";

        return false;
    }

    auto location = self->location();
    this->m_context = nullptr;

    if (avformat_alloc_output_context2(&this->m_context,
                                       format,
                                       "mp4",
                                       location.toStdString().c_str()) < 0) {
        qCritical() << "Can't allocate the context";

        return false;
    }

    if (avio_open(&this->m_context->pb,
                  location.toStdString().c_str(),
                  AVIO_FLAG_READ_WRITE) < 0) {
        qCritical() << "Failed to open file";
        avformat_free_context(this->m_context);
        this->m_context = nullptr;

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
    videoStream->codecpar->bit_rate = videoCaps.bitrate();
    videoStream->codecpar->width = videoCaps.rawCaps().width();
    videoStream->codecpar->height = videoCaps.rawCaps().height();
    videoStream->codecpar->sample_aspect_ratio.num = 0;
    videoStream->codecpar->sample_aspect_ratio.den = 1;
    videoStream->codecpar->framerate.num = videoCaps.rawCaps().fps().num();
    videoStream->codecpar->framerate.den = videoCaps.rawCaps().fps().den();
    videoStream->codecpar->video_delay = 0;
    videoStream->codecpar->color_range = AVCOL_RANGE_UNSPECIFIED;
    videoStream->codecpar->color_primaries = AVCOL_PRI_UNSPECIFIED;
    videoStream->codecpar->color_trc = AVCOL_TRC_UNSPECIFIED;
    videoStream->codecpar->color_space = AVCOL_SPC_UNSPECIFIED;
    videoStream->codecpar->chroma_location = AVCHROMA_LOC_UNSPECIFIED;
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

    if (!videoPrivateData.isEmpty())
        this->readHeaders(videoStream->codecpar, videoPrivateData);

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
        auto ffFormat =
                SampleFormatsTable::byFormat(audioCaps.rawCaps().format(),
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
        audioStream->codecpar->sample_rate = audioCaps.rawCaps().rate();
        audioStream->codecpar->frame_size = 1024;
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
    av_dict_set(&options, "movflags", "faststart+isml", 0);

    if (avformat_init_output(this->m_context, &options) < 1) {
        qCritical() << "Failed initializing the context";
        avformat_free_context(this->m_context);
        this->m_context = nullptr;
        av_dict_free(&options);

        return false;
    }

    // Write the format headers

    if (avformat_write_header(this->m_context, &options) < 0) {
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

    qInfo() << "Starting Mp4 muxing";
    this->m_initialized = true;

    return true;
}

void VideoMuxerFFMp4ElementPrivate::uninit()
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

void VideoMuxerFFMp4ElementPrivate::packetReady(const AkPacket &packet)
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

#include "moc_videomuxerffmp4element.cpp"

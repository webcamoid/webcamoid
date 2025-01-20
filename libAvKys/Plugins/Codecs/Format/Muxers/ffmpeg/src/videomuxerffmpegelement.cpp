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
#include <QThread>
#include <QVariant>
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
    #include <libavutil/opt.h>
    #include <libavutil/parseutils.h>
}

#include "videomuxerffmpegelement.h"

// Custom audio codecs

#define AudioCodecID_ffopus   AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xFF, 'O', 'P', 'U'))
#define AudioCodecID_ffvorbis AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xFF, 'V', 'O', 'R'))
#define AudioCodecID_ffaac    AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xFF, 'A', 'A', 'C'))
#define AudioCodecID_ffmp3    AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xFF, 'M', 'P', '3'))

// Custom video codecs

#define VideoCodecID_ffvp8  AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xFF, 'V', 'P', '8'))
#define VideoCodecID_ffvp9  AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xFF, 'V', 'P', '9'))
#define VideoCodecID_ffav1  AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xFF, 'A', 'V', '1'))
#define VideoCodecID_ffh264 AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xFF, 'A', 'V', 'C'))
#define VideoCodecID_ffhevc AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xFF, 'H', 'E', 'V'))

struct VideoMuxer
{
    const char *muxer;
    const char *mimeType;
    const char *description;
    const char *extension;
    AkVideoMuxer::FormatID formatID;
    AkCompressedAudioCaps::AudioCodecID audioCodecs[16];
    AkCompressedVideoCaps::VideoCodecID videoCodecs[16];

    inline static const VideoMuxer *table()
    {
        static const VideoMuxer ffmpegMuxerFormatsTable[] = {
            {"webm", "video/webm", "Webm (FFmpeg)", "webm", AkVideoMuxer::FormatID_webm,
                {AudioCodecID_ffvorbis,
                 AudioCodecID_ffopus,
                 AkCompressedAudioCaps::AudioCodecID_unknown},
                {VideoCodecID_ffvp8,
                 VideoCodecID_ffvp9,
                 VideoCodecID_ffav1,
                 AkCompressedVideoCaps::VideoCodecID_unknown}},
            {"mp4", "video/mp4", "MP4 (FFmpeg)", "mp4", AkVideoMuxer::FormatID_mp4,
                {AudioCodecID_ffaac,
                 AudioCodecID_ffmp3,
                 AkCompressedAudioCaps::AudioCodecID_unknown},
                {VideoCodecID_ffh264,
                 VideoCodecID_ffhevc,
                 AkCompressedVideoCaps::VideoCodecID_unknown}},
            {nullptr, nullptr, nullptr, nullptr, AkVideoMuxer::FormatID_unknown,
                {AkCompressedAudioCaps::AudioCodecID_unknown},
                {AkCompressedVideoCaps::VideoCodecID_unknown}},
        };

        return ffmpegMuxerFormatsTable;
    }
};

struct AudioCodecsTable
{
    AkCompressedAudioCaps::AudioCodecID codecID;
    AVCodecID ffID;

    inline static const AudioCodecsTable *table()
    {
        static const AudioCodecsTable ffmpegAudioCodecsTable[] {
            {AudioCodecID_ffvorbis                      , AV_CODEC_ID_VORBIS},
            {AudioCodecID_ffopus                        , AV_CODEC_ID_OPUS  },
            {AudioCodecID_ffaac                         , AV_CODEC_ID_AAC   },
            {AudioCodecID_ffmp3                         , AV_CODEC_ID_MP3   },
            {AkCompressedAudioCaps::AudioCodecID_unknown, AV_CODEC_ID_NONE  },
        };

        return ffmpegAudioCodecsTable;
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

    inline static const VideoCodecsTable *table()
    {
        static const VideoCodecsTable ffmpegVideoCodecsTable[] {
            {VideoCodecID_ffvp8                         , AV_CODEC_ID_VP8 },
            {VideoCodecID_ffvp9                         , AV_CODEC_ID_VP9 },
            {VideoCodecID_ffav1                         , AV_CODEC_ID_AV1 },
            {VideoCodecID_ffh264                        , AV_CODEC_ID_H264},
            {VideoCodecID_ffhevc                        , AV_CODEC_ID_HEVC},
            {AkCompressedVideoCaps::VideoCodecID_unknown, AV_CODEC_ID_NONE},
        };

        return ffmpegVideoCodecsTable;
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

struct Muxer
{
    const VideoMuxer *format;
    const AVOutputFormat *avFormat;
    AkPropertyOptions options;
};

class VideoMuxerFFmpegElementPrivate
{
    public:
        VideoMuxerFFmpegElement *self;
        QVector<Muxer> m_muxers;
        AVFormatContext *m_context {nullptr};
        QMutex m_mutex;
        bool m_initialized {false};
        bool m_paused {false};
        int64_t m_packetPos {0};
        AkElementPtr m_packetSync {akPluginManager->create<AkElement>("Utils/PacketSync")};

        explicit VideoMuxerFFmpegElementPrivate(VideoMuxerFFmpegElement *self);
        ~VideoMuxerFFmpegElementPrivate();
        void listMuxers();
        AkPropertyOption::OptionType optionType(AVOptionType avType) const;
        void readParameters(QByteArray &privateData,
                            const AVCodec **codec,
                            AVCodecParameters *codecpar);
        AVDictionary *readFormatOptions() const;
        bool init();
        void uninit();
        void packetReady(const AkPacket &packet);
};

VideoMuxerFFmpegElement::VideoMuxerFFmpegElement():
    AkVideoMuxer()
{
    this->d = new VideoMuxerFFmpegElementPrivate(this);
    this->d->listMuxers();
    this->setMuxer(this->muxers().value(0));
}

VideoMuxerFFmpegElement::~VideoMuxerFFmpegElement()
{
    this->d->uninit();
    delete this->d;
}

QStringList VideoMuxerFFmpegElement::muxers() const
{
    QStringList muxers;

    for (auto muxer = VideoMuxer::table(); muxer->muxer; ++muxer)
        muxers << muxer->muxer;

    return muxers;
}

AkVideoMuxer::FormatID VideoMuxerFFmpegElement::formatID(const QString &muxer) const
{
    auto it = std::find_if(this->d->m_muxers.constBegin(),
                           this->d->m_muxers.constEnd(),
                           [&muxer] (const Muxer &formatMuxer) -> bool {
        return formatMuxer.format->muxer == muxer;
    });

    if (it == this->d->m_muxers.constEnd())
        return AkVideoMuxer::FormatID_unknown;

    return it->format->formatID;
}

QString VideoMuxerFFmpegElement::description(const QString &muxer) const
{
    auto it = std::find_if(this->d->m_muxers.constBegin(),
                           this->d->m_muxers.constEnd(),
                           [&muxer] (const Muxer &formatMuxer) -> bool {
        return formatMuxer.format->muxer == muxer;
    });

    if (it == this->d->m_muxers.constEnd())
        return {};

    return {it->format->description};
}

QString VideoMuxerFFmpegElement::extension(const QString &muxer) const
{
    auto it = std::find_if(this->d->m_muxers.constBegin(),
                           this->d->m_muxers.constEnd(),
                           [&muxer] (const Muxer &formatMuxer) -> bool {
        return formatMuxer.format->muxer == muxer;
    });

    if (it == this->d->m_muxers.constEnd())
        return {};

    return {it->format->extension};
}

bool VideoMuxerFFmpegElement::gapsAllowed(AkCodecType type) const
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

QList<AkCodecID> VideoMuxerFFmpegElement::supportedCodecs(const QString &muxer,
                                                          AkCodecType type) const
{
    auto it = std::find_if(this->d->m_muxers.constBegin(),
                           this->d->m_muxers.constEnd(),
                           [&muxer] (const Muxer &formatMuxer) -> bool {
        return formatMuxer.format->muxer == muxer;
    });

    if (it == this->d->m_muxers.constEnd())
        return {};

    QList<AkCodecID> codecs;

    if (type == AkCompressedCaps::CapsType_Audio
        || type == AkCompressedCaps::CapsType_Unknown) {
        for (auto codec = it->format->audioCodecs;
             *codec != AkCompressedAudioCaps::AudioCodecID::AudioCodecID_unknown;
             ++codec)
            codecs << *codec;
    }

    if (type == AkCompressedCaps::CapsType_Video
        || type == AkCompressedCaps::CapsType_Unknown) {
        for (auto codec = it->format->videoCodecs;
             *codec != AkCompressedVideoCaps::VideoCodecID::VideoCodecID_unknown;
             ++codec)
            codecs << *codec;
    }

    return codecs;
 }

AkCodecID VideoMuxerFFmpegElement::defaultCodec(const QString &muxer,
                                                AkCodecType type) const
{
    auto codecs = this->supportedCodecs(muxer, type);

    if (codecs.isEmpty())
        return 0;

    return codecs.first();
}

AkPropertyOptions VideoMuxerFFmpegElement::options() const
{
    auto it = std::find_if(this->d->m_muxers.constBegin(),
                           this->d->m_muxers.constEnd(),
                           [this] (const Muxer &muxer) -> bool {
        return muxer.avFormat->name == this->muxer();
    });

    if (it == this->d->m_muxers.constEnd())
        return {};

    return it->options;
}

AkPacket VideoMuxerFFmpegElement::iStream(const AkPacket &packet)
{
    if (this->d->m_paused || !this->d->m_initialized || !this->d->m_packetSync)
        return {};

    return this->d->m_packetSync->iStream(packet);
}

bool VideoMuxerFFmpegElement::setState(ElementState state)
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

VideoMuxerFFmpegElementPrivate::VideoMuxerFFmpegElementPrivate(VideoMuxerFFmpegElement *self):
    self(self)
{
    if (this->m_packetSync)
        QObject::connect(this->m_packetSync.data(),
                         &AkElement::oStream,
                         [this] (const AkPacket &packet) {
                             this->packetReady(packet);
                         });
}

VideoMuxerFFmpegElementPrivate::~VideoMuxerFFmpegElementPrivate()
{

}

void VideoMuxerFFmpegElementPrivate::listMuxers()
{
    for (auto muxer = VideoMuxer::table(); muxer->muxer; ++muxer) {
        auto avFormat = av_guess_format(muxer->muxer,
                                        nullptr,
                                        muxer->mimeType);

        AkPropertyOptions options;
        QMap<QString, AkMenu> menu;
        QMap<QString, QString> units;

        if (avFormat && avFormat->priv_class)
            for (auto option = avFormat->priv_class->option;
                 option;
                 option = av_opt_next(&avFormat->priv_class, option)) {
                if (option->flags & AV_OPT_FLAG_DEPRECATED)
                    continue;
                auto optionType = this->optionType(option->type);

                if (optionType == AkPropertyOption::OptionType_Unknown)
                    continue;

                QVariant value;
                qreal step = 0.0;

                switch (option->type) {
                    case AV_OPT_TYPE_FLAGS:
                    case AV_OPT_TYPE_INT:
                    case AV_OPT_TYPE_INT64:
                    case AV_OPT_TYPE_CONST:
                    case AV_OPT_TYPE_DURATION:
                    case AV_OPT_TYPE_BOOL:
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(59, 17, 100)
                    case AV_OPT_TYPE_UINT:
#endif
                        value = qint64(option->default_val.i64);
                        step = 1.0;
                        break;
                    case AV_OPT_TYPE_DOUBLE:
                    case AV_OPT_TYPE_FLOAT:
                        value = option->default_val.dbl;
                        step = 0.01;
                        break;
                    case AV_OPT_TYPE_STRING:
                        value = option->default_val.str?
                                    QString(option->default_val.str):
                                    QString();
                        break;
                    case AV_OPT_TYPE_RATIONAL:
                        value = QString("%1/%2")
                                    .arg(option->default_val.q.num)
                                    .arg(option->default_val.q.den);
                        break;
                    default:
                        continue;
                }

                if (option->type == AV_OPT_TYPE_CONST) {
                    AkMenuOption menuOption(option->name,
                                            option->name,
                                            option->help,
                                            value);

                    if (menu.contains(option->unit))
                        menu[option->unit] << menuOption;
                    else
                        menu[option->unit] = {menuOption};
                } else {
                    options << AkPropertyOption(option->name,
                                                option->name,
                                                option->help,
                                                optionType,
                                                option->min,
                                                option->max,
                                                step,
                                                value,
                                                {});
                    units[option->name] = option->unit;
                }
            }

        for (auto &option: options)
            if (units.contains(option.name()))
                option = {option.name(),
                          option.description(),
                          option.help(),
                          option.type(),
                          option.min(),
                          option.max(),
                          option.step(),
                          option.defaultValue(),
                          menu[units[option.name()]]};

        this->m_muxers << Muxer {muxer, avFormat, options};
    }
}

AkPropertyOption::OptionType VideoMuxerFFmpegElementPrivate::optionType(AVOptionType avType) const
{
    static const struct
    {
        AVOptionType avType;
        AkPropertyOption::OptionType type;
    } ffmpegVideoMuxOptionTypes [] = {
        {AV_OPT_TYPE_BOOL    , AkPropertyOption::OptionType_Boolean},
        {AV_OPT_TYPE_CONST   , AkPropertyOption::OptionType_Number },
        {AV_OPT_TYPE_DOUBLE  , AkPropertyOption::OptionType_Number },
        {AV_OPT_TYPE_DURATION, AkPropertyOption::OptionType_Number },
        {AV_OPT_TYPE_FLAGS   , AkPropertyOption::OptionType_Flags  },
        {AV_OPT_TYPE_FLOAT   , AkPropertyOption::OptionType_Number },
        {AV_OPT_TYPE_INT     , AkPropertyOption::OptionType_Number },
        {AV_OPT_TYPE_INT64   , AkPropertyOption::OptionType_Number },
        {AV_OPT_TYPE_RATIONAL, AkPropertyOption::OptionType_Frac   },
        {AV_OPT_TYPE_STRING  , AkPropertyOption::OptionType_String },
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(59, 17, 100)
        {AV_OPT_TYPE_UINT    , AkPropertyOption::OptionType_Number },
#endif
        {AV_OPT_TYPE_UINT64  , AkPropertyOption::OptionType_Number },
        {AVOptionType(0)     , AkPropertyOption::OptionType_Unknown},
    };

    auto type = ffmpegVideoMuxOptionTypes;

    for (; type->type != AkPropertyOption::OptionType_Unknown; ++type)
        if (type->avType == avType)
            return type->type;

    return type->type;
}

void VideoMuxerFFmpegElementPrivate::readParameters(QByteArray &privateData,
                                                   const AVCodec **codec,
                                                   AVCodecParameters *codecpar)
{
    QDataStream ds(&privateData, QIODeviceBase::ReadOnly);
    quint64 codecPtr = 0;
    ds >> codecPtr;
    *codec = reinterpret_cast<const AVCodec *>(codecPtr);
    ds.readRawData(reinterpret_cast<char *>(codecpar),
                   sizeof(AVCodecParameters));
}

AVDictionary *VideoMuxerFFmpegElementPrivate::readFormatOptions() const
{
    auto it = std::find_if(this->m_muxers.constBegin(),
                           this->m_muxers.constEnd(),
                           [this] (const Muxer &muxer) -> bool {
        return muxer.format->muxer == self->muxer();
    });

    if (it == this->m_muxers.constEnd())
        return nullptr;

    AVDictionary *options = nullptr;

    for (auto &option: it->options) {
        if (!self->isOptionSet(option.name()))
            continue;

        auto value = self->optionValue(option.name()).toString();
        av_dict_set(&options,
                    option.name().toStdString().c_str(),
                    value.isEmpty()? nullptr: value.toStdString().c_str(),
                    0);
    }

    return options;
}

bool VideoMuxerFFmpegElementPrivate::init()
{
    this->uninit();

    if (!this->m_packetSync)
        return false;

    auto fit = std::find_if(this->m_muxers.begin(),
                            this->m_muxers.end(),
                            [this] (const Muxer &muxer) -> bool {
        return muxer.format->muxer == self->muxer();
    });

    if (fit == this->m_muxers.constEnd()) {
        qCritical() << "Invalid muxer:" << self->muxer();

        return false;
    }

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
    this->m_context = nullptr;
    auto result =
            avformat_alloc_output_context2(&this->m_context,
                                           fit->avFormat,
                                           fit->format->extension,
                                           location.toStdString().c_str());

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);
        qCritical() << "Can't allocate the context:" << error;

        return false;
    }

    result = avio_open(&this->m_context->pb,
                       location.toStdString().c_str(),
                       AVIO_FLAG_READ_WRITE);

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);
        qCritical() << "Failed to open file:" << error;
        avformat_free_context(this->m_context);
        this->m_context = nullptr;

        return false;
    }

    // Add the video track to the muxer

    auto videoHeaders =
            self->streamHeaders(AkCompressedCaps::CapsType_Video);
    const AVCodec *codec = nullptr;
    AVCodecParameters parameters;
    this->readParameters(videoHeaders,
                         &codec,
                         &parameters);
    auto videoStream = avformat_new_stream(this->m_context, codec);

    if (!videoStream) {
        qCritical() << "Failed to add video stream";
        avformat_free_context(this->m_context);
        this->m_context = nullptr;

        return false;
    }

    videoStream->index = 0;
    videoStream->id = 0;
    avcodec_parameters_copy(videoStream->codecpar, &parameters);

    // Add the audio track to the muxer

    if (audioCaps) {
        auto audioHeaders =
                self->streamHeaders(AkCompressedCaps::CapsType_Audio);
        const AVCodec *codec = nullptr;
        AVCodecParameters parameters;
        this->readParameters(audioHeaders,
                             &codec,
                             &parameters);
        auto audioStream = avformat_new_stream(this->m_context, codec);

        if (!audioStream) {
            qCritical() << "Failed to add audio stream";
            avformat_free_context(this->m_context);
            this->m_context = nullptr;

            return false;
        }

        audioStream->index = 1;
        audioStream->id = 1;
        avcodec_parameters_copy(audioStream->codecpar, &parameters);
    }

    // Initialze the muxer

    auto options = this->readFormatOptions();
    result = avformat_init_output(this->m_context, &options);
    av_dict_free(&options);

    if (result < 1) {
        qCritical() << "Failed initializing the context";
        avformat_free_context(this->m_context);
        this->m_context = nullptr;
        av_dict_free(&options);

        return false;
    }

    // Write the format headers

    result = avformat_write_header(this->m_context, &options);

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);
        qCritical() << "Error writting the file header:" << error;
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

    qInfo() << "Starting FFmpeg muxing";
    this->m_initialized = true;

    return true;
}

void VideoMuxerFFmpegElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;
    this->m_packetSync->setState(AkElement::ElementStateNull);
    auto result = av_write_trailer(this->m_context);

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);
        qCritical() << "Failed writting the trailer:" << error;
    }

    avio_close(this->m_context->pb);
    avformat_free_context(this->m_context);
    this->m_context = nullptr;

    this->m_paused = false;
}

void VideoMuxerFFmpegElementPrivate::packetReady(const AkPacket &packet)
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
    auto result = av_interleaved_write_frame(this->m_context, avPacket);

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);

        if (isAudio)
            qCritical() << "Failed to write the audio packet:" << error;
        else
            qCritical() << "Failed to write the video packet:" << error;
    }

    av_packet_free(&avPacket);
    this->m_packetPos++;
}

#include "moc_videomuxerffmpegelement.cpp"

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
#include <akfrac.h>
#include <akaudiocaps.h>
#include <akaudiopacket.h>
#include <akcompressedaudiocaps.h>
#include <akcompressedaudiopacket.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavutil/parseutils.h>
}

#include "audioencoderffmpegelement.h"

#define CODEC_COMPLIANCE FF_COMPLIANCE_VERY_STRICT

#define AudioCodecID_ffopus   AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xFF, 'O', 'P', 'U'))
#define AudioCodecID_ffvorbis AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xFF, 'V', 'O', 'R'))
#define AudioCodecID_ffaac    AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xFF, 'A', 'A', 'C'))
#define AudioCodecID_ffmp3    AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xFF, 'M', 'P', '3'))

struct FFmpegCodecs
{
    AkAudioEncoderCodecID codecID;
    AVCodecID ffCodecID;

    static inline const FFmpegCodecs *table()
    {
        static const FFmpegCodecs ffmpegAudioEncCodecsTable[] = {
            {AudioCodecID_ffvorbis                      , AV_CODEC_ID_VORBIS},
            {AudioCodecID_ffopus                        , AV_CODEC_ID_OPUS  },
            {AudioCodecID_ffaac                         , AV_CODEC_ID_AAC   },
            {AudioCodecID_ffmp3                         , AV_CODEC_ID_MP3   },
            {AkCompressedAudioCaps::AudioCodecID_unknown, AV_CODEC_ID_NONE  },
        };

        return ffmpegAudioEncCodecsTable;
    }

    static inline QList<AVCodecID> ffCodecs()
    {
        QList<AVCodecID> ffCodecs;

        for (auto codec = table();
             codec->codecID != AkCompressedAudioCaps::AudioCodecID_unknown;
             codec++) {
            ffCodecs << codec->ffCodecID;
        }

        return ffCodecs;
    }

    static inline const FFmpegCodecs *byCodecID(AkAudioEncoderCodecID codecID)
    {
        auto codec = table();

        for (;
             codec->codecID != AkCompressedAudioCaps::AudioCodecID_unknown;
             codec++) {
            if (codec->codecID == codecID)
                return codec;
        }

        return codec;
    }

    static inline const FFmpegCodecs *byFFCodecID(AVCodecID ffCodecID)
    {
        auto codec = table();

        for (;
             codec->codecID != AkCompressedAudioCaps::AudioCodecID_unknown;
             codec++) {
            if (codec->ffCodecID == ffCodecID)
                return codec;
        }

        return codec;
    }
};

using SampleFormatPair = QPair<AkAudioCaps::SampleFormat, bool>;

struct CodecInfo
{
    QString name;
    QString description;
    AkAudioEncoderCodecID codecID;
    QVector<SampleFormatPair> formats;
    QVector<int> channels;
    QVector<int> sampleRates;
    AkPropertyOptions options;
};

struct SampleFormatsTable
{
    AkAudioCaps::SampleFormat format;
    bool planar;
    AVSampleFormat ffFormat;

    inline static const SampleFormatsTable *table()
    {
        static const SampleFormatsTable ffmpegAudioEncFormatsTable[] {
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

        return ffmpegAudioEncFormatsTable;
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

    inline static const SampleFormatsTable *byFFFormat(AVSampleFormat format)
    {
        auto item = table();

        for (; item->format; ++item)
            if (item->ffFormat == format)
                return item;

        return item;
    }

    inline static QVector<AVSampleFormat> supportedFFSampleFormats()
    {
        QVector<AVSampleFormat> formats;
        auto item = table();

        for (; item->format; ++item)
            formats << item->ffFormat;

        return formats;
    }
};

using AVCodecParametersPtr = QSharedPointer<AVCodecParameters>;

class AudioEncoderFFmpegElementPrivate
{
    public:
        AudioEncoderFFmpegElement *self;
        AkCompressedAudioCaps m_outputCaps;
        bool m_globalHeaders {true};
        QByteArray m_headers;
        AVCodecParametersPtr m_codecParameters;
        QVector<CodecInfo> m_codecs;
        AVCodecContext *m_context {nullptr};
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};
        bool m_paused {false};
        qint64 m_encodedTimePts {0};
        AkElementPtr m_fillAudioGaps {akPluginManager->create<AkElement>("AudioFilter/FillAudioGaps")};

        explicit AudioEncoderFFmpegElementPrivate(AudioEncoderFFmpegElement *self);
        ~AudioEncoderFFmpegElementPrivate();
        bool isAvailable(const QString &codec) const;
        void listCodecs();
        void adjustDefaults();
        AkPropertyOption::OptionType optionType(AVOptionType avType) const;
        AVDictionary *readCodecOptions() const;
        int nearestChannels(const QVector<int> &supportedChannels, int channels) const;
        int nearestSampleRate(const QVector<int> &sampleRates, int rate) const;
        bool init();
        void uninit();
        void updateHeaders();
        void updateOutputCaps();
        void encodeFrame(const AkAudioPacket &src);
        void sendFrame(const AVPacket *avPacket) const;
};

AudioEncoderFFmpegElement::AudioEncoderFFmpegElement():
    AkAudioEncoder()
{
    this->d = new AudioEncoderFFmpegElementPrivate(this);
    this->d->listCodecs();
    this->d->adjustDefaults();
    auto encoder = avcodec_find_encoder(FFmpegCodecs::ffCodecs().first());

    if (encoder)
        this->setCodec(encoder->name);

    QObject::connect(this,
                     &AkAudioEncoder::codecChanged,
                     [this] () {
        this->d->updateOutputCaps();
        emit this->optionsChanged(this->options());
    });
}

AudioEncoderFFmpegElement::~AudioEncoderFFmpegElement()
{
    this->d->uninit();
    delete this->d;
}

QStringList AudioEncoderFFmpegElement::codecs() const
{
    QStringList codecs;

    for (auto &codec: this->d->m_codecs)
        codecs << codec.name;

    return codecs;
}

AkAudioEncoderCodecID AudioEncoderFFmpegElement::codecID(const QString &codec) const
{
    auto it = std::find_if(this->d->m_codecs.constBegin(),
                           this->d->m_codecs.constEnd(),
                           [&codec] (const CodecInfo &codecInfo) -> bool {
        return codecInfo.name == codec;
    });

    if (it == this->d->m_codecs.constEnd())
        return AkCompressedAudioCaps::AudioCodecID_unknown;

    return it->codecID;
}

QString AudioEncoderFFmpegElement::codecDescription(const QString &codec) const
{
    auto it = std::find_if(this->d->m_codecs.constBegin(),
                           this->d->m_codecs.constEnd(),
                           [&codec] (const CodecInfo &codecInfo) -> bool {
        return codecInfo.name == codec;
    });

    if (it == this->d->m_codecs.constEnd())
        return {};

    return it->description;
}

AkCompressedAudioCaps AudioEncoderFFmpegElement::outputCaps() const
{
    return this->d->m_outputCaps;
}

QByteArray AudioEncoderFFmpegElement::headers() const
{
    return this->d->m_headers;
}

qint64 AudioEncoderFFmpegElement::encodedTimePts() const
{
    return this->d->m_encodedTimePts;
}

bool AudioEncoderFFmpegElement::globalHeaders() const
{
    return this->d->m_globalHeaders;
}

AkPropertyOptions AudioEncoderFFmpegElement::options() const
{
    auto it = std::find_if(this->d->m_codecs.constBegin(),
                           this->d->m_codecs.constEnd(),
                           [this] (const CodecInfo &codec) -> bool {
        return codec.name == this->codec();
    });

    if (it == this->d->m_codecs.constEnd())
        return {};

    return it->options;
}

AkPacket AudioEncoderFFmpegElement::iAudioStream(const AkAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (this->d->m_paused
        || !this->d->m_initialized
        || !this->d->m_fillAudioGaps)
        return {};

    this->d->m_fillAudioGaps->iStream(packet);

    return {};
}

void AudioEncoderFFmpegElement::setGlobalHeaders(bool globalHeaders)
{
    if (globalHeaders == this->d->m_globalHeaders)
        return;

    this->d->m_globalHeaders = globalHeaders;
    emit this->globalHeadersChanged(globalHeaders);
}

void AudioEncoderFFmpegElement::resetGlobalHeaders()
{
    this->setGlobalHeaders(true);
}

bool AudioEncoderFFmpegElement::setState(ElementState state)
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

AudioEncoderFFmpegElementPrivate::AudioEncoderFFmpegElementPrivate(AudioEncoderFFmpegElement *self):
    self(self)
{
    if (this->m_fillAudioGaps)
        QObject::connect(this->m_fillAudioGaps.data(),
                         &AkElement::oStream,
                         [this] (const AkPacket &packet) {
                             this->encodeFrame(packet);
                         });

    QObject::connect(self,
                     &AkAudioEncoder::inputCapsChanged,
                     [this] (const AkAudioCaps &inputCaps) {
                        Q_UNUSED(inputCaps)

                        this->updateOutputCaps();
                     });
}

AudioEncoderFFmpegElementPrivate::~AudioEncoderFFmpegElementPrivate()
{

}

bool AudioEncoderFFmpegElementPrivate::isAvailable(const QString &codec) const
{
    static struct
    {
        const char *codec;
        bool isAvailable;
    } ffmpegAudioEncAvailableCodecs[16];
    static size_t ffmpegAudioEncAvailableCodecsSize = 0;

    for (size_t i = 0; i < ffmpegAudioEncAvailableCodecsSize; ++i)
        if (ffmpegAudioEncAvailableCodecs[i].codec == codec)
            return ffmpegAudioEncAvailableCodecs[i].isAvailable;

    auto encoder =
            avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!encoder)
        return false;

    bool isAvailable = false;
    auto context = avcodec_alloc_context3(encoder);

    if (context) {
        const AVSampleFormat *avFormats = nullptr;
        int nFormats = 0;
        const AVChannelLayout *avLayouts = nullptr;
        int nLayouts = 0;
        const int *avSampleRates = nullptr;
        int nSampleRates = 0;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(61, 13, 100)
        avcodec_get_supported_config(nullptr,
                                     encoder,
                                     AV_CODEC_CONFIG_SAMPLE_FORMAT,
                                     0,
                                     reinterpret_cast<const void **>(&avFormats),
                                     &nFormats);
        avcodec_get_supported_config(nullptr,
                                     encoder,
                                     AV_CODEC_CONFIG_CHANNEL_LAYOUT,
                                     0,
                                     reinterpret_cast<const void **>(&avLayouts),
                                     &nLayouts);
        avcodec_get_supported_config(nullptr,
                                     encoder,
                                     AV_CODEC_CONFIG_SAMPLE_RATE,
                                     0,
                                     reinterpret_cast<const void **>(&avSampleRates),
                                     &nSampleRates);
#else
        avFormats = encoder->sample_fmts;

        for (auto fmt = encoder->sample_fmts; fmt && *fmt != AV_SAMPLE_FMT_NONE; ++fmt)
            ++nFormats;

        avLayouts = encoder->ch_layouts;

        for (auto lyt = encoder->ch_layouts; lyt && lyt->nb_channels != 0; ++lyt)
            ++nLayouts;

        avSampleRates = encoder->supported_samplerates;

        for (auto rate = encoder->supported_samplerates; rate && *rate != 0; ++rate)
            ++nSampleRates;
#endif

        context->sample_fmt = nFormats > 0? *avFormats: AV_SAMPLE_FMT_S16;
        av_channel_layout_default(&context->ch_layout,
                                  nLayouts > 0? avLayouts->nb_channels: 2);
        context->sample_rate = nSampleRates > 0? *avSampleRates: 48000;
        context->time_base = {1, context->sample_rate};
        context->bit_rate = 128000;

        isAvailable = avcodec_open2(context, encoder, nullptr) >= 0;
        avcodec_free_context(&context);
    }

    auto i = ffmpegAudioEncAvailableCodecsSize++;
    ffmpegAudioEncAvailableCodecs[i].codec = encoder->name;
    ffmpegAudioEncAvailableCodecs[i].isAvailable = isAvailable;

    return isAvailable;
}

void AudioEncoderFFmpegElementPrivate::listCodecs()
{
    qInfo() << "Listing the available audio codecs";

    auto supportedFormats = SampleFormatsTable::supportedFFSampleFormats();
    void *opaqueCdc = nullptr;
    auto supportedCodecs = FFmpegCodecs::ffCodecs();

    while (auto codec = av_codec_iterate(&opaqueCdc)) {
        if (codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL
            && CODEC_COMPLIANCE > FF_COMPLIANCE_EXPERIMENTAL)
            continue;

        if (!supportedCodecs.contains(codec->id) || !av_codec_is_encoder(codec))
            continue;

        if (!this->isAvailable(codec->name))
            continue;

        const AVSampleFormat *avFormats = nullptr;
        int nFormats = 0;
        const AVChannelLayout *avChannelLayouts = nullptr;
        int nChannelLayouts = 0;
        const int *avSampleRates = nullptr;
        int nSampleRates = 0;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(61, 13, 100)
        avcodec_get_supported_config(nullptr,
                                     codec,
                                     AV_CODEC_CONFIG_SAMPLE_FORMAT,
                                     0,
                                     reinterpret_cast<const void **>(&avFormats),
                                     &nFormats);
        avcodec_get_supported_config(nullptr,
                                     codec,
                                     AV_CODEC_CONFIG_CHANNEL_LAYOUT,
                                     0,
                                     reinterpret_cast<const void **>(&avChannelLayouts),
                                     &nChannelLayouts);
        avcodec_get_supported_config(nullptr,
                                     codec,
                                     AV_CODEC_CONFIG_SAMPLE_RATE,
                                     0,
                                     reinterpret_cast<const void **>(&avSampleRates),
                                     &nSampleRates);
#else
        avFormats = codec->sample_fmts;

        for (auto fmt = codec->sample_fmts; fmt && *fmt != AV_SAMPLE_FMT_NONE; ++fmt)
            ++nFormats;

        avChannelLayouts = codec->ch_layouts;

        for (auto lyt = codec->ch_layouts; lyt && lyt->nb_channels != 0; ++lyt)
            ++nChannelLayouts;

        avSampleRates = codec->supported_samplerates;

        for (auto rate = codec->supported_samplerates; rate && *rate != 0; ++rate)
            ++nSampleRates;
#endif

        QVector<SampleFormatPair> formats;

        for (int i = 0; i < nFormats; i++)
            if (supportedFormats.contains(avFormats[i])) {
                auto format = SampleFormatsTable::byFFFormat(avFormats[i]);
                formats << SampleFormatPair(format->format, format->planar);
            }

        if (formats.isEmpty())
            continue;

        QVector<int> channels;

        for (int i = 0; i < nChannelLayouts; i++)
            if (avChannelLayouts[i].nb_channels >= 1
                && avChannelLayouts[i].nb_channels <= 2
                && !channels.contains(avChannelLayouts[i].nb_channels))
                channels << avChannelLayouts[i].nb_channels;

        QVector<int> sampleRates;

        for (int i = 0; i < nSampleRates; i++)
            sampleRates << avSampleRates[i];

        AkPropertyOptions options;
        QMap<QString, AkMenu> menu;
        QMap<QString, QString> units;

        if (codec->priv_class)
            for (auto option = codec->priv_class->option;
                 option;
                 option = av_opt_next(&codec->priv_class, option)) {
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

        QString description = QString(codec->long_name).isEmpty()?
                                QString(codec->name):
                                QString(codec->long_name);
        this->m_codecs << CodecInfo {QString(codec->name),
                                     description,
                                     FFmpegCodecs::byFFCodecID(codec->id)->codecID,
                                     formats,
                                     channels,
                                     sampleRates,
                                     options};
    }

    qInfo() << "Audio codecs found:";

    for (auto &info: this->m_codecs)
        qInfo() << "    " << info.name;
}

void AudioEncoderFFmpegElementPrivate::adjustDefaults()
{
}

AkPropertyOption::OptionType AudioEncoderFFmpegElementPrivate::optionType(AVOptionType avType) const
{
    static const struct
    {
        AVOptionType avType;
        AkPropertyOption::OptionType type;
    } ffmpegAudioEncCodecOptionTypes [] = {
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

    auto type = ffmpegAudioEncCodecOptionTypes;

    for (; type->type != AkPropertyOption::OptionType_Unknown; ++type)
        if (type->avType == avType)
            return type->type;

    return type->type;
}

AVDictionary *AudioEncoderFFmpegElementPrivate::readCodecOptions() const
{
    AVDictionary *options = nullptr;

    auto codec
            = std::find_if(this->m_codecs.constBegin(),
                           this->m_codecs.constEnd(),
                           [this] (const CodecInfo &codec) -> bool {
        return codec.name == self->codec();
    });

    if (codec == this->m_codecs.constEnd())
        return nullptr;

    for (auto &option: codec->options) {
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

int AudioEncoderFFmpegElementPrivate::nearestChannels(const QVector<int> &supportedChannels,
                                                      int channels) const
{
    if (supportedChannels.isEmpty())
        return qBound(1, channels, 2);

    int nearest = channels;
    quint64 q = std::numeric_limits<quint64>::max();

    for (auto &schannels: supportedChannels) {
        quint64 k = qAbs(schannels - channels);

        if (k < q) {
            nearest = schannels;
            q = k;
        }
    }

    return nearest;
}

int AudioEncoderFFmpegElementPrivate::nearestSampleRate(const QVector<int> &sampleRates,
                                                        int rate) const
{
    if (sampleRates.isEmpty())
        return rate;

    int nearest = rate;
    quint64 q = std::numeric_limits<quint64>::max();

    for (auto &srate: sampleRates) {
        quint64 k = qAbs(srate - rate);

        if (k < q) {
            nearest = srate;
            q = k;
        }
    }

    return nearest;
}

bool AudioEncoderFFmpegElementPrivate::init()
{
    this->uninit();

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    auto encoder =
            avcodec_find_encoder_by_name(self->codec().toStdString().c_str());

    if (!encoder) {
        qCritical() << "Encoder not found";

        return false;
    }

    this->m_context = avcodec_alloc_context3(encoder);

    if (!this->m_context) {
        qCritical() << "Context not created";

        return false;
    }

    this->m_context->sample_fmt =
            SampleFormatsTable::byFormat(this->m_outputCaps.rawCaps().format(),
                                         this->m_outputCaps.rawCaps().planar())->ffFormat;
    av_channel_layout_default(&this->m_context->ch_layout,
                              this->m_outputCaps.rawCaps().channels());
    this->m_context->sample_rate = this->m_outputCaps.rawCaps().rate();
    this->m_context->time_base = {this->m_context->framerate.den,
                                  this->m_context->framerate.num};
    this->m_context->bit_rate = self->bitrate();

    if (this->m_globalHeaders)
        this->m_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    auto options = this->readCodecOptions();
    auto result = avcodec_open2(this->m_context, encoder, &options);
    av_dict_free(&options);

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);
        qCritical() << "Failed to open the codec:" << error;

        return false;
    }

    this->updateHeaders();

    if (this->m_fillAudioGaps) {
        this->m_fillAudioGaps->setProperty("fillGaps", self->fillGaps());
        this->m_fillAudioGaps->setProperty("outputSamples",
                                           this->m_context->frame_size);
        this->m_fillAudioGaps->setState(AkElement::ElementStatePlaying);
    }

    this->m_encodedTimePts = 0;
    this->m_initialized = true;

    return true;
}

void AudioEncoderFFmpegElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    if (this->m_fillAudioGaps)
        this->m_fillAudioGaps->setState(AkElement::ElementStateNull);

    auto result = avcodec_send_frame(this->m_context, nullptr);

    if (result >= 0) {
        auto packet = av_packet_alloc();

        while (avcodec_receive_packet(this->m_context, packet) >= 0)
            this->sendFrame(packet);

        av_packet_free(&packet);
    } else {
        char error[1024];
        av_strerror(result, error, 1024);
        qCritical() << "Failed to encode the frame:" << error;
    }

    avcodec_free_context(&this->m_context);
    this->m_context = nullptr;

    this->m_codecParameters.clear();

    this->m_paused = false;
}

void AudioEncoderFFmpegElementPrivate::updateHeaders()
{
    QByteArray headers;
    QDataStream ds(&headers, QIODeviceBase::WriteOnly);
    ds << quint64(this->m_context->codec);
    this->m_codecParameters =
            AVCodecParametersPtr(avcodec_parameters_alloc(),
                                 [] (AVCodecParameters *parameters) {
        avcodec_parameters_free(&parameters);
    });
    auto result =
            avcodec_parameters_from_context(this->m_codecParameters.data(),
                                            this->m_context);

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);
        qCritical() << "Failed to read the codec parameters:" << error;

        return;
    }
    ds.writeRawData(reinterpret_cast<char *>(this->m_codecParameters.data()),
                    sizeof(AVCodecParameters));

    if (this->m_headers == headers)
        return;

    this->m_headers = headers;
    emit self->headersChanged(headers);
}

void AudioEncoderFFmpegElementPrivate::updateOutputCaps()
{
    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = AkCompressedAudioCaps();
        emit self->outputCapsChanged(this->m_outputCaps);

        return;
    }

    auto codecID = self->codecID(self->codec());

    if (codecID == AkCompressedAudioCaps::AudioCodecID_unknown) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = AkCompressedAudioCaps();
        emit self->outputCapsChanged(this->m_outputCaps);

        return;
    }

    auto it = std::find_if(this->m_codecs.constBegin(),
                           this->m_codecs.constEnd(),
                           [this] (const CodecInfo &codec) -> bool {
        return codec.name == self->codec();
    });

    if (it == this->m_codecs.constEnd()) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = AkCompressedAudioCaps();
        emit self->outputCapsChanged(this->m_outputCaps);

        return;
    }

    AkAudioCaps::SampleFormat format = AkAudioCaps::SampleFormat_none;
    bool isPlanar = false;
    SampleFormatPair formatPair(inputCaps.format(), inputCaps.planar());

    if (it->formats.contains(formatPair)) {
        format = inputCaps.format();
        isPlanar = inputCaps.planar();
    } else {
        format = it->formats.first().first;
        isPlanar = it->formats.first().second;
    }

    int channels = this->nearestChannels(it->channels, inputCaps.channels());
    int rate = this->nearestSampleRate(it->sampleRates, inputCaps.rate());
    AkAudioCaps rawCaps(format,
                        AkAudioCaps::defaultChannelLayout(channels),
                        isPlanar,
                        rate);
    AkCompressedAudioCaps outputCaps(codecID, rawCaps);

    if (this->m_fillAudioGaps)
        this->m_fillAudioGaps->setProperty("outputCaps",
                                           QVariant::fromValue(rawCaps));

    if (this->m_outputCaps == outputCaps)
        return;

    this->m_outputCaps = outputCaps;
    emit self->outputCapsChanged(outputCaps);
}

void AudioEncoderFFmpegElementPrivate::encodeFrame(const AkAudioPacket &src)
{
    this->m_id = src.id();
    this->m_index = src.index();

    // Write the current frame.
    auto frame = av_frame_alloc();

    for (int plane = 0; plane < src.planes(); ++plane) {
        frame->linesize[plane] = src.planeSize(plane);
        frame->data[plane] = const_cast<quint8 *>(src.constPlane(plane));
    }

    frame->format = this->m_context->sample_fmt;
    frame->ch_layout = this->m_context->ch_layout;
    frame->sample_rate = this->m_context->sample_rate;
    frame->nb_samples = src.samples();
    frame->pts = src.pts();

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 30, 100)
    frame->duration = src.duration();
#else
    frame->pkt_duration = src.duration();
#endif

    frame->time_base = {int(src.timeBase().num()), int(src.timeBase().den())};

    auto result = avcodec_send_frame(this->m_context, frame);
    av_frame_free(&frame);

    if (result >= 0) {
        auto packet = av_packet_alloc();

        while (avcodec_receive_packet(this->m_context, packet) >= 0)
            this->sendFrame(packet);

        av_packet_free(&packet);
    } else {
        char error[1024];
        av_strerror(result, error, 1024);
        qCritical() << "Failed to encode the frame:" << error;
    }

    this->m_encodedTimePts = src.pts() + src.duration();
    emit self->encodedTimePtsChanged(this->m_encodedTimePts);
}

void AudioEncoderFFmpegElementPrivate::sendFrame(const AVPacket *avPacket) const
{
    AkCompressedAudioPacket packet(this->m_outputCaps, avPacket->size);
    memcpy(packet.data(), avPacket->data, packet.size());
    packet.setFlags(avPacket->flags & AV_PKT_FLAG_KEY?
                        AkCompressedAudioPacket::AudioPacketTypeFlag_KeyFrame:
                        AkCompressedAudioPacket::AudioPacketTypeFlag_None);
    packet.setPts(avPacket->pts);
    packet.setDts(avPacket->dts);
    packet.setDuration(avPacket->duration);
    packet.setTimeBase({1, this->m_outputCaps.rawCaps().rate()});
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);
}

#include "moc_audioencoderffmpegelement.cpp"

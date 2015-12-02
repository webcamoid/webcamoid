/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <limits>

#include "mediasink.h"

typedef QMap<AVMediaType, QString> AvMediaTypeStrMap;

inline AvMediaTypeStrMap initAvMediaTypeStrMap()
{
    AvMediaTypeStrMap mediaTypeToStr;
    mediaTypeToStr[AVMEDIA_TYPE_UNKNOWN] = "unknown/x-raw";
    mediaTypeToStr[AVMEDIA_TYPE_VIDEO] = "video/x-raw";
    mediaTypeToStr[AVMEDIA_TYPE_AUDIO] = "audio/x-raw";
    mediaTypeToStr[AVMEDIA_TYPE_DATA] = "data/x-raw";
    mediaTypeToStr[AVMEDIA_TYPE_SUBTITLE] = "text/x-raw";
    mediaTypeToStr[AVMEDIA_TYPE_ATTACHMENT] = "attachment/x-raw";
    mediaTypeToStr[AVMEDIA_TYPE_NB] = "nb/x-raw";

    return mediaTypeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(AvMediaTypeStrMap, mediaTypeToStr, (initAvMediaTypeStrMap()))

MediaSink::MediaSink(QObject *parent): QObject(parent)
{
    av_register_all();
    avcodec_register_all();

    this->m_formatContext = NULL;

    QObject::connect(this,
                     &MediaSink::outputFormatChanged,
                     this,
                     &MediaSink::updateStreams);
}

MediaSink::~MediaSink()
{
    this->uninit();
}

QString MediaSink::location() const
{
    return this->m_location;
}

QString MediaSink::outputFormat() const
{
    return this->m_outputFormat;
}

QVariantList MediaSink::streams() const
{
    QVariantList streams;

    foreach (QVariantMap stream, this->m_streamConfigs)
        streams << stream;

    return streams;
}

QStringList MediaSink::supportedFormats()
{
    QStringList formats;
    AVOutputFormat *outputFormat = NULL;

    while ((outputFormat = av_oformat_next(outputFormat))) {
        QString format(outputFormat->name);

        if (!formats.contains(format))
            formats << format;
    }

    return formats;
}

QStringList MediaSink::fileExtensions(const QString &format)
{
    AVOutputFormat *outputFormat = av_guess_format(format.toStdString().c_str(),
                                                   NULL,
                                                   NULL);

    if (!outputFormat)
        return QStringList();

    QString extensions(outputFormat->extensions);

    if (extensions.isEmpty())
        return QStringList();

    return extensions.split(",");
}

QString MediaSink::formatDescription(const QString &format)
{
    AVOutputFormat *outputFormat = av_guess_format(format.toStdString().c_str(),
                                                   NULL,
                                                   NULL);

    if (!outputFormat)
        return QString();

    return QString(outputFormat->long_name);
}

QStringList MediaSink::supportedCodecs(const QString &format,
                                       const QString &type)
{
    AVOutputFormat *outputFormat = av_guess_format(format.toStdString().c_str(),
                                                   NULL,
                                                   NULL);

    if (!outputFormat)
        return QStringList();

    QStringList codecs;
    AVCodec *codec = NULL;

    while ((codec = av_codec_next(codec))) {
        if ((type.isEmpty() || mediaTypeToStr->value(codec->type) == type)
            && av_codec_is_encoder(codec)
            && avformat_query_codec(outputFormat,
                                    codec->id,
                                    FF_COMPLIANCE_NORMAL) > 0)
            codecs << QString(codec->name);
    }

    return codecs;
}

QString MediaSink::defaultCodec(const QString &format, const QString &type)
{
    AVOutputFormat *outputFormat = av_guess_format(format.toStdString().c_str(),
                                                   NULL,
                                                   NULL);

    if (!outputFormat)
        return QString();

    AVCodecID codecId = type == "audio/x-raw"?
                            outputFormat->audio_codec:
                        type == "video/x-raw"?
                            outputFormat->video_codec:
                        type == "text/x-raw"?
                            outputFormat->subtitle_codec:
                            AV_CODEC_ID_NONE;

    if (codecId == AV_CODEC_ID_NONE)
        return QString();

    AVCodec *codec = avcodec_find_encoder(codecId);

    return QString(codec->name);
}

QString MediaSink::codecDescription(const QString &codec)
{
    AVCodec *avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return QString();

    return QString(avCodec->long_name);
}

QString MediaSink::codecType(const QString &codec)
{
    AVCodec *avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return QString();

    switch (avCodec->type) {
    case AVMEDIA_TYPE_AUDIO:
        return QString("audio/x-raw");
    case AVMEDIA_TYPE_VIDEO:
        return QString("video/x-raw");
    case AVMEDIA_TYPE_SUBTITLE:
        return QString("text/x-raw");
    default:
        break;
    }

    return QString();
}

QVariantMap MediaSink::defaultCodecParams(const QString &codec)
{
    AVCodec *avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return QVariantMap();

    QVariantMap codecParams;
    AVCodecContext *codecContext = avcodec_alloc_context3(avCodec);

    if (avCodec->type == AVMEDIA_TYPE_AUDIO) {
        QVariantList supportedSampleRates;

        if (avCodec->supported_samplerates)
            for (int i = 0; int sampleRate = avCodec->supported_samplerates[i]; i++)
                supportedSampleRates << sampleRate;

        QStringList supportedSampleFormats;

        if (avCodec->sample_fmts)
            for (int i = 0; ; i++) {
                AVSampleFormat sampleFormat = avCodec->sample_fmts[i];

                if (sampleFormat == AV_SAMPLE_FMT_NONE)
                    break;

                supportedSampleFormats << QString(av_get_sample_fmt_name(sampleFormat));
            }

        QStringList supportedChannelLayouts;
        char layout[1024];

        if (avCodec->channel_layouts)
            for (int i = 0; uint64_t channelLayout = avCodec->channel_layouts[i]; i++) {
                int channels = av_get_channel_layout_nb_channels(channelLayout);
                av_get_channel_layout_string(layout, 1024, channels, channelLayout);
                supportedChannelLayouts << QString(layout);
            }

        codecParams["supportedSampleRates"] = supportedSampleRates;
        codecParams["supportedSampleFormats"] = supportedSampleFormats;
        codecParams["supportedChannelLayouts"] = supportedChannelLayouts;
        codecParams["defaultSampleFormat"] = codecContext->sample_fmt != AV_SAMPLE_FMT_NONE?
                                                QString(av_get_sample_fmt_name(codecContext->sample_fmt)):
                                                supportedSampleFormats.value(0, "s16");
        codecParams["defaultBitRate"] = codecContext->bit_rate?
                                            codecContext->bit_rate: 128000;
        codecParams["defaultSampleRate"] = codecContext->sample_rate?
                                               codecContext->sample_rate:
                                               supportedSampleRates.value(0, 44100);

        int channels = av_get_channel_layout_nb_channels(codecContext->channel_layout);
        av_get_channel_layout_string(layout, 1024, channels, codecContext->channel_layout);

        QString channelLayout = codecContext->channel_layout?
                                    QString(layout):
                                    supportedChannelLayouts.value(0, "mono");

        codecParams["defaultChannelLayout"] = channelLayout;

        int channelsCount = av_get_channel_layout_nb_channels(av_get_channel_layout(channelLayout.toStdString().c_str()));
;
        codecParams["defaultChannels"] = codecContext->channels?
                                             codecContext->channels:
                                             channelsCount;
    } else if (avCodec->type == AVMEDIA_TYPE_VIDEO) {
        QVariantList supportedFrameRates;

        if (avCodec->supported_framerates)
            for (int i = 0; ; i++) {
                AVRational frameRate = avCodec->supported_framerates[i];

                if (frameRate.num == 0 && frameRate.den == 0)
                    break;

                supportedFrameRates << QVariant::fromValue(QbFrac(frameRate.num, frameRate.den));
            }

        codecParams["supportedFrameRates"] = supportedFrameRates;

        QStringList supportedPixelFormats;

        if (avCodec->pix_fmts)
            for (int i = 0; ; i++) {
                AVPixelFormat pixelFormat = avCodec->pix_fmts[i];

                if (pixelFormat == AV_PIX_FMT_NONE)
                    break;

                supportedPixelFormats << QString(av_get_pix_fmt_name(pixelFormat));
            }

        codecParams["supportedPixelFormats"] = supportedPixelFormats;
        codecParams["defaultGOP"] = codecContext->gop_size > 0?
                                        codecContext->gop_size: 12;
        codecParams["defaultBitRate"] = codecContext->bit_rate?
                                            codecContext->bit_rate: 200000;
        codecParams["defaultPixelFormat"] = codecContext->pix_fmt != AV_PIX_FMT_NONE?
                                            QString(av_get_pix_fmt_name(codecContext->pix_fmt)):
                                            supportedPixelFormats.value(0, "yuv420p");
    }

    av_free(codecContext);

    return codecParams;
}

QVariantMap MediaSink::addStream(int streamIndex,
                                 const QbCaps &streamCaps,
                                 const QVariantMap &codecParams)
{
    QString outputFormat;

    if (this->supportedFormats().contains(this->m_outputFormat))
        outputFormat = this->m_outputFormat;
    else {
        AVOutputFormat *format =
                av_guess_format(NULL,
                                this->m_location.toStdString().c_str(),
                                NULL);

        if (format)
            outputFormat = QString(format->name);
    }

    if (outputFormat.isEmpty())
        return QVariantMap();

    QVariantMap outputParams;

    outputParams["index"] = streamIndex;
    QString codec;

    if (codecParams.contains("codec")) {
        if (this->supportedCodecs(outputFormat, streamCaps.mimeType())
            .contains(codecParams["codec"].toString())) {
            codec = codecParams["codec"].toString();
        } else
            codec = this->defaultCodec(outputFormat, streamCaps.mimeType());
    } else
        codec = this->defaultCodec(outputFormat, streamCaps.mimeType());

    outputParams["codec"] = codec;

    QVariantMap codecDefaults = this->defaultCodecParams(codec);

    outputParams["codecOptions"] = codecParams.value("codecOptions", QVariantMap());

    if (streamCaps.mimeType() == "audio/x-raw") {
        int bitRate = codecParams.value("bitRate",
                                        codecDefaults["defaultBitRate"]).toInt();
        outputParams["bitRate"] = bitRate > 0?
                                      bitRate:
                                      codecDefaults["defaultBitRate"].toInt();

        QbAudioCaps audioCaps(streamCaps);
        QString sampleFormat = QbAudioCaps::sampleFormatToString(audioCaps.format());
        QStringList supportedSampleFormats = codecDefaults["supportedSampleFormats"].toStringList();

        if (!supportedSampleFormats.isEmpty() && !supportedSampleFormats.contains(sampleFormat)) {
            QString defaultSampleFormat = codecDefaults["defaultSampleFormat"].toString();
            audioCaps.format() = QbAudioCaps::sampleFormatFromString(defaultSampleFormat);
            audioCaps.bps() = av_get_bytes_per_sample(av_get_sample_fmt(defaultSampleFormat.toStdString().c_str()));
        }

        QVariantList supportedSampleRates = codecDefaults["supportedSampleRates"].toList();

        if (!supportedSampleRates.isEmpty()) {
            int sampleRate = 0;
            int maxDiff = std::numeric_limits<int>::max();

            foreach (QVariant rate, supportedSampleRates) {
                int diff = qAbs(audioCaps.rate() - rate.toInt());

                if (diff < maxDiff) {
                    sampleRate = rate.toInt();

                    if (!diff)
                        break;

                    maxDiff = diff;
                }
            }

            audioCaps.rate() = sampleRate;
        }

        QString channelLayout = QbAudioCaps::channelLayoutToString(audioCaps.layout());
        QStringList supportedChannelLayouts = codecDefaults["supportedChannelLayouts"].toStringList();

        if (!supportedChannelLayouts.isEmpty() && !supportedChannelLayouts.contains(channelLayout)) {
            QString defaultChannelLayout = codecDefaults["defaultChannelLayout"].toString();
            audioCaps.layout() = QbAudioCaps::channelLayoutFromString(defaultChannelLayout);
            audioCaps.channels() = av_get_channel_layout_nb_channels(av_get_channel_layout(defaultChannelLayout.toStdString().c_str()));
        }

        outputParams["caps"] = QVariant::fromValue(audioCaps.toCaps());
        outputParams["timeBase"] = QVariant::fromValue(QbFrac(1, audioCaps.rate()));
    } else if (streamCaps.mimeType() == "video/x-raw") {
        int bitRate = codecParams.value("bitRate",
                                        codecDefaults["defaultBitRate"]).toInt();
        outputParams["bitRate"] = bitRate > 0?
                                      bitRate:
                                      codecDefaults["defaultBitRate"].toInt();
        int gop = codecParams.value("gop",
                                    codecDefaults["defaultGOP"]).toInt();
        outputParams["gop"] = gop > 0?
                                  gop:
                                  codecDefaults["defaultGOP"].toInt();

        QbVideoCaps videoCaps(streamCaps);
        QString pixelFormat = QbVideoCaps::pixelFormatToString(videoCaps.format());
        QStringList supportedPixelFormats = codecDefaults["supportedPixelFormats"].toStringList();

        if (!supportedPixelFormats.isEmpty() && !supportedPixelFormats.contains(pixelFormat)) {
            QString defaultPixelFormat = codecDefaults["defaultPixelFormat"].toString();
            videoCaps.format() = QbVideoCaps::pixelFormatFromString(defaultPixelFormat);
        }

        QVariantList supportedFrameRates = codecDefaults["supportedFrameRates"].toList();

        if (!supportedFrameRates.isEmpty()) {
            QbFrac frameRate;
            qreal maxDiff = std::numeric_limits<qreal>::max();

            foreach (QVariant rate, supportedFrameRates) {
                qreal diff = qAbs(videoCaps.fps().value() - rate.value<QbFrac>().value());

                if (diff < maxDiff) {
                    frameRate = rate.value<QbFrac>();

                    if (!diff)
                        break;

                    maxDiff = diff;
                }
            }

            videoCaps.fps() = frameRate;
        }

        outputParams["caps"] = QVariant::fromValue(videoCaps.toCaps());
        outputParams["timeBase"] = QVariant::fromValue(videoCaps.fps().invert());
    } else if (streamCaps.mimeType() == "text/x-raw") {
        outputParams["caps"] = QVariant::fromValue(streamCaps);
    }

    this->m_streamConfigs << outputParams;
    this->streamsChanged(this->streams());

    return outputParams;
}

QVariantMap MediaSink::updateStream(int index, const QVariantMap &codecParams)
{
    QString outputFormat;

    if (this->supportedFormats().contains(this->m_outputFormat))
        outputFormat = this->m_outputFormat;
    else {
        AVOutputFormat *format =
                av_guess_format(NULL,
                                this->m_location.toStdString().c_str(),
                                NULL);

        if (format)
            outputFormat = QString(format->name);
    }

    if (outputFormat.isEmpty())
        return QVariantMap();

    QbCaps streamCaps = this->m_streamConfigs[index]["caps"].value<QbCaps>();
    QString codec;
    bool streamChanged = false;

    if (codecParams.contains("codec")) {
        if (this->supportedCodecs(outputFormat, streamCaps.mimeType())
            .contains(codecParams["codec"].toString())) {
            codec = codecParams["codec"].toString();
        } else
            codec = this->defaultCodec(outputFormat, streamCaps.mimeType());

        this->m_streamConfigs[index]["codec"] = codec;
        streamChanged |= true;

        // Update sample format.
        QVariantMap codecDefaults = this->defaultCodecParams(codec);
        QbAudioCaps audioCaps(this->m_streamConfigs[index]["caps"].value<QbCaps>());
        QString sampleFormat = QbAudioCaps::sampleFormatToString(audioCaps.format());
        QStringList supportedSampleFormats = codecDefaults["supportedSampleFormats"].toStringList();

        if (!supportedSampleFormats.isEmpty() && !supportedSampleFormats.contains(sampleFormat)) {
            QString defaultSampleFormat = codecDefaults["defaultSampleFormat"].toString();
            audioCaps.format() = QbAudioCaps::sampleFormatFromString(defaultSampleFormat);
            audioCaps.bps() = av_get_bytes_per_sample(av_get_sample_fmt(defaultSampleFormat.toStdString().c_str()));
        }

        QVariantList supportedSampleRates = codecDefaults["supportedSampleRates"].toList();

        if (!supportedSampleRates.isEmpty()) {
            int sampleRate = 0;
            int maxDiff = std::numeric_limits<int>::max();

            foreach (QVariant rate, supportedSampleRates) {
                int diff = qAbs(audioCaps.rate() - rate.toInt());

                if (diff < maxDiff) {
                    sampleRate = rate.toInt();

                    if (!diff)
                        break;

                    maxDiff = diff;
                }
            }

            audioCaps.rate() = sampleRate;
        }

        QString channelLayout = QbAudioCaps::channelLayoutToString(audioCaps.layout());
        QStringList supportedChannelLayouts = codecDefaults["supportedChannelLayouts"].toStringList();

        if (!supportedChannelLayouts.isEmpty() && !supportedChannelLayouts.contains(channelLayout)) {
            QString defaultChannelLayout = codecDefaults["defaultChannelLayout"].toString();
            audioCaps.layout() = QbAudioCaps::channelLayoutFromString(defaultChannelLayout);
            audioCaps.channels() = av_get_channel_layout_nb_channels(av_get_channel_layout(defaultChannelLayout.toStdString().c_str()));
        }

        this->m_streamConfigs[index]["caps"] = QVariant::fromValue(audioCaps.toCaps());
    } else
        codec = this->m_streamConfigs[index]["codec"].toString();

    QVariantMap codecDefaults = this->defaultCodecParams(codec);

    if ((streamCaps.mimeType() == "audio/x-raw"
         || streamCaps.mimeType() == "video/x-raw")
        && codecParams.contains("bitRate")) {
        int bitRate = codecParams["bitRate"].toInt();
        this->m_streamConfigs[index]["bitRate"] = bitRate > 0?
                                                      bitRate:
                                                      codecDefaults["defaultBitRate"].toInt();
        streamChanged |= true;
    }

    if (streamCaps.mimeType() == "video/x-raw"
        && codecParams.contains("gop")) {
        int gop = codecParams["gop"].toInt();
        this->m_streamConfigs[index]["gop"] = gop > 0?
                                                  gop:
                                                  codecDefaults["defaultGOP"].toInt();
        streamChanged |= true;
    }

    if (codecParams.contains("codecOptions")) {
        this->m_streamConfigs[index]["codecOptions"] = codecParams["codecOptions"];
        streamChanged |= true;
    }

    if (streamChanged)
        this->streamUpdated(index);

    return this->m_streamConfigs[index];
}

void MediaSink::flushStreams()
{
    for (uint i = 0; i < this->m_formatContext->nb_streams; i++) {
        AVStream *stream = this->m_formatContext->streams[i];
        AVMediaType mediaType = stream->codec->codec_type;

        if (mediaType == AVMEDIA_TYPE_AUDIO) {
            if (stream->codec->frame_size <= 1)
                continue;

            qint64 pts = this->m_streamParams[i].audioPts();
            int ptsDiff = stream->codec->frame_size < 1?
                              1: stream->codec->frame_size;

            forever {
                AVPacket pkt;
                av_init_packet(&pkt);
                pkt.data = NULL;
                pkt.size = 0;

                int gotPacket;

                if (avcodec_encode_audio2(stream->codec,
                                          &pkt,
                                          NULL,
                                          &gotPacket) < 0)
                    break;

                if (!gotPacket)
                    break;

                pkt.pts = pkt.dts = pts;
                pts += ptsDiff;
                av_packet_rescale_ts(&pkt, stream->codec->time_base, stream->time_base);
                pkt.stream_index = i;
                av_interleaved_write_frame(this->m_formatContext, &pkt);
                av_free_packet(&pkt);
            }
        } else if (mediaType == AVMEDIA_TYPE_VIDEO) {
            if (this->m_formatContext->oformat->flags & AVFMT_RAWPICTURE
                && stream->codec->codec->id == AV_CODEC_ID_RAWVIDEO)
                continue;

            forever {
                AVPacket pkt;
                av_init_packet(&pkt);
                pkt.data = NULL;
                pkt.size = 0;

                int gotPacket;

                if (avcodec_encode_video2(stream->codec,
                                          &pkt,
                                          NULL,
                                          &gotPacket) < 0)
                    break;

                if (!gotPacket)
                    break;

                pkt.pts = pkt.dts = this->m_streamParams[i].nextPts(0, 0);
                av_packet_rescale_ts(&pkt, stream->codec->time_base, stream->time_base);
                pkt.stream_index = i;
                av_interleaved_write_frame(this->m_formatContext, &pkt);
                av_free_packet(&pkt);
            }
        }
    }
}

void MediaSink::setLocation(const QString &location)
{
    if (this->m_location == location)
        return;

    this->m_location = location;
    emit this->locationChanged(location);
}

void MediaSink::setOutputFormat(const QString &outputFormat)
{
    if (this->m_outputFormat == outputFormat)
        return;

    this->m_outputFormat = outputFormat;
    emit this->outputFormatChanged(outputFormat);
}

void MediaSink::resetLocation()
{
    this->setLocation("");
}

void MediaSink::resetOutputFormat()
{
    this->setOutputFormat("");
}

void MediaSink::writeAudioPacket(const QbAudioPacket &packet)
{
    if (!this->m_formatContext)
        return;

    int streamIndex = -1;

    for (int i = 0; i < this->m_streamParams.size(); i++)
        if (this->m_streamParams[i].inputIndex() == packet.index()) {
            streamIndex = i;

            break;
        }

    if (streamIndex < 0)
        return;

    AVStream *stream = this->m_formatContext->streams[streamIndex];
    AVCodecContext *codecContext = stream->codec;

    AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));
    iFrame.format = codecContext->sample_fmt;
    iFrame.channels = codecContext->channels;
    iFrame.channel_layout = codecContext->channel_layout;
    iFrame.sample_rate = codecContext->sample_rate;

    if (!this->m_streamParams[streamIndex].convert(packet, &iFrame)) {
        av_freep(&iFrame.data);

        return;
    }

    QbFrac outTimeBase(codecContext->time_base.num,
                       codecContext->time_base.den);
    qint64 pts = qRound(packet.pts()
                        * packet.timeBase().value()
                        / outTimeBase.value());
    iFrame.pts = iFrame.pkt_pts = pts;

    this->m_streamParams[streamIndex].addAudioSamples(&iFrame, packet.id());

    int outSamples = codecContext->frame_size < 1?
                        iFrame.nb_samples:
                        codecContext->frame_size;

    av_freep(&iFrame.data);

    forever {
        pts = this->m_streamParams[streamIndex].audioPts();
        QByteArray buffer = this->m_streamParams[streamIndex].readAudioSamples(outSamples);

        if (buffer.isEmpty())
            break;

        AVFrame oFrame;
        memset(&oFrame, 0, sizeof(AVFrame));
        oFrame.format = codecContext->sample_fmt;
        oFrame.channels = codecContext->channels;
        oFrame.channel_layout = codecContext->channel_layout;
        oFrame.sample_rate = codecContext->sample_rate;
        oFrame.nb_samples = outSamples;
        oFrame.pts = oFrame.pkt_pts = pts;

        if (avcodec_fill_audio_frame(&oFrame,
                                     codecContext->channels,
                                     codecContext->sample_fmt,
                                     (const uint8_t *) buffer.data(),
                                     buffer.size(),
                                     1) < 0) {
            continue;
        }

        // Initialize audio packet.
        AVPacket pkt;
        memset(&pkt, 0, sizeof(AVPacket));
        av_init_packet(&pkt);

        // Compress audio packet.
        int gotPacket;
        int result = avcodec_encode_audio2(codecContext,
                                           &pkt,
                                           &oFrame,
                                           &gotPacket);

        if (result < 0) {
            char error[1024];
            av_strerror(result, error, 1024);
            qDebug() << "Error: " << error;

            break;
        }

        if (!gotPacket)
            continue;

        pkt.stream_index = streamIndex;
        av_packet_rescale_ts(&pkt, codecContext->time_base, stream->time_base);

        // Write audio packet.
        this->m_mutex.lock();
        av_interleaved_write_frame(this->m_formatContext, &pkt);
        this->m_mutex.unlock();
    }
}

void MediaSink::writeVideoPacket(const QbVideoPacket &packet)
{
    if (!this->m_formatContext)
        return;

    int streamIndex = -1;

    for (int i = 0; i < this->m_streamParams.size(); i++)
        if (this->m_streamParams[i].inputIndex() == packet.index()) {
            streamIndex = i;

            break;
        }

    if (streamIndex < 0)
        return;

    AVStream *stream = this->m_formatContext->streams[streamIndex];
    AVCodecContext *codecContext = stream->codec;

    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));
    oFrame.format = codecContext->pix_fmt;
    oFrame.width = codecContext->width;
    oFrame.height = codecContext->height;

    if (!this->m_streamParams[streamIndex].convert(packet, &oFrame)) {
        av_freep(&oFrame.data);

        return;
    }

    QbFrac outTimeBase(codecContext->time_base.num,
                       codecContext->time_base.den);

    qint64 pts = qRound(packet.pts()
                        * packet.timeBase().value()
                        / outTimeBase.value());

    oFrame.pts = oFrame.pkt_pts =
            this->m_streamParams[streamIndex].nextPts(pts, packet.id());

    if (oFrame.pts < 0) {
        avpicture_free((AVPicture *) &oFrame);

        return;
    }

    AVPacket pkt;
    av_init_packet(&pkt);

    if (this->m_formatContext->oformat->flags & AVFMT_RAWPICTURE) {
        // Raw video case - directly store the picture in the packet
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.data = oFrame.data[0];
        pkt.size = sizeof(AVPicture);
        pkt.pts = oFrame.pts;
        pkt.stream_index = streamIndex;

        av_packet_rescale_ts(&pkt, codecContext->time_base, stream->time_base);

        this->m_mutex.lock();
        av_interleaved_write_frame(this->m_formatContext, &pkt);
        this->m_mutex.unlock();
    } else {
        // encode the image
        pkt.data = NULL; // packet data will be allocated by the encoder
        pkt.size = 0;

        int gotPacket;

        if (avcodec_encode_video2(stream->codec,
                                  &pkt,
                                  &oFrame,
                                  &gotPacket) < 0) {
            avpicture_free((AVPicture *) &oFrame);

            return;
        }

        // If size is zero, it means the image was buffered.
        if (gotPacket) {
            pkt.stream_index = streamIndex;

            av_packet_rescale_ts(&pkt, codecContext->time_base, stream->time_base);

            // Write the compressed frame to the media file.
            this->m_mutex.lock();
            av_interleaved_write_frame(this->m_formatContext, &pkt);
            this->m_mutex.unlock();
        }
    }

    avpicture_free((AVPicture *) &oFrame);
}

void MediaSink::writeSubtitlePacket(const QbPacket &packet)
{
    Q_UNUSED(packet)
    // TODO: Implement this.
}

void MediaSink::clearStreams()
{
    this->m_streamConfigs.clear();
    this->streamsChanged(this->streams());
}

bool MediaSink::init()
{
    if (avformat_alloc_output_context2(&this->m_formatContext,
                                       NULL,
                                       this->m_outputFormat.isEmpty()?
                                            NULL: this->m_outputFormat.toStdString().c_str(),
                                       this->m_location.toStdString().c_str()) < 0)
        return false;

    for (int i = 0; i < this->m_streamConfigs.count(); i++) {
        QVariantMap streamConfigs = this->m_streamConfigs[i];
        QString codecName = streamConfigs["codec"].toString();

        AVCodec *codec = avcodec_find_encoder_by_name(codecName.toStdString().c_str());
        AVStream *stream = avformat_new_stream(this->m_formatContext, codec);

        stream->id = i;

        // Some formats want stream headers to be separate.
        if (this->m_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
            stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

        // Confihure streams parameters.
        QbCaps streamCaps = streamConfigs["caps"].value<QbCaps>();

        if (streamCaps.mimeType() == "audio/x-raw") {
            stream->codec->bit_rate = streamConfigs["bitRate"].toInt();

            QbAudioCaps caps(streamConfigs["caps"].value<QbCaps>());
            QString sampleFormat = QbAudioCaps::sampleFormatToString(caps.format());
            stream->codec->sample_fmt = av_get_sample_fmt(sampleFormat.toStdString().c_str());
            stream->codec->sample_rate = caps.rate();
            QString layout = QbAudioCaps::channelLayoutToString(caps.layout());
            stream->codec->channel_layout = av_get_channel_layout(layout.toStdString().c_str());
            stream->codec->channels = caps.channels();

            QbFrac timeBase(streamConfigs["timeBase"].value<QbFrac>());

            stream->time_base.num = timeBase.num();
            stream->time_base.den = timeBase.den();
        } else if (streamCaps.mimeType() == "video/x-raw") {
            stream->codec->bit_rate = streamConfigs["bitRate"].toInt();

            QbVideoCaps caps(streamConfigs["caps"].value<QbCaps>());
            QString pixelFormat = QbVideoCaps::pixelFormatToString(caps.format());
            stream->codec->pix_fmt = av_get_pix_fmt(pixelFormat.toStdString().c_str());
            stream->codec->width = caps.width();
            stream->codec->height = caps.height();

            QbFrac timeBase(streamConfigs["timeBase"].value<QbFrac>());
            stream->time_base.num = timeBase.num();
            stream->time_base.den = timeBase.den();
            stream->codec->time_base = stream->time_base;

            stream->codec->gop_size = streamConfigs["gop"].toInt();
        } else if (streamCaps.mimeType() == "text/x-raw") {
        }

        // Set codec options.
        AVDictionary *options = NULL;
        QVariantMap codecOptions = streamConfigs.value("codecOptions").toMap();

        foreach (QString key, codecOptions.keys()) {
            QString value = codecOptions[key].toString();

            av_dict_set(&options,
                        key.toStdString().c_str(),
                        value.toStdString().c_str(),
                        0);
        }

        // Open stream.
        int error = avcodec_open2(stream->codec, codec, &options);

        if (error < 0) {
            char errorStr[1024];
            av_strerror(AVERROR(error), errorStr, 1024);
            qDebug() << "Can't open codec " << codec->name << ": " << errorStr;
            av_dict_free(&options);
            this->uninit();

            return false;
        }

        av_dict_free(&options);
        this->m_streamParams << OutputParams(streamConfigs["index"].toInt());
    }

    // Print recording information.
    av_dump_format(this->m_formatContext,
                   0,
                   this->m_location.toStdString().c_str(),
                   1);

    // Open file.
    if (!(this->m_formatContext->oformat->flags & AVFMT_NOFILE)) {
        int error = avio_open(&this->m_formatContext->pb,
                              this->m_location.toStdString().c_str(),
                              AVIO_FLAG_WRITE);

        if (error < 0) {
            char errorStr[1024];
            av_strerror(AVERROR(error), errorStr, 1024);
            qDebug() << "Can't open output file: " << errorStr;
            this->uninit();

            return false;
        }
    }

    // Write file header.
    int error = avformat_write_header(this->m_formatContext, NULL);

    if (error < 0) {
        char errorStr[1024];
        av_strerror(AVERROR(error), errorStr, 1024);
        qDebug() << "Can't write header: " << errorStr;
        this->uninit();

        return false;
    }

    return true;
}

void MediaSink::uninit()
{
    if (!this->m_formatContext)
        return;

    // Write remaining frames in the file.
    this->flushStreams();
    this->m_streamParams.clear();

    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close().
    av_write_trailer(this->m_formatContext);

    if (!(this->m_formatContext->oformat->flags & AVFMT_NOFILE))
        // Close the output file.
        avio_close(this->m_formatContext->pb);

    avformat_free_context(this->m_formatContext);
    this->m_formatContext = NULL;
}

void MediaSink::updateStreams()
{
    QList<QVariantMap> streamConfigs = this->m_streamConfigs;
    this->clearStreams();

    foreach (QVariantMap configs, streamConfigs) {
        QbCaps caps = configs["caps"].value<QbCaps>();
        int index = configs["index"].toInt();
        this->addStream(index, caps, configs);
    }
}

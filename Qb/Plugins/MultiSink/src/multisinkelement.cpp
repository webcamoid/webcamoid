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

extern "C"
{
    #include <libavutil/imgutils.h>
}

#include "multisinkelement.h"
#include "customdeleters.h"

MultiSinkElement::MultiSinkElement(): QbElement()
{
    av_register_all();

    this->m_flushPts = -1;
}

MultiSinkElement::~MultiSinkElement()
{
    this->setState(ElementStateNull);
}

QString MultiSinkElement::location()
{
    return this->m_location;
}

QString MultiSinkElement::options()
{
    return this->m_options;
}

QVariantMap MultiSinkElement::streamCaps()
{
    return this->m_streamCaps;
}

void MultiSinkElement::stateChange(QbElement::ElementState from, QbElement::ElementState to)
{
    this->m_mutex.lock();

    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused) {
        this->updateOutputParams();

        this->m_outputFormat.open(this->location(),
                                  this->m_outputParams,
                                  this->m_commands.outputOptions(),
                                  this->m_commands.inputs());
    } else if (from == QbElement::ElementStatePaused
               && to == QbElement::ElementStateNull) {
        this->flushStreams();
        this->m_outputFormat.close();
    }

    this->m_mutex.unlock();
}

QList<AVPixelFormat> MultiSinkElement::pixelFormats(AVCodec *videoCodec)
{
    QList<AVPixelFormat> pixelFormats;

    if (!videoCodec)
        return pixelFormats;

    for (const AVPixelFormat *pixelFmt = videoCodec->pix_fmts;
         pixelFmt && *pixelFmt != AV_PIX_FMT_NONE;
         pixelFmt++)
        pixelFormats << *pixelFmt;

    return pixelFormats;
}

QList<AVSampleFormat> MultiSinkElement::sampleFormats(AVCodec *audioCodec)
{
    QList<AVSampleFormat> sampleFormats;

    for (const AVSampleFormat *sampleFmt = audioCodec->sample_fmts;
         sampleFmt && *sampleFmt != AV_SAMPLE_FMT_NONE;
         sampleFmt++)
        sampleFormats << *sampleFmt;

    return sampleFormats;
}

QList<int> MultiSinkElement::sampleRates(AVCodec *audioCodec)
{
    QList<int> sampleRates;

    for (const int *sampleRate = audioCodec->supported_samplerates;
         sampleRate && *sampleRate != 0;
         sampleRate++)
        sampleRates << *sampleRate;

    return sampleRates;
}

QList<quint64> MultiSinkElement::channelLayouts(AVCodec *audioCodec)
{
    QList<quint64> channelLayouts;

    for (quint64 *channelLayout = (quint64 *) audioCodec->channel_layouts;
         channelLayout && *channelLayout != 0;
         channelLayout++)
        channelLayouts << *channelLayout;

    return channelLayouts;
}

OutputParams MultiSinkElement::createOutputParams(int inputIndex,
                                                  const QbCaps &inputCaps,
                                                  const QVariantMap &options)
{
    QString fmt = this->m_commands.outputOptions()["f"].toString();
    AVOutputFormat *outputFormat = av_guess_format(fmt.toStdString().c_str(), NULL, NULL);

    QString acodec(avcodec_get_name(outputFormat->audio_codec));
    QString vcodec(avcodec_get_name(outputFormat->video_codec));

    QString codecName;

    if (inputCaps.mimeType() == "audio/x-raw")
        codecName = options.contains("c:a")?
                        options["c:a"].toString():
                        acodec;
    else if (inputCaps.mimeType() == "video/x-raw")
        codecName = options.contains("c:v")?
                        options["c:v"].toString():
                        vcodec;

    AVCodec *codec = avcodec_find_encoder_by_name(codecName.toStdString().c_str());

    CodecContextPtr codecContext(avcodec_alloc_context3(codec),
                                 CustomDeleters::deleteCodecContext);

    codecContext->codec = codec;

    if (inputCaps.mimeType() == "audio/x-raw") {
        QList<AVSampleFormat> sampleFormats = this->sampleFormats(codec);

        AVSampleFormat defaultSampleFormat = sampleFormats.isEmpty()?
                                                 codecContext->sample_fmt:
                                                 sampleFormats[0];

        AVSampleFormat iSampleFormat = av_get_sample_fmt(inputCaps.property("format")
                                                                  .toString()
                                                                  .toStdString()
                                                                  .c_str());

        codecContext->sample_fmt = sampleFormats.contains(iSampleFormat)?
                                        iSampleFormat:
                                        defaultSampleFormat;

        if (options.contains("b:a"))
            codecContext->bit_rate = options["b:a"].toInt();

        QList<int> sampleRates = this->sampleRates(codec);

        int defaultSampleRate = sampleRates.isEmpty()?
                                    codecContext->sample_rate:
                                    sampleRates[0];

        int iSampleRate = inputCaps.property("rate").toInt();
        int forcedSampleRate = options["ar"].toInt();

        if (options.contains("ar") &&
            (sampleRates.isEmpty() || sampleRates.contains(forcedSampleRate)))
            codecContext->sample_rate = forcedSampleRate;
        else if(sampleRates.isEmpty() || sampleRates.contains(iSampleRate))
            codecContext->sample_rate = iSampleRate;
        else
            codecContext->sample_rate = defaultSampleRate;

        QList<quint64> channelLayouts = this->channelLayouts(codec);

        quint64 defaultChannelLayout = channelLayouts.isEmpty()?
                                            codecContext->channel_layout:
                                            channelLayouts[0];

        quint64 iChannelLayout = inputCaps.contains("layout")?
                                      av_get_channel_layout(inputCaps.property("layout")
                                                                .toString()
                                                                .toStdString()
                                                                .c_str()):
                                      0;

        quint64 forcedChannelLayout = options.contains("channel_layout")?
                                           av_get_channel_layout(options["channel_layout"].toString()
                                                                                          .toStdString()
                                                                                          .c_str()):
                                           0;

        if (options.contains("channel_layout") &&
            (channelLayouts.isEmpty() || channelLayouts.contains(forcedChannelLayout)))
            codecContext->channel_layout = forcedChannelLayout;
        else if(channelLayouts.isEmpty() || channelLayouts.contains(iChannelLayout))
            codecContext->channel_layout = iChannelLayout;
        else
            codecContext->channel_layout = defaultChannelLayout;

        if (options.contains("ac"))
            codecContext->channels = options["ac"].toInt();
        else if (inputCaps.contains("channels"))
            codecContext->channels = inputCaps.property("channels").toInt();
        else
            codecContext->channels = av_get_channel_layout_nb_channels(codecContext->channel_layout);

        codecContext->time_base.num = 1;
        codecContext->time_base.den = codecContext->sample_rate;
    }
    else if (inputCaps.mimeType() == "video/x-raw") {
        QList<AVPixelFormat> pixelFormats = this->pixelFormats(codec);

        AVPixelFormat defaultPixelFormat = pixelFormats.isEmpty()?
                                             codecContext->pix_fmt:
                                             pixelFormats[0];

        AVPixelFormat iPixelFormat = av_get_pix_fmt(inputCaps.property("format")
                                                           .toString()
                                                           .toStdString()
                                                           .c_str());

        codecContext->pix_fmt = pixelFormats.contains(iPixelFormat)?
                                        iPixelFormat:
                                        defaultPixelFormat;

        if (options.contains("b:v"))
            codecContext->bit_rate = options["b:v"].toInt();

        // Resolution must be a multiple of two.
        if (options.contains("s")) {
            QSize size = options["s"].toSize();

            codecContext->width = size.width();
            codecContext->height = size.height();
        } else {
            codecContext->width = inputCaps.property("width").toInt();
            codecContext->height = inputCaps.property("height").toInt();
        }

        // timebase: This is the fundamental unit of time (in seconds) in terms
        // of which frame timestamps are represented. For fixed-fps content,
        // timebase should be 1/framerate and timestamp increments should be
        // identical to 1.
        QbFrac fps = options.contains("r")?
                         options["r"].value<QbFrac>():
                         QbFrac(inputCaps.property("fps").toString());

        codecContext->time_base.num = fps.den();
        codecContext->time_base.den = fps.num();

        if (options.contains("g"))
            codecContext->gop_size = options["g"].toInt();
        else
            codecContext->gop_size = 5 * fps.value();
    }

    int outputIndex = options.contains("oi")? options["oi"].toInt(): inputIndex;

    return OutputParams(codecContext, outputIndex);
}

void MultiSinkElement::setLocation(const QString &location)
{
    if (this->m_location == location)
        return;

    this->m_location = location;
    emit this->locationChanged(location);
}

void MultiSinkElement::setOptions(const QString &options)
{
    if (this->m_options == options)
        return;

    this->m_options = options;
    emit this->optionsChanged(options);

    if (this->m_options.isEmpty())
        this->m_commands.clear();
    else if (!this->m_commands.parseCmd(this->m_options))
        qDebug() << this->m_commands.error();
}

void MultiSinkElement::setStreamCaps(const QVariantMap &streamCaps)
{
    if (this->m_streamCaps == streamCaps)
        return;

    this->m_streamCaps = streamCaps;
    emit this->streamCapsChanged(streamCaps);
}

void MultiSinkElement::resetLocation()
{
    this->setLocation("");
}

void MultiSinkElement::resetOptions()
{
    this->setOptions("");
}

void MultiSinkElement::resetStreamCaps()
{
    this->setStreamCaps(QVariantMap());
}

QbPacket MultiSinkElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        this->state() != ElementStatePlaying)
        return QbPacket();

    if (!this->m_outputFormat.outputContext())
        this->m_outputFormat.open(this->location(),
                                  this->m_outputParams,
                                  this->m_commands.outputOptions(),
                                  this->m_commands.inputs());

    QString input = QString("%1").arg(packet.index());

    if (this->m_outputParams.contains(input)) {
        if (packet.caps().mimeType() == "audio/x-raw")
            this->processAFrame(packet);
        else if (packet.caps().mimeType() == "video/x-raw")
            this->processVFrame(packet);
    }

    return QbPacket();
}

void MultiSinkElement::processVFrame(const QbPacket &packet)
{
    QString inputIndex = QString("%1").arg(packet.index());
    StreamPtr videoStream = this->m_outputFormat.streams()[inputIndex];

    if (!videoStream)
        return;

    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));

    if (!this->m_outputParams[inputIndex].convert(packet, &oFrame)) {
        avpicture_free((AVPicture *) &oFrame);

        return;
    }

    qint64 pts = qRound(packet.pts() * packet.timeBase().value()
                        * videoStream->time_base.den
                        / videoStream->time_base.num);

    oFrame.pts = this->m_outputParams[inputIndex].nextPts(pts, packet.id());

    if (oFrame.pts < 0) {
        avpicture_free((AVPicture *) &oFrame);

        return;
    }

    int outputIndex = this->m_outputParams[inputIndex].outputIndex();

    AVPacket pkt;
    av_init_packet(&pkt);

    if (this->m_outputFormat.outputContext()->oformat->flags & AVFMT_RAWPICTURE) {
        // Raw video case - directly store the picture in the packet
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.data = oFrame.data[0];
        pkt.size = sizeof(AVPicture);
        pkt.pts = oFrame.pts;

        this->m_mutex.lock();
        av_interleaved_write_frame(this->m_outputFormat.outputContext().data(),
                                   &pkt);
        this->m_mutex.unlock();
    } else {
        // encode the image
        pkt.data = NULL; // packet data will be allocated by the encoder
        pkt.size = 0;

        int gotPacket;

        if (avcodec_encode_video2(videoStream->codec,
                                  &pkt,
                                  &oFrame,
                                  &gotPacket) < 0) {
            avpicture_free((AVPicture *) &oFrame);

            return;
        }

        // If size is zero, it means the image was buffered.
        if (gotPacket) {
            pkt.stream_index = outputIndex;

            // Write the compressed frame to the media file.
            this->m_mutex.lock();
            av_interleaved_write_frame(this->m_outputFormat.outputContext().data(),
                                       &pkt);
            this->m_mutex.unlock();
        }
    }

    avpicture_free((AVPicture *) &oFrame);
}

void MultiSinkElement::processAFrame(const QbPacket &packet)
{
    QString inputIndex = QString("%1").arg(packet.index());
    int outputIndex = this->m_outputParams[QString("%1").arg(packet.index())].outputIndex();
    StreamPtr audioStream = this->m_outputFormat.streams()[inputIndex];

    if (!audioStream)
        return;

    AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));

    if (!this->m_outputParams[inputIndex].convert(packet, &iFrame)) {
        av_freep(&iFrame.data);

        return;
    }

    AVCodecContext *codecContext = audioStream->codec;

    int samples = iFrame.nb_samples;
    int frameSize = codecContext->frame_size;

    if (frameSize < 1)
        frameSize = samples;

    QbFrac outTimeBase(codecContext->time_base.num,
                       codecContext->time_base.den);

    qint64 pts = qRound(packet.pts()
                        * packet.timeBase().value()
                        / outTimeBase.value());

    pts = this->m_outputParams[inputIndex].nextPts(pts, packet.id());

    if (pts < 0) {
        av_freep(&iFrame.data);

        return;
    }

    qreal ptsDiff = qRound(qreal(frameSize)
                           / iFrame.sample_rate
                           / outTimeBase.value());

    for (int offset = 0; samples > 0; offset += frameSize) {
        samples -= frameSize;
        frameSize = qMin(frameSize, samples);

        int oBufferSize = av_samples_get_buffer_size(NULL,
                                                     codecContext->channels,
                                                     frameSize,
                                                     codecContext->sample_fmt,
                                                     1);

        QByteArray oBuffer(oBufferSize, 0);

        static AVFrame oFrame;
        memset(&oFrame, 0, sizeof(AVFrame));

        oFrame.nb_samples = frameSize;

        if (avcodec_fill_audio_frame(&oFrame,
                                     codecContext->channels,
                                     codecContext->sample_fmt,
                                     (const uint8_t *) oBuffer.constData(),
                                     oBufferSize,
                                     1) < 0)
            continue;

        if (av_samples_copy(oFrame.data,
                            iFrame.data,
                            0,
                            offset,
                            frameSize,
                            codecContext->channels,
                            codecContext->sample_fmt) < 0)
            continue;

        AVPacket pkt;
        av_init_packet(&pkt);

        // data and size must be 0;
        pkt.data = NULL;
        pkt.size = 0;

        if (offset != 0) {
            pts = this->m_outputParams[inputIndex].nextPts(pts, packet.id());

            if (pts < 0)
                continue;
        }

        oFrame.pts = pts;
        pts += ptsDiff;

        int gotPacket;
        int result = avcodec_encode_audio2(codecContext,
                                           &pkt,
                                           &oFrame,
                                           &gotPacket);

        if (result  < 0) {
            char error[1024];
            av_strerror(result, error, 1024);
            qDebug() << "Error: " << error;

            continue;
        }

        if (!gotPacket)
            continue;

        pkt.stream_index = outputIndex;

        this->m_mutex.lock();
        av_interleaved_write_frame(this->m_outputFormat.outputContext().data(),
                                   &pkt);
        this->m_mutex.unlock();
    }

    av_freep(&iFrame.data);
}

void MultiSinkElement::updateOutputParams()
{
    QVariantMap inputOptions = this->m_commands.inputs();
    QVariantMap inputCaps = this->streamCaps();
    this->m_outputParams.clear();

    foreach (QString input, inputCaps.keys())
        if (inputOptions.contains(input)) {
            OutputParams outputParams = this->createOutputParams(input.toInt(),
                                                                 inputCaps[input].toString(),
                                                                 inputOptions[input].toMap());

            if (outputParams.codecContext() && outputParams.codecContext()->codec)
                this->m_outputParams[input] = outputParams;
        }
}

void MultiSinkElement::flushStream(int inputIndex, AVCodecContext *encoder)
{
    QString inputIndexStr = QString("%1").arg(inputIndex);
    AVStream *stream = this->m_outputFormat.streams()[inputIndexStr].data();

    if (this->m_flushPts < 0)
        this->m_flushPts = stream->cur_dts;
    else if (this->m_flushPts < stream->cur_dts)
        this->m_flushPts = stream->cur_dts;

    while (true) {
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        int gotPacket = false;
        int ret = -1;

        if (encoder->codec_type == AVMEDIA_TYPE_AUDIO)
            ret = avcodec_encode_audio2(encoder, &pkt, NULL, &gotPacket);
        else if (encoder->codec_type == AVMEDIA_TYPE_VIDEO)
            ret = avcodec_encode_video2(encoder, &pkt, NULL, &gotPacket);

        if (ret < 0 || !gotPacket)
            break;

        pkt.pts = ++this->m_flushPts;
        pkt.dts = pkt.pts;

        av_interleaved_write_frame(this->m_outputFormat.outputContext().data(),
                                   &pkt);
    }
}

void MultiSinkElement::flushStreams()
{
    StreamMapPtr streams = this->m_outputFormat.streams();
    int oFlags = this->m_outputFormat.outputContext()->oformat->flags;
    this->m_flushPts = -1;

    for (int i = 0; i < streams.count(); i++) {
        QString streamIndex = QString("%1").arg(i);
        StreamPtr stream = streams[streamIndex];
        AVCodecContext *encoder = stream->codec;

        if (encoder->codec_type == AVMEDIA_TYPE_AUDIO &&
            encoder->frame_size <= 1)
            continue;

        if (encoder->codec_type == AVMEDIA_TYPE_VIDEO &&
            (oFlags & AVFMT_RAWPICTURE) &&
            encoder->codec->id == AV_CODEC_ID_RAWVIDEO)
            continue;

        this->flushStream(i, encoder);
    }
}

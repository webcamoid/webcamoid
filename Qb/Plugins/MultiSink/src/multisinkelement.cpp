/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
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

    this->resetLocation();
    this->resetOptions();
    this->resetStreamCaps();
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
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        this->m_outputFormat.open(this->location(),
                                             this->m_outputParams,
                                             this->m_commands.outputOptions());
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull) {
        this->flushStreams();
        this->m_outputFormat.close();
    }
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

OutputParams MultiSinkElement::createOutputParams(int inputIndex, const QbCaps &inputCaps,
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
    QbCaps outputCaps(inputCaps);

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

        outputCaps.setProperty("format", av_get_sample_fmt_name(codecContext->sample_fmt));

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

        outputCaps.setProperty("rate", codecContext->sample_rate);

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

        outputCaps.setProperty("channels", codecContext->channels);

        char layout[256];

        av_get_channel_layout_string(layout,
                                     sizeof(layout),
                                     codecContext->channels,
                                     codecContext->channel_layout);

        outputCaps.setProperty("layout", QString(layout));
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

        outputCaps.setProperty("format", av_get_pix_fmt_name(codecContext->pix_fmt));

        if (options.contains("b:v"))
            codecContext->bit_rate = options["b:v"].toInt();

        // Resolution must be a multiple of two.
        if (options.contains("s")) {
            QSize size = options["s"].toSize();

            codecContext->width = size.width();
            codecContext->height = size.height();
        }
        else {
            codecContext->width = inputCaps.property("width").toInt();
            codecContext->height = inputCaps.property("height").toInt();
        }

        outputCaps.setProperty("width", codecContext->width);
        outputCaps.setProperty("height", codecContext->height);

        // timebase: This is the fundamental unit of time (in seconds) in terms
        // of which frame timestamps are represented. For fixed-fps content,
        // timebase should be 1/framerate and timestamp increments should be
        // identical to 1.
        QbFrac fps = options.contains("r")?
                         options["r"].value<QbFrac>():
                         QbFrac(inputCaps.property("fps").toString());

        codecContext->time_base.num = fps.den();
        codecContext->time_base.den = fps.num();

        outputCaps.setProperty("fps", fps.toString());

        if (options.contains("g"))
            codecContext->gop_size = options["g"].toInt();
        else
            codecContext->gop_size = 5 * fps.value();
    }

    QbElementPtr filter;

    if (inputCaps.mimeType() == "audio/x-raw") {
        filter = Qb::create("ACapsConvert");
        filter->setProperty("caps", outputCaps.toString());

        QObject::connect(filter.data(),
                         SIGNAL(oStream(const QbPacket &)),
                         this,
                         SLOT(processAFrame(const QbPacket &)));
    }
    else if (inputCaps.mimeType() == "video/x-raw") {
        filter = Qb::create("VCapsConvert");
        filter->setProperty("caps", outputCaps.toString());
        filter->setProperty("keepAspectRatio", true);

        QObject::connect(filter.data(),
                         SIGNAL(oStream(const QbPacket &)),
                         this,
                         SLOT(processVFrame(const QbPacket &)),
                         Qt::DirectConnection);
    }

    if (filter)
        QObject::connect(this,
                         SIGNAL(stateChanged(QbElement::ElementState)),
                         filter.data(),
                         SLOT(setState(QbElement::ElementState)),
                         Qt::DirectConnection);

    int outputIndex = options.contains("oi")? options["oi"].toInt(): inputIndex;

    return OutputParams(codecContext, filter, outputIndex);
}

void MultiSinkElement::setLocation(QString fileName)
{
    this->m_location = fileName;
}

void MultiSinkElement::setOptions(QString options)
{
    this->m_options = options;

    if (this->m_options.isEmpty())
        this->m_commands.clear();
    else if (!this->m_commands.parseCmd(this->m_options))
        qDebug() << this->m_commands.error();

    this->updateOutputParams();
}

void MultiSinkElement::setStreamCaps(QVariantMap streamCaps)
{
    this->m_streamCaps = streamCaps;

    this->updateOutputParams();
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

void MultiSinkElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        this->state() != ElementStatePlaying)
        return;

    if (!this->m_outputFormat.outputContext())
        this->m_outputFormat.open(this->location(),
                                  this->m_outputParams,
                                  this->m_commands.outputOptions());

    QString input = QString("%1").arg(packet.index());

    if (this->m_outputParams.contains(input)) {
        QbElementPtr filter = this->m_outputParams[input].filter();

        if (filter)
            filter->iStream(packet);
    }
}

void MultiSinkElement::processVFrame(const QbPacket &packet)
{
    int iWidth = packet.caps().property("width").toInt();
    int iHeight = packet.caps().property("height").toInt();
    QString format = packet.caps().property("format").toString();

    AVPixelFormat iFormat = av_get_pix_fmt(format.toStdString().c_str());

    AVPacket pkt;
    av_init_packet(&pkt);

    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));

    avpicture_fill((AVPicture *) &oFrame,
                   (uint8_t *) packet.buffer().data(),
                   iFormat,
                   iWidth,
                   iHeight);

    QString inputIndex = QString("%1").arg(packet.index());
    int outputIndex = this->m_outputParams[inputIndex].outputIndex();
    StreamPtr videoStream = this->m_outputFormat.streams()[inputIndex];

    if (!videoStream)
        return;

    int pts = (packet.pts()
              * packet.timeBase()
              / QbFrac(videoStream->time_base.num,
                       videoStream->time_base.den)).value();

    if (!this->m_outputParams[inputIndex].setPts(pts))
        return;

    if (this->m_outputFormat.outputContext()->oformat->flags & AVFMT_RAWPICTURE) {
        // Raw video case - directly store the picture in the packet
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = outputIndex;
        pkt.data = oFrame.data[0];
        pkt.size = sizeof(AVPicture);

        pkt.pts = this->m_outputParams[inputIndex].pts();

        av_interleaved_write_frame(this->m_outputFormat.outputContext().data(),
                                   &pkt);
    }
    else {
        // encode the image
        pkt.data = NULL; // packet data will be allocated by the encoder
        pkt.size = 0;

        oFrame.format = iFormat,
        oFrame.width = iWidth,
        oFrame.height = iHeight;

        oFrame.pts = this->m_outputParams[inputIndex].pts();

        int gotPacket;

        if (avcodec_encode_video2(videoStream->codec,
                                  &pkt,
                                  &oFrame,
                                  &gotPacket) < 0)
            return;

        // If size is zero, it means the image was buffered.
        if (gotPacket) {
            if (videoStream->codec->coded_frame->key_frame)
                pkt.flags |= AV_PKT_FLAG_KEY;

            pkt.stream_index = outputIndex;

            // Write the compressed frame to the media file.
            av_interleaved_write_frame(this->m_outputFormat.outputContext().data(),
                                       &pkt);
        }
    }
}

void MultiSinkElement::processAFrame(const QbPacket &packet)
{
    QString inputIndex = QString("%1").arg(packet.index());
    int outputIndex = this->m_outputParams[QString("%1").arg(packet.index())].outputIndex();
    StreamPtr audioStream = this->m_outputFormat.streams()[inputIndex];

    if (!audioStream)
        return;

    AVCodecContext *codecContext = audioStream->codec;

    int samples = packet.caps().property("samples").toInt();
    int frameSize = codecContext->frame_size;

    if (frameSize < 1)
        frameSize = samples;

    QbFrac timeBase(audioStream->time_base.num,
                    audioStream->time_base.den);

    float pts = (packet.pts()
              * packet.timeBase()
              / timeBase).value();

    if (!this->m_outputParams[inputIndex].setPts(pts))
        return;

    float ptsOffset = pts - (int) pts;

    float ptsDiff = frameSize /
                     (packet.caps().property("rate").toFloat() *
                      timeBase.value());

    static AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));

    iFrame.nb_samples = samples;
    bool align = packet.caps().property("align").toBool();

    if (avcodec_fill_audio_frame(&iFrame,
                                 codecContext->channels,
                                 codecContext->sample_fmt,
                                 (uint8_t *) packet.buffer().data(),
                                 packet.bufferSize(),
                                 align? 0: 1) < 0)
        return;

    for (int offset = 0; samples > 0; offset += frameSize) {
        samples -= frameSize;
        frameSize = qMin(frameSize, samples);

        int oBufferSize = av_samples_get_buffer_size(NULL,
                                                     codecContext->channels,
                                                     frameSize,
                                                     codecContext->sample_fmt,
                                                     align? 0: 1);

        QByteArray oBuffer(oBufferSize, 0);

        static AVFrame oFrame;
        memset(&oFrame, 0, sizeof(AVFrame));

        oFrame.nb_samples = frameSize;

        if (avcodec_fill_audio_frame(&oFrame,
                                     codecContext->channels,
                                     codecContext->sample_fmt,
                                     (const uint8_t *) oBuffer.constData(),
                                     oBufferSize,
                                     align? 0: 1) < 0)
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

        oFrame.pts = this->m_outputParams[inputIndex].pts() + ptsOffset;
        ptsOffset += ptsDiff;

        int gotPacket;

        if (avcodec_encode_audio2(codecContext, &pkt, &oFrame, &gotPacket) < 0 ||
            !gotPacket)
            continue;

        pkt.stream_index = outputIndex;

        av_interleaved_write_frame(this->m_outputFormat.outputContext().data(),
                                   &pkt);
    }
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
    qint64 pts = this->m_outputParams[inputIndexStr].pts() + 1;

    while (true) {
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        int gotPacket;
        int ret;

        if (encoder->codec_type == AVMEDIA_TYPE_AUDIO)
            ret = avcodec_encode_audio2(encoder, &pkt, NULL, &gotPacket);
        else if (encoder->codec_type == AVMEDIA_TYPE_VIDEO)
            ret = avcodec_encode_video2(encoder, &pkt, NULL, &gotPacket);
        else
            return;

        if (ret < 0 || !gotPacket)
            return;

        pkt.pts = pts;
        pkt.dts = pts;
        pts++;

        av_interleaved_write_frame(this->m_outputFormat.outputContext().data(),
                                   &pkt);
    }
}

void MultiSinkElement::flushStreams()
{
    StreamMapPtr streams = this->m_outputFormat.streams();
    int oFlags = this->m_outputFormat.outputContext()->oformat->flags;

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

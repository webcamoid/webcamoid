/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

#include "multisinkelement.h"

MultiSinkElement::MultiSinkElement(): QbElement()
{
    av_register_all();

    this->m_outputContext = NULL;

    this->m_pictureAlloc = -1;

    this->m_vFilter = Qb::create("VFilter");
    this->m_aCapsConvert = Qb::create("ACapsConvert");

    QObject::connect(this->m_vFilter.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processVFrame(const QbPacket &)));

    QObject::connect(this->m_aCapsConvert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processAFrame(const QbPacket &)));

    this->resetLocation();
    this->resetOptions();
    this->resetFrameSize();

    // File options:
    this->m_optionParser.addOption("f", Option::OptionFlagsHasValue);

    // Video options:
    this->m_optionParser.addOption("r", Option::OptionFlagsHasValue);
    this->m_optionParser.addOption("vn");
    this->m_optionParser.addOption("vcodec", Option::OptionFlagsHasValue);
    this->m_optionParser.addOption("b:v", Option::OptionFlagsHasValue);

    // Audio options:
    this->m_optionParser.addOption("ar", Option::OptionFlagsHasValue);
    this->m_optionParser.addOption("ac", Option::OptionFlagsHasValue);
    this->m_optionParser.addOption("an");
    this->m_optionParser.addOption("acodec", Option::OptionFlagsHasValue);
    this->m_optionParser.addOption("b:a", Option::OptionFlagsHasValue);
    this->m_optionParser.addOption("channel_layout", Option::OptionFlagsHasValue);

    this->m_vFormatToFF["I420"] = PIX_FMT_YUV420P;
    this->m_vFormatToFF["YUY2"] = PIX_FMT_YUV422P;
    this->m_vFormatToFF["UYVY"] = PIX_FMT_UYVY422;
    this->m_vFormatToFF["AYUV"] = PIX_FMT_YUVA420P;
    this->m_vFormatToFF["RGBx"] = PIX_FMT_RGB0;
    this->m_vFormatToFF["BGRx"] = PIX_FMT_BGR0;
    this->m_vFormatToFF["xRGB"] = PIX_FMT_0RGB;
    this->m_vFormatToFF["xBGR"] = PIX_FMT_0BGR;
    this->m_vFormatToFF["RGBA"] = PIX_FMT_RGBA;
    this->m_vFormatToFF["BGRA"] = PIX_FMT_BGRA;
    this->m_vFormatToFF["ARGB"] = PIX_FMT_ARGB;
    this->m_vFormatToFF["ABGR"] = PIX_FMT_ABGR;
    this->m_vFormatToFF["RGB"] = PIX_FMT_RGB24;
    this->m_vFormatToFF["BGR"] = PIX_FMT_BGR24;
    this->m_vFormatToFF["Y41B"] = PIX_FMT_YUV411P;
    this->m_vFormatToFF["Y42B"] = PIX_FMT_YUV422P;
    this->m_vFormatToFF["YVYU"] = PIX_FMT_UYVY422;
    this->m_vFormatToFF["Y444"] = PIX_FMT_YUV444P;
    this->m_vFormatToFF["v210"] = PIX_FMT_YUV422P10LE;
    this->m_vFormatToFF["v216"] = PIX_FMT_YUV422P16LE;
    this->m_vFormatToFF["NV12"] = PIX_FMT_NV12;
    this->m_vFormatToFF["NV21"] = PIX_FMT_NV21;
    this->m_vFormatToFF["GRAY8"] = PIX_FMT_GRAY8;
    this->m_vFormatToFF["GRAY16_BE"] = PIX_FMT_GRAY16BE;
    this->m_vFormatToFF["GRAY16_LE"] = PIX_FMT_GRAY16LE;
    this->m_vFormatToFF["v308"] = PIX_FMT_YUV444P;
    this->m_vFormatToFF["RGB16"] = PIX_FMT_RGB565LE;
    this->m_vFormatToFF["BGR16"] = PIX_FMT_BGR565LE;
    this->m_vFormatToFF["RGB15"] = PIX_FMT_RGB555LE;
    this->m_vFormatToFF["BGR15"] = PIX_FMT_BGR555LE;
    this->m_vFormatToFF["UYVP"] = PIX_FMT_YUV422P12LE;
    this->m_vFormatToFF["A420"] = PIX_FMT_YUVA420P;
    this->m_vFormatToFF["RGB8P"] = PIX_FMT_RGB8;
    this->m_vFormatToFF["IYU1"] = PIX_FMT_YUV411P;
    this->m_vFormatToFF["I420_10LE"] = PIX_FMT_YUV420P10LE;
    this->m_vFormatToFF["I420_10BE"] = PIX_FMT_YUV420P10BE;
    this->m_vFormatToFF["I422_10LE"] = PIX_FMT_YUV422P10LE;
    this->m_vFormatToFF["I422_10BE"] = PIX_FMT_YUV422P10BE;

    this->m_aFormatToFF["U8"] = AV_SAMPLE_FMT_U8;
    this->m_aFormatToFF["S16LE"] = AV_SAMPLE_FMT_S16;
    this->m_aFormatToFF["S32LE"] = AV_SAMPLE_FMT_S32;
    this->m_aFormatToFF["F32LE"] = AV_SAMPLE_FMT_FLT;
    this->m_aFormatToFF["F64LE"] = AV_SAMPLE_FMT_DBL;
}

MultiSinkElement::~MultiSinkElement()
{
    this->setState(ElementStateNull);
    this->cleanAll();
}

QString MultiSinkElement::location()
{
    return this->m_location;
}

QString MultiSinkElement::options()
{
    return this->m_options;
}

QSize MultiSinkElement::frameSize()
{
    return this->m_frameSize;
}

bool MultiSinkElement::init()
{
    if (this->m_outputContext)
        this->uninit();

    if (this->m_optionsMap.contains("f"))
        avformat_alloc_output_context2(&this->m_outputContext,
                                       NULL,
                                       this->m_optionsMap["f"].toString().toUtf8().constData(),
                                       this->location().toUtf8().constData());
    else
        avformat_alloc_output_context2(&this->m_outputContext,
                                       NULL,
                                       NULL,
                                       this->location().toUtf8().constData());

    if (!this->m_outputContext)
        return false;

    AVOutputFormat *outputFormat = this->m_outputContext->oformat;
    this->m_audioStream = NULL;
    this->m_videoStream = NULL;

    if (!this->m_optionsMap.contains("an"))
    {
        AVCodec *ffAudioCodec = NULL;

        this->m_audioStream = this->addStream(&ffAudioCodec, this->m_optionsMap["acodec"].toString(), AVMEDIA_TYPE_AUDIO);
        this->m_audioStream->id = 1;

        if (this->m_audioStream)
            if (avcodec_open2(this->m_audioStream->codec, ffAudioCodec, NULL) < 0)
                return false;
    }

    if (!this->m_optionsMap.contains("vn"))
    {
        AVCodec *ffVideoCodec = NULL;

        this->m_videoStream = this->addStream(&ffVideoCodec, this->m_optionsMap["vcodec"].toString(), AVMEDIA_TYPE_VIDEO);
        this->m_videoStream->id = 0;

        if (this->m_videoStream)
            if (avcodec_open2(this->m_videoStream->codec, ffVideoCodec, NULL) < 0)
                return false;
    }

    if (!this->m_audioStream && !this->m_videoStream)
        return false;

    av_dump_format(this->m_outputContext,
                   0,
                   this->location().toUtf8().constData(),
                   1);

    if (!(outputFormat->flags & AVFMT_NOFILE))
        if (avio_open(&this->m_outputContext->pb,
                      this->location().toUtf8().constData(),
                      AVIO_FLAG_WRITE) < 0)
            return false;

    if (avformat_write_header(this->m_outputContext, NULL) < 0)
        return false;

    avcodec_get_frame_defaults(&this->m_vFrame);
    this->m_vFrame.pts = 0;

    return true;
}

void MultiSinkElement::uninit()
{
    if (!this->m_outputContext)
        return;

    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close().
    av_write_trailer(this->m_outputContext);

    // Close each codec.
    if (this->m_audioStream)
        avcodec_close(this->m_audioStream->codec);

    if (this->m_videoStream)
        avcodec_close(this->m_videoStream->codec);

    // Free the streams.
    for (uint i = 0; i < this->m_outputContext->nb_streams; i++)
    {
        av_freep(&this->m_outputContext->streams[i]->codec);
        av_freep(&this->m_outputContext->streams[i]);
    }

    if (!(this->m_outputContext->oformat->flags & AVFMT_NOFILE))
        // Close the output file.
        avio_close(this->m_outputContext->pb);

    // free the stream
    av_free(this->m_outputContext);

    this->m_outputContext = NULL;
}

QList<PixelFormat> MultiSinkElement::pixelFormats(AVCodec *videoCodec)
{
    QList<PixelFormat> pixelFormats;

    for (const PixelFormat *pixelFmt = videoCodec->pix_fmts;
         pixelFmt && *pixelFmt != PIX_FMT_NONE;
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

QList<uint64_t> MultiSinkElement::channelLayouts(AVCodec *audioCodec)
{
    QList<uint64_t> channelLayouts;

    for (const uint64_t *channelLayout = audioCodec->channel_layouts;
         channelLayout && *channelLayout != 0;
         channelLayout++)
        channelLayouts << *channelLayout;

    return channelLayouts;
}

AVStream *MultiSinkElement::addStream(AVCodec **codec, QString codecName, AVMediaType mediaType)
{
    AVOutputFormat *outputFormat = this->m_outputContext->oformat;

    // find the encoder
    if (codecName.isEmpty())
    {
        switch (mediaType)
        {
            case AVMEDIA_TYPE_AUDIO:
                *codec = avcodec_find_encoder(outputFormat->audio_codec);
            break;

            case AVMEDIA_TYPE_VIDEO:
                *codec = avcodec_find_encoder(outputFormat->video_codec);
            break;

            default:
                *codec = NULL;
            break;
        }
    }
    else
        *codec = avcodec_find_encoder_by_name(codecName.toUtf8().constData());

    if (!(*codec))
        return NULL;

    AVStream *stream = avformat_new_stream(this->m_outputContext, *codec);

    if (!stream)
    {
        *codec = NULL;

        return NULL;
    }

    AVCodecContext *codecContext = stream->codec;
    QList<AVSampleFormat> sampleFormats = this->sampleFormats(*codec);
    QList<PixelFormat> pixelFormats = this->pixelFormats(*codec);
    QList<int> sampleRates = this->sampleRates(*codec);
    QList<uint64_t> channelLayouts = this->channelLayouts(*codec);

    int iNChannels = 0;
    int iSampleRate = 0;
    uint64_t iChannelLayout = 0;

    if (mediaType == AVMEDIA_TYPE_AUDIO)
    {
        QbCaps caps;

        foreach (QbElement *src, this->m_srcs)
        {
            foreach (QbCaps cap, src->oCaps())
                if (cap.mimeType() == "audio/x-raw")
                {
                    caps = cap;

                    break;
                }

            if (caps.isValid())
                break;
        }

        iNChannels = caps.property("channels").toInt();
        iSampleRate = caps.property("rate").toInt();
        const char *layout = caps.property("layout").toString().toUtf8().constData();
        iChannelLayout = av_get_channel_layout(layout);
    }

    switch ((*codec)->type)
    {
        case AVMEDIA_TYPE_AUDIO:
            if (sampleFormats.isEmpty())
                codecContext->sample_fmt = AV_SAMPLE_FMT_S16;
            else
                codecContext->sample_fmt = sampleFormats[0];

            if (this->m_optionsMap.contains("b:a"))
                codecContext->bit_rate = this->m_optionsMap["b:a"].toInt();

            if (this->m_optionsMap.contains("ar"))
                codecContext->sample_rate = this->m_optionsMap["ar"].toInt();
            else if (!sampleRates.isEmpty() && sampleRates[0])
                codecContext->sample_rate = sampleRates[0];

            if (!codecContext->sample_rate)
                codecContext->sample_rate = iSampleRate;

            if (this->m_optionsMap.contains("ac"))
                codecContext->channels = this->m_optionsMap["ac"].toInt();

            if (!codecContext->channels)
                codecContext->channels = iNChannels;

            if (this->m_optionsMap.contains("channel_layout"))
                codecContext->channel_layout = av_get_channel_layout(this->m_optionsMap["channel_layout"].toString()
                                                                                                         .toUtf8()
                                                                                                         .constData());
            else if (!channelLayouts.isEmpty())
                codecContext->channel_layout = channelLayouts[0];

            if (!codecContext->channel_layout)
                codecContext->channel_layout = iChannelLayout;
        break;

        case AVMEDIA_TYPE_VIDEO:
            avcodec_get_context_defaults3(codecContext, *codec);
            codecContext->codec_id = outputFormat->video_codec;

            if (this->m_optionsMap.contains("b:v"))
                codecContext->bit_rate = this->m_optionsMap["b:v"].toInt();

            // Resolution must be a multiple of two.
            codecContext->width = this->m_frameSize.width();
            codecContext->height = this->m_frameSize.height();

            // timebase: This is the fundamental unit of time (in seconds) in terms
            // of which frame timestamps are represented. For fixed-fps content,
            // timebase should be 1/framerate and timestamp increments should be
            // identical to 1.
            if (this->m_optionsMap.contains("r"))
                codecContext->time_base.den = this->m_optionsMap["r"].toInt();

            codecContext->time_base.num = 1;
            codecContext->gop_size = 12; // emit one intra frame every twelve frames at most.

            if (pixelFormats.isEmpty())
                codecContext->pix_fmt = PIX_FMT_YUV420P;
            else
                codecContext->pix_fmt = pixelFormats[0];

            this->m_vFilter->setProperty("format", this->m_vFormatToFF.key(codecContext->pix_fmt));

            if (codecContext->codec_id == AV_CODEC_ID_MPEG2VIDEO)
                // just for testing, we also add B frames
                codecContext->max_b_frames = 2;

            if (codecContext->codec_id == AV_CODEC_ID_MPEG1VIDEO)
                // Needed to avoid using macroblocks in which some coeffs overflow.
                // This does not happen with normal video, it just happens here as
                // the motion of the chroma plane does not match the luma plane.
                codecContext->mb_decision = 2;
        break;

        default:
        break;
    }

    // Some formats want stream headers to be separate.
    if (this->m_outputContext->oformat->flags & AVFMT_GLOBALHEADER)
        codecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return stream;
}

void MultiSinkElement::adjustToInputFrameSize(QSize frameSize)
{
    frameSize.scale(this->m_frameSize, Qt::KeepAspectRatio);

    int padX = (this->m_frameSize.width() - frameSize.width()) >> 1;
    int padY = (this->m_frameSize.height() - frameSize.height()) >> 1;

    this->m_vFilter->setProperty("description",
                                 QString("scale=%1:%2,"
                                         "pad=%3:%4:%5:%6:black").arg(frameSize.width())
                                                                 .arg(frameSize.height())
                                                                 .arg(this->m_frameSize.width())
                                                                 .arg(this->m_frameSize.height())
                                                                 .arg(padX)
                                                                 .arg(padY));
}

void MultiSinkElement::cleanAll()
{
    if (this->m_pictureAlloc >= 0)
    {
        avpicture_free(&this->m_oPicture);
        this->m_pictureAlloc = -1;
    }
}

void MultiSinkElement::setLocation(QString fileName)
{
    this->m_location = fileName;
}

void MultiSinkElement::setOptions(QString options)
{
    this->m_options = options;

    if (this->m_options.isEmpty())
        this->m_options.clear();
    else
        this->m_optionsMap = this->m_optionParser.parse(this->m_options);
}

void MultiSinkElement::setFrameSize(QSize frameSize)
{
    this->m_frameSize = frameSize;
}

void MultiSinkElement::resetLocation()
{
    this->setLocation("");
}

void MultiSinkElement::resetOptions()
{
    this->setOptions("");
}

void MultiSinkElement::resetFrameSize()
{
    this->setFrameSize(QSize());
}

void MultiSinkElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        this->state() != ElementStatePlaying)
        return;

    QString mimeType = packet.caps().mimeType();

    if (!this->m_outputContext)
        this->init();

    if (this->m_audioStream)
        this->m_audioPts = (double) this->m_audioStream->pts.val *
                                    this->m_audioStream->time_base.num /
                                    this->m_audioStream->time_base.den;
    else
        this->m_audioPts = 0.0;

    if (mimeType == "audio/x-raw")
    {
        if (!this->m_audioStream)
            return;

        if (packet.caps() != this->m_curAInputCaps)
        {
            AVCodecContext *codecContext = this->m_audioStream->codec;

            char layout[256];

            av_get_channel_layout_string(layout,
                                         sizeof(layout),
                                         codecContext->channels,
                                         codecContext->channel_layout);

            QString caps = QString("audio/x-raw,"
                                   "format=%1,"
                                   "channels=%2,"
                                   "rate=%3,"
                                   "layout=%4").arg(this->m_aFormatToFF.key(codecContext->sample_fmt))
                                               .arg(codecContext->channels)
                                               .arg(codecContext->sample_rate)
                                               .arg(layout);

            this->m_aCapsConvert->setProperty("caps", caps);
            this->m_curAInputCaps = packet.caps();
        }

        this->m_aCapsConvert->iStream(packet);
    }
    else if (mimeType == "video/x-raw")
    {
        if (!this->m_videoStream)
            return;

        if (packet.caps() != this->m_curVInputCaps)
        {
            QSize size(packet.caps().property("width").toInt(),
                       packet.caps().property("height").toInt());

            QSize curSize(this->m_curVInputCaps.property("width").toInt(),
                          this->m_curVInputCaps.property("height").toInt());

            if (size != curSize)
                this->adjustToInputFrameSize(size);

            this->m_curVInputCaps = packet.caps();
        }

        this->m_vFilter->iStream(packet);
    }
}

void MultiSinkElement::setState(ElementState state)
{
    QbElement::setState(state);
    this->m_vFilter->setState(this->m_state);
    this->m_aCapsConvert->setState(this->m_state);

    if (this->m_state == ElementStateNull)
        this->uninit();
}

void MultiSinkElement::processVFrame(const QbPacket &packet)
{
    int iWidth = packet.caps().property("width").toInt();
    int iHeight = packet.caps().property("height").toInt();
    QString mimeType = packet.caps().property("format").toString();

    PixelFormat iFormat;

    if (this->m_vFormatToFF.contains(mimeType))
        iFormat = this->m_vFormatToFF[mimeType];
    else
        return;

    AVPacket pkt;
    av_init_packet(&pkt);

    if (this->m_outputContext->oformat->flags & AVFMT_RAWPICTURE)
    {
        // Raw video case - directly store the picture in the packet
        static QbCaps caps;

        if (packet.caps() != caps)
        {
            this->cleanAll();

            this->m_pictureAlloc = avpicture_alloc(&this->m_oPicture,
                                                   iFormat,
                                                   iWidth,
                                                   iHeight);

            caps = packet.caps();
        }

        avpicture_fill(&this->m_oPicture,
                       (uint8_t *) packet.data(),
                       iFormat,
                       iWidth,
                       iHeight);

        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = this->m_videoStream->index;
        pkt.data = this->m_oPicture.data[0];
        pkt.size = sizeof(AVPicture);

        av_interleaved_write_frame(this->m_outputContext, &pkt);
    }
    else
    {
        // encode the image
        pkt.data = NULL; // packet data will be allocated by the encoder
        pkt.size = 0;

        avpicture_fill((AVPicture *) &this->m_vFrame,
                       (uint8_t *) packet.data(),
                       iFormat,
                       iWidth,
                       iHeight);

        this->m_vFrame.format = iFormat,
        this->m_vFrame.width = iWidth,
        this->m_vFrame.height = iHeight;
        this->m_vFrame.type = AVMEDIA_TYPE_VIDEO;

        AVRational timeBase = {packet.timeBase().num(),
                               packet.timeBase().den()};

        this->m_vFrame.pts = av_rescale_q(packet.pts(),
                                          timeBase,
                                          this->m_videoStream->time_base);

        int got_output;

        if (avcodec_encode_video2(this->m_videoStream->codec,
                                  &pkt,
                                  &this->m_vFrame,
                                  &got_output) < 0)
            return;

        // If size is zero, it means the image was buffered.
        if (got_output)
        {
            if (this->m_videoStream->codec->coded_frame->key_frame)
                pkt.flags |= AV_PKT_FLAG_KEY;

            pkt.stream_index = this->m_videoStream->index;

            // Write the compressed frame to the media file.
            av_interleaved_write_frame(this->m_outputContext, &pkt);
        }
    }
}

void MultiSinkElement::processAFrame(const QbPacket &packet)
{
    AVCodecContext *codecContext = this->m_audioStream->codec;

    AVRational timeBase = {packet.timeBase().num(),
                           packet.timeBase().den()};

    int64_t pts = av_rescale_q(packet.pts(),
                               timeBase,
                               this->m_audioStream->time_base);

    int64_t ptsDiff = codecContext->frame_size /
                      (packet.caps().property("rate").toFloat() *
                       packet.timeBase().value());

    int samples = packet.caps().property("samples").toInt();

    static AVFrame iFrame;
    avcodec_get_frame_defaults(&iFrame);

    iFrame.nb_samples = samples;

    if (avcodec_fill_audio_frame(&iFrame,
                                 codecContext->channels,
                                 codecContext->sample_fmt,
                                 (uint8_t *) packet.data(),
                                 packet.dataSize(),
                                 1) < 0)
        return;

    for (int offset = 0; offset < samples; offset += codecContext->frame_size)
    {
        QByteArray oBuffer(codecContext->frame_size *
                           av_get_bytes_per_sample(codecContext->sample_fmt) *
                           codecContext->channels,
                           0);

        static AVFrame oFrame;
        avcodec_get_frame_defaults(&oFrame);

        oFrame.nb_samples = codecContext->frame_size;

        if (avcodec_fill_audio_frame(&oFrame,
                                     codecContext->channels,
                                     codecContext->sample_fmt,
                                     (const uint8_t *) oBuffer.constData(),
                                     codecContext->frame_size *
                                     av_get_bytes_per_sample(codecContext->sample_fmt) *
                                     codecContext->channels,
                                     1) < 0)
            continue;

        if (av_samples_copy(oFrame.data,
                            iFrame.data,
                            0,
                            offset,
                            codecContext->frame_size,
                            codecContext->channels,
                            codecContext->sample_fmt) < 0)
            continue;

        AVPacket pkt;
        av_init_packet(&pkt);

        // data and size must be 0;
        pkt.data = 0;
        pkt.size = 0;

        oFrame.pts = pts;
        pts += ptsDiff;

        int got_packet;

        if (avcodec_encode_audio2(codecContext, &pkt, &oFrame, &got_packet) < 0 ||
            !got_packet)
            continue;

        pkt.stream_index = this->m_audioStream->index;

        av_interleaved_write_frame(this->m_outputContext, &pkt);
    }
}

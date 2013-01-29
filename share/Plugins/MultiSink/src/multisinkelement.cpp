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
    this->m_pictureAlloc = -1;

    this->m_vFilter = this->m_pipeline.add("VFilter");

    QObject::connect(this->m_vFilter,
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processVFrame(const QbPacket &)));

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

    this->m_mimeToFF["I420"] = PIX_FMT_YUV420P;
    this->m_mimeToFF["YUY2"] = PIX_FMT_YUV422P;
    this->m_mimeToFF["UYVY"] = PIX_FMT_UYVY422;
    this->m_mimeToFF["AYUV"] = PIX_FMT_YUVA420P;
    this->m_mimeToFF["RGBx"] = PIX_FMT_RGB0;
    this->m_mimeToFF["BGRx"] = PIX_FMT_BGR0;
    this->m_mimeToFF["xRGB"] = PIX_FMT_0RGB;
    this->m_mimeToFF["xBGR"] = PIX_FMT_0BGR;
    this->m_mimeToFF["RGBA"] = PIX_FMT_RGBA;
    this->m_mimeToFF["BGRA"] = PIX_FMT_BGRA;
    this->m_mimeToFF["ARGB"] = PIX_FMT_ARGB;
    this->m_mimeToFF["ABGR"] = PIX_FMT_ABGR;
    this->m_mimeToFF["RGB"] = PIX_FMT_RGB24;
    this->m_mimeToFF["BGR"] = PIX_FMT_BGR24;
    this->m_mimeToFF["Y41B"] = PIX_FMT_YUV411P;
    this->m_mimeToFF["Y42B"] = PIX_FMT_YUV422P;
    this->m_mimeToFF["YVYU"] = PIX_FMT_UYVY422;
    this->m_mimeToFF["Y444"] = PIX_FMT_YUV444P;
    this->m_mimeToFF["v210"] = PIX_FMT_YUV422P10LE;
    this->m_mimeToFF["v216"] = PIX_FMT_YUV422P16LE;
    this->m_mimeToFF["NV12"] = PIX_FMT_NV12;
    this->m_mimeToFF["NV21"] = PIX_FMT_NV21;
    this->m_mimeToFF["GRAY8"] = PIX_FMT_GRAY8;
    this->m_mimeToFF["GRAY16_BE"] = PIX_FMT_GRAY16BE;
    this->m_mimeToFF["GRAY16_LE"] = PIX_FMT_GRAY16LE;
    this->m_mimeToFF["v308"] = PIX_FMT_YUV444P;
    this->m_mimeToFF["RGB16"] = PIX_FMT_RGB565LE;
    this->m_mimeToFF["BGR16"] = PIX_FMT_BGR565LE;
    this->m_mimeToFF["RGB15"] = PIX_FMT_RGB555LE;
    this->m_mimeToFF["BGR15"] = PIX_FMT_BGR555LE;
    this->m_mimeToFF["UYVP"] = PIX_FMT_YUV422P12LE;
    this->m_mimeToFF["A420"] = PIX_FMT_YUVA420P;
    this->m_mimeToFF["RGB8P"] = PIX_FMT_RGB8;
    this->m_mimeToFF["IYU1"] = PIX_FMT_YUV411P;
    this->m_mimeToFF["I420_10LE"] = PIX_FMT_YUV420P10LE;
    this->m_mimeToFF["I420_10BE"] = PIX_FMT_YUV420P10BE;
    this->m_mimeToFF["I422_10LE"] = PIX_FMT_YUV422P10LE;
    this->m_mimeToFF["I422_10BE"] = PIX_FMT_YUV422P10BE;
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

        this->m_audioStream = this->addStream(&ffAudioCodec, this->m_optionsMap["acodec"].toString());
        this->m_audioStream->id = 1;

        if (this->m_audioStream)
            if (avcodec_open2(this->m_audioStream->codec, ffAudioCodec, NULL) < 0)
                return false;
    }

    if (!this->m_optionsMap.contains("vn"))
    {
        AVCodec *ffVideoCodec = NULL;

        this->m_videoStream = this->addStream(&ffVideoCodec, this->m_optionsMap["vcodec"].toString());
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

    return true;
}

void MultiSinkElement::uninit()
{
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

    if (!(m_outputContext->oformat->flags & AVFMT_NOFILE))
        // Close the output file.
        avio_close(this->m_outputContext->pb);

    // free the stream
    av_free(this->m_outputContext);
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

AVStream *MultiSinkElement::addStream(AVCodec **codec, QString codecName)
{
    AVOutputFormat *outputFormat = this->m_outputContext->oformat;

    // find the encoder
    if (codecName.isEmpty())
    {
        if (outputFormat->video_codec == AV_CODEC_ID_NONE)
        {
            *codec = NULL;

            return NULL;
        }

        *codec = avcodec_find_encoder(outputFormat->video_codec);
    }
    else
        *codec = avcodec_find_encoder_by_name(codecName.toUtf8().constData());

    if (!(*codec))
    {
        *codec = NULL;

        return NULL;
    }

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
            else if (!sampleRates.isEmpty())
                codecContext->sample_rate = sampleRates[0];

            if (this->m_optionsMap.contains("ac"))
                codecContext->channels = this->m_optionsMap["ac"].toInt();
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

            this->m_vFilter->setProperty("format", this->m_mimeToFF.key(codecContext->pix_fmt));

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

void MultiSinkElement::processVFrame(const QbPacket &packet)
{
    int iWidth = packet.caps().property("width").toInt();
    int iHeight = packet.caps().property("height").toInt();
    QString mimeType = packet.caps().property("format").toString();

    PixelFormat iFormat;

    if (this->m_mimeToFF.contains(mimeType))
        iFormat = this->m_mimeToFF[mimeType];
    else
        return;

    if (this->m_outputContext->oformat->flags & AVFMT_RAWPICTURE)
    {
        // Raw video case - directly store the picture in the packet
        AVPacket pkt;
        av_init_packet(&pkt);

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
        AVPacket pkt;
        int got_output;

        av_init_packet(&pkt);
        pkt.data = NULL; // packet data will be allocated by the encoder
        pkt.size = 0;

        AVFrame frame;

        avcodec_get_frame_defaults(&frame);

        avpicture_fill((AVPicture *) &frame,
                       (uint8_t *) packet.data(),
                       iFormat,
                       iWidth,
                       iHeight);

        frame.format = iFormat,
        frame.width = iWidth,
        frame.height = iHeight;
        frame.type = AVMEDIA_TYPE_VIDEO;

        frame.pkt_pts = packet.pts();
        frame.pkt_dts = packet.dts();
        frame.pkt_duration = packet.duration();
        frame.pts = av_frame_get_best_effort_timestamp(&frame);

        if (avcodec_encode_video2(this->m_videoStream->codec,
                                  &pkt,
                                  &frame,
                                  &got_output) < 0)
            return;

        // If size is zero, it means the image was buffered.
        if (got_output)
        {
            if (this->m_videoStream->codec->coded_frame->key_frame)
                pkt.flags |= AV_PKT_FLAG_KEY;

            pkt.stream_index = this->m_videoStream->index;

            /// Write the compressed frame to the media file.
            av_interleaved_write_frame(this->m_outputContext, &pkt);
        }
    }
}

void MultiSinkElement::iStream(const QbPacket &packet)
{
    Q_UNUSED(packet)

    QString mimeType = packet.caps().mimeType();

    if (mimeType == "audio/x-raw")
    {
        qDebug() << "audio";
    }
    else if (mimeType == "video/x-raw")
    {
        if (packet.caps() != this->m_curInputCaps)
        {
            QSize size(packet.caps().property("width").toInt(),
                       packet.caps().property("height").toInt());

            QSize curSize(this->m_curInputCaps.property("width").toInt(),
                          this->m_curInputCaps.property("height").toInt());

            if (size != curSize)
                this->adjustToInputFrameSize(size);

            this->m_curInputCaps = packet.caps();
        }

        this->m_vFilter->iStream(packet);
    }
}

void MultiSinkElement::setState(ElementState state)
{
    ElementState preState = this->state();

    switch (state)
    {
        case ElementStateNull:
            switch (preState)
            {
                case ElementStatePaused:
                case ElementStatePlaying:
                    this->setState(ElementStateReady);

                    if (this->state() != ElementStateReady)
                        return;

                case ElementStateReady:
                    this->uninit();
                    this->m_state = state;
                break;

                default:
                break;
            }
        break;

        case ElementStateReady:
            switch (preState)
            {
                case ElementStateNull:
                    if (this->init())
                        this->m_state = state;
                    else
                        this->m_state = ElementStateNull;
                break;

                case ElementStatePlaying:
                    this->setState(ElementStatePaused);

                    if (this->state() != ElementStatePaused)
                        return;

                case ElementStatePaused:
                    this->m_state = state;
                break;

                default:
                break;
            }
        break;

        case ElementStatePaused:
            switch (preState)
            {
                case ElementStateNull:
                    this->setState(ElementStateReady);

                    if (this->state() != ElementStateReady)
                        return;

                case ElementStateReady:
                case ElementStatePlaying:
                    this->m_state = state;
                break;

                default:
                break;
            }
        break;

        case ElementStatePlaying:
            switch (preState)
            {
                case ElementStateNull:
                case ElementStateReady:
                    this->setState(ElementStatePaused);

                    if (this->state() != ElementStatePaused)
                        return;

                case ElementStatePaused:
                    this->m_state = state;
                break;

                default:
                break;
            }
        break;

        default:
        break;
    }

    this->m_pipeline.setState(this->m_state);
}

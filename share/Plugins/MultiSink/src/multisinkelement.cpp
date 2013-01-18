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

QSize MultiSinkElement::frameSize()
{
    return this->m_frameSize;
}

bool MultiSinkElement::init()
{
    const char *location = this->location().toUtf8().constData();

    if (this->m_optionsMap.contains("f"))
        avformat_alloc_output_context2(&this->m_outputContext,
                                       NULL,
                                       this->m_optionsMap["f"].toString().toUtf8().constData(),
                                       location);
    else
        avformat_alloc_output_context2(&this->m_outputContext,
                                       NULL,
                                       NULL,
                                       location);

    if (!this->m_outputContext)
        return false;

    AVOutputFormat *outputFormat = this->m_outputContext->oformat;

    AVStream *videoStream = NULL;
    AVStream *audioStream = NULL;

    AVCodec *ffAudioCodec = NULL;
    AVCodec *ffVideoCodec = NULL;

    if (outputFormat->video_codec != AV_CODEC_ID_NONE)
        videoStream = addStream(this->m_outputContext, &ffVideoCodec, outputFormat->video_codec);

    if (outputFormat->audio_codec != AV_CODEC_ID_NONE)
        audioStream = addStream(this->m_outputContext, &ffAudioCodec, outputFormat->audio_codec);

    if (videoStream)
        open_video(this->m_outputContext, ffVideoCodec, videoStream);

    if (audioStream)
        open_audio(this->m_outputContext, ffAudioCodec, audioStream);

    av_dump_format(this->m_outputContext, 0, location, 1);

    if (!(outputFormat->flags & AVFMT_NOFILE))
        if (avio_open(&this->m_outputContext->pb, location, AVIO_FLAG_WRITE) < 0)
            return false;

    if (avformat_write_header(this->m_outputContext, NULL) < 0)
        return false;

    return true;
}

void MultiSinkElement::uninit()
{
}

AVStream *MultiSinkElement::addStream(AVCodec **codec, QString codecName)
{
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
    Q_UNUSED(packet)
/*
    QImage iFrame((const uchar *) packet.data(),
                  packet.caps().property("width").toInt(),
                  packet.caps().property("height").toInt(),
                  QImage::Format_RGB888);

    if (!iFrame.size().isValid())
        return;

    if (iFrame.size() != this->m_frameSize)
    {
        QImage scaledFrame(iFrame.scaled(this->m_frameSize, Qt::KeepAspectRatio));

        QPoint point((this->m_frameSize.width() - scaledFrame.width()) >> 1,
                     (this->m_frameSize.height() - scaledFrame.height()) >> 1);

        iFrame = QImage(this->m_frameSize, QImage::Format_RGB888);
        iFrame.fill(QColor(0, 0, 0));

        QPainter painter;

        painter.begin(&iFrame);
        painter.drawImage(point, scaledFrame);
        painter.end();
    }*/
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
}

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

#include "audiooutputelement.h"

AudioOutputElement::AudioOutputElement(): QbElement()
{
    this->m_audioOutput = NULL;
    this->m_audioDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    this->m_audioConvert = Qb::create("ACapsConvert");

    QObject::connect(this->m_audioConvert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->m_pullTimer.moveToThread(this->thread());

    QObject::connect(&this->m_pullTimer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(pullFrame()));
}

int AudioOutputElement::bufferSize() const
{
    return this->m_audioOutput? this->m_audioOutput->bufferSize(): 0;
}

QbCaps AudioOutputElement::findBestOptions(const QbCaps &caps,
                                           const QAudioDeviceInfo &deviceInfo,
                                           QAudioFormat *bestOption) const
{
    if (caps.mimeType() != "audio/x-raw")
        return QbCaps();

    QMap<AVSampleFormat, QAudioFormat::SampleType> formatToType;
    formatToType[AV_SAMPLE_FMT_NONE] = QAudioFormat::Unknown;
    formatToType[AV_SAMPLE_FMT_U8] = QAudioFormat::UnSignedInt;
    formatToType[AV_SAMPLE_FMT_S16] = QAudioFormat::SignedInt;
    formatToType[AV_SAMPLE_FMT_S32] = QAudioFormat::SignedInt;
    formatToType[AV_SAMPLE_FMT_FLT] = QAudioFormat::Float;
    formatToType[AV_SAMPLE_FMT_DBL] = QAudioFormat::Float;
    formatToType[AV_SAMPLE_FMT_U8P] = QAudioFormat::UnSignedInt;
    formatToType[AV_SAMPLE_FMT_S16P] = QAudioFormat::SignedInt;
    formatToType[AV_SAMPLE_FMT_S32P] = QAudioFormat::SignedInt;
    formatToType[AV_SAMPLE_FMT_FLTP] = QAudioFormat::Float;
    formatToType[AV_SAMPLE_FMT_DBLP] = QAudioFormat::Float;

    AVSampleFormat sampleFormat = av_get_sample_fmt(caps.property("format")
                                                        .toString()
                                                        .toStdString()
                                                        .c_str());

    QAudioFormat preferredAudioFormat;
    preferredAudioFormat.setByteOrder(QAudioFormat::BigEndian);
    preferredAudioFormat.setChannelCount(caps.property("channels").toInt());
    preferredAudioFormat.setCodec("audio/pcm");
    preferredAudioFormat.setSampleRate(caps.property("rate").toInt());
    preferredAudioFormat.setSampleSize(8 * caps.property("bps").toInt());
    preferredAudioFormat.setSampleType(formatToType[sampleFormat]);

    QAudioFormat bestAudioFormat = deviceInfo.nearestFormat(preferredAudioFormat);
    AVSampleFormat oFormat = AV_SAMPLE_FMT_NONE;

    foreach (AVSampleFormat format, formatToType.keys(bestAudioFormat.sampleType()))
        if (av_get_bytes_per_sample(format) == (bestAudioFormat.sampleSize() >> 3))
        {
            oFormat = format;

            break;
        }

    char layout[256];
    int64_t channelLayout = av_get_default_channel_layout(bestAudioFormat.channelCount());

    av_get_channel_layout_string(layout,
                                 sizeof(layout),
                                 bestAudioFormat.channelCount(),
                                 channelLayout);

    QbCaps oCaps(QString("audio/x-raw,"
                         "format=%1,"
                         "bps=%2,"
                         "channels=%3,"
                         "rate=%4,"
                         "layout=%5,"
                         "align=%6").arg(av_get_sample_fmt_name(oFormat))
                                    .arg(bestAudioFormat.sampleSize() >> 3)
                                    .arg(bestAudioFormat.channelCount())
                                    .arg(bestAudioFormat.sampleRate())
                                    .arg(layout)
                                    .arg(false));

    if (bestOption)
        *bestOption = bestAudioFormat;

    return oCaps;
}

void AudioOutputElement::processFrame(const QbPacket &packet)
{
    int bufferSize = packet.caps().property("bps").toInt()
                     * packet.caps().property("channels").toInt()
                     * packet.caps().property("samples").toInt();

    this->m_audioBuffer.append((const char *) packet.buffer().data(),
                               bufferSize);
}

void AudioOutputElement::pullFrame()
{
    if (this->m_audioOutput)
    {
        int writeBytes = qMin(qMin(this->m_audioOutput->bytesFree(),
                                   this->m_audioOutput->periodSize()),
                              this->m_audioBuffer.size());

        if (writeBytes > 0)
        {
            qDebug() << "writeBytes" << writeBytes;
            this->m_outputDevice->write(this->m_audioBuffer.mid(0, writeBytes));
            this->m_audioBuffer.remove(0, writeBytes);
/*
            if (this->m_audioBuffer.size() < this->m_audioOutput->bufferSize())
                this->m_waitCondition.wakeAll();*/
        }
    }
}

void AudioOutputElement::iStream(const QbPacket &packet)
{
/*
    this->m_mutex.lock();

    if (this->m_audioOutput
        && this->m_audioBuffer.size() > this->m_audioOutput->bufferSize())
        this->m_waitCondition.wait(&this->m_mutex);

    this->m_mutex.unlock();
*/
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "audio/x-raw" ||
        this->state() != ElementStatePlaying)
        return;

    static QbCaps curCaps;

    QbCaps caps1(packet.caps());
    QbCaps caps2(curCaps);

    caps1.setProperty("samples", QVariant());
    caps2.setProperty("samples", QVariant());

    if (caps1 != caps2)
    {
        this->m_audioConvert->setState(QbElement::ElementStateNull);

        QAudioFormat outputFormat;
        QbCaps caps = this->findBestOptions(packet.caps(), this->m_audioDeviceInfo, &outputFormat);
        this->m_audioConvert->setProperty("caps", caps.toString());

        if (this->m_audioOutput)
            delete this->m_audioOutput;

        this->m_audioOutput = new QAudioOutput(this->m_audioDeviceInfo,
                                               outputFormat,
                                               this);

        this->m_outputDevice = this->m_audioOutput->start();

        curCaps = packet.caps();
    }

    if (this->m_audioConvert->state() != QbElement::ElementStatePlaying)
        this->m_audioConvert->setState(QbElement::ElementStatePlaying);

    this->m_audioConvert->iStream(packet);
}

void AudioOutputElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);

    if (state == QbElement::ElementStateNull)
        if (this->m_audioOutput)
            delete this->m_audioOutput;

    if (state == QbElement::ElementStatePlaying)
        this->m_pullTimer.start();
    else
        this->m_pullTimer.stop();

    this->m_audioConvert->setState(state);
}

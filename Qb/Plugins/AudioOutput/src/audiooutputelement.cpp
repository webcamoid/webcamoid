/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "audiooutputelement.h"

AudioOutputElement::AudioOutputElement(): QbElement()
{
    this->m_outputDevice = NULL;
    this->m_timeDrift = 0;
    this->m_streamId = -1;
    this->m_audioDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    this->m_convert = QbElement::create("ACapsConvert");
    this->resetBufferSize();

    QObject::connect(this,
                     SIGNAL(stateChanged(QbElement::ElementState)),
                     this->m_convert.data(),
                     SLOT(setState(QbElement::ElementState)));

    QObject::connect(&this->m_audioBuffer,
                     SIGNAL(cleared()),
                     this,
                     SLOT(releaseInput()));

    QObject::connect(&this->m_audioBuffer,
                     SIGNAL(bytesConsumed()),
                     this,
                     SLOT(updateClock()));
}

AudioOutputElement::~AudioOutputElement()
{
    this->uninit();
}

int AudioOutputElement::bufferSize() const
{
    return this->m_bufferSize;
}

double AudioOutputElement::clock() const
{
    return this->hwClock() + this->m_timeDrift;
}

QbCaps AudioOutputElement::findBestOptions(const QAudioFormat &audioFormat) const
{
    QString format = QbUtils::defaultSampleFormat(audioFormat.sampleType(),
                                                  audioFormat.sampleSize(),
                                                  false);
    QString layout = QbUtils::defaultChannelLayout(audioFormat.channelCount());

    QbCaps caps(QString("audio/x-raw,"
                        "format=%1,"
                        "bps=%2,"
                        "channels=%3,"
                        "rate=%4,"
                        "layout=%5,"
                        "align=%6").arg(format)
                                   .arg(audioFormat.sampleSize() >> 3)
                                   .arg(audioFormat.channelCount())
                                   .arg(audioFormat.sampleRate())
                                   .arg(layout)
                                   .arg(false));

    return caps;
}

double AudioOutputElement::hwClock() const
{
    if (!this->m_audioOutput)
        return 0;

    int bytesInBuffer = this->m_audioOutput->bufferSize()
                        - this->m_audioOutput->bytesFree();

    int sampleSize = this->m_audioOutput->format().sampleSize();
    int channels = this->m_audioOutput->format().channelCount();
    int sampleRate = this->m_audioOutput->format().sampleRate();

    double usInBuffer = 8.0 * bytesInBuffer
                        / (sampleSize
                           * channels
                           * sampleRate);

    double pts = this->m_audioOutput->processedUSecs()
                 / 1.0e6
                 - usInBuffer;

    return pts;
}

bool AudioOutputElement::init()
{
    QAudioDeviceInfo audioDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    QAudioFormat outputFormat = audioDeviceInfo.preferredFormat();

    QbCaps bestCaps = this->findBestOptions(outputFormat);

    this->m_convert->setProperty("caps", bestCaps.toString());

    this->m_audioOutput = AudioOutputPtr(new QAudioOutput(audioDeviceInfo,
                                                          outputFormat));

    if (this->m_audioOutput) {
        this->m_timeDrift = 0;
        int bps = bestCaps.property("bps").toInt();
        int channels = bestCaps.property("channels").toInt();
        qint64 bufferSize = bps * channels * this->m_bufferSize;

        this->m_audioOutput->setBufferSize(bufferSize);
        this->m_audioBuffer.setMaxSize(bufferSize << 5);
        this->m_audioBuffer.open(QIODevice::ReadWrite);
        this->m_audioOutput->start(&this->m_audioBuffer);
    }

    return this->m_outputDevice? true: false;
}

void AudioOutputElement::uninit()
{
    this->m_mutex.lock();
    this->m_bufferEmpty.wakeAll();
    this->m_mutex.unlock();

    if (this->m_audioOutput) {
        this->m_audioOutput->stop();
        this->m_audioOutput.clear();
        this->m_outputDevice = NULL;
    }

    this->m_audioBuffer.close();
}

void AudioOutputElement::stateChange(QbElement::ElementState from,
                                     QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        this->init();
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull)
        this->uninit();
}

void AudioOutputElement::setBufferSize(int bufferSize)
{
    this->m_bufferSize = bufferSize;
}

void AudioOutputElement::resetBufferSize()
{
    this->setBufferSize(4096);
}

void AudioOutputElement::releaseInput()
{
    this->m_mutex.lock();
    this->m_bufferEmpty.wakeAll();
    this->m_mutex.unlock();
}

void AudioOutputElement::updateClock()
{
    emit this->elapsedTime(this->clock());
}

QbPacket AudioOutputElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() != "audio/x-raw"
        || !this->m_audioOutput)
        return QbPacket();

    if (packet.id() != this->m_streamId) {
        this->m_mutex.lock();

        if (this->m_audioBuffer.size() > 0)
            this->m_bufferEmpty.wait(&this->m_mutex);

        this->m_mutex.unlock();
        this->m_streamId = packet.id();

        this->m_timeDrift = packet.pts()
                            * packet.timeBase().value()
                            - this->hwClock();
    }

    emit this->elapsedTime(this->clock());

    QbPacket iPacket = this->m_convert->iStream(packet);
    this->m_audioBuffer.writePacket(iPacket);

    return QbPacket();
}

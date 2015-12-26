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

#include <qb.h>

#include "audioinputelement.h"

AudioInputElement::AudioInputElement(): QbElement()
{
    this->m_bufferSize = 1024;

    QbAudioCaps::SampleFormat sampleFormat;
    int channels;
    int sampleRate;
    this->m_audioDevice.preferredFormat(AudioDevice::DeviceModeCapture,
                                        &sampleFormat,
                                        &channels,
                                        &sampleRate);

    QbAudioCaps caps;
    caps.isValid() = true;
    caps.format() = sampleFormat;
    caps.bps() = QbAudioCaps::bitsPerSample(sampleFormat);
    caps.channels() = channels;
    caps.rate() = sampleRate;
    caps.layout() = channels > 1? QbAudioCaps::Layout_mono: QbAudioCaps::Layout_stereo;
    caps.align() = false;

    this->m_caps = caps.toCaps();
    this->m_streamId = -1;
    this->m_timeBase = QbFrac(1, caps.rate());
    this->m_threadedRead = true;

    QObject::connect(&this->m_timer,
                     &QTimer::timeout,
                     this,
                     &AudioInputElement::readFrame);
}

AudioInputElement::~AudioInputElement()
{
    this->uninit();
}

int AudioInputElement::bufferSize() const
{
    return this->m_bufferSize;
}

QbCaps AudioInputElement::caps() const
{
    return this->m_caps;
}

void AudioInputElement::sendPacket(AudioInputElement *element,
                                   const QbPacket &packet)
{
    emit element->oStream(packet);
}

void AudioInputElement::stateChange(QbElement::ElementState from,
                                    QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        this->init();
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull)
        this->uninit();
}

void AudioInputElement::setBufferSize(int bufferSize)
{
    if (this->m_bufferSize == bufferSize)
        return;

    this->m_bufferSize = bufferSize;
    emit this->bufferSizeChanged(bufferSize);
}

void AudioInputElement::setCaps(const QbCaps &caps)
{
    if (this->m_caps == caps)
        return;

    this->m_caps = caps;
    emit this->capsChanged(caps);
}

void AudioInputElement::resetBufferSize()
{
    this->setBufferSize(1024);
}

void AudioInputElement::resetCaps()
{
    QbAudioCaps::SampleFormat sampleFormat;
    int channels;
    int sampleRate;
    this->m_audioDevice.preferredFormat(AudioDevice::DeviceModeCapture,
                                        &sampleFormat,
                                        &channels,
                                        &sampleRate);

    QbAudioCaps caps;
    caps.isValid() = true;
    caps.format() = sampleFormat;
    caps.bps() = QbAudioCaps::bitsPerSample(sampleFormat);
    caps.channels() = channels;
    caps.rate() = sampleRate;
    caps.layout() = channels > 1? QbAudioCaps::Layout_mono: QbAudioCaps::Layout_stereo;
    caps.align() = false;

    this->setCaps(caps.toCaps());
}

bool AudioInputElement::init()
{
    QbAudioCaps caps(this->m_caps);
    this->m_streamId = Qb::id();
    this->m_timeBase = QbFrac(1, caps.rate());

    this->m_mutex.lock();
    bool result = this->m_audioDevice.init(AudioDevice::DeviceModeCapture,
                                           caps.format(),
                                           caps.channels(),
                                           caps.rate());
    this->m_mutex.unlock();
    this->m_timer.start();

    return result;
}

void AudioInputElement::uninit()
{
    this->m_timer.stop();
    this->m_mutex.lock();
    this->m_audioDevice.uninit();
    this->m_mutex.unlock();
}

void AudioInputElement::readFrame()
{
    this->m_mutex.lock();
    QByteArray buffer = this->m_audioDevice.read(this->m_bufferSize);
    this->m_mutex.unlock();

    if (buffer.isEmpty())
        return;

    QByteArray oBuffer(buffer.size(), Qt::Uninitialized);
    memcpy(oBuffer.data(), buffer.constData(), buffer.size());

    QbCaps caps = this->m_caps;
    caps.setProperty("samples", this->m_bufferSize);

    QbPacket packet(caps, oBuffer);

    qint64 pts = QTime::currentTime().msecsSinceStartOfDay()
                 / this->m_timeBase.value();

    packet.setPts(pts);
    packet.setTimeBase(this->m_timeBase);
    packet.setIndex(0);
    packet.setId(this->m_streamId);

    if (!this->m_threadedRead) {
        emit this->oStream(packet);

        return;
    }

    if (!this->m_threadStatus.isRunning()) {
        this->m_curPacket = packet;

        this->m_threadStatus = QtConcurrent::run(&this->m_threadPool,
                                                 this->sendPacket,
                                                 this,
                                                 this->m_curPacket);
    }
}

/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include "audiodeviceelement.h"

typedef QMap<AudioDeviceElement::DeviceMode, QString> DeviceModeMap;

inline DeviceModeMap initDeviceModeMap()
{
    DeviceModeMap deviceModeToStr;
    deviceModeToStr[AudioDeviceElement::DeviceModeInput] = "input";
    deviceModeToStr[AudioDeviceElement::DeviceModeOutput] = "output";

    return deviceModeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(DeviceModeMap, deviceModeToStr, (initDeviceModeMap()))

AudioDeviceElement::AudioDeviceElement(): AkElement()
{
    this->m_bufferSize = 1024;
    this->m_mode = DeviceModeOutput;
    AkAudioCaps audioCaps = this->defaultCaps(this->m_mode);
    this->m_caps = audioCaps.toCaps();
    this->m_streamId = -1;
    this->m_timeBase = AkFrac(1, audioCaps.rate());
    this->m_threadedRead = true;

    QObject::connect(&this->m_timer,
                     &QTimer::timeout,
                     this,
                     &AudioDeviceElement::readFrame);
}

AudioDeviceElement::~AudioDeviceElement()
{
    this->uninit();
}

int AudioDeviceElement::bufferSize() const
{
    return this->m_bufferSize;
}

AkCaps AudioDeviceElement::caps() const
{
    return this->m_caps;
}

QString AudioDeviceElement::mode() const
{
    return deviceModeToStr->value(this->m_mode);
}

AkAudioCaps AudioDeviceElement::defaultCaps(AudioDeviceElement::DeviceMode mode)
{
    AkAudioCaps::SampleFormat sampleFormat;
    int channels;
    int sampleRate;
    this->m_audioDevice.preferredFormat(mode == DeviceModeOutput?
                                            AudioDev::DeviceModePlayback:
                                            AudioDev::DeviceModeCapture,
                                        &sampleFormat,
                                        &channels,
                                        &sampleRate);

    AkAudioCaps caps;
    caps.isValid() = true;
    caps.format() = sampleFormat;
    caps.bps() = AkAudioCaps::bitsPerSample(sampleFormat);
    caps.channels() = channels;
    caps.rate() = sampleRate;
    caps.layout() = AkAudioCaps::defaultChannelLayout(channels);
    caps.align() = false;

    return caps;
}

void AudioDeviceElement::sendPacket(AudioDeviceElement *element,
                                   const AkPacket &packet)
{
    emit element->oStream(packet);
}

void AudioDeviceElement::stateChange(AkElement::ElementState from,
                                    AkElement::ElementState to)
{
    if (from == AkElement::ElementStateNull
        && to == AkElement::ElementStatePaused)
        this->init();
    else if (from == AkElement::ElementStatePaused
             && to == AkElement::ElementStateNull)
        this->uninit();
}

void AudioDeviceElement::setBufferSize(int bufferSize)
{
    if (this->m_bufferSize == bufferSize)
        return;

    this->m_bufferSize = bufferSize;
    emit this->bufferSizeChanged(bufferSize);
}

void AudioDeviceElement::setCaps(const AkCaps &caps)
{
    if (this->m_caps == caps)
        return;

    this->m_caps = caps;
    emit this->capsChanged(caps);
}

void AudioDeviceElement::setMode(const QString &mode)
{
    DeviceMode modeEnum = deviceModeToStr->key(mode, DeviceModeOutput);

    if (this->m_mode == modeEnum)
        return;

    this->m_mode = modeEnum;
    this->setCaps(this->defaultCaps(this->m_mode).toCaps());
    emit this->modeChanged(mode);
}

void AudioDeviceElement::resetBufferSize()
{
    this->setBufferSize(1024);
}

void AudioDeviceElement::resetCaps()
{
    this->setCaps(this->defaultCaps(this->m_mode).toCaps());
}

void AudioDeviceElement::resetMode()
{
    this->setMode("output");
}

AkPacket AudioDeviceElement::iStream(const AkAudioPacket &packet)
{
    this->m_mutex.lock();

    if (this->m_convert) {
        AkPacket iPacket = this->m_convert->iStream(packet.toPacket());
        this->m_audioDevice.write(iPacket.buffer());
    }

    this->m_mutex.unlock();

    return AkPacket();
}

bool AudioDeviceElement::init()
{
    this->m_mutex.lock();
    AkAudioCaps caps(this->m_caps);
    DeviceMode mode = this->m_mode;

    if (mode == DeviceModeInput) {
        this->m_streamId = Ak::id();
        this->m_timeBase = AkFrac(1, caps.rate());
    } else {
        this->m_convert = AkElement::create("ACapsConvert");

        QObject::connect(this,
                         SIGNAL(stateChanged(AkElement::ElementState)),
                         this->m_convert.data(),
                         SLOT(setState(AkElement::ElementState)));

        this->m_convert->setProperty("caps", caps.toString());
    }

    bool result = this->m_audioDevice.init(mode == DeviceModeOutput?
                                               AudioDev::DeviceModePlayback:
                                               AudioDev::DeviceModeCapture,
                                           caps.format(),
                                           caps.channels(),
                                           caps.rate());

    if (result && mode == DeviceModeInput)
        this->m_timer.start();

    this->m_mutex.unlock();

    return result;
}

void AudioDeviceElement::uninit()
{
    this->m_mutex.lock();
    this->m_timer.stop();
    this->m_audioDevice.uninit();
    this->m_mutex.unlock();
}

void AudioDeviceElement::readFrame()
{
    this->m_mutex.lock();
    QByteArray buffer = this->m_audioDevice.read(this->m_bufferSize);
    this->m_mutex.unlock();

    if (buffer.isEmpty())
        return;

    QByteArray oBuffer(buffer.size(), Qt::Uninitialized);
    memcpy(oBuffer.data(), buffer.constData(), buffer.size());

    AkCaps caps = this->m_caps;
    caps.setProperty("samples", this->m_bufferSize);

    AkPacket packet(caps, oBuffer);

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

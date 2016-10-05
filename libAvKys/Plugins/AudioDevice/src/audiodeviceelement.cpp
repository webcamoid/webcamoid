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

#define PAUSE_TIMEOUT 500

typedef QMap<AudioDeviceElement::DeviceMode, QString> DeviceModeMap;

inline DeviceModeMap initDeviceModeMap()
{
    DeviceModeMap deviceModeToStr = {
        {AudioDeviceElement::DeviceModeInput      , "input"      },
        {AudioDeviceElement::DeviceModeOutput     , "output"     },
        {AudioDeviceElement::DeviceModeDummyOutput, "dummyoutput"}
    };

    return deviceModeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(DeviceModeMap, deviceModeToStr, (initDeviceModeMap()))

AudioDeviceElement::AudioDeviceElement(): AkElement()
{
    this->m_bufferSize = 1024;
    this->m_mode = DeviceModeOutput;
    AkAudioCaps audioCaps = this->defaultCaps(this->m_mode);
    this->m_caps = audioCaps.toCaps();
    this->m_readFramesLoop = false;
    this->m_pause = false;

    QObject::connect(&this->m_audioDevice,
                     &AudioDev::defaultInputChanged,
                     this,
                     &AudioDeviceElement::defaultInputChanged);
    QObject::connect(&this->m_audioDevice,
                     &AudioDev::defaultOutputChanged,
                     this,
                     &AudioDeviceElement::defaultOutputChanged);
    QObject::connect(&this->m_audioDevice,
                     &AudioDev::inputsChanged,
                     this,
                     &AudioDeviceElement::inputsChanged);
    QObject::connect(&this->m_audioDevice,
                     &AudioDev::outputsChanged,
                     this,
                     &AudioDeviceElement::outputsChanged);
}

AudioDeviceElement::~AudioDeviceElement()
{
    this->setState(AkElement::ElementStateNull);
}

QString AudioDeviceElement::defaultInput()
{
    return this->m_audioDevice.defaultInput();
}

QString AudioDeviceElement::defaultOutput()
{
    return this->m_audioDevice.defaultOutput();
}

QStringList AudioDeviceElement::inputs()
{
    return this->m_audioDevice.inputs();
}

QStringList AudioDeviceElement::outputs()
{
    return this->m_audioDevice.outputs();
}

QString AudioDeviceElement::description(const QString &device)
{
    return this->m_audioDevice.description(device);
}

QString AudioDeviceElement::device() const
{
    return this->m_device;
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
    if (mode == AudioDeviceElement::DeviceModeDummyOutput)
        return AkAudioCaps("audio/x-raw,format=s16,bps=2,channels=2,rate=44100,layout=stereo,align=false");

    return this->m_audioDevice.preferredFormat(mode == DeviceModeInput?
                                                   this->m_audioDevice.defaultInput():
                                                   this->m_audioDevice.defaultOutput());
}

void AudioDeviceElement::readFramesLoop(AudioDeviceElement *self)
{
#ifdef Q_OS_WIN32
    // Initialize the COM library in multithread mode.
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif

    QString defaultDevice = self->defaultInput();
    AkAudioCaps caps(self->m_caps);
    qint64 streamId = Ak::id();
    AkFrac timeBase(1, caps.rate());

    if (self->m_audioDevice.init(defaultDevice, caps)) {
        while (self->m_readFramesLoop) {
            if (self->m_pause) {
                QThread::msleep(PAUSE_TIMEOUT);

                continue;
            }

            int bufferSize = self->m_bufferSize;
            QByteArray buffer = self->m_audioDevice.read(bufferSize);

            if (buffer.isEmpty())
                return;

            QByteArray oBuffer(buffer.size(), Qt::Uninitialized);
            memcpy(oBuffer.data(), buffer.constData(), size_t(buffer.size()));

            caps.samples() = bufferSize;
            AkAudioPacket packet(caps, oBuffer);

            qint64 pts = qint64(QTime::currentTime().msecsSinceStartOfDay()
                                / timeBase.value() / 1e3);

            packet.setPts(pts);
            packet.setTimeBase(timeBase);
            packet.setIndex(0);
            packet.setId(streamId);

            emit self->oStream(packet.toPacket());
        }

        self->m_audioDevice.uninit();
    }

#ifdef Q_OS_WIN32
    // Close COM library.
    CoUninitialize();
#endif
}

void AudioDeviceElement::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(device);
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

void AudioDeviceElement::resetDevice()
{
    this->setDevice("");
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

    if (this->m_mode == AudioDeviceElement::DeviceModeDummyOutput)
        QThread::usleep(ulong(1e6 * packet.caps().samples() / packet.caps().rate()));
    else if (this->m_convert) {
        AkPacket iPacket = this->m_convert->iStream(packet.toPacket());
        this->m_audioDevice.write(iPacket.buffer());
    }

    this->m_mutex.unlock();

    return AkPacket();
}

bool AudioDeviceElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused: {
            if (this->m_mode == DeviceModeInput) {
                this->m_pause = true;
                this->m_readFramesLoop = true;
                this->m_readFramesLoopResult = QtConcurrent::run(&this->m_threadPool,
                                                                 this->readFramesLoop,
                                                                 this);
            }

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePlaying: {
            switch (this->m_mode) {
            case DeviceModeInput: {
                this->m_pause = false;
                this->m_readFramesLoop = true;
                this->m_readFramesLoopResult = QtConcurrent::run(&this->m_threadPool,
                                                                 this->readFramesLoop,
                                                                 this);
                break;
            }
            case DeviceModeOutput: {
                this->m_mutex.lock();

                QString defaulDevice = this->defaultOutput();
                AkAudioCaps caps(this->m_caps);
                this->m_convert = AkElement::create("ACapsConvert");

                QObject::connect(this,
                                 SIGNAL(stateChanged(AkElement::ElementState)),
                                 this->m_convert.data(),
                                 SLOT(setState(AkElement::ElementState)));

                this->m_convert->setProperty("caps", caps.toString());

                if (!this->m_audioDevice.init(defaulDevice, caps)) {
                    this->m_convert.clear();
                    this->m_mutex.unlock();

                    return false;
                } else {
                    this->m_mutex.unlock();

                    break;
                }
            }
            case DeviceModeDummyOutput: {
                break;
            }
            }

            return AkElement::setState(state);
        }
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            switch (this->m_mode) {
            case DeviceModeInput: {
                this->m_pause = false;
                this->m_readFramesLoop = false;
                this->m_readFramesLoopResult.waitForFinished();

                break;
            }
            case DeviceModeOutput: {
                this->m_audioDevice.uninit();

                break;
            }
            case DeviceModeDummyOutput: {
                break;
            }
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            switch (this->m_mode) {
            case DeviceModeInput: {
                this->m_pause = false;

                break;
            }
            case DeviceModeOutput: {
                this->m_mutex.lock();

                QString defaultDevice = this->defaultOutput();
                AkAudioCaps caps(this->m_caps);
                this->m_convert = AkElement::create("ACapsConvert");

                QObject::connect(this,
                                 SIGNAL(stateChanged(AkElement::ElementState)),
                                 this->m_convert.data(),
                                 SLOT(setState(AkElement::ElementState)));

                this->m_convert->setProperty("caps", caps.toString());

                if (!this->m_audioDevice.init(defaultDevice, caps)) {
                    this->m_convert.clear();
                    this->m_mutex.unlock();

                    return false;
                } else {
                    this->m_mutex.unlock();

                    break;
                }
            }
            case DeviceModeDummyOutput: {
                break;
            }
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            switch (this->m_mode) {
            case DeviceModeInput: {
                this->m_pause = false;
                this->m_readFramesLoop = false;
                this->m_readFramesLoopResult.waitForFinished();

                break;
            }
            case DeviceModeOutput: {
                this->m_audioDevice.uninit();

                break;
            }
            case DeviceModeDummyOutput: {
                break;
            }
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            switch (this->m_mode) {
            case DeviceModeInput: {
                this->m_pause = true;

                break;
            }
            case DeviceModeOutput: {
                this->m_audioDevice.uninit();

                break;
            }
            case DeviceModeDummyOutput: {
                break;
            }
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

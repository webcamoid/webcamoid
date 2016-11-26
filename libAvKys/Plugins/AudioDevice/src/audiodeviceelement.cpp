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
#define DUMMY_OUTPUT_DEVICE ":dummyout:"

#ifdef Q_OS_WIN32
Q_GLOBAL_STATIC_WITH_ARGS(QStringList, preferredLibrary, ({"wasapi"}))
#elif defined(Q_OS_OSX)
Q_GLOBAL_STATIC_WITH_ARGS(QStringList, preferredLibrary, ({"coreaudio"}))
#else
Q_GLOBAL_STATIC_WITH_ARGS(QStringList, preferredLibrary, ({"pulseaudio", "jack", "alsa"}))
#endif

template<typename T>
inline QSharedPointer<T> obj_cast(QObject *obj)
{
    return QSharedPointer<T>(dynamic_cast<T *>(obj));
}

AudioDeviceElement::AudioDeviceElement(): AkElement()
{
    this->m_bufferSize = 1024;
    this->m_readFramesLoop = false;
    this->m_pause = false;
    this->m_convert = AkElement::create("ACapsConvert");

    QObject::connect(this,
                     SIGNAL(stateChanged(AkElement::ElementState)),
                     this->m_convert.data(),
                     SLOT(setState(AkElement::ElementState)));
    QObject::connect(this,
                     &AudioDeviceElement::audioLibChanged,
                     this,
                     &AudioDeviceElement::audioLibUpdated);

    this->resetAudioLib();
}

AudioDeviceElement::~AudioDeviceElement()
{
    this->setState(AkElement::ElementStateNull);
}

QString AudioDeviceElement::defaultInput()
{
    this->m_mutexLib.lock();

    auto defaultInput = this->m_audioDevice?
                            this->m_audioDevice->defaultInput(): QString();

    this->m_mutexLib.unlock();

    return defaultInput;
}

QString AudioDeviceElement::defaultOutput()
{
    this->m_mutexLib.lock();

    auto defaultOutput = this->m_audioDevice?
                             this->m_audioDevice->defaultOutput(): QString();

    this->m_mutexLib.unlock();

    return defaultOutput;
}

QStringList AudioDeviceElement::inputs()
{
    this->m_mutexLib.lock();

    auto inputs = this->m_audioDevice?
                      this->m_audioDevice->inputs(): QStringList();

    this->m_mutexLib.unlock();

    return inputs;
}

QStringList AudioDeviceElement::outputs()
{
    this->m_mutexLib.lock();

    auto outputs =
            this->m_audioDevice?
                this->m_audioDevice->outputs(): QStringList();

    this->m_mutexLib.unlock();

    return outputs + QStringList {DUMMY_OUTPUT_DEVICE};
}

QString AudioDeviceElement::description(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return QString("Dummy Output");

    this->m_mutexLib.lock();

    auto description = this->m_audioDevice?
                           this->m_audioDevice->description(device): QString();

    this->m_mutexLib.unlock();

    return description;
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

QString AudioDeviceElement::audioLib() const
{
    return this->m_audioLib;
}

void AudioDeviceElement::readFramesLoop(AudioDeviceElement *self)
{
#ifdef Q_OS_WIN32
    // Initialize the COM library in multithread mode.
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif

    QString device = self->m_device;
    AkAudioCaps caps(self->m_caps);
    qint64 streamId = Ak::id();
    AkFrac timeBase(1, caps.rate());

    if (self->m_audioDevice->init(device, caps)) {
        while (self->m_readFramesLoop) {
            if (self->m_pause) {
                QThread::msleep(PAUSE_TIMEOUT);

                continue;
            }

            int bufferSize = self->m_bufferSize;
            QByteArray buffer = self->m_audioDevice->read(bufferSize);

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

        self->m_audioDevice->uninit();
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

    this->m_mutexLib.lock();

    auto preferredFormat =
            this->m_audioDevice?
                this->m_audioDevice->preferredFormat(device): AkAudioCaps();

    this->m_mutexLib.unlock();

    AkAudioCaps audioCaps = device == DUMMY_OUTPUT_DEVICE?
                                AkAudioCaps("audio/x-raw,"
                                            "format=s16,"
                                            "bps=2,"
                                            "channels=2,"
                                            "rate=44100,"
                                            "layout=stereo,"
                                            "align=false"):
                                preferredFormat;

    this->setCaps(audioCaps.toCaps());
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

void AudioDeviceElement::setAudioLib(const QString &audioLib)
{
    if (this->m_audioLib == audioLib)
        return;

    this->m_audioLib = audioLib;
    emit this->audioLibChanged(audioLib);
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
    this->m_mutexLib.lock();

    auto preferredFormat =
            this->m_audioDevice?
                this->m_audioDevice->preferredFormat(this->m_device): AkAudioCaps();

    this->m_mutexLib.unlock();

    auto audioCaps = this->m_device == DUMMY_OUTPUT_DEVICE?
                                AkAudioCaps("audio/x-raw,"
                                            "format=s16,"
                                            "bps=2,"
                                            "channels=2,"
                                            "rate=44100,"
                                            "layout=stereo,"
                                            "align=false"):
                                preferredFormat;

    this->setCaps(audioCaps.toCaps());
}

void AudioDeviceElement::resetAudioLib()
{
    auto subModules = this->listSubModules("AudioDevice");

    for (const QString &framework: *preferredLibrary)
        if (subModules.contains(framework)) {
            this->setAudioLib(framework);

            return;
        }

    if (this->m_audioLib.isEmpty() && !subModules.isEmpty())
        this->setAudioLib(subModules.first());
    else
        this->setAudioLib("");
}

AkPacket AudioDeviceElement::iStream(const AkAudioPacket &packet)
{
    this->m_mutex.lock();

    if (this->state() != ElementStatePlaying) {
        this->m_mutex.unlock();

        return AkPacket();
    }

    if (this->m_device == DUMMY_OUTPUT_DEVICE)
        QThread::usleep(ulong(1e6 * packet.caps().samples() / packet.caps().rate()));
    else if (this->m_convert) {
        AkPacket iPacket = this->m_convert->iStream(packet.toPacket());

        this->m_mutexLib.lock();

        if (this->m_audioDevice)
            this->m_audioDevice->write(iPacket.buffer());

        this->m_mutexLib.unlock();
    }

    this->m_mutex.unlock();

    return AkPacket();
}

bool AudioDeviceElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();
    QMutexLocker locker(&this->m_mutexLib);

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused: {
            if (this->m_audioDevice
                && this->m_audioDevice->inputs().contains(this->m_device)) {
                this->m_pause = true;
                this->m_readFramesLoop = true;
                this->m_readFramesLoopResult = QtConcurrent::run(&this->m_threadPool,
                                                                 this->readFramesLoop,
                                                                 this);
            }

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePlaying: {
            if (this->m_audioDevice) {
                if (this->m_audioDevice->inputs().contains(this->m_device)) {
                    this->m_pause = false;
                    this->m_readFramesLoop = true;
                    this->m_readFramesLoopResult = QtConcurrent::run(&this->m_threadPool,
                                                                     this->readFramesLoop,
                                                                     this);
                } else if (this->m_device != DUMMY_OUTPUT_DEVICE
                           && this->m_audioDevice->outputs().contains(this->m_device)) {
                    this->m_mutex.lock();

                    QString device = this->m_device;
                    AkAudioCaps caps(this->m_caps);
                    this->m_convert->setProperty("caps", caps.toString());

                    if (!this->m_audioDevice->init(device, caps)) {
                        this->m_mutex.unlock();

                        return false;
                    }

                    this->m_mutex.unlock();
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
            if (this->m_audioDevice) {
                if (this->m_audioDevice->inputs().contains(this->m_device)) {
                    this->m_pause = false;
                    this->m_readFramesLoop = false;
                    this->m_readFramesLoopResult.waitForFinished();
                } else if (this->m_device != DUMMY_OUTPUT_DEVICE
                           && this->m_audioDevice->outputs().contains(this->m_device)) {
                    this->m_audioDevice->uninit();
                }
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            if (this->m_audioDevice) {
                if (this->m_audioDevice->inputs().contains(this->m_device)) {
                    this->m_pause = false;
                } else if (this->m_device != DUMMY_OUTPUT_DEVICE
                           && this->m_audioDevice->outputs().contains(this->m_device)) {
                    this->m_mutex.lock();

                    QString device = this->m_device;
                    AkAudioCaps caps(this->m_caps);
                    this->m_convert->setProperty("caps", caps.toString());

                    if (!this->m_audioDevice->init(device, caps)) {
                        this->m_mutex.unlock();

                        return false;
                    }

                    this->m_mutex.unlock();
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
            this->m_mutex.lock();

            if (this->m_audioDevice) {
                if (this->m_audioDevice->inputs().contains(this->m_device)) {
                    this->m_pause = false;
                    this->m_readFramesLoop = false;
                    this->m_readFramesLoopResult.waitForFinished();
                } else if (this->m_device != DUMMY_OUTPUT_DEVICE
                           && this->m_audioDevice->outputs().contains(this->m_device)) {
                    this->m_audioDevice->uninit();
                }
            }

            this->m_mutex.unlock();

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            this->m_mutex.lock();

            if (this->m_audioDevice) {
                if (this->m_audioDevice->inputs().contains(this->m_device)) {
                    this->m_pause = true;
                } else if (this->m_device != DUMMY_OUTPUT_DEVICE
                           && this->m_audioDevice->outputs().contains(this->m_device)) {
                    this->m_audioDevice->uninit();
                }
            }

            this->m_mutex.unlock();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

void AudioDeviceElement::audioLibUpdated(const QString &audioLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    this->m_mutexLib.lock();

    this->m_audioDevice =
            obj_cast<AudioDev>(this->loadSubModule("AudioDevice",
                                                   audioLib));

    if (this->m_audioDevice) {
        QObject::connect(this->m_audioDevice.data(),
                         &AudioDev::defaultInputChanged,
                         this,
                         &AudioDeviceElement::defaultInputChanged);
        QObject::connect(this->m_audioDevice.data(),
                         &AudioDev::defaultOutputChanged,
                         this,
                         &AudioDeviceElement::defaultOutputChanged);
        QObject::connect(this->m_audioDevice.data(),
                         &AudioDev::inputsChanged,
                         this,
                         &AudioDeviceElement::inputsChanged);
        QObject::connect(this->m_audioDevice.data(),
                         &AudioDev::outputsChanged,
                         this,
                         &AudioDeviceElement::outputsChanged);
    }

    this->m_mutexLib.unlock();

    emit this->inputsChanged(this->inputs());
    emit this->outputsChanged(this->outputs());
    emit this->defaultInputChanged(this->defaultInput());
    emit this->defaultOutputChanged(this->defaultOutput());
    emit this->deviceChanged(this->device());
    emit this->capsChanged(this->caps());

    if (this->m_audioDevice)
        this->setState(state);
}

/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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
#include "audiodeviceglobals.h"

#define PAUSE_TIMEOUT 500
#define DUMMY_OUTPUT_DEVICE ":dummyout:"

#ifdef Q_OS_WIN32
#include <combaseapi.h>
#endif

Q_GLOBAL_STATIC(AudioDeviceGlobals, globalAudioDevice)

template<typename T>
inline QSharedPointer<T> ptr_init(QObject *obj=nullptr)
{
    if (!obj)
        return QSharedPointer<T>(new T());

    return QSharedPointer<T>(static_cast<T *>(obj));
}

AudioDeviceElement::AudioDeviceElement():
    AkElement(),
    m_audioDevice(ptr_init<AudioDev>())
{
    this->m_bufferSize = 1024;
    this->m_readFramesLoop = false;
    this->m_pause = false;
    this->m_convert = AkElement::create("ACapsConvert");

    QObject::connect(globalAudioDevice,
                     SIGNAL(audioLibChanged(const QString &)),
                     this,
                     SIGNAL(audioLibChanged(const QString &)));
    QObject::connect(globalAudioDevice,
                     SIGNAL(audioLibChanged(const QString &)),
                     this,
                     SLOT(audioLibUpdated(const QString &)));

    this->audioLibUpdated(globalAudioDevice->audioLib());
}

AudioDeviceElement::~AudioDeviceElement()
{
    this->setState(AkElement::ElementStateNull);
}

QString AudioDeviceElement::defaultInput()
{
    this->m_mutexLib.lock();
    auto defaultInput = this->m_audioDevice->defaultInput();
    this->m_mutexLib.unlock();

    return defaultInput;
}

QString AudioDeviceElement::defaultOutput()
{
    this->m_mutexLib.lock();
    auto defaultOutput = this->m_audioDevice->defaultOutput();
    this->m_mutexLib.unlock();

    return defaultOutput;
}

QStringList AudioDeviceElement::inputs()
{
    return this->m_inputs;
}

QStringList AudioDeviceElement::outputs()
{
    return this->m_outputs + QStringList {DUMMY_OUTPUT_DEVICE};
}

QString AudioDeviceElement::description(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return QString("Dummy Output");

    this->m_mutexLib.lock();
    auto description = this->m_audioDevice->description(device);
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

AkAudioCaps AudioDeviceElement::preferredFormat(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return AkAudioCaps(AkAudioCaps::SampleFormat_s16,
                           2,
                           44100);

    this->m_mutexLib.lock();
    auto preferredFormat = this->m_audioDevice->preferredFormat(device);
    this->m_mutexLib.unlock();

    return preferredFormat;
}

QList<AkAudioCaps::SampleFormat> AudioDeviceElement::supportedFormats(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return QList<AkAudioCaps::SampleFormat> {
            AkAudioCaps::SampleFormat_flt,
            AkAudioCaps::SampleFormat_s32,
            AkAudioCaps::SampleFormat_s16,
            AkAudioCaps::SampleFormat_u8
        };

    this->m_mutexLib.lock();
    auto supportedFormats = this->m_audioDevice->supportedFormats(device);
    this->m_mutexLib.unlock();

    return supportedFormats;
}

QList<int> AudioDeviceElement::supportedChannels(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return QList<int> {1, 2};

    this->m_mutexLib.lock();
    auto supportedChannels = this->m_audioDevice->supportedChannels(device);
    this->m_mutexLib.unlock();

    return supportedChannels;
}

QList<int> AudioDeviceElement::supportedSampleRates(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return this->m_audioDevice->m_commonSampleRates.toList();

    this->m_mutexLib.lock();
    auto supportedSampleRates = this->m_audioDevice->supportedSampleRates(device);
    this->m_mutexLib.unlock();

    return supportedSampleRates;
}

QString AudioDeviceElement::audioLib() const
{
    return globalAudioDevice->audioLib();
}

void AudioDeviceElement::readFramesLoop()
{
#ifdef Q_OS_WIN32
    // Initialize the COM library in multithread mode.
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif

    QString device = this->m_device;
    AkAudioCaps caps(this->m_caps);
    qint64 streamId = Ak::id();
    AkFrac timeBase(1, caps.rate());

    if (this->m_audioDevice->init(device, caps)) {
        while (this->m_readFramesLoop) {
            if (this->m_pause) {
                QThread::msleep(PAUSE_TIMEOUT);

                continue;
            }

            int bufferSize = this->m_bufferSize;
            QByteArray buffer = this->m_audioDevice->read(bufferSize);

            if (buffer.isEmpty())
                return;

            QByteArray oBuffer(buffer.size(), 0);
            memcpy(oBuffer.data(), buffer.constData(), size_t(buffer.size()));

            caps.samples() = bufferSize;
            AkAudioPacket packet(caps, oBuffer);

            qint64 pts = qint64(QTime::currentTime().msecsSinceStartOfDay()
                                / timeBase.value() / 1e3);

            packet.setPts(pts);
            packet.setTimeBase(timeBase);
            packet.setIndex(0);
            packet.setId(streamId);

            emit this->oStream(packet.toPacket());
        }

        this->m_audioDevice->uninit();
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
    this->m_convert->setProperty("caps", caps.toString());
    emit this->capsChanged(caps);
}

void AudioDeviceElement::setAudioLib(const QString &audioLib)
{
    globalAudioDevice->setAudioLib(audioLib);
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
    auto preferredFormat = this->preferredFormat(this->m_device);
    this->m_mutexLib.unlock();

    this->setCaps(preferredFormat.toCaps());
}

void AudioDeviceElement::resetAudioLib()
{
    globalAudioDevice->resetAudioLib();
}

AkPacket AudioDeviceElement::iStream(const AkAudioPacket &packet)
{
    this->m_mutex.lock();

    if (this->state() != ElementStatePlaying) {
        this->m_mutex.unlock();

        return AkPacket();
    }

    auto device = this->m_device;
    this->m_mutex.unlock();

    if (device == DUMMY_OUTPUT_DEVICE)
        QThread::usleep(ulong(1e6
                              * packet.caps().samples()
                              / packet.caps().rate()));
    else {
        AkPacket iPacket;

        this->m_mutex.lock();

        if (this->m_convert)
            iPacket = this->m_convert->iStream(packet.toPacket());

        this->m_mutex.unlock();

        if (iPacket) {
            this->m_mutexLib.lock();
            this->m_audioDevice->write(iPacket);
            this->m_mutexLib.unlock();
        }
    }

    return AkPacket();
}

bool AudioDeviceElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused: {
            if (this->m_inputs.contains(this->m_device)) {
                this->m_convert->setState(state);
                this->m_pause = true;
                this->m_readFramesLoop = true;
                this->m_readFramesLoopResult = QtConcurrent::run(&this->m_threadPool,
                                                                 this,
                                                                 &AudioDeviceElement::readFramesLoop);
            }

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePlaying: {
            if (this->m_inputs.contains(this->m_device)) {
                this->m_convert->setState(state);
                this->m_pause = false;
                this->m_readFramesLoop = true;
                this->m_readFramesLoopResult = QtConcurrent::run(&this->m_threadPool,
                                                                 this,
                                                                 &AudioDeviceElement::readFramesLoop);
            } else if (this->m_device != DUMMY_OUTPUT_DEVICE
                       && this->m_outputs.contains(this->m_device)) {
                this->m_convert->setState(state);
                QString device = this->m_device;
                AkAudioCaps caps(this->m_caps);

                this->m_mutexLib.lock();
                auto isInit = this->m_audioDevice->init(device, caps);
                this->m_mutexLib.unlock();

                if (!isInit)
                    return false;
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
            if (this->m_inputs.contains(this->m_device)) {
                this->m_pause = false;
                this->m_readFramesLoop = false;
                this->m_readFramesLoopResult.waitForFinished();
                this->m_convert->setState(state);
            } else if (this->m_device != DUMMY_OUTPUT_DEVICE
                       && this->m_outputs.contains(this->m_device)) {
                this->m_mutexLib.lock();
                this->m_audioDevice->uninit();
                this->m_mutexLib.unlock();

                this->m_convert->setState(state);
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            if (this->m_inputs.contains(this->m_device)) {
                this->m_convert->setState(state);
                this->m_pause = false;
            } else if (this->m_device != DUMMY_OUTPUT_DEVICE
                       && this->m_outputs.contains(this->m_device)) {
                this->m_convert->setState(state);
                QString device = this->m_device;
                AkAudioCaps caps(this->m_caps);

                this->m_mutexLib.lock();
                auto isInit = this->m_audioDevice->init(device, caps);
                this->m_mutexLib.unlock();

                if (!isInit)
                    return false;
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
            if (this->m_inputs.contains(this->m_device)) {
                this->m_pause = false;
                this->m_readFramesLoop = false;
                this->m_readFramesLoopResult.waitForFinished();
                this->m_convert->setState(state);
            } else if (this->m_device != DUMMY_OUTPUT_DEVICE
                       && this->m_outputs.contains(this->m_device)) {
                this->m_mutexLib.lock();
                this->m_audioDevice->uninit();
                this->m_mutexLib.unlock();
                this->m_convert->setState(state);
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            if (this->m_inputs.contains(this->m_device)) {
                this->m_pause = true;
                this->m_convert->setState(state);
            } else if (this->m_device != DUMMY_OUTPUT_DEVICE
                       && this->m_outputs.contains(this->m_device)) {
                this->m_mutexLib.lock();
                this->m_audioDevice->uninit();
                this->m_mutexLib.unlock();
                this->m_convert->setState(state);
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

void AudioDeviceElement::setInputs(const QStringList &inputs)
{
    if (this->m_inputs == inputs)
        return;

    this->m_inputs = inputs;
    emit this->inputsChanged(inputs);
}

void AudioDeviceElement::setOutputs(const QStringList &outputs)
{
    if (this->m_outputs == outputs)
        return;

    this->m_outputs = outputs;
    emit this->outputsChanged(outputs);
}

void AudioDeviceElement::audioLibUpdated(const QString &audioLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    bool isInput = this->m_inputs.contains(this->m_device);

    this->m_mutexLib.lock();

    this->m_audioDevice =
            ptr_init<AudioDev>(this->loadSubModule("AudioDevice",
                                                   audioLib));

    this->m_mutexLib.unlock();

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
                     &AudioDeviceElement::setInputs);
    QObject::connect(this->m_audioDevice.data(),
                     &AudioDev::outputsChanged,
                     this,
                     &AudioDeviceElement::setOutputs);

    this->setInputs(this->m_audioDevice->inputs());
    this->setOutputs(this->m_audioDevice->outputs());
    emit this->defaultInputChanged(this->m_audioDevice->defaultInput());
    emit this->defaultOutputChanged(this->m_audioDevice->defaultOutput());

    if (this->m_device != DUMMY_OUTPUT_DEVICE) {
        this->setDevice(isInput?
                            this->m_audioDevice->defaultInput():
                            this->m_audioDevice->defaultOutput());
        auto preferredFormat = this->m_audioDevice->preferredFormat(this->m_device);
        this->setCaps(preferredFormat.toCaps());
    }

    this->setState(state);
}

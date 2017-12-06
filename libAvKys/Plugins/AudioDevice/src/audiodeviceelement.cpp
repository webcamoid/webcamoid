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

#include <QSharedPointer>
#include <QtConcurrent>
#include <QThreadPool>
#include <QFuture>
#include <QTime>
#include <ak.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akaudiopacket.h>

#include "audiodeviceelement.h"
#include "audiodeviceglobals.h"
#include "audiodev.h"

#define PAUSE_TIMEOUT 500
#define DUMMY_OUTPUT_DEVICE ":dummyout:"

#ifdef Q_OS_WIN32
#include <combaseapi.h>
#endif

Q_GLOBAL_STATIC(AudioDeviceGlobals, globalAudioDevice)

template<typename T>
inline QSharedPointer<T> ptr_cast(QObject *obj=nullptr)
{
    return QSharedPointer<T>(static_cast<T *>(obj));
}

typedef QSharedPointer<AudioDev> AudioDevPtr;

class AudioDeviceElementPrivate
{
    public:
        AudioDeviceElement *self;
        QStringList m_inputs;
        QStringList m_outputs;
        QString m_device;
        int m_bufferSize;
        AkCaps m_caps;
        AudioDevPtr m_audioDevice;
        AkElementPtr m_convert;
        QThreadPool m_threadPool;
        QFuture<void> m_readFramesLoopResult;
        QMutex m_mutex;
        QMutex m_mutexLib;
        bool m_readFramesLoop;
        bool m_pause;

        AudioDeviceElementPrivate(AudioDeviceElement *self):
            self(self),
            m_bufferSize(1024),
            m_readFramesLoop(false),
            m_pause(false)
        {
            this->m_convert = AkElement::create("ACapsConvert");
        }

        inline void readFramesLoop();
};

AudioDeviceElement::AudioDeviceElement():
    AkElement()
{
    this->d = new AudioDeviceElementPrivate(this);

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
    delete this->d;
}

QString AudioDeviceElement::defaultInput()
{
    QString defaultInput;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        defaultInput = this->d->m_audioDevice->defaultInput();

    this->d->m_mutexLib.unlock();

    return defaultInput;
}

QString AudioDeviceElement::defaultOutput()
{
    QString defaultOutput;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        defaultOutput = this->d->m_audioDevice->defaultOutput();

    this->d->m_mutexLib.unlock();

    return defaultOutput;
}

QStringList AudioDeviceElement::inputs()
{
    return this->d->m_inputs;
}

QStringList AudioDeviceElement::outputs()
{
    return this->d->m_outputs + QStringList {DUMMY_OUTPUT_DEVICE};
}

QString AudioDeviceElement::description(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return QString("Dummy Output");

    QString description;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        description = this->d->m_audioDevice->description(device);

    this->d->m_mutexLib.unlock();

    return description;
}

QString AudioDeviceElement::device() const
{
    return this->d->m_device;
}

int AudioDeviceElement::bufferSize() const
{
    return this->d->m_bufferSize;
}

AkCaps AudioDeviceElement::caps() const
{
    return this->d->m_caps;
}

AkAudioCaps AudioDeviceElement::preferredFormat(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return AkAudioCaps(AkAudioCaps::SampleFormat_s16,
                           2,
                           44100);

    AkAudioCaps preferredFormat;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        preferredFormat = this->d->m_audioDevice->preferredFormat(device);

    this->d->m_mutexLib.unlock();

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

    QList<AkAudioCaps::SampleFormat> supportedFormats;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        supportedFormats = this->d->m_audioDevice->supportedFormats(device);

    this->d->m_mutexLib.unlock();

    return supportedFormats;
}

QList<int> AudioDeviceElement::supportedChannels(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return QList<int> {1, 2};

    QList<int> supportedChannels;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        supportedChannels = this->d->m_audioDevice->supportedChannels(device);

    this->d->m_mutexLib.unlock();

    return supportedChannels;
}

QList<int> AudioDeviceElement::supportedSampleRates(const QString &device)
{
    if (device == DUMMY_OUTPUT_DEVICE)
        return this->d->m_audioDevice->commonSampleRates().toList();

    QList<int> supportedSampleRates;

    this->d->m_mutexLib.lock();

    if (this->d->m_audioDevice)
        supportedSampleRates = this->d->m_audioDevice->supportedSampleRates(device);

    this->d->m_mutexLib.unlock();

    return supportedSampleRates;
}

QString AudioDeviceElement::audioLib() const
{
    return globalAudioDevice->audioLib();
}

void AudioDeviceElementPrivate::readFramesLoop()
{
    if (!this->m_audioDevice)
        return;

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

            emit self->oStream(packet.toPacket());
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
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;
    emit this->deviceChanged(device);
}

void AudioDeviceElement::setBufferSize(int bufferSize)
{
    if (this->d->m_bufferSize == bufferSize)
        return;

    this->d->m_bufferSize = bufferSize;
    emit this->bufferSizeChanged(bufferSize);
}

void AudioDeviceElement::setCaps(const AkCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    this->d->m_convert->setProperty("caps", caps.toString());
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
    this->d->m_mutexLib.lock();
    auto preferredFormat = this->preferredFormat(this->d->m_device);
    this->d->m_mutexLib.unlock();

    this->setCaps(preferredFormat.toCaps());
}

void AudioDeviceElement::resetAudioLib()
{
    globalAudioDevice->resetAudioLib();
}

AkPacket AudioDeviceElement::iStream(const AkAudioPacket &packet)
{
    if (!this->d->m_audioDevice)
        return AkPacket();

    this->d->m_mutex.lock();

    if (this->state() != ElementStatePlaying) {
        this->d->m_mutex.unlock();

        return AkPacket();
    }

    auto device = this->d->m_device;
    this->d->m_mutex.unlock();

    if (device == DUMMY_OUTPUT_DEVICE)
        QThread::usleep(ulong(1e6
                              * packet.caps().samples()
                              / packet.caps().rate()));
    else {
        AkPacket iPacket;

        this->d->m_mutex.lock();

        if (this->d->m_convert)
            iPacket = this->d->m_convert->iStream(packet.toPacket());

        this->d->m_mutex.unlock();

        if (iPacket) {
            this->d->m_mutexLib.lock();
            this->d->m_audioDevice->write(iPacket);
            this->d->m_mutexLib.unlock();
        }
    }

    return AkPacket();
}

bool AudioDeviceElement::setState(AkElement::ElementState state)
{
    if (!this->d->m_audioDevice)
        return false;

    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused: {
            if (this->d->m_inputs.contains(this->d->m_device)) {
                this->d->m_convert->setState(state);
                this->d->m_pause = true;
                this->d->m_readFramesLoop = true;
                this->d->m_readFramesLoopResult = QtConcurrent::run(&this->d->m_threadPool,
                                                                     this->d,
                                                                     &AudioDeviceElementPrivate::readFramesLoop);
            }

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePlaying: {
            if (this->d->m_inputs.contains(this->d->m_device)) {
                this->d->m_convert->setState(state);
                this->d->m_pause = false;
                this->d->m_readFramesLoop = true;
                this->d->m_readFramesLoopResult = QtConcurrent::run(&this->d->m_threadPool,
                                                                     this->d,
                                                                     &AudioDeviceElementPrivate::readFramesLoop);
            } else if (this->d->m_device != DUMMY_OUTPUT_DEVICE
                       && this->d->m_outputs.contains(this->d->m_device)) {
                this->d->m_convert->setState(state);
                QString device = this->d->m_device;
                AkAudioCaps caps(this->d->m_caps);

                this->d->m_mutexLib.lock();
                auto isInit = this->d->m_audioDevice->init(device, caps);
                this->d->m_mutexLib.unlock();

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
            if (this->d->m_inputs.contains(this->d->m_device)) {
                this->d->m_pause = false;
                this->d->m_readFramesLoop = false;
                this->d->m_readFramesLoopResult.waitForFinished();
                this->d->m_convert->setState(state);
            } else if (this->d->m_device != DUMMY_OUTPUT_DEVICE
                       && this->d->m_outputs.contains(this->d->m_device)) {
                this->d->m_mutexLib.lock();
                this->d->m_audioDevice->uninit();
                this->d->m_mutexLib.unlock();

                this->d->m_convert->setState(state);
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            if (this->d->m_inputs.contains(this->d->m_device)) {
                this->d->m_convert->setState(state);
                this->d->m_pause = false;
            } else if (this->d->m_device != DUMMY_OUTPUT_DEVICE
                       && this->d->m_outputs.contains(this->d->m_device)) {
                this->d->m_convert->setState(state);
                QString device = this->d->m_device;
                AkAudioCaps caps(this->d->m_caps);

                this->d->m_mutexLib.lock();
                auto isInit = this->d->m_audioDevice->init(device, caps);
                this->d->m_mutexLib.unlock();

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
            if (this->d->m_inputs.contains(this->d->m_device)) {
                this->d->m_pause = false;
                this->d->m_readFramesLoop = false;
                this->d->m_readFramesLoopResult.waitForFinished();
                this->d->m_convert->setState(state);
            } else if (this->d->m_device != DUMMY_OUTPUT_DEVICE
                       && this->d->m_outputs.contains(this->d->m_device)) {
                this->d->m_mutexLib.lock();
                this->d->m_audioDevice->uninit();
                this->d->m_mutexLib.unlock();
                this->d->m_convert->setState(state);
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            if (this->d->m_inputs.contains(this->d->m_device)) {
                this->d->m_pause = true;
                this->d->m_convert->setState(state);
            } else if (this->d->m_device != DUMMY_OUTPUT_DEVICE
                       && this->d->m_outputs.contains(this->d->m_device)) {
                this->d->m_mutexLib.lock();
                this->d->m_audioDevice->uninit();
                this->d->m_mutexLib.unlock();
                this->d->m_convert->setState(state);
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
    if (this->d->m_inputs == inputs)
        return;

    this->d->m_inputs = inputs;
    emit this->inputsChanged(inputs);
}

void AudioDeviceElement::setOutputs(const QStringList &outputs)
{
    if (this->d->m_outputs == outputs)
        return;

    this->d->m_outputs = outputs;
    emit this->outputsChanged(outputs);
}

void AudioDeviceElement::audioLibUpdated(const QString &audioLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    bool isInput = this->d->m_inputs.contains(this->d->m_device);

    this->d->m_mutexLib.lock();

    this->d->m_audioDevice =
            ptr_cast<AudioDev>(this->loadSubModule("AudioDevice",
                                                   audioLib));

    if (!this->d->m_audioDevice) {
        this->d->m_mutexLib.unlock();

        return;
    }

    this->d->m_mutexLib.unlock();

    QObject::connect(this->d->m_audioDevice.data(),
                     &AudioDev::defaultInputChanged,
                     this,
                     &AudioDeviceElement::defaultInputChanged);
    QObject::connect(this->d->m_audioDevice.data(),
                     &AudioDev::defaultOutputChanged,
                     this,
                     &AudioDeviceElement::defaultOutputChanged);
    QObject::connect(this->d->m_audioDevice.data(),
                     &AudioDev::inputsChanged,
                     this,
                     &AudioDeviceElement::setInputs);
    QObject::connect(this->d->m_audioDevice.data(),
                     &AudioDev::outputsChanged,
                     this,
                     &AudioDeviceElement::setOutputs);

    this->setInputs(this->d->m_audioDevice->inputs());
    this->setOutputs(this->d->m_audioDevice->outputs());
    emit this->defaultInputChanged(this->d->m_audioDevice->defaultInput());
    emit this->defaultOutputChanged(this->d->m_audioDevice->defaultOutput());

    if (this->d->m_device != DUMMY_OUTPUT_DEVICE) {
        this->setDevice(isInput?
                            this->d->m_audioDevice->defaultInput():
                            this->d->m_audioDevice->defaultOutput());
        auto preferredFormat = this->d->m_audioDevice->preferredFormat(this->d->m_device);
        this->setCaps(preferredFormat.toCaps());
    }

    this->setState(state);
}

#include "moc_audiodeviceelement.cpp"

/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <QCoreApplication>
#include <QMap>
#include <QVector>
#include <QMutex>
#include <QWaitCondition>
#include <ak.h>
#include <akaudiopacket.h>
#include <jack/jack.h>

#include "audiodevjack.h"
#include "jackserver.h"

typedef QMap<jack_status_t, QString> JackErrorCodes;

inline JackErrorCodes initJackErrorCodes()
{
    JackErrorCodes jackErrorCodes = {
        {JackFailure      , "Overall operation failed"                                 },
        {JackInvalidOption, "The operation contained an invalid or unsupported option" },
        {JackNameNotUnique, "The desired client name was not unique"                   },
        {JackServerStarted, "The JACK server was started as a result of this operation"},
        {JackServerFailed , "Unable to connect to the JACK server"                     },
        {JackServerError  , "Communication error with the JACK server"                 },
        {JackNoSuchClient , "Requested client does not exist"                          },
        {JackLoadFailure  , "Unable to load internal client"                           },
        {JackInitFailure  , "Unable to initialize client"                              },
        {JackShmFailure   , "Unable to access shared memory"                           },
        {JackVersionError , "Client's protocol version does not match"                 },
        {JackBackendError , "Backend error"                                            },
        {JackClientZombie , "Client zombified failure"                                 }
    };

    return jackErrorCodes;
}

Q_GLOBAL_STATIC_WITH_ARGS(JackErrorCodes, jackErrorCodes, (initJackErrorCodes()))

class AudioDevJackPrivate
{
    public:
        QString m_error;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkAudioCaps> m_caps;
        QMap<QString, QStringList> m_devicePorts;
        QList<jack_port_t *> m_appPorts;
        QString m_curDevice;
        int m_sampleRate;
        int m_curChannels;
        int m_maxBufferSize;
        bool m_isInput;
        QByteArray m_buffer;
        jack_client_t *m_client;
        QMutex m_mutex;
        QWaitCondition m_canWrite;
        QWaitCondition m_samplesAvailable;

        AudioDevJackPrivate():
            m_sampleRate(0),
            m_curChannels(0),
            m_maxBufferSize(0),
            m_isInput(false),
            m_client(nullptr)
        {
        }

        static int onProcessCallback(jack_nframes_t nframes, void *userData);
        static void onShutdownCallback(void *userData);
};

AudioDevJack::AudioDevJack(QObject *parent):
    AudioDev(parent)
{
    this->d = new AudioDevJackPrivate;
    this->d->m_descriptions = {
        {":jackinput:" , "JACK Audio Connection Kit Input" },
        {":jackoutput:", "JACK Audio Connection Kit Output"},
    };

    auto appName = QCoreApplication::applicationName()
                   + QString("_%1").arg(Ak::id());
    int maxNameSize = jack_client_name_size() - 1;

    if (appName.size() > maxNameSize)
        appName = appName.mid(0, maxNameSize);

    jack_status_t status;
    this->d->m_client = jack_client_open(appName.toStdString().c_str(),
                                         JackNullOption,
                                         &status);

    if (!this->d->m_client) {
        this->d->m_error = jackErrorCodes->value(status);
        Q_EMIT this->errorChanged(this->d->m_error);

        return;
    }

    // Setup callbacks

    jack_set_process_callback(this->d->m_client,
                              AudioDevJackPrivate::onProcessCallback,
                              this);
    jack_on_shutdown(this->d->m_client,
                     AudioDevJackPrivate::onShutdownCallback,
                     this);

    QMap<QString, JackPortFlags> portTypeMap = {
        {":jackinput:" , JackPortIsOutput},
        {":jackoutput:", JackPortIsInput }
    };

    // Query the number of channels
    this->d->m_sampleRate = int(jack_get_sample_rate(this->d->m_client));

    for (auto deviceId: portTypeMap.keys()) {
        auto ports = jack_get_ports(this->d->m_client,
                                    nullptr,
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsPhysical | portTypeMap[deviceId]);
        int channels = 0;

        for (auto portName = ports; portName && *portName; portName++, channels++)
            this->d->m_devicePorts[deviceId] << *portName;

        if (ports)
            jack_free(ports);

        if (channels > 0)
            this->d->m_caps[deviceId] =
                    AkAudioCaps(AkAudioCaps::SampleFormat_flt,
                                channels,
                                this->d->m_sampleRate);
    }
}

AudioDevJack::~AudioDevJack()
{
    this->uninit();

    if (this->d->m_client)
        jack_client_close(this->d->m_client);

    delete this->d;
}

QString AudioDevJack::error() const
{
    return this->d->m_error;
}

QString AudioDevJack::defaultInput()
{
    return this->d->m_caps.contains(":jackinput:")?
                QString(":jackinput:"): QString();
}

QString AudioDevJack::defaultOutput()
{
    return this->d->m_caps.contains(":jackoutput:")?
                QString(":jackoutput:"): QString();
}

QStringList AudioDevJack::inputs()
{
    return this->d->m_caps.contains(":jackinput:")?
                QStringList {":jackinput:"}: QStringList();
}

QStringList AudioDevJack::outputs()
{
    return this->d->m_caps.contains(":jackoutput:")?
                QStringList {":jackoutput:"}: QStringList();
}

QString AudioDevJack::description(const QString &device)
{
    return this->d->m_caps.contains(device)?
                this->d->m_descriptions.value(device): QString();
}

AkAudioCaps AudioDevJack::preferredFormat(const QString &device)
{
    return this->d->m_caps.value(device);
}

QList<AkAudioCaps::SampleFormat> AudioDevJack::supportedFormats(const QString &device)
{
    Q_UNUSED(device)

    return QList<AkAudioCaps::SampleFormat> {AkAudioCaps::SampleFormat_flt};
}

QList<int> AudioDevJack::supportedChannels(const QString &device)
{
    QList<int> supportedChannels;

    for (int i = 0; i < this->d->m_devicePorts.value(device).size(); i++)
        supportedChannels << i + 1;

    return supportedChannels;
}

QList<int> AudioDevJack::supportedSampleRates(const QString &device)
{
    Q_UNUSED(device)

    return QList<int> {this->d->m_sampleRate};
}

bool AudioDevJack::init(const QString &device, const AkAudioCaps &caps)
{
    if (!this->d->m_caps.contains(device)
        || caps.channels() < 1
        || caps.channels() > 2
        || caps.rate() != this->d->m_sampleRate
        || caps.format() != AkAudioCaps::SampleFormat_flt)
        return false;

    this->d->m_appPorts.clear();
    this->d->m_curChannels = 0;
    this->d->m_buffer.clear();

    QString portName = device == ":jackinput:"?
                           "input": "output";
    JackPortFlags portFlags = device == ":jackinput:"?
                                  JackPortIsInput: JackPortIsOutput;

    // Create ports for sending/receiving data
    for (int channel = 0; channel < caps.channels(); channel++) {
        auto port = jack_port_register(this->d->m_client,
                                       QString("%1_%2")
                                           .arg(portName)
                                           .arg(channel + 1).toStdString().c_str(),
                                       JACK_DEFAULT_AUDIO_TYPE,
                                       portFlags,
                                       0);

        if (port)
            this->d->m_appPorts << port;
    }

    if (this->d->m_appPorts.size() < caps.channels()) {
        this->d->m_error = "AudioDevJack::init: No more JACK ports available";
        Q_EMIT this->errorChanged(this->d->m_error);
        this->uninit();

        return false;
    }

    auto bufferSize = jack_get_buffer_size(this->d->m_client);

    // Activate JACK client

    if (auto error = jack_status_t(jack_activate(this->d->m_client))) {
        this->d->m_error = jackErrorCodes->value(error);
        Q_EMIT this->errorChanged(this->d->m_error);
        this->uninit();

        return false;
    }

    if (caps.channels() == 1) {
        if (device == ":jackinput:") {
            for (auto port: this->d->m_devicePorts[device])
                jack_connect(this->d->m_client,
                             port.toStdString().c_str(),
                             jack_port_name(this->d->m_appPorts.first()));
        } else {
            for (auto port: this->d->m_devicePorts[device])
                jack_connect(this->d->m_client,
                             jack_port_name(this->d->m_appPorts.first()),
                             port.toStdString().c_str());
        }
    } else {
        auto ports = this->d->m_devicePorts[device];

        if (device == ":jackinput:") {
            for (int i = 0; i < this->d->m_appPorts.size(); i++)
                jack_connect(this->d->m_client,
                             ports[i].toStdString().c_str(),
                             jack_port_name(this->d->m_appPorts[i]));
        } else {
            for (int i = 0; i < this->d->m_appPorts.size(); i++)
                jack_connect(this->d->m_client,
                             jack_port_name(this->d->m_appPorts[i]),
                             ports[i].toStdString().c_str());
        }
    }

    this->d->m_curDevice = device;
    this->d->m_curChannels = caps.channels();
    this->d->m_maxBufferSize = int(2
                             * sizeof(jack_default_audio_sample_t)
                             * uint(caps.channels())
                             * bufferSize);
    this->d->m_isInput = device == ":jackinput:";

    return true;
}

QByteArray AudioDevJack::read(int samples)
{
    int bufferSize = 2
                     * int(sizeof(jack_default_audio_sample_t))
                     * this->d->m_curChannels
                     * samples;

    QByteArray audioData;

    this->d->m_mutex.lock();

    while (audioData.size() < bufferSize) {
        if (this->d->m_buffer.size() < 1)
            this->d->m_samplesAvailable.wait(&this->d->m_mutex);

        int copyBytes = qMin(this->d->m_buffer.size(),
                             bufferSize - audioData.size());
        audioData += this->d->m_buffer.mid(0, copyBytes);
        this->d->m_buffer.remove(0, copyBytes);
    }

    this->d->m_mutex.unlock();

    return audioData;
}

bool AudioDevJack::write(const AkAudioPacket &packet)
{
    this->d->m_mutex.lock();

    if (this->d->m_buffer.size() >= this->d->m_maxBufferSize)
        this->d->m_canWrite.wait(&this->d->m_mutex);

    this->d->m_buffer += packet.buffer();
    this->d->m_mutex.unlock();

    return true;
}

bool AudioDevJack::uninit()
{
    jack_deactivate(this->d->m_client);

    for (auto port: this->d->m_appPorts)
        jack_port_unregister(this->d->m_client, port);

    this->d->m_appPorts.clear();
    this->d->m_curChannels = 0;
    this->d->m_buffer.clear();

    return true;
}

int AudioDevJackPrivate::onProcessCallback(jack_nframes_t nframes,
                                           void *userData)
{
    auto self = reinterpret_cast<AudioDevJack *>(userData);

    if (self->d->m_isInput) {
        self->d->m_mutex.lock();
        QVector<const jack_default_audio_sample_t *> ports;

        for (auto port: self->d->m_appPorts)
            ports << reinterpret_cast<const jack_default_audio_sample_t *>(jack_port_get_buffer(port,
                                                                                                nframes));

        int samples = int(nframes) * self->d->m_curChannels;
        auto oldLen = self->d->m_buffer.size();
        self->d->m_buffer.resize(oldLen
                                 + samples
                                 * int(sizeof(jack_default_audio_sample_t)));
        auto buffer = reinterpret_cast<jack_default_audio_sample_t *>(self->d->m_buffer.data())
                      + oldLen;

        // Copy samples
        for (int i = 0; i < samples; i++)
            buffer[i] = ports[i % self->d->m_curChannels][i / self->d->m_curChannels];

        // We use a ring buffer and all old samples are discarded.
        if (self->d->m_buffer.size() > self->d->m_maxBufferSize) {
            int k = int(sizeof(jack_default_audio_sample_t))
                    * self->d->m_curChannels;
            int bufferSize = k * int(self->d->m_maxBufferSize / k);

            self->d->m_buffer =
                    self->d->m_buffer.mid(self->d->m_buffer.size() - bufferSize,
                                          bufferSize);
        }

        self->d->m_samplesAvailable.wakeAll();
        self->d->m_mutex.unlock();
    } else {
        self->d->m_mutex.lock();
        QVector<jack_default_audio_sample_t *> ports;

        for (auto port: self->d->m_appPorts) {
            ports << reinterpret_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(port,
                                                                                          nframes));
            std::fill_n(ports.last(), nframes, 0.);
        }

        auto buffer = reinterpret_cast<const jack_default_audio_sample_t *>(self->d->m_buffer.constData());
        int samples = qMin(self->d->m_buffer.size() / int(sizeof(jack_default_audio_sample_t)),
                           int(nframes) * self->d->m_curChannels);

        // Copy samples
        for (int i = 0; i < samples; i++)
            ports[i % self->d->m_curChannels][i / self->d->m_curChannels] = buffer[i];

        if (samples > 0)
            self->d->m_buffer.remove(0,
                                     samples
                                     * int(sizeof(jack_default_audio_sample_t)));

        if (self->d->m_buffer.size() <= self->d->m_maxBufferSize)
            self->d->m_canWrite.wakeAll();

        self->d->m_mutex.unlock();
    }

    return 0;
}

void AudioDevJackPrivate::onShutdownCallback(void *userData)
{
    auto self = reinterpret_cast<AudioDevJack *>(userData);
    QMetaObject::invokeMethod(self, "uninit");
}

#include "moc_audiodevjack.cpp"

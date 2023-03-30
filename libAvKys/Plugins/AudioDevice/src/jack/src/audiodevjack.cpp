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

using JackErrorCodes = QMap<jack_status_t, QString>;

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
        QByteArray m_buffer;
        jack_client_t *m_client {nullptr};
        QMutex m_mutex;
        QWaitCondition m_canWrite;
        QWaitCondition m_samplesAvailable;
        int m_samples {0};
        int m_sampleRate {0};
        int m_curChannels {0};
        int m_maxBufferSize {0};
        bool m_isInput {false};

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

    static const QMap<QString, JackPortFlags> portTypeMap {
        {":jackinput:" , JackPortIsOutput},
        {":jackoutput:", JackPortIsInput }
    };

    // Query the number of channels
    this->d->m_sampleRate = int(jack_get_sample_rate(this->d->m_client));

    for (auto it = portTypeMap.begin(); it != portTypeMap.end(); it++) {
        auto ports = jack_get_ports(this->d->m_client,
                                    nullptr,
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsPhysical | it.value());
        int channels = 0;

        for (auto portName = ports; portName && *portName; portName++, channels++)
            this->d->m_devicePorts[it.key()] << *portName;

        if (ports)
            jack_free(ports);

        if (channels > 0)
            this->d->m_caps[it.key()] =
                    AkAudioCaps(AkAudioCaps::SampleFormat_flt,
                                AkAudioCaps::defaultChannelLayout(qBound(1, channels, 2)),
                                false,
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

QList<AkAudioCaps::ChannelLayout> AudioDevJack::supportedChannelLayouts(const QString &device)
{
    Q_UNUSED(device)

    return {AkAudioCaps::Layout_mono, AkAudioCaps::Layout_stereo};
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
        for (auto &port: this->d->m_appPorts)
            jack_port_unregister(this->d->m_client, port);

        this->d->m_appPorts.clear();
        this->d->m_error = "AudioDevJack::init: No more JACK ports available";
        Q_EMIT this->errorChanged(this->d->m_error);

        return false;
    }

    auto bufferSize = jack_get_buffer_size(this->d->m_client);

    // Activate JACK client

    if (auto error = jack_status_t(jack_activate(this->d->m_client))) {
        for (auto &port: this->d->m_appPorts)
            jack_port_unregister(this->d->m_client, port);

        this->d->m_appPorts.clear();
        this->d->m_error = jackErrorCodes->value(error);
        Q_EMIT this->errorChanged(this->d->m_error);

        return false;
    }

    if (caps.channels() == 1) {
        if (device == ":jackinput:") {
            for (auto &port: this->d->m_devicePorts[device])
                jack_connect(this->d->m_client,
                             port.toStdString().c_str(),
                             jack_port_name(this->d->m_appPorts.first()));
        } else {
            for (auto &port: this->d->m_devicePorts[device])
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
    this->d->m_samples = qMax(this->latency() * caps.rate() / 1000, 1);

    return true;
}

QByteArray AudioDevJack::read()
{
    int bufferSize = 2
                     * int(sizeof(jack_default_audio_sample_t))
                     * this->d->m_curChannels
                     * this->d->m_samples;
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

    this->d->m_buffer += {packet.constData(), int(packet.size())};
    this->d->m_mutex.unlock();

    return true;
}

bool AudioDevJack::uninit()
{
    jack_deactivate(this->d->m_client);

    for (auto &port: this->d->m_appPorts)
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

        for (auto &port: self->d->m_appPorts)
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

        for (auto &port: self->d->m_appPorts) {
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

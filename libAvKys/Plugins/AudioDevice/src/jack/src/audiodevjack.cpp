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

#include <QCoreApplication>
#include <QMap>

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

AudioDevJack::AudioDevJack(QObject *parent):
    AudioDev(parent)
{
    this->m_curChannels = 0;
    this->m_curSampleRate = 0;
    this->m_maxBufferSize = 0;
    this->m_isInput = false;
    this->m_client = NULL;

    this->m_descriptions = {
        {":jackinput:" , "JACK Audio Connection Kit Input" },
        {":jackoutput:", "JACK Audio Connection Kit Output"},
    };

    auto appName = QCoreApplication::applicationName()
                   + QString("_%1").arg(Ak::id());
    int maxNameSize = jack_client_name_size() - 1;

    if (appName.size() > maxNameSize)
        appName = appName.mid(0, maxNameSize);

    jack_status_t status;
    this->m_client = jack_client_open(appName.toStdString().c_str(),
                                      JackNullOption,
                                      &status);

    if (!this->m_client) {
        this->m_error = jackErrorCodes->value(status);
        Q_EMIT this->errorChanged(this->m_error);

        return;
    }

    // Setup callbacks

    jack_set_process_callback(this->m_client,
                              AudioDevJack::onProcessCallback,
                              this);
    jack_on_shutdown(this->m_client,
                     AudioDevJack::onShutdownCallback,
                     this);

    AkAudioCaps audioCaps;
    audioCaps.isValid() = true;
    audioCaps.format() = AkAudioCaps::SampleFormat_flt;
    audioCaps.bps() = AkAudioCaps::bitsPerSample(audioCaps.format());
    audioCaps.rate() = int(jack_get_sample_rate(this->m_client));
    audioCaps.layout() = AkAudioCaps::defaultChannelLayout(audioCaps.channels());
    audioCaps.align() = false;

    QMap<QString, JackPortFlags> portTypeMap = {
        {":jackinput:" , JackPortIsOutput},
        {":jackoutput:", JackPortIsInput }
    };

    // Query the number orr channels

    for (auto deviceId: portTypeMap.keys()) {
        auto ports = jack_get_ports(this->m_client,
                                    NULL,
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsPhysical | portTypeMap[deviceId]);
        int channels = 0;

        for (auto portName = ports; portName && *portName; portName++, channels++)
            this->m_devicePorts[deviceId] << *portName;

        if (ports)
            jack_free(ports);

        if (channels > 0) {
            audioCaps.channels() = channels;
            this->m_caps[deviceId] = audioCaps;
        }
    }
}

AudioDevJack::~AudioDevJack()
{
    this->uninit();

    if (this->m_client)
        jack_client_close(this->m_client);
}

QString AudioDevJack::error() const
{
    return this->m_error;
}

QString AudioDevJack::defaultInput()
{
    return this->m_caps.contains(":jackinput:")?
                QString(":jackinput:"): QString();
}

QString AudioDevJack::defaultOutput()
{
    return this->m_caps.contains(":jackoutput:")?
                QString(":jackoutput:"): QString();
}

QStringList AudioDevJack::inputs()
{
    return this->m_caps.contains(":jackinput:")?
                QStringList {":jackinput:"}: QStringList();
}

QStringList AudioDevJack::outputs()
{
    return this->m_caps.contains(":jackoutput:")?
                QStringList {":jackoutput:"}: QStringList();
}

QString AudioDevJack::description(const QString &device)
{
    return this->m_caps.contains(device)?
                this->m_descriptions.value(device): QString();
}

AkAudioCaps AudioDevJack::preferredFormat(const QString &device)
{
    return this->m_caps.value(device);
}

bool AudioDevJack::init(const QString &device, const AkAudioCaps &caps)
{
    if (!this->m_caps.contains(device)
        || caps.channels() < 1
        || caps.channels() > 2
        || caps.rate() != int(jack_get_sample_rate(this->m_client))
        || caps.format() != AkAudioCaps::SampleFormat_flt)
        return false;

    this->m_appPorts.clear();
    this->m_curChannels = 0;
    this->m_curSampleRate = 0;
    this->m_buffer.clear();

    QString portName = device == ":jackinput:"?
                           "input": "output";
    JackPortFlags portFlags = device == ":jackinput:"?
                                  JackPortIsInput: JackPortIsOutput;

    // Create ports for sending/receiving data
    for (int channel = 0; channel < caps.channels(); channel++) {
        auto port = jack_port_register(this->m_client,
                                       QString("%1_%2")
                                           .arg(portName)
                                           .arg(channel + 1).toStdString().c_str(),
                                       JACK_DEFAULT_AUDIO_TYPE,
                                       portFlags,
                                       0);

        if (port)
            this->m_appPorts << port;
    }

    if (this->m_appPorts.size() < caps.channels()) {
        this->m_error = "AudioDevJack::init: No more JACK ports available";
        Q_EMIT this->errorChanged(this->m_error);
        this->uninit();

        return false;
    }

    auto bufferSize = jack_get_buffer_size(this->m_client);

    // Activate JACK client

    if (auto error = jack_status_t(jack_activate(this->m_client))) {
        this->m_error = jackErrorCodes->value(error);
        Q_EMIT this->errorChanged(this->m_error);
        this->uninit();

        return false;
    }

    if (caps.channels() == 1) {
        if (device == ":jackinput:") {
            for (auto port: this->m_devicePorts[device])
                jack_connect(this->m_client,
                             port.toStdString().c_str(),
                             jack_port_name(this->m_appPorts.first()));
        } else {
            for (auto port: this->m_devicePorts[device])
                jack_connect(this->m_client,
                             jack_port_name(this->m_appPorts.first()),
                             port.toStdString().c_str());
        }
    } else {
        auto ports = this->m_devicePorts[device];

        if (device == ":jackinput:") {
            for (int i = 0; i < this->m_appPorts.size(); i++)
                jack_connect(this->m_client,
                             ports[i].toStdString().c_str(),
                             jack_port_name(this->m_appPorts[i]));
        } else {
            for (int i = 0; i < this->m_appPorts.size(); i++)
                jack_connect(this->m_client,
                             jack_port_name(this->m_appPorts[i]),
                             ports[i].toStdString().c_str());
        }
    }

    this->m_curDevice = device;
    this->m_curChannels = caps.channels();
    this->m_curSampleRate = caps.rate();
    this->m_maxBufferSize = int(2
                                * sizeof(jack_default_audio_sample_t)
                                * uint(caps.channels())
                                * bufferSize);
    this->m_isInput = device == ":jackinput:";

    return true;
}

QByteArray AudioDevJack::read(int samples)
{
    int bufferSize = 2
                     * int(sizeof(jack_default_audio_sample_t))
                     * this->m_curChannels
                     * samples;

    QByteArray audioData;

    this->m_mutex.lock();

    while (audioData.size() < bufferSize) {
        if (this->m_buffer.size() < 1)
            this->m_samplesAvailable.wait(&this->m_mutex);

        int copyBytes = qMin(this->m_buffer.size(),
                             bufferSize - audioData.size());
        audioData += this->m_buffer.mid(0, copyBytes);
        this->m_buffer.remove(0, copyBytes);
    }

    this->m_mutex.unlock();

    return audioData;
}

bool AudioDevJack::write(const QByteArray &frame)
{
    this->m_mutex.lock();

    if (this->m_buffer.size() >= this->m_maxBufferSize)
        this->m_canWrite.wait(&this->m_mutex);

    this->m_buffer += frame;
    this->m_mutex.unlock();

    return true;
}

bool AudioDevJack::uninit()
{
    jack_deactivate(this->m_client);

    for (auto port: this->m_appPorts)
        jack_port_unregister(this->m_client, port);

    this->m_appPorts.clear();
    this->m_curChannels = 0;
    this->m_curSampleRate = 0;
    this->m_buffer.clear();

    return true;
}

int AudioDevJack::onProcessCallback(jack_nframes_t nframes, void *userData)
{
    auto self = reinterpret_cast<AudioDevJack *>(userData);

    if (self->m_isInput) {
        self->m_mutex.lock();
        QVector<const jack_default_audio_sample_t *> ports;

        for (auto port: self->m_appPorts)
            ports << reinterpret_cast<const jack_default_audio_sample_t *>(jack_port_get_buffer(port,
                                                                                                nframes));

        int samples = int(nframes) * self->m_curChannels;
        auto oldLen = self->m_buffer.size();
        self->m_buffer.resize(oldLen
                              + samples
                              * int(sizeof(jack_default_audio_sample_t)));
        auto buffer = reinterpret_cast<jack_default_audio_sample_t *>(self->m_buffer.data())
                      + oldLen;

        // Copy samples
        for (int i = 0; i < samples; i++)
            buffer[i] = ports[i % self->m_curChannels][i / self->m_curChannels];

        // We use a ring buffer and all old samples are discarded.
        if (self->m_buffer.size() > self->m_maxBufferSize) {
            int k = int(sizeof(jack_default_audio_sample_t))
                    * self->m_curChannels;
            int bufferSize = k * int(self->m_maxBufferSize / k);

            self->m_buffer =
                    self->m_buffer.mid(self->m_buffer.size() - bufferSize,
                                       bufferSize);
        }

        self->m_samplesAvailable.wakeAll();
        self->m_mutex.unlock();
    } else {
        self->m_mutex.lock();
        QVector<jack_default_audio_sample_t *> ports;

        for (auto port: self->m_appPorts) {
            ports << reinterpret_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(port,
                                                                                          nframes));
            std::fill_n(ports.last(), nframes, 0.);
        }

        auto buffer = reinterpret_cast<const jack_default_audio_sample_t *>(self->m_buffer.constData());
        int samples = qMin(self->m_buffer.size() / int(sizeof(jack_default_audio_sample_t)),
                           int(nframes) * self->m_curChannels);

        // Copy samples
        for (int i = 0; i < samples; i++)
            ports[i % self->m_curChannels][i / self->m_curChannels] = buffer[i];

        if (samples > 0)
            self->m_buffer.remove(0,
                                  samples
                                  * int(sizeof(jack_default_audio_sample_t)));

        if (self->m_buffer.size() <= self->m_maxBufferSize)
            self->m_canWrite.wakeAll();

        self->m_mutex.unlock();
    }

    return 0;
}

void AudioDevJack::onShutdownCallback(void *userData)
{
    auto self = reinterpret_cast<AudioDevJack *>(userData);
    QMetaObject::invokeMethod(self, "uninit");
}

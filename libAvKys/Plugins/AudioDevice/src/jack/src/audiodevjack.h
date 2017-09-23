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

#ifndef AUDIODEVJACK_H
#define AUDIODEVJACK_H

#include <QWaitCondition>
#include <QMutex>
#include <ak.h>
#include <jack/jack.h>

#include "audiodev.h"

class AudioDevJack: public AudioDev
{
    Q_OBJECT

    public:
        explicit AudioDevJack(QObject *parent=nullptr);
        ~AudioDevJack();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE QString defaultInput();
        Q_INVOKABLE QString defaultOutput();
        Q_INVOKABLE QStringList inputs();
        Q_INVOKABLE QStringList outputs();
        Q_INVOKABLE QString description(const QString &device);
        Q_INVOKABLE AkAudioCaps preferredFormat(const QString &device);
        Q_INVOKABLE QList<AkAudioCaps::SampleFormat> supportedFormats(const QString &device);
        Q_INVOKABLE QList<int> supportedChannels(const QString &device);
        Q_INVOKABLE QList<int> supportedSampleRates(const QString &device);
        Q_INVOKABLE bool init(const QString &device, const AkAudioCaps &caps);
        Q_INVOKABLE QByteArray read(int samples);
        Q_INVOKABLE bool write(const AkAudioPacket &packet);
        Q_INVOKABLE bool uninit();

    private:
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

        static int onProcessCallback(jack_nframes_t nframes, void *userData);
        static void onShutdownCallback(void *userData);
};

#endif // AUDIODEVJACK_H

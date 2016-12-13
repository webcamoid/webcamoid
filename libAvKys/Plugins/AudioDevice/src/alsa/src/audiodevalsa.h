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

#ifndef AUDIODEVALSA_H
#define AUDIODEVALSA_H

#include <QMutex>
#include <QTimer>
#include <QFileSystemWatcher>
#include <akaudiocaps.h>
#include <alsa/asoundlib.h>

#include "audiodev.h"

class AudioDevAlsa: public AudioDev
{
    Q_OBJECT

    public:
        explicit AudioDevAlsa(QObject *parent=NULL);
        ~AudioDevAlsa();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE QString defaultInput();
        Q_INVOKABLE QString defaultOutput();
        Q_INVOKABLE QStringList inputs();
        Q_INVOKABLE QStringList outputs();
        Q_INVOKABLE QString description(const QString &device);
        Q_INVOKABLE AkAudioCaps preferredFormat(const QString &device);
        Q_INVOKABLE bool init(const QString &device, const AkAudioCaps &caps);
        Q_INVOKABLE QByteArray read(int samples);
        Q_INVOKABLE bool write(const AkAudioPacket &packet);
        Q_INVOKABLE bool uninit();

    private:
        QString m_error;
        QString m_defaultSink;
        QString m_defaultSource;
        QStringList m_sinks;
        QStringList m_sources;
        QMap<QString, AkAudioCaps> m_pinCapsMap;
        QMap<QString, QString> m_pinDescriptionMap;
        QMutex m_mutex;
        AkAudioCaps m_curCaps;
        snd_pcm_t *m_pcmHnd;
        QFileSystemWatcher *m_fsWatcher;
        QTimer m_timer;

        AkAudioCaps deviceCaps(const QString &device) const;

    private slots:
        void updateDevices();
};

#endif // AUDIODEVALSA_H

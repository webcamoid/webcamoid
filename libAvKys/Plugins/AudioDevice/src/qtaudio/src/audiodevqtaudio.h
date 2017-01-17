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

#ifndef AUDIODEVQTAUDIO_H
#define AUDIODEVQTAUDIO_H

#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include <QAudioInput>
#include <QAudioOutput>
#include <ak.h>

#include "audiodev.h"
#include "audiodevicebuffer.h"

inline bool operator <(const QAudioDeviceInfo &info1,
                       const QAudioDeviceInfo &info2)
{
    return info1.deviceName() < info2.deviceName();
}

class AudioDevQtAudio: public AudioDev
{
    Q_OBJECT

    public:
        explicit AudioDevQtAudio(QObject *parent=NULL);
        ~AudioDevQtAudio();

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
        Q_INVOKABLE bool init(const QString &device,
                              const AkAudioCaps &caps);
        Q_INVOKABLE QByteArray read(int samples);
        Q_INVOKABLE bool write(const AkAudioPacket &packet);
        Q_INVOKABLE bool uninit();

    private:
        QString m_error;
        QString m_defaultSink;
        QString m_defaultSource;
        QMap<QAudioDeviceInfo, QString> m_sinks;
        QMap<QAudioDeviceInfo, QString> m_sources;
        QMap<QString, AkAudioCaps> m_pinCapsMap;
        QMap<QString, QString> m_pinDescriptionMap;
        QMap<QString, QList<AkAudioCaps::SampleFormat>> m_supportedFormats;
        QMap<QString, QList<int>> m_supportedChannels;
        QMap<QString, QList<int>> m_supportedSampleRates;
        AudioDeviceBuffer m_outputDeviceBuffer;
        QIODevice *m_inputDeviceBuffer;
        QAudioInput *m_input;
        QAudioOutput *m_output;
        QMutex m_mutex;
        int m_maxAudioBuffer;

        AkAudioCaps::SampleFormat qtFormatToAk(const QAudioFormat &format) const;
        QAudioFormat qtFormatFromCaps(const AkAudioCaps &caps) const;

    private slots:
        void updateDevices();
};

#endif // AUDIODEVQTAUDIO_H

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

#ifndef AUDIODEVQTAUDIO_H
#define AUDIODEVQTAUDIO_H

#include "audiodev.h"

class AudioDevQtAudioPrivate;

class AudioDevQtAudio: public AudioDev
{
    Q_OBJECT

    public:
        AudioDevQtAudio(QObject *parent=nullptr);
        ~AudioDevQtAudio();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE QString defaultInput();
        Q_INVOKABLE QString defaultOutput();
        Q_INVOKABLE QStringList inputs();
        Q_INVOKABLE QStringList outputs();
        Q_INVOKABLE QString description(const QString &device);
        Q_INVOKABLE AkAudioCaps preferredFormat(const QString &device);
        Q_INVOKABLE QList<AkAudioCaps::SampleFormat> supportedFormats(const QString &device);
        Q_INVOKABLE QList<AkAudioCaps::ChannelLayout> supportedChannelLayouts(const QString &device);
        Q_INVOKABLE QList<int> supportedSampleRates(const QString &device);
        Q_INVOKABLE bool init(const QString &device,
                              const AkAudioCaps &caps);
        Q_INVOKABLE QByteArray read(int samples);
        Q_INVOKABLE bool write(const AkAudioPacket &packet);
        Q_INVOKABLE bool uninit();

    private:
        AudioDevQtAudioPrivate *d;

    private slots:
        void updateDevices();
};

#endif // AUDIODEVQTAUDIO_H

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

#ifndef AUDIODEVPULSEAUDIO_H
#define AUDIODEVPULSEAUDIO_H

#include "audiodev.h"

/* In GNU/Linux load the OSS drivers as:
 *
 * sudo modprobe snd_pcm_oss snd_mixer_oss snd_seq_oss
 */

class AudioDevOSSPrivate;

class AudioDevOSS: public AudioDev
{
    Q_OBJECT

    public:
        explicit AudioDevOSS(QObject *parent=nullptr);
        ~AudioDevOSS();

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
        Q_INVOKABLE bool write(const AkAudioPacket &frame);
        Q_INVOKABLE bool uninit();

    private:
        AudioDevOSSPrivate *d;

    private slots:
        void updateDevices();

        friend class AudioDevOSSPrivate;
};

#endif // AUDIODEVPULSEAUDIO_H

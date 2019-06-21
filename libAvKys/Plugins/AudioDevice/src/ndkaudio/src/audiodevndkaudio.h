/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

#ifndef AUDIODEVNDKAUDIO_H
#define AUDIODEVNDKAUDIO_H

#include "audiodev.h"

class AudioDevNDKAudioPrivate;

class AudioDevNDKAudio: public AudioDev
{
    Q_OBJECT

    public:
        AudioDevNDKAudio(QObject *parent=nullptr);
        ~AudioDevNDKAudio();

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
        Q_INVOKABLE bool init(const QString &device, const AkAudioCaps &caps);
        Q_INVOKABLE QByteArray read();
        Q_INVOKABLE bool write(const AkAudioPacket &frame);
        Q_INVOKABLE bool uninit();

    private:
        AudioDevNDKAudioPrivate *d;

        friend class AudioDevNDKAudioPrivate;
};

#endif // AUDIODEVNDKAUDIO_H

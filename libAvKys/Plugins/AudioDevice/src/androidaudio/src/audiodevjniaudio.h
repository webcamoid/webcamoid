/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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

#ifndef AUDIODEVJNIAUDIO_H
#define AUDIODEVJNIAUDIO_H

#include "audiodev.h"

class AudioDevJNIAudioPrivate;

class AudioDevJNIAudio: public AudioDev
{
    Q_OBJECT

    public:
        AudioDevJNIAudio(QObject *parent=nullptr);
        ~AudioDevJNIAudio();

        Q_INVOKABLE QString error() const override;
        Q_INVOKABLE QString defaultInput() override;
        Q_INVOKABLE QString defaultOutput() override;
        Q_INVOKABLE QStringList inputs() override;
        Q_INVOKABLE QStringList outputs() override;
        Q_INVOKABLE QString description(const QString &device) override;
        Q_INVOKABLE AkAudioCaps preferredFormat(const QString &device) override;
        Q_INVOKABLE QList<AkAudioCaps::SampleFormat> supportedFormats(const QString &device) override;
        Q_INVOKABLE QList<AkAudioCaps::ChannelLayout> supportedChannelLayouts(const QString &device) override;
        Q_INVOKABLE QList<int> supportedSampleRates(const QString &device) override;
        Q_INVOKABLE bool init(const QString &device, const AkAudioCaps &caps) override;
        Q_INVOKABLE QByteArray read() override;
        Q_INVOKABLE bool write(const AkAudioPacket &frame) override;
        Q_INVOKABLE bool uninit() override;

    private:
        AudioDevJNIAudioPrivate *d;

        friend class AudioDevJNIAudioPrivate;
};

#endif // AUDIODEVJNIAUDIO_H

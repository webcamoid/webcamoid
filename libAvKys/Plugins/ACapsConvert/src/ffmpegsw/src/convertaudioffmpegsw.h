/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#ifndef CONVERTAUDIOFFMPEGSW_H
#define CONVERTAUDIOFFMPEGSW_H

#include "convertaudio.h"

class ConvertAudioFFmpegSWPrivate;

class ConvertAudioFFmpegSW: public ConvertAudio
{
    Q_OBJECT

    public:
        ConvertAudioFFmpegSW(QObject *parent=nullptr);
        ~ConvertAudioFFmpegSW();

        Q_INVOKABLE bool init(const AkAudioCaps &caps);
        Q_INVOKABLE AkPacket convert(const AkAudioPacket &packet);
        Q_INVOKABLE void uninit();

    private:
        ConvertAudioFFmpegSWPrivate *d;
};

#endif // CONVERTAUDIOFFMPEGSW_H

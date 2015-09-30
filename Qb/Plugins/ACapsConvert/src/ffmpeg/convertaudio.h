/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef CONVERTAUDIO_H
#define CONVERTAUDIO_H

#include <qbaudiopacket.h>

extern "C"
{
    #include <libavutil/channel_layout.h>
    #include <libswresample/swresample.h>
}

class ConvertAudio: public QObject
{
    Q_OBJECT

    public:
        explicit ConvertAudio(QObject *parent=NULL);
        ~ConvertAudio();

        Q_INVOKABLE QbPacket convert(const QbAudioPacket &packet,
                                     const QbCaps &oCaps);

    private:
        SwrContext *m_resampleContext;
};

#endif // CONVERTAUDIO_H

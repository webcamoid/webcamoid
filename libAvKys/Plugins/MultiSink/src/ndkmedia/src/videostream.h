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

#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include <akvideocaps.h>

#include "abstractstream.h"

class VideoStreamPrivate;

class VideoStream: public AbstractStream
{
    Q_OBJECT

    public:
        VideoStream(uint index=0,
                    int streamIndex=-1,
                    const QVariantMap &configs={},
                    const QMap<QString, QVariantMap> &codecOptions={},
                    MediaWriterNDKMedia *mediaWriter=nullptr,
                    QObject *parent=nullptr);
        ~VideoStream();

        Q_INVOKABLE static int32_t colorFormatFromPixelFormat(AkVideoCaps::PixelFormat format);

    private:
        VideoStreamPrivate *d;

    protected:
        void convertPacket(const AkPacket &packet) override;
        void encode(const AkPacket &packet,
                    uint8_t *buffer,
                    size_t bufferSize) override;
        AkPacket avPacketDequeue(size_t bufferSize) override;
};

#endif // VIDEOSTREAM_H

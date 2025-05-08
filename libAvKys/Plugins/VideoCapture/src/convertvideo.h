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

#ifndef CONVERTVIDEO_H
#define CONVERTVIDEO_H

#include <akcompressedvideocaps.h>

class ConvertVideo;
class AkCaps;
class AkPacket;

using ConvertVideoPtr = QSharedPointer<ConvertVideo>;

class ConvertVideo: public QObject
{
    Q_OBJECT

    public:
        ConvertVideo(QObject *parent=nullptr);
        virtual ~ConvertVideo() = default;

        Q_INVOKABLE virtual AkCompressedVideoCaps::VideoCodecList supportedCodecs() const;
        Q_INVOKABLE virtual void packetEnqueue(const AkPacket &packet);
        Q_INVOKABLE virtual bool init(const AkCaps &caps);
        Q_INVOKABLE virtual void uninit();

    signals:
        void frameReady(const AkPacket &packet);
};

#endif // CONVERTVIDEO_H

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

#ifndef AKUTILS_H
#define AKUTILS_H

#include <QSize>

#include "akvideocaps.h"

class AkPacket;
class AkVideoPacket;

namespace AkUtils
{
    AKCOMMONS_EXPORT AkPacket imageToPacket(const QImage &image,
                                            const AkPacket &defaultPacket);
    AKCOMMONS_EXPORT QImage packetToImage(const AkPacket &packet);
    AKCOMMONS_EXPORT AkPacket roundSizeTo(const AkPacket &packet, int align);
    AKCOMMONS_EXPORT AkVideoPacket convertVideo(const AkVideoPacket &packet,
                                                AkVideoCaps::PixelFormat format,
                                                const QSize &size=QSize());
}

#endif // AKUTILS_H

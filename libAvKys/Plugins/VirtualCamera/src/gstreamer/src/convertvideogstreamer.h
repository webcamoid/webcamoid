/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef CONVERTVIDEOGSTREAMER_H
#define CONVERTVIDEOGSTREAMER_H

#include <akvideopacket.h>
#include <gst/video/video.h>

#include "convertvideo.h"

class ConvertVideoGStreamer: public ConvertVideo
{
    Q_OBJECT

    public:
        explicit ConvertVideoGStreamer(QObject *parent=nullptr);
        ~ConvertVideoGStreamer();

        Q_INVOKABLE AkPacket convert(const AkPacket &packet,
                                     const AkCaps &oCaps);
};

#endif // CONVERTVIDEOGSTREAMER_H

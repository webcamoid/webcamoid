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

#include <QImage>
#include <QVariant>
#include <ak.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akvideocaps.h>
#include <akpacket.h>
#include <akvideopacket.h>

#include "convertvideogeneric.h"

using ImgFmtMap = QMap<QString, AkVideoCaps::PixelFormat>;

inline ImgFmtMap initImgFmtMap()
{
    ImgFmtMap rawToFF = {
        {"XRGB"       , AkVideoCaps::Format_0rgb    },
        {"ARGB"       , AkVideoCaps::Format_argb    },
        {"RGBA"       , AkVideoCaps::Format_rgba    },
        {"RGBX"       , AkVideoCaps::Format_rgb0    },
        {"RGB565"     , AkVideoCaps::Format_rgb565le},
        {"RGB555"     , AkVideoCaps::Format_rgb555le},
        {"RGB"        , AkVideoCaps::Format_rgb24   },
        {"RGB444"     , AkVideoCaps::Format_rgb444le},
        {"BGR"        , AkVideoCaps::Format_bgr24   },
        {"GRAY8"      , AkVideoCaps::Format_gray8   },
        {"NV16"       , AkVideoCaps::Format_nv16    },
        {"NV21"       , AkVideoCaps::Format_nv21    },
        {"YUY2"       , AkVideoCaps::Format_yuyv422 },
        {"YUYV"       , AkVideoCaps::Format_yuyv422 },
        {"YV12"       , AkVideoCaps::Format_yuv420p },
        {"Y422"       , AkVideoCaps::Format_yuv422p },
        {"YU12"       , AkVideoCaps::Format_yuv420p },
        {"I420"       , AkVideoCaps::Format_yuv420p },
        {"YUV420P_888", AkVideoCaps::Format_yuv420p }
    };

    return rawToFF;
}

Q_GLOBAL_STATIC_WITH_ARGS(ImgFmtMap, fourccToFormat, (initImgFmtMap()))

ConvertVideoGeneric::ConvertVideoGeneric(QObject *parent):
    ConvertVideo(parent)
{
}

ConvertVideoGeneric::~ConvertVideoGeneric()
{
}

void ConvertVideoGeneric::packetEnqueue(const AkPacket &packet)
{
    auto fourcc = packet.caps().property("fourcc").toString();
    AkVideoPacket videoPacket(packet);

    if (fourcc == "JPEG") {
        videoPacket =
                AkVideoPacket::fromImage(QImage::fromData(packet.buffer()),
                                         packet);
    } else {
        AkVideoCaps caps(fourccToFormat->value(fourcc,
                                               AkVideoCaps::Format_none),
                         packet.caps().property("width").toInt(),
                         packet.caps().property("height").toInt(),
                         packet.caps().property("fps").toString());

        if (packet.caps().contains("align"))
            caps.setAlign(packet.caps().property("align").toInt());

        videoPacket.caps() = caps;
        videoPacket.buffer() = packet.buffer();
        videoPacket.copyMetadata(packet);
        videoPacket = videoPacket.convert(AkVideoCaps::Format_rgb24);
    }

    if (videoPacket)
        emit this->frameReady(videoPacket);
}

bool ConvertVideoGeneric::init(const AkCaps &caps)
{
    Q_UNUSED(caps)

    return true;
}

void ConvertVideoGeneric::uninit()
{
}

#include "moc_convertvideogeneric.cpp"

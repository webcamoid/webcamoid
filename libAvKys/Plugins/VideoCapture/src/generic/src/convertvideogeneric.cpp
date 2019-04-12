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

#include <QImage>
#include <QMutex>
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
        {"B0W1", AkVideoCaps::Format_monob   },
        {"RGB4", AkVideoCaps::Format_0rgb    },
        {"ARGB", AkVideoCaps::Format_argb    },
        {"RGBP", AkVideoCaps::Format_rgb565le},
        {"RGBO", AkVideoCaps::Format_rgb555le},
        {"RGB3", AkVideoCaps::Format_rgb24   },
        {"R444", AkVideoCaps::Format_rgb444le},
        {"BGR3", AkVideoCaps::Format_bgr24   },
        {"Y800", AkVideoCaps::Format_gray    },
        {"NV16", AkVideoCaps::Format_nv16    },
        {"NV21", AkVideoCaps::Format_nv21    }, //
        {"YUY2", AkVideoCaps::Format_yuyv422 },
        {"YUYV", AkVideoCaps::Format_yuyv422 },
        {"YV12", AkVideoCaps::Format_yuv420p },
        {"Y422", AkVideoCaps::Format_yuv422p },
        {"YU12", AkVideoCaps::Format_yuv420p },
        {"I420", AkVideoCaps::Format_yuv420p },
    };

    return rawToFF;
}

Q_GLOBAL_STATIC_WITH_ARGS(ImgFmtMap, fourccToFormat, (initImgFmtMap()))

class ConvertVideoGenericPrivate
{
    public:
        ConvertVideoGeneric *self {nullptr};
        qint64 m_id {-1};
        AkFrac m_fps;

        explicit ConvertVideoGenericPrivate(ConvertVideoGeneric *self):
            self(self)
        {
        }
};

ConvertVideoGeneric::ConvertVideoGeneric(QObject *parent):
    ConvertVideo(parent)
{
    this->d = new ConvertVideoGenericPrivate(this);
}

ConvertVideoGeneric::~ConvertVideoGeneric()
{
    this->uninit();
    delete this->d;
}

#include <QtDebug>

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
        videoPacket.caps() = caps;
        videoPacket.buffer() = packet.buffer();
        videoPacket.copyMetadata(packet);
        videoPacket = videoPacket.convert(AkVideoCaps::Format_rgb24);
    }

    if (videoPacket)
        emit this->frameReady(videoPacket.toPacket());
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

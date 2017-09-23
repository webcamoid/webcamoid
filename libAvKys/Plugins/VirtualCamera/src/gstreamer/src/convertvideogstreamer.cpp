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

#include <QMetaEnum>

#include "convertvideogstreamer.h"

typedef QMap<QString, QString> StringStringMap;

inline StringStringMap initGstToFF()
{
    StringStringMap gstToFF {
        {"I420", "yuv420p"},
//        {"YV12", ""},
        {"YUY2", "yuyv422"},
        {"YUYV", "yuyv422"},
        {"UYVY", "uyvy422"},
//        {"AYUV", ""},
        {"RGBx", "rgb0"   },
        {"BGRx", "bgr0"   },
        {"xRGB", "0rgb"   },
        {"xBGR", "0bgr"   },
        {"RGBA", "rgba"   },
        {"BGRA", "bgra"   },
        {"ARGB", "argb"   },
        {"ABGR", "abgr"   },
        {"RGB" , "rgb24"  },
        {"BGR" , "bgr24"  },
        {"Y41B", "yuv411p"},
        {"Y42B", "yuv422p"},
//        {"YVYU". ""},
        {"Y444"     , "yuv444p" },
        {"v210"     , "v210"    },
        {"v216"     , "v216"    },
        {"NV12"     , "nv12"    },
        {"NV21"     , "nv21"    },
        {"GRAY8"    , "gray8"   },
        {"GRAY16_BE", "gray16be"},
        {"GRAY16_LE", "gray16le"},
        {"V308"     , "v308"    },
        {"RGB16"    , "rgb565"  },
        {"BGR16"    , "bgr565le"},
        {"RGB15"    , "rgb555"  },
        {"BGR15"    , "rgb555le"},
//        {"UYVP", ""},
        {"A420"     , "yuva420p"},
        {"RGB8P"    , "pal8"    },
        {"YUV9"     , "yuv410p" },
//        {"YVU9"  , ""},
//        {"IYU1"  , ""},
//        {"ARGB64", ""},
        {"AYUV64"   , "ayuv64le"},
//        {"r210", ""},
        {"I420_10BE", "yuv420p10be"},
        {"I420_10LE", "yuv420p10le"},
        {"I422_10BE", "yuv422p10be"},
        {"I422_10LE", "yuv422p10le"},
        {"Y444_10BE", "yuv444p10be"},
        {"Y444_10LE", "yuv444p10le"},
        {"GBR"      , "gbrp"       },
        {"GBR_10BE" , "gbrp10be"   },
        {"GBR_10LE" , "gbrp10le"   },
        {"NV16"     , "nv16"       },
//        {"NV24"      , ""},
//        {"NV12_64Z32", ""},
        {"A420_10BE", "yuva420p10be"},
        {"A420_10LE", "yuva420p10le"},
        {"A422_10BE", "yuva422p10be"},
        {"A422_10LE", "yuva422p10le"},
        {"A444_10BE", "yuva444p10be"},
        {"A444_10LE", "yuva444p10le"},
//        {"NV61", ""},
    };

    return gstToFF;
}

Q_GLOBAL_STATIC_WITH_ARGS(StringStringMap, gstToFF, (initGstToFF()))

ConvertVideoGStreamer::ConvertVideoGStreamer(QObject *parent):
    ConvertVideo(parent)
{
//    setenv("GST_DEBUG", "2", 1);
    gst_init(nullptr, nullptr);
}

ConvertVideoGStreamer::~ConvertVideoGStreamer()
{
}

AkPacket ConvertVideoGStreamer::convert(const AkPacket &packet, const AkCaps &oCaps)
{
    AkVideoPacket videoPacket(packet);
    AkVideoCaps oVideoCaps(oCaps);

    GstBuffer *iBuffer = gst_buffer_new_allocate(nullptr,
                                                 gsize(videoPacket.buffer().size()),
                                                 nullptr);

    GstMapInfo info;
    gst_buffer_map(iBuffer, &info, GST_MAP_WRITE);
    memcpy(info.data,
           videoPacket.buffer().constData(),
           size_t(videoPacket.buffer().size()));
    gst_buffer_unmap(iBuffer, &info);

    QString iFormat = AkVideoCaps::pixelFormatToString(videoPacket.caps().format());
    iFormat = gstToFF->key(iFormat, "I420");
    GstCaps *iCaps = gst_caps_new_simple("video/x-raw",
                                         "format", G_TYPE_STRING, iFormat.toStdString().c_str(),
                                         "width", G_TYPE_INT, videoPacket.caps().width(),
                                         "height", G_TYPE_INT, videoPacket.caps().height(),
                                         "framerate", GST_TYPE_FRACTION,
                                                      int(videoPacket.caps().fps().num()),
                                                      int(videoPacket.caps().fps().den()),
                                         nullptr);

    GstSample *iSample = gst_sample_new(iBuffer,
                                        iCaps,
                                        nullptr,
                                        nullptr);

    gst_caps_unref(iCaps);

    QString oFormat = AkVideoCaps::pixelFormatToString(oVideoCaps.format());
    oFormat = gstToFF->key(iFormat, "I420");
    GstCaps *oGstCaps = gst_caps_new_simple("video/x-raw",
                                            "format", G_TYPE_STRING, oFormat.toStdString().c_str(),
                                            "width", G_TYPE_INT, oVideoCaps.width(),
                                            "height", G_TYPE_INT, oVideoCaps.height(),
                                            nullptr);

    GError *error = nullptr;
    GstSample *oSample = gst_video_convert_sample(iSample,
                                                  oGstCaps,
                                                  GST_CLOCK_TIME_NONE,
                                                  &error);

    gst_caps_unref(oGstCaps);

    if (error) {
        qDebug() << "Error:" << error->message;
        g_error_free(error);

        if (oSample)
            gst_sample_unref(oSample);

        if (iBuffer)
            gst_buffer_unref(iBuffer);

        return AkPacket();
    }

    GstBuffer *bufffer = gst_sample_get_buffer(oSample);
    gst_buffer_map(bufffer, &info, GST_MAP_READ);
    QByteArray oBuffer(int(info.size), 0);
    memcpy(oBuffer.data(), info.data, info.size);
    gst_buffer_unmap(bufffer, &info);

    if (iSample)
        gst_sample_unref(iSample);

    if (oSample)
        gst_sample_unref(oSample);

    if (iBuffer)
        gst_buffer_unref(iBuffer);

    // Create packet
    AkVideoPacket oPacket(packet);
    oPacket.caps() = oVideoCaps;
    oPacket.buffer() = oBuffer;

    return oPacket.toPacket();
}

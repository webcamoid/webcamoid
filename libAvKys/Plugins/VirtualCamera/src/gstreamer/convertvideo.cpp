/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include <QMetaEnum>

#include "convertvideo.h"

typedef QMap<QString, QString> StringStringMap;

inline StringStringMap initGstToFF()
{
    StringStringMap gstToFF;
    gstToFF["I420"] = "yuv420p";
//    gstToFF["YV12"] = "";
    gstToFF["YUY2"] = "yuyv422";
    gstToFF["UYVY"] = "uyvy422";
//    gstToFF["AYUV"] = "";
    gstToFF["RGBx"] = "rgb0";
    gstToFF["BGRx"] = "bgr0";
    gstToFF["xRGB"] = "0rgb";
    gstToFF["xBGR"] = "0bgr";
    gstToFF["RGBA"] = "rgba";
    gstToFF["BGRA"] = "bgra";
    gstToFF["ARGB"] = "argb";
    gstToFF["ABGR"] = "abgr";
    gstToFF["RGB"] = "rgb24";
    gstToFF["BGR"] = "bgr24";
    gstToFF["Y41B"] = "yuv411p";
    gstToFF["Y42B"] = "yuv422p";
//    gstToFF["YVYU"] = "";
    gstToFF["Y444"] = "yuv444p";
    gstToFF["v210"] = "v210";
    gstToFF["v216"] = "v216";
    gstToFF["NV12"] = "nv12";
    gstToFF["NV21"] = "nv21";
    gstToFF["GRAY8"] = "gray8";
    gstToFF["GRAY16_BE"] = "gray16be";
    gstToFF["GRAY16_LE"] = "gray16le";
    gstToFF["V308"] = "v308";
    gstToFF["RGB16"] = "rgb565";
    gstToFF["BGR16"] = "bgr565le";
    gstToFF["RGB15"] = "rgb555";
    gstToFF["BGR15"] = "rgb555le";
//    gstToFF["UYVP"] = "";
    gstToFF["A420"] = "yuva420p";
    gstToFF["RGB8P"] = "pal8";
    gstToFF["YUV9"] = "yuv410p";
//    gstToFF["YVU9"] = "";
//    gstToFF["IYU1"] = "";
//    gstToFF["ARGB64"] = "";
    gstToFF["AYUV64"] = "ayuv64le";
//    gstToFF["r210"] = "";
    gstToFF["I420_10BE"] = "yuv420p10be";
    gstToFF["I420_10LE"] = "yuv420p10le";
    gstToFF["I422_10BE"] = "yuv422p10be";
    gstToFF["I422_10LE"] = "yuv422p10le";
    gstToFF["Y444_10BE"] = "yuv444p10be";
    gstToFF["Y444_10LE"] = "yuv444p10le";
    gstToFF["GBR"] = "gbrp";
    gstToFF["GBR_10BE"] = "gbrp10be";
    gstToFF["GBR_10LE"] = "gbrp10le";
    gstToFF["NV16"] = "nv16";
//    gstToFF["NV24"] = "";
//    gstToFF["NV12_64Z32"] = "";
    gstToFF["A420_10BE"] = "yuva420p10be";
    gstToFF["A420_10LE"] = "yuva420p10le";
    gstToFF["A422_10BE"] = "yuva422p10be";
    gstToFF["A422_10LE"] = "yuva422p10le";
    gstToFF["A444_10BE"] = "yuva444p10be";
    gstToFF["A444_10LE"] = "yuva444p10le";
//    gstToFF["NV61"] = "";

    return gstToFF;
}

Q_GLOBAL_STATIC_WITH_ARGS(StringStringMap, gstToFF, (initGstToFF()))

ConvertVideo::ConvertVideo(QObject *parent):
    QObject(parent)
{
}

ConvertVideo::~ConvertVideo()
{
}

AkPacket ConvertVideo::convert(const AkPacket &packet, const AkCaps &oCaps)
{
    AkVideoPacket videoPacket(packet);
    AkVideoCaps oVideoCaps(oCaps);

    GstBuffer *iBuffer = gst_buffer_new_allocate(NULL,
                                                 videoPacket.buffer().size(),
                                                 NULL);
    GstMapInfo info;
    gst_buffer_map(iBuffer, &info, GST_MAP_WRITE);
    memcpy(info.data,
           videoPacket.buffer().constData(),
           videoPacket.buffer().size());
    gst_buffer_unmap(iBuffer, &info);

    QString iFormat = AkVideoCaps::pixelFormatToString(videoPacket.caps().format());
    iFormat = gstToFF->key(iFormat, "I420");
    GstCaps *iCaps = gst_caps_new_simple("video/x-raw",
                                         "format", G_TYPE_STRING, iFormat.toStdString().c_str(),
                                         "width", G_TYPE_INT, videoPacket.caps().width(),
                                         "height", G_TYPE_INT, videoPacket.caps().height(),
                                         "framerate", GST_TYPE_FRACTION,
                                                      (int) videoPacket.caps().fps().num(),
                                                      (int) videoPacket.caps().fps().den(),
                                         NULL);

    GstSample *iSample = gst_sample_new(iBuffer,
                                        iCaps,
                                        NULL,
                                        NULL);

    gst_caps_unref(iCaps);

    QString oFormat = AkVideoCaps::pixelFormatToString(oVideoCaps.format());
    oFormat = gstToFF->key(iFormat, "I420");
    GstCaps *oGstCaps = gst_caps_new_simple("video/x-raw",
                                            "format", G_TYPE_STRING, oFormat.toStdString().c_str(),
                                            "width", G_TYPE_INT, oVideoCaps.width(),
                                            "height", G_TYPE_INT, oVideoCaps.height(),
                                            NULL);

    GError *error = NULL;
    GstSample *oSample = gst_video_convert_sample(iSample,
                                                  oGstCaps,
                                                  GST_CLOCK_TIME_NONE,
                                                  &error);

    gst_caps_unref(oGstCaps);

    if (error) {
        qDebug() << "Error:" << error->message;
        g_error_free(error);

        if (oSample != NULL)
            gst_sample_unref(oSample);

        return AkPacket();
    }

    GstBuffer *bufffer = gst_sample_get_buffer(oSample);
    gst_buffer_map(bufffer, &info, GST_MAP_READ);
    QByteArray oBuffer(info.size, Qt::Uninitialized);
    memcpy(oBuffer.data(), info.data, info.size);
    gst_buffer_unmap(bufffer, &info);

    if (iSample != NULL)
        gst_sample_unref(iSample);

    if (oSample != NULL)
        gst_sample_unref(oSample);

    // Create packet
    AkVideoPacket oPacket(packet);
    oPacket.caps() = oVideoCaps;
    oPacket.buffer() = oBuffer;

    return oPacket.toPacket();
}

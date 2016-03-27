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
 * Web-Site: http://webcamoid.github.io/
 */

#include <QMetaEnum>

#include "convertvideo.h"

typedef QMap<QString, QString> StringStringMap;

inline StringStringMap initFourCCToGst()
{
    StringStringMap fourCCToGst;

    // RGB formats
    fourCCToGst["RGBO"] = "video/x-raw,format=RGB15";
    fourCCToGst["RGBP"] = "video/x-raw,format=RGB16";
    fourCCToGst["BGR3"] = "video/x-raw,format=BGR";
    fourCCToGst["RGB3"] = "video/x-raw,format=RGB";
    fourCCToGst["BGR4"] = "video/x-raw,format=BGRx";
    fourCCToGst["RGB4"] = "video/x-raw,format=xRGB";

    // Grey formats
    fourCCToGst["GREY"] = "video/x-raw,format=GRAY8";
    fourCCToGst["Y04 "] = "video/x-raw,format=Y41P";

    // Luminance+Chrominance formats
    fourCCToGst["YVU9"] = "video/x-raw,format=YVU9";
    fourCCToGst["YV12"] = "video/x-raw,format=YV12";
    fourCCToGst["YUYV"] = "video/x-raw,format=YUY2";
    fourCCToGst["YVYU"] = "video/x-raw,format=YVYU";
    fourCCToGst["UYVY"] = "video/x-raw,format=UYVY";
    fourCCToGst["422P"] = "video/x-raw,format=Y42B";
    fourCCToGst["411P"] = "video/x-raw,format=Y41B";
    fourCCToGst["Y41P"] = "video/x-raw,format=Y41P";
    fourCCToGst["YUV9"] = "video/x-raw,format=YUV9";
    fourCCToGst["YU12"] = "video/x-raw,format=I420";

    // two planes -- one Y, one Cr + Cb interleaved
    fourCCToGst["NV12"] = "video/x-raw,format=NV12";
    fourCCToGst["NV21"] = "video/x-raw,format=NV21";

    // two non contiguous planes - one Y, one Cr + Cb interleaved
    fourCCToGst["NM12"] = "video/x-raw,format=NV12";
    fourCCToGst["NM21"] = "video/x-raw,format=NV21";
    fourCCToGst["TM12"] = "video/x-raw,format=NV12_64Z32";

    // Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm
    fourCCToGst["BA81"] = "video/x-bayer,format=bggr";
    fourCCToGst["GBRG"] = "video/x-bayer,format=gbrg";
    fourCCToGst["GRBG"] = "video/x-bayer,format=grbg";
    fourCCToGst["RGGB"] = "video/x-bayer,format=rggb";

    // compressed formats
    fourCCToGst["MJPG"] = "image/jpeg";
    fourCCToGst["JPEG"] = "image/jpeg";
    fourCCToGst["dvsd"] = "video/x-dv,systemstream=true";
    fourCCToGst["MPEG"] = "video/mpegts,systemstream=true";
    fourCCToGst["H264"] = "video/x-h264,stream-format=byte-stream,alignment=au";
    fourCCToGst["H263"] = "video/x-h263,variant=itu";
    fourCCToGst["MPG1"] = "video/mpeg,mpegversion=2";
    fourCCToGst["MPG2"] = "video/mpeg,mpegversion=2";
    fourCCToGst["MPG4"] = "video/mpeg,mpegversion=4,systemstream=false";
    fourCCToGst["VP80"] = "video/x-vp8";

    // Vendor-specific formats
    fourCCToGst["S910"] = "video/x-sonix";
    fourCCToGst["PWC1"] = "video/x-pwc1";
    fourCCToGst["PWC2"] = "video/x-pwc2";
    fourCCToGst["PJPG"] = "image/jpeg";

    return fourCCToGst;
}

Q_GLOBAL_STATIC_WITH_ARGS(StringStringMap, fourCCToGst, (initFourCCToGst()))

ConvertVideo::ConvertVideo(QObject *parent):
    QObject(parent)
{
    //    setenv("GST_DEBUG", "2", 1);
    gst_init(NULL, NULL);

    this->m_pipeline = gst_pipeline_new(NULL);
    this->m_mainLoop = NULL;

    this->m_source = gst_element_factory_make("appsrc", NULL);
    gst_app_src_set_stream_type(GST_APP_SRC(this->m_source), GST_APP_STREAM_TYPE_STREAM);
    g_object_set(G_OBJECT(this->m_source), "format", GST_FORMAT_TIME, NULL);

    GstElement *decodebin = gst_element_factory_make("decodebin", NULL);
    GstElement *videoConvert = gst_element_factory_make("videoconvert", NULL);
    this->m_sink = gst_element_factory_make("appsink", NULL);

    GstCaps *outCaps = gst_caps_new_simple("video/x-raw",
                                           "format", G_TYPE_STRING, "BGRA",
                                           NULL);
    outCaps = gst_caps_fixate(outCaps);
    gst_app_sink_set_caps(GST_APP_SINK(this->m_sink), outCaps);
    gst_caps_unref(outCaps);

    gst_bin_add_many(GST_BIN(this->m_pipeline),
                     this->m_source,
                     decodebin,
                     videoConvert,
                     this->m_sink,
                     NULL);

    gst_element_link_many(this->m_source,
                          decodebin,
                          videoConvert,
                          this->m_sink,
                          NULL);

    // Configure the message bus.
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->m_pipeline));
    this->m_busWatchId = gst_bus_add_watch(bus, this->busCallback, this);
    gst_object_unref(bus);
}

ConvertVideo::~ConvertVideo()
{
    if (this->m_pipeline) {
        gst_element_set_state(this->m_pipeline, GST_STATE_NULL);
        this->waitState(GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(this->m_pipeline));
        g_source_remove(this->m_busWatchId);
        this->m_pipeline = NULL;
        this->m_busWatchId = 0;
    }

    if (this->m_mainLoop) {
        g_main_loop_quit(this->m_mainLoop);
        g_main_loop_unref(this->m_mainLoop);
        this->m_mainLoop = NULL;
    }
}

AkPacket ConvertVideo::convert(const AkPacket &packet)
{
    QString fourcc = packet.caps().property("fourcc").toString();
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();
    AkFrac fps = packet.caps().property("fps").toString();

    AkCaps gstCaps = fourCCToGst->value(fourcc);
    GstCaps *inCaps = NULL;

    if (gstCaps.mimeType() == "video/x-raw"
        || gstCaps.mimeType() == "video/x-bayer"
        || gstCaps.mimeType() == "video/x-pwc1"
        || gstCaps.mimeType() == "video/x-pwc2"
        || gstCaps.mimeType() == "video/x-sonix") {
        gstCaps.setProperty("width", width);
        gstCaps.setProperty("height", height);
        gstCaps.setProperty("framerate", fps.toString());
        inCaps = gst_caps_from_string(gstCaps.toString().toStdString().c_str());
    } else if (!gstCaps.mimeType().isEmpty())
        inCaps = gst_caps_from_string(gstCaps.toString().toStdString().c_str());
    else
        return AkPacket();

    inCaps = gst_caps_fixate(inCaps);
    GstCaps *sourceCaps = gst_app_src_get_caps(GST_APP_SRC(this->m_source));

    if (!sourceCaps || !gst_caps_is_equal(sourceCaps, inCaps))
        gst_app_src_set_caps(GST_APP_SRC(this->m_source), inCaps);

    gst_caps_unref(inCaps);

    if (sourceCaps)
        gst_caps_unref(sourceCaps);

    // Start pipeline if it's not it.
    GstState state;
    gst_element_get_state(this->m_pipeline,
                          &state,
                          NULL,
                          GST_CLOCK_TIME_NONE);

    if (state != GST_STATE_PLAYING) {
        // Run the main GStreamer loop.
        this->m_mainLoop = g_main_loop_new(NULL, FALSE);
        QtConcurrent::run(&this->m_threadPool, g_main_loop_run, this->m_mainLoop);
        gst_element_set_state(this->m_pipeline, GST_STATE_PLAYING);
    }

    // Write audio frame to the pipeline.
    GstBuffer *buffer = gst_buffer_new_allocate(NULL,
                                                packet.buffer().size(),
                                                NULL);
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);
    memcpy(info.data, packet.buffer().constData(), info.size);
    gst_buffer_unmap(buffer, &info);

    GST_BUFFER_PTS(buffer) = packet.pts() * packet.timeBase().value() * GST_SECOND;
    GST_BUFFER_DTS(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_OFFSET(buffer) = GST_BUFFER_OFFSET_NONE;

    gst_app_src_push_buffer(GST_APP_SRC(this->m_source), buffer);

    // Read audio frame from the pipeline.
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(this->m_sink));

    if (!sample)
        return AkPacket();

    buffer = gst_sample_get_buffer(sample);

    gst_buffer_map(buffer, &info, GST_MAP_READ);
    QByteArray oBuffer(info.size, Qt::Uninitialized);
    memcpy(oBuffer.data(), info.data, info.size);
    gst_buffer_unmap(buffer, &info);
    qint64 pts = GST_BUFFER_PTS(buffer) / packet.timeBase().value() / GST_SECOND;
    gst_sample_unref(sample);

    // Create a package and return it.
    AkVideoPacket oVideoPacket;
    oVideoPacket.caps().isValid() = true;
    oVideoPacket.caps().format() = AkVideoCaps::Format_bgra;
    oVideoPacket.caps().bpp() = AkVideoCaps::bitsPerPixel(oVideoPacket.caps().format());
    oVideoPacket.caps().width() = width;
    oVideoPacket.caps().height() = height;
    oVideoPacket.caps().fps() = fps;
    oVideoPacket.buffer() = oBuffer;
    oVideoPacket.pts() = pts;
    oVideoPacket.timeBase() = packet.timeBase();
    oVideoPacket.index() = packet.index();
    oVideoPacket.id() = packet.id();

    return oVideoPacket.toPacket();
}

bool ConvertVideo::init(const AkCaps &caps)
{
    if (this->m_caps == caps)
        return true;

    this->m_caps = caps;

    return true;
}

void ConvertVideo::uninit()
{

}

void ConvertVideo::waitState(GstState state)
{
    forever {
        GstState curState;
        GstStateChangeReturn ret = gst_element_get_state(this->m_pipeline,
                                                         &curState,
                                                         NULL,
                                                         GST_CLOCK_TIME_NONE);

        if (ret == GST_STATE_CHANGE_FAILURE)
            break;

        if (ret == GST_STATE_CHANGE_SUCCESS
            && curState == state)
            break;
    }
}

gboolean ConvertVideo::busCallback(GstBus *bus,
                                   GstMessage *message,
                                   gpointer userData)
{
    Q_UNUSED(bus)
    ConvertVideo *self = static_cast<ConvertVideo *>(userData);

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        GError *err = NULL;
        gchar *debug = NULL;
        gst_message_parse_error(message, &err, &debug);

        qDebug() << "ERROR: from element"
                 << GST_MESSAGE_SRC_NAME(message)
                 << ":"
                 << err->message;

        if (debug)
            qDebug() << "Additional debug info:\n"
                     << debug;

        GstElement *element = GST_ELEMENT(GST_MESSAGE_SRC(message));

        for (const GList *padItem = GST_ELEMENT_PADS(element); padItem; padItem = g_list_next(padItem)) {
            GstPad *pad = GST_PAD_CAST(padItem->data);
            GstCaps *curCaps = gst_pad_get_current_caps(pad);
            gchar *curCapsStr = gst_caps_to_string(curCaps);

            qDebug() << "    Current caps:" << curCapsStr;

            g_free(curCapsStr);
            gst_caps_unref(curCaps);

            GstCaps *allCaps = gst_pad_get_allowed_caps(pad);
            gchar *allCapsStr = gst_caps_to_string(allCaps);

            qDebug() << "    Allowed caps:" << allCapsStr;

            g_free(allCapsStr);
            gst_caps_unref(allCaps);
        }

        g_error_free(err);
        g_free(debug);
        g_main_loop_quit(self->m_mainLoop);

        break;
    }
    case GST_MESSAGE_EOS:
        g_main_loop_quit(self->m_mainLoop);
    break;
    case GST_MESSAGE_STATE_CHANGED: {
        GstState oldstate;
        GstState newstate;
        GstState pending;
        gst_message_parse_state_changed(message, &oldstate, &newstate, &pending);
        qDebug() << "State changed from"
                 << gst_element_state_get_name(oldstate)
                 << "to"
                 << gst_element_state_get_name(newstate);

        break;
    }
    case GST_MESSAGE_STREAM_STATUS: {
        GstStreamStatusType type;
        GstElement *owner = NULL;
        gst_message_parse_stream_status(message, &type, &owner);
        qDebug() << "Stream Status:"
                 << GST_ELEMENT_NAME(owner)
                 << "is"
                 << type;

        break;
    }
    case GST_MESSAGE_LATENCY: {
        qDebug() << "Recalculating latency";
        gst_bin_recalculate_latency(GST_BIN(self->m_pipeline));
        break;
    }
    case GST_MESSAGE_STREAM_START: {
        qDebug() << "Stream started";
        break;
    }
    case GST_MESSAGE_ASYNC_DONE: {
        GstClockTime runningTime;
        gst_message_parse_async_done(message, &runningTime);
        qDebug() << "ASYNC done";
        break;
    }
    case GST_MESSAGE_NEW_CLOCK: {
        GstClock *clock = NULL;
        gst_message_parse_new_clock(message, &clock);
        qDebug() << "New clock:" << (clock? GST_OBJECT_NAME(clock): "NULL");
        break;
    }
    case GST_MESSAGE_DURATION_CHANGED: {
        GstFormat format;
        gint64 duration;
        gst_message_parse_duration(message, &format, &duration);
        qDebug() << "Duration changed:"
                 << gst_format_get_name(format)
                 << ","
                 << qreal(duration);
        break;
    }
    case GST_MESSAGE_TAG: {
        GstTagList *tagList = NULL;
        gst_message_parse_tag(message, &tagList);
        gchar *tags = gst_tag_list_to_string(tagList);
//        qDebug() << "Tags:" << tags;
        g_free(tags);
        gst_tag_list_unref(tagList);
        break;
    }
    case GST_MESSAGE_ELEMENT: {
        const GstStructure *messageStructure = gst_message_get_structure(message);
        gchar *structure = gst_structure_to_string(messageStructure);
//        qDebug() << structure;
        g_free(structure);
        break;
    }
    default:
        qDebug() << "Unhandled message:" << GST_MESSAGE_TYPE_NAME(message);
    break;
    }

    return TRUE;
}

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

inline StringStringMap initFourCCToGst()
{
    StringStringMap fourCCToGst = {
        // RGB formats
        {"RGBO", "video/x-raw,format=RGB15"},
        {"RGBP", "video/x-raw,format=RGB16"},
        {"BGR3", "video/x-raw,format=BGR"  },
        {"RGB3", "video/x-raw,format=RGB"  },
        {"BGR4", "video/x-raw,format=BGRx" },
        {"RGB4", "video/x-raw,format=xRGB" },

        // Grey formats
        {"GREY", "video/x-raw,format=GRAY8"},
        {"Y04 ", "video/x-raw,format=Y41P" },

        // Luminance+Chrominance formats
        {"YVU9", "video/x-raw,format=YVU9"},
        {"YV12", "video/x-raw,format=YV12"},
        {"YUYV", "video/x-raw,format=YUY2"},
        {"YVYU", "video/x-raw,format=YVYU"},
        {"UYVY", "video/x-raw,format=UYVY"},
        {"422P", "video/x-raw,format=Y42B"},
        {"411P", "video/x-raw,format=Y41B"},
        {"Y41P", "video/x-raw,format=Y41P"},
        {"YUV9", "video/x-raw,format=YUV9"},
        {"YU12", "video/x-raw,format=I420"},

        // two planes -- one Y, one Cr + Cb interleaved
        {"NV12", "video/x-raw,format=NV12"},
        {"NV21", "video/x-raw,format=NV21"},

        // two non contiguous planes - one Y, one Cr + Cb interleaved
        {"NM12", "video/x-raw,format=NV12"      },
        {"NM21", "video/x-raw,format=NV21"      },
        {"TM12", "video/x-raw,format=NV12_64Z32"},

        // Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm
        {"BA81", "video/x-bayer,format=bggr"},
        {"GBRG", "video/x-bayer,format=gbrg"},
        {"GRBG", "video/x-bayer,format=grbg"},
        {"RGGB", "video/x-bayer,format=rggb"},

        // compressed formats
        {"MJPG", "image/jpeg"                                         },
        {"JPEG", "image/jpeg"                                         },
        {"dvsd", "video/x-dv,systemstream=true"                       },
        {"MPEG", "video/mpegts,systemstream=true"                     },
        {"H264", "video/x-h264,stream-format=byte-stream,alignment=au"},
        {"H263", "video/x-h263,variant=itu"                           },
        {"MPG1", "video/mpeg,mpegversion=2"                           },
        {"MPG2", "video/mpeg,mpegversion=2"                           },
        {"MPG4", "video/mpeg,mpegversion=4,systemstream=false"        },
        {"VP80", "video/x-vp8"                                        },

        // Vendor-specific formats
        {"S910", "video/x-sonix"},
        {"PWC1", "video/x-pwc1" },
        {"PWC2", "video/x-pwc2" },
        {"PJPG", "image/jpeg"   }
    };

    return fourCCToGst;
}

Q_GLOBAL_STATIC_WITH_ARGS(StringStringMap, fourCCToGst, (initFourCCToGst()))

ConvertVideoGStreamer::ConvertVideoGStreamer(QObject *parent):
    ConvertVideo(parent)
{
//    setenv("GST_DEBUG", "2", 1);
    gst_init(NULL, NULL);

    this->m_pipeline = NULL;
    this->m_source = NULL;
    this->m_sink = NULL;
    this->m_mainLoop = NULL;
    this->m_busWatchId = 0;
    this->m_id = -1;
    this->m_ptsDiff = AkNoPts<qint64>();
}

ConvertVideoGStreamer::~ConvertVideoGStreamer()
{
    this->uninit();
}

void ConvertVideoGStreamer::packetEnqueue(const AkPacket &packet)
{
    // Write audio frame to the pipeline.
    GstBuffer *buffer = gst_buffer_new_allocate(NULL,
                                                gsize(packet.buffer().size()),
                                                NULL);
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);
    memcpy(info.data, packet.buffer().constData(), info.size);
    gst_buffer_unmap(buffer, &info);

    if (this->m_ptsDiff == AkNoPts<qint64>())
        this->m_ptsDiff = packet.pts();

    qint64 pts = packet.pts() - this->m_ptsDiff;

    GST_BUFFER_PTS(buffer) = GstClockTime(pts * packet.timeBase().value() * GST_SECOND);
    GST_BUFFER_DTS(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_OFFSET(buffer) = GST_BUFFER_OFFSET_NONE;

    gst_app_src_push_buffer(GST_APP_SRC(this->m_source), buffer);
}

bool ConvertVideoGStreamer::init(const AkCaps &caps)
{
    QString fourcc = caps.property("fourcc").toString();
    int width = caps.property("width").toInt();
    int height = caps.property("height").toInt();
    AkFrac fps = caps.property("fps").toString();

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
    } else if (!gstCaps.mimeType().isEmpty()) {
        inCaps = gst_caps_from_string(gstCaps.toString().toStdString().c_str());
    } else
        return false;

    inCaps = gst_caps_fixate(inCaps);

    this->m_source = gst_element_factory_make("appsrc", NULL);
    gst_app_src_set_stream_type(GST_APP_SRC(this->m_source), GST_APP_STREAM_TYPE_STREAM);
    gst_app_src_set_caps(GST_APP_SRC(this->m_source), inCaps);
    g_object_set(G_OBJECT(this->m_source),
                 "format", GST_FORMAT_TIME,
                 "do-timestamp", TRUE,
                 "is-live", TRUE,
                 NULL);

    GstElement *decoder = this->decoderFromCaps(inCaps);
    gst_caps_unref(inCaps);
    GstElement *videoConvert = gst_element_factory_make("videoconvert", NULL);
    this->m_sink = gst_element_factory_make("appsink", NULL);
    g_object_set(G_OBJECT(this->m_sink),
                 "emit-signals", TRUE,
                 NULL);

    GstCaps *outCaps = gst_caps_new_simple("video/x-raw",
                                           "format", G_TYPE_STRING, "RGB",
                                           NULL);
    outCaps = gst_caps_fixate(outCaps);
    gst_app_sink_set_caps(GST_APP_SINK(this->m_sink), outCaps);
    gst_caps_unref(outCaps);

    g_signal_connect(this->m_sink,
                     "new-sample",
                     G_CALLBACK(this->videoBufferCallback),
                     this);

    this->m_pipeline = gst_pipeline_new(NULL);

    gst_bin_add_many(GST_BIN(this->m_pipeline),
                     this->m_source,
                     decoder,
                     videoConvert,
                     this->m_sink,
                     NULL);

    gst_element_link_many(this->m_source,
                          decoder,
                          videoConvert,
                          this->m_sink,
                          NULL);

    // Configure the message bus.
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->m_pipeline));
    this->m_busWatchId = gst_bus_add_watch(bus, this->busCallback, this);
    gst_object_unref(bus);

    this->m_id = Ak::id();
    this->m_ptsDiff = AkNoPts<qint64>();

    // Run the main GStreamer loop.
    this->m_mainLoop = g_main_loop_new(NULL, FALSE);
    QtConcurrent::run(&this->m_threadPool, g_main_loop_run, this->m_mainLoop);
    gst_element_set_state(this->m_pipeline, GST_STATE_PLAYING);

    return true;
}

void ConvertVideoGStreamer::uninit()
{
    if (this->m_pipeline) {
        gst_app_src_end_of_stream(GST_APP_SRC(this->m_source));
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

GstElement *ConvertVideoGStreamer::decoderFromCaps(const GstCaps *caps) const
{
    GstElement *decoder = NULL;
    static GstStaticCaps staticRawCaps =
            GST_STATIC_CAPS("video/x-raw;"
                            "audio/x-raw;"
                            "text/x-raw;"
                            "subpicture/x-dvd;"
                            "subpicture/x-pgs");

    GstCaps *rawCaps = gst_static_caps_get(&staticRawCaps);

    GList *decodersList = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_DECODER,
                                                                GST_RANK_PRIMARY);

    if (gst_caps_can_intersect(caps, rawCaps))
        decoder = gst_element_factory_make("identity", NULL);
    else {
        GList *decoders = gst_element_factory_list_filter(decodersList,
                                                          caps,
                                                          GST_PAD_SINK,
                                                          FALSE);

        for (GList *decoderItem = decoders; decoderItem; decoderItem = g_list_next(decoderItem)) {
            GstElementFactory *decoderFactory = (GstElementFactory *) decoderItem->data;
            decoder = gst_element_factory_make(GST_OBJECT_NAME(decoderFactory), NULL);

            break;
        }

        gst_plugin_feature_list_free(decoders);
    }

    gst_plugin_feature_list_free(decodersList);
    gst_caps_unref(rawCaps);

    return decoder;
}

void ConvertVideoGStreamer::waitState(GstState state)
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

gboolean ConvertVideoGStreamer::busCallback(GstBus *bus,
                                            GstMessage *message,
                                            gpointer userData)
{
    Q_UNUSED(bus)
    auto self = static_cast<ConvertVideoGStreamer *>(userData);

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
    case GST_MESSAGE_QOS: {
        qDebug() << QString("Received QOS from element %1:")
                        .arg(GST_MESSAGE_SRC_NAME(message)).toStdString().c_str();

        GstFormat format;
        guint64 processed;
        guint64 dropped;
        gst_message_parse_qos_stats(message, &format, &processed, &dropped);
        const gchar *formatStr = gst_format_get_name(format);
        qDebug() << "    Processed" << processed << formatStr;
        qDebug() << "    Dropped" << dropped << formatStr;

        gint64 jitter;
        gdouble proportion;
        gint quality;
        gst_message_parse_qos_values(message, &jitter, &proportion, &quality);
        qDebug() << "    Jitter =" << jitter;
        qDebug() << "    Proportion =" << proportion;
        qDebug() << "    Quality =" << quality;

        gboolean live;
        guint64 runningTime;
        guint64 streamTime;
        guint64 timestamp;
        guint64 duration;
        gst_message_parse_qos(message, &live, &runningTime, &streamTime, &timestamp, &duration);
        qDebug() << "    Is live stream =" << (live? true: false);
        qDebug() << "    Runninng time =" << runningTime;
        qDebug() << "    Stream time =" << streamTime;
        qDebug() << "    Timestamp =" << timestamp;
        qDebug() << "    Duration =" << duration;

        break;
    }
    default:
        qDebug() << "Unhandled message:" << GST_MESSAGE_TYPE_NAME(message);
    break;
    }

    return TRUE;
}

GstFlowReturn ConvertVideoGStreamer::videoBufferCallback(GstElement *videoOutput,
                                                         gpointer userData)
{
    auto self = static_cast<ConvertVideoGStreamer *>(userData);

    // Read audio frame from the pipeline.
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(videoOutput));

    if (!sample)
        return GST_FLOW_OK;

    GstCaps *caps = gst_sample_get_caps(sample);
    GstVideoInfo *videoInfo = gst_video_info_new();
    gst_video_info_from_caps(videoInfo, caps);

    // Create a package and return it.
    AkVideoPacket oVideoPacket;
    oVideoPacket.caps().isValid() = true;
    oVideoPacket.caps().format() = AkVideoCaps::Format_rgb24;
    oVideoPacket.caps().bpp() = AkVideoCaps::bitsPerPixel(oVideoPacket.caps().format());
    oVideoPacket.caps().width() = videoInfo->width;
    oVideoPacket.caps().height() = videoInfo->height;
    oVideoPacket.caps().fps() = AkFrac(videoInfo->fps_n, videoInfo->fps_d);

    gst_video_info_free(videoInfo);

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_READ);

    QByteArray oBuffer(int(info.size), Qt::Uninitialized);
    memcpy(oBuffer.data(), info.data, info.size);

    oVideoPacket.buffer() = oBuffer;
    oVideoPacket.pts() = qint64(GST_BUFFER_PTS(buffer));
    oVideoPacket.timeBase() = AkFrac(1, GST_SECOND);
    oVideoPacket.index() = 0;
    oVideoPacket.id() = self->m_id;

    gst_buffer_unmap(buffer, &info);
    gst_sample_unref(sample);

    emit self->frameReady(oVideoPacket.toPacket());

    return GST_FLOW_OK;
}

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

#include <QMetaEnum>
#include <QtConcurrent>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akcompressedvideocaps.h>
#include <akvideocaps.h>
#include <akvideopacket.h>
#include <gst/video/video.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#include "convertvideogstreamer.h"

using GstCodecMap = QMap<QString, QString>;

inline const GstCodecMap &initCompressedGstToStr()
{
    static const GstCodecMap fourCCToGst {
        {"video/mjpg"                                         , "mjpg"},
        {"image/jpeg"                                         , "jpeg"},
        {"video/x-dv,systemstream=true"                       , "dvsd"},
        {"video/mpegts,systemstream=true"                     , "mpeg"},
        {"video/x-h264,stream-format=byte-stream,alignment=au", "h264"},
        {"video/x-h263,variant=itu"                           , "h263"},
        {"video/mpeg,mpegversion=1"                           , "mpg1"},
        {"video/mpeg,mpegversion=2"                           , "mpg2"},
        {"video/mpeg,mpegversion=4,systemstream=false"        , "mpg4"},
        {"video/x-vp8"                                        , "vp80"},
    };

    return fourCCToGst;
}

Q_GLOBAL_STATIC_WITH_ARGS(GstCodecMap, compressedGstToStr, (initCompressedGstToStr()))

class ConvertVideoGStreamerPrivate
{
    public:
        QThreadPool m_threadPool;
        GstElement *m_pipeline {nullptr};
        GstElement *m_source {nullptr};
        GstElement *m_sink {nullptr};
        GMainLoop *m_mainLoop {nullptr};
        QFuture<void> m_mainLoopResult;
        guint m_busWatchId {0};
        qint64 m_id {-1};
        qint64 m_ptsDiff {AkNoPts<qint64>()};

        GstElement *decoderFromCaps(const GstCaps *caps) const;
        void waitState(GstState state);
        static gboolean busCallback(GstBus *bus,
                                    GstMessage *message,
                                    gpointer userData);
        static GstFlowReturn videoBufferCallback(GstElement *videoOutput,
                                                 gpointer userData);
};

ConvertVideoGStreamer::ConvertVideoGStreamer(QObject *parent):
    ConvertVideo(parent)
{
    //qputenv("GST_DEBUG", "2");
    auto binDir = QDir(BINDIR).absolutePath();
    auto gstPluginsDir = QDir(GST_PLUGINS_PATH).absolutePath();
    auto relGstPluginsDir = QDir(binDir).relativeFilePath(gstPluginsDir);
    QDir appDir = QCoreApplication::applicationDirPath();

    if (appDir.cd(relGstPluginsDir)) {
        auto path = appDir.absolutePath();
        path.replace("/", QDir::separator());

        if (QFileInfo::exists(path)) {
            if (qEnvironmentVariableIsEmpty("GST_PLUGIN_PATH"))
                qputenv("GST_PLUGIN_PATH", path.toLocal8Bit());

            auto scanner = QFileInfo(GST_PLUGINS_SCANNER_PATH).baseName();

            if (!scanner.isEmpty()) {
                auto scannerPath = path + "/" + scanner;

                if (QFileInfo::exists(scannerPath)) {
                    if (qEnvironmentVariableIsEmpty("GST_PLUGIN_SCANNER"))
                        qputenv("GST_PLUGIN_SCANNER", scannerPath.toLocal8Bit());
                }
            }
        }
    }

    gst_init(nullptr, nullptr);

    this->d = new ConvertVideoGStreamerPrivate;
}

ConvertVideoGStreamer::~ConvertVideoGStreamer()
{
    this->uninit();
    delete this->d;
}

void ConvertVideoGStreamer::packetEnqueue(const AkPacket &packet)
{
    // Write audio frame to the pipeline.
    GstBuffer *buffer = gst_buffer_new_allocate(nullptr,
                                                gsize(packet.size()),
                                                nullptr);
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);
    memcpy(info.data, packet.constData(), info.size);
    gst_buffer_unmap(buffer, &info);

    if (this->d->m_ptsDiff == AkNoPts<qint64>())
        this->d->m_ptsDiff = packet.pts();

    qint64 pts = packet.pts() - this->d->m_ptsDiff;

    GST_BUFFER_PTS(buffer) = GstClockTime(pts * packet.timeBase().value() * GST_SECOND);
    GST_BUFFER_DTS(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_OFFSET(buffer) = GST_BUFFER_OFFSET_NONE;

    gst_app_src_push_buffer(GST_APP_SRC(this->d->m_source), buffer);
}

bool ConvertVideoGStreamer::init(const AkCaps &caps)
{
    AkCompressedVideoCaps videoCaps(caps);
    QString format = videoCaps.format();
    auto gstCaps = compressedGstToStr->key(format);

    if (gstCaps.isEmpty())
        return false;

    auto inCaps = gst_caps_from_string(gstCaps.toStdString().c_str());

    if (gstCaps.startsWith("video/x-raw")
        || gstCaps.startsWith("video/x-bayer")
        || gstCaps.startsWith("video/x-pwc1")
        || gstCaps.startsWith("video/x-pwc2")
        || gstCaps.startsWith("video/x-sonix")) {
        auto fps = videoCaps.fps();
        gst_caps_set_simple(inCaps,
                            "width", videoCaps.width(),
                            "height", videoCaps.height(),
                            "framerate", fps.toString().toStdString().c_str(),
                            nullptr);
    }

    inCaps = gst_caps_fixate(inCaps);

    this->d->m_source = gst_element_factory_make("appsrc", nullptr);
    gst_app_src_set_stream_type(GST_APP_SRC(this->d->m_source),
                                GST_APP_STREAM_TYPE_STREAM);
    gst_app_src_set_caps(GST_APP_SRC(this->d->m_source), inCaps);
    g_object_set(G_OBJECT(this->d->m_source),
                 "format", GST_FORMAT_TIME,
                 "do-timestamp", TRUE,
                 "is-live", TRUE,
                 nullptr);

    GstElement *decoder = this->d->decoderFromCaps(inCaps);
    gst_caps_unref(inCaps);
    auto videoConvert = gst_element_factory_make("videoconvert", nullptr);
    this->d->m_sink = gst_element_factory_make("appsink", nullptr);
    g_object_set(G_OBJECT(this->d->m_sink),
                 "emit-signals", TRUE,
                 nullptr);

    GstCaps *outCaps = gst_caps_new_simple("video/x-raw",
                                           "format", G_TYPE_STRING, "RGB",
                                           nullptr);
    outCaps = gst_caps_fixate(outCaps);
    gst_app_sink_set_caps(GST_APP_SINK(this->d->m_sink), outCaps);
    gst_caps_unref(outCaps);

    g_signal_connect(this->d->m_sink,
                     "new-sample",
                     G_CALLBACK(this->d->videoBufferCallback),
                     this);

    this->d->m_pipeline = gst_pipeline_new(nullptr);

    gst_bin_add_many(GST_BIN(this->d->m_pipeline),
                     this->d->m_source,
                     decoder,
                     videoConvert,
                     this->d->m_sink,
                     nullptr);
    gst_element_link_many(this->d->m_source,
                          decoder,
                          videoConvert,
                          this->d->m_sink,
                          nullptr);

    // Configure the message bus.
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->d->m_pipeline));
    this->d->m_busWatchId = gst_bus_add_watch(bus, this->d->busCallback, this);
    gst_object_unref(bus);

    this->d->m_id = Ak::id();
    this->d->m_ptsDiff = AkNoPts<qint64>();

    // Run the main GStreamer loop.
    this->d->m_mainLoop = g_main_loop_new(nullptr, FALSE);
    this->d->m_mainLoopResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              g_main_loop_run,
                              this->d->m_mainLoop);
    gst_element_set_state(this->d->m_pipeline, GST_STATE_PLAYING);

    return true;
}

void ConvertVideoGStreamer::uninit()
{
    if (this->d->m_pipeline) {
        gst_app_src_end_of_stream(GST_APP_SRC(this->d->m_source));
        gst_element_set_state(this->d->m_pipeline, GST_STATE_NULL);
        this->d->waitState(GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(this->d->m_pipeline));
        g_source_remove(this->d->m_busWatchId);
        this->d->m_pipeline = nullptr;
        this->d->m_busWatchId = 0;
    }

    if (this->d->m_mainLoop) {
        g_main_loop_quit(this->d->m_mainLoop);
        g_main_loop_unref(this->d->m_mainLoop);
        this->d->m_mainLoop = nullptr;
    }
}

GstElement *ConvertVideoGStreamerPrivate::decoderFromCaps(const GstCaps *caps) const
{
    GstElement *decoder = nullptr;
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
        decoder = gst_element_factory_make("identity", nullptr);
    else {
        GList *decoders = gst_element_factory_list_filter(decodersList,
                                                          caps,
                                                          GST_PAD_SINK,
                                                          FALSE);

        for (auto decoderItem = decoders; decoderItem; decoderItem = g_list_next(decoderItem)) {
            auto decoderFactory = reinterpret_cast<GstElementFactory *>(decoderItem->data);
            decoder = gst_element_factory_make(GST_OBJECT_NAME(decoderFactory), nullptr);

            break;
        }

        gst_plugin_feature_list_free(decoders);
    }

    gst_plugin_feature_list_free(decodersList);
    gst_caps_unref(rawCaps);

    return decoder;
}

void ConvertVideoGStreamerPrivate::waitState(GstState state)
{
    forever {
        GstState curState;
        GstStateChangeReturn ret = gst_element_get_state(this->m_pipeline,
                                                         &curState,
                                                         nullptr,
                                                         GST_CLOCK_TIME_NONE);

        if (ret == GST_STATE_CHANGE_FAILURE)
            break;

        if (ret == GST_STATE_CHANGE_SUCCESS
            && curState == state)
            break;
    }
}

gboolean ConvertVideoGStreamerPrivate::busCallback(GstBus *bus,
                                                   GstMessage *message,
                                                   gpointer userData)
{
    Q_UNUSED(bus)
    auto self = static_cast<ConvertVideoGStreamer *>(userData);

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        GError *err = nullptr;
        gchar *debug = nullptr;
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
        g_main_loop_quit(self->d->m_mainLoop);

        break;
    }
    case GST_MESSAGE_EOS:
        g_main_loop_quit(self->d->m_mainLoop);
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
        GstElement *owner = nullptr;
        gst_message_parse_stream_status(message, &type, &owner);
        qDebug() << "Stream Status:"
                 << GST_ELEMENT_NAME(owner)
                 << "is"
                 << type;

        break;
    }
    case GST_MESSAGE_LATENCY: {
        qDebug() << "Recalculating latency";
        gst_bin_recalculate_latency(GST_BIN(self->d->m_pipeline));
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
        GstClock *clock = nullptr;
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
        GstTagList *tagList = nullptr;
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
        qDebug() << "    Is live stream =" << live;
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

GstFlowReturn ConvertVideoGStreamerPrivate::videoBufferCallback(GstElement *videoOutput,
                                                                gpointer userData)
{
    auto self = static_cast<ConvertVideoGStreamer *>(userData);

    // Read audio frame from the pipeline.
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(videoOutput));

    if (!sample)
        return GST_FLOW_OK;

    auto caps = gst_sample_get_caps(sample);
    auto videoInfo = gst_video_info_new();
    gst_video_info_from_caps(videoInfo, caps);

    // Create a package and return it.
    AkVideoCaps videoCaps(AkVideoCaps::Format_rgb24,
                          videoInfo->width,
                          videoInfo->height,
                          AkFrac(videoInfo->fps_n, videoInfo->fps_d));
    AkVideoPacket oVideoPacket(videoCaps);
    auto buffer = gst_sample_get_buffer(sample);
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_READ);

    for (int plane = 0; plane < GST_VIDEO_INFO_N_PLANES(videoInfo); ++plane) {
        auto planeData = info.data + GST_VIDEO_INFO_PLANE_OFFSET(videoInfo, plane);
        auto oLineSize = GST_VIDEO_INFO_PLANE_STRIDE(videoInfo, plane);
        auto lineSize = qMin<size_t>(oVideoPacket.lineSize(plane), oLineSize);
        auto heightDiv = oVideoPacket.heightDiv(plane);

        for (int y = 0; y < videoInfo->height; ++y) {
            auto ys = y >> heightDiv;
            memcpy(oVideoPacket.line(plane, y),
                   planeData + ys * oLineSize,
                   lineSize);
        }
    }

    oVideoPacket.setPts(qint64(GST_BUFFER_PTS(buffer)));
    oVideoPacket.setTimeBase({1, GST_SECOND});
    oVideoPacket.setIndex(0);
    oVideoPacket.setId(self->d->m_id);

    gst_buffer_unmap(buffer, &info);
    gst_sample_unref(sample);
    gst_video_info_free(videoInfo);

    emit self->frameReady(oVideoPacket);

    return GST_FLOW_OK;
}

#include "moc_convertvideogstreamer.cpp"

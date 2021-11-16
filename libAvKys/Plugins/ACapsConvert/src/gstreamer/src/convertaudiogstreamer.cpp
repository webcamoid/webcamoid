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

#include <QMap>
#include <QtConcurrent>
#include <QThreadPool>
#include <QMutex>
#include <akfrac.h>
#include <akpacket.h>
#include <akaudiocaps.h>
#include <akaudiopacket.h>

#include <gst/audio/audio.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#include "convertaudiogstreamer.h"

using GstFormatMap = QMap<AkAudioCaps::SampleFormat, QString>;

class ConvertAudioGStreamerPrivate
{
    public:
        AkAudioCaps m_caps;
        QThreadPool m_threadPool;
        GstElement *m_pipeline {nullptr};
        GstElement *m_source {nullptr};
        GstElement *m_sink {nullptr};
        GMainLoop *m_mainLoop {nullptr};
        QMutex m_mutex;
        guint m_busWatchId {0};

        inline void waitState(GstState state);
        inline static gboolean busCallback(GstBus *bus,
                                           GstMessage *message,
                                           gpointer userData);

        inline static const GstFormatMap &gstToFormat()
        {
            static const GstFormatMap gstToFormat {
                {AkAudioCaps::SampleFormat_s8   , "S8"   },
                {AkAudioCaps::SampleFormat_u8   , "U8"   },
                {AkAudioCaps::SampleFormat_s16le, "S16LE"},
                {AkAudioCaps::SampleFormat_s16be, "S16BE"},
                {AkAudioCaps::SampleFormat_u16le, "U16LE"},
                {AkAudioCaps::SampleFormat_u16be, "U16BE"},
                {AkAudioCaps::SampleFormat_s32le, "S32LE"},
                {AkAudioCaps::SampleFormat_s32be, "S32BE"},
                {AkAudioCaps::SampleFormat_u32le, "U32LE"},
                {AkAudioCaps::SampleFormat_u32be, "U32BE"},
                {AkAudioCaps::SampleFormat_fltle, "F32LE"},
                {AkAudioCaps::SampleFormat_fltbe, "F32BE"},
                {AkAudioCaps::SampleFormat_dblle, "F64LE"},
                {AkAudioCaps::SampleFormat_dblbe, "F64BE"},
            };

            return gstToFormat;
        }
};

ConvertAudioGStreamer::ConvertAudioGStreamer(QObject *parent):
    ConvertAudio(parent)
{
    this->d = new ConvertAudioGStreamerPrivate;
    //qputenv("GST_DEBUG", "2");
    auto binDir = QDir(BINDIR).absolutePath();
    auto gstPluginsDir = QDir(GST_PLUGINS_PATH).absolutePath();
    auto relGstPluginsDir = QDir(binDir).relativeFilePath(gstPluginsDir);
    QDir appDir = QCoreApplication::applicationDirPath();
    appDir.cd(relGstPluginsDir);
    auto path = appDir.absolutePath();
    path.replace("/", QDir::separator());

    if (QFileInfo::exists(path)) {
        if (qEnvironmentVariableIsEmpty("GST_PLUGIN_PATH"))
            qputenv("GST_PLUGIN_PATH", path.toLocal8Bit());
    }

    gst_init(nullptr, nullptr);
}

ConvertAudioGStreamer::~ConvertAudioGStreamer()
{
    this->uninit();
    delete this->d;
}

bool ConvertAudioGStreamer::init(const AkAudioCaps &caps)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    this->d->m_pipeline = gst_pipeline_new(nullptr);
    this->d->m_source = gst_element_factory_make("appsrc", nullptr);
    gst_app_src_set_stream_type(GST_APP_SRC(this->d->m_source),
                                GST_APP_STREAM_TYPE_STREAM);
    g_object_set(G_OBJECT(this->d->m_source),
                 "format",
                 GST_FORMAT_TIME,
                 nullptr);

    auto audioConvert = gst_element_factory_make("audioconvert", nullptr);
    auto audioResample = gst_element_factory_make("audioresample", nullptr);
    auto audioRate = gst_element_factory_make("audiorate", nullptr);
    this->d->m_sink = gst_element_factory_make("appsink", nullptr);

    gst_bin_add_many(GST_BIN(this->d->m_pipeline),
                     this->d->m_source,
                     audioResample,
                     audioRate,
                     audioConvert,
                     this->d->m_sink,
                     nullptr);
    gst_element_link_many(this->d->m_source,
                          audioResample,
                          audioRate,
                          audioConvert,
                          this->d->m_sink,
                          nullptr);

    // Configure the message bus.
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->d->m_pipeline));
    this->d->m_busWatchId =
            gst_bus_add_watch(bus,
                              ConvertAudioGStreamerPrivate::busCallback,
                              this);
    gst_object_unref(bus);

    this->d->m_caps = caps;

    return true;
}

AkPacket ConvertAudioGStreamer::convert(const AkAudioPacket &packet)
{
    auto packetCaps = packet.caps();
    packetCaps.setSamples(0);
    packetCaps.setPlaneSize({0});

    if (packetCaps == this->d->m_caps)
        return packet;

    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_pipeline
        || !this->d->m_source
        || !this->d->m_sink
        || !this->d->m_caps)
        return AkPacket();

    auto gstIFormat =
            ConvertAudioGStreamerPrivate::gstToFormat().value(packet.caps().format());

    const char *gstInLayout =
            packet.caps().planar()?
                "non-interleaved": "interleaved";

    auto inCaps =
            gst_caps_new_simple("audio/x-raw",
                                "format", G_TYPE_STRING, gstIFormat.toStdString().c_str(),
                                "layout", G_TYPE_STRING, gstInLayout,
                                "rate", G_TYPE_INT, packet.caps().rate(),
                                "channels", G_TYPE_INT, packet.caps().channels(),
                                nullptr);

    inCaps = gst_caps_fixate(inCaps);
    auto sourceCaps = gst_app_src_get_caps(GST_APP_SRC(this->d->m_source));

    if (!sourceCaps || !gst_caps_is_equal(sourceCaps, inCaps))
        gst_app_src_set_caps(GST_APP_SRC(this->d->m_source), inCaps);

    gst_caps_unref(inCaps);

    if (sourceCaps)
        gst_caps_unref(sourceCaps);

    auto gstOFormat =
            ConvertAudioGStreamerPrivate::gstToFormat().value(this->d->m_caps.format());

    const char *gstOutLayout = this->d->m_caps.planar()?
                                   "non-interleaved": "interleaved";

    auto outCaps =
            gst_caps_new_simple("audio/x-raw",
                                "format", G_TYPE_STRING, gstOFormat.toStdString().c_str(),
                                "layout", G_TYPE_STRING, gstOutLayout,
                                "rate", G_TYPE_INT, this->d->m_caps.rate(),
                                "channels", G_TYPE_INT, this->d->m_caps.channels(),
                                nullptr);

    outCaps = gst_caps_fixate(outCaps);
    auto sinkCaps = gst_app_sink_get_caps(GST_APP_SINK(this->d->m_sink));

    if (!sinkCaps || !gst_caps_is_equal(sinkCaps, outCaps))
        gst_app_sink_set_caps(GST_APP_SINK(this->d->m_sink), outCaps);

    gst_caps_unref(outCaps);

    if (sourceCaps)
        gst_caps_unref(sinkCaps);

    // Start pipeline if it's not it.
    GstState state;
    gst_element_get_state(this->d->m_pipeline,
                          &state,
                          nullptr,
                          GST_CLOCK_TIME_NONE);

    if (state != GST_STATE_PLAYING) {
        // Run the main GStreamer loop.
        this->d->m_mainLoop = g_main_loop_new(nullptr, FALSE);
        QtConcurrent::run(&this->d->m_threadPool,
                          g_main_loop_run,
                          this->d->m_mainLoop);
        gst_element_set_state(this->d->m_pipeline, GST_STATE_PLAYING);
    }

    // Write audio frame to the pipeline.
    auto buffer = gst_buffer_new_allocate(nullptr,
                                          gsize(packet.buffer().size()),
                                          nullptr);
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);
    memcpy(info.data, packet.buffer().constData(), info.size);
    gst_buffer_unmap(buffer, &info);

    GST_BUFFER_PTS(buffer) =
            GstClockTime(packet.pts() * packet.timeBase().value() * GST_SECOND);
    GST_BUFFER_DTS(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_OFFSET(buffer) = GST_BUFFER_OFFSET_NONE;

    gst_app_src_push_buffer(GST_APP_SRC(this->d->m_source), buffer);

    // Read audio frame from the pipeline.
    auto sample = gst_app_sink_pull_sample(GST_APP_SINK(this->d->m_sink));

    if (!sample)
        return AkPacket();

    buffer = gst_sample_get_buffer(sample);
    gst_buffer_map(buffer, &info, GST_MAP_READ);
    QByteArray oBuffer(int(info.size), 0);
    memcpy(oBuffer.data(), info.data, info.size);
    gst_buffer_unmap(buffer, &info);
    auto pts = qint64(GST_BUFFER_PTS(buffer) / packet.timeBase().value() / GST_SECOND);
    gst_sample_unref(sample);

    // Create a package and return it.
    int nSamples = 8 * int(info.size)
                   / AkAudioCaps::bitsPerSample(this->d->m_caps.format())
                   / this->d->m_caps.channels();

    AkAudioPacket oAudioPacket;
    oAudioPacket.caps() = this->d->m_caps;
    oAudioPacket.caps().setSamples(nSamples);
    oAudioPacket.buffer() = oBuffer;
    oAudioPacket.pts() = pts;
    oAudioPacket.timeBase() = AkFrac(1, this->d->m_caps.rate());
    oAudioPacket.index() = packet.index();
    oAudioPacket.id() = packet.id();

    return oAudioPacket;
}

void ConvertAudioGStreamer::uninit()
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    this->d->m_caps = AkAudioCaps();

    if (this->d->m_pipeline) {
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

void ConvertAudioGStreamerPrivate::waitState(GstState state)
{
    forever {
        GstState curState;
        auto ret = gst_element_get_state(this->m_pipeline,
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

gboolean ConvertAudioGStreamerPrivate::busCallback(GstBus *bus,
                                                   GstMessage *message,
                                                   gpointer userData)
{
    Q_UNUSED(bus)
    auto self = static_cast<ConvertAudioGStreamer *>(userData);

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

        for (const GList *padItem = GST_ELEMENT_PADS(element);
             padItem;
             padItem = g_list_next(padItem)) {
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
        gst_message_parse_state_changed(message,
                                        &oldstate,
                                        &newstate,
                                        &pending);
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
        auto messageStructure = gst_message_get_structure(message);
        auto structure = gst_structure_to_string(messageStructure);
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
        gst_message_parse_qos(message,
                              &live,
                              &runningTime,
                              &streamTime,
                              &timestamp,
                              &duration);
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

#include "moc_convertaudiogstreamer.cpp"

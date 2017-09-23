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

#include "convertaudiogstreamer.h"

typedef QMap<QString, QString> StringStringMap;

inline StringStringMap initGstToFF()
{
    StringStringMap gstToFF = {
        {"S8"      , "s8"     },
        {"U8"      , "u8"     },
        {"S16LE"   , "s16le"  },
        {"S16BE"   , "s16be"  },
        {"U16LE"   , "u16le"  },
        {"U16BE"   , "u16be"  },
        {"S24_32LE", "s2432le"},
        {"S24_32BE", "s2432be"},
        {"U24_32LE", "u2432le"},
        {"U24_32BE", "u2432be"},
        {"S32LE"   , "s32le"  },
        {"S32BE"   , "s32be"  },
        {"U32LE"   , "u32le"  },
        {"U32BE"   , "u32be"  },
        {"S24LE"   , "s24le"  },
        {"S24BE"   , "s24be"  },
        {"U24LE"   , "u24le"  },
        {"U24BE"   , "u24be"  },
        {"S20LE"   , "s20le"  },
        {"S20BE"   , "s20be"  },
        {"U20LE"   , "u20le"  },
        {"U20BE"   , "u20be"  },
        {"S18LE"   , "s18le"  },
        {"S18BE"   , "s18be"  },
        {"U18LE"   , "u18le"  },
        {"U18BE"   , "u18le"  },
        {"F32LE"   , "fltle"  },
        {"F32BE"   , "fltbe"  },
        {"F64LE"   , "dblle"  },
        {"F64BE"   , "dblbe"  },
        {"S16"     , "s16"    },
        {"U16"     , "u16"    },
        {"S24_32"  , "s2432"  },
        {"U24_32"  , "u2432"  },
        {"S32"     , "s32"    },
        {"U32"     , "u32"    },
        {"S24"     , "s24"    },
        {"U24"     , "u24"    },
        {"S20"     , "s20"    },
        {"U20"     , "u20"    },
        {"S18"     , "s18"    },
        {"U18"     , "u18"    },
        {"F32"     , "flt"    },
        {"F64"     , "dbl"    }
    };

    return gstToFF;
}

Q_GLOBAL_STATIC_WITH_ARGS(StringStringMap, gstToFF, (initGstToFF()))

ConvertAudioGStreamer::ConvertAudioGStreamer(QObject *parent):
    ConvertAudio(parent),
    m_pipeline(nullptr),
    m_source(nullptr),
    m_sink(nullptr),
    m_mainLoop(nullptr),
    m_busWatchId(0)
{
//    setenv("GST_DEBUG", "2", 1);
    gst_init(nullptr, nullptr);
}

ConvertAudioGStreamer::~ConvertAudioGStreamer()
{
    this->uninit();
}

bool ConvertAudioGStreamer::init(const AkAudioCaps &caps)
{
    QMutexLocker mutexLocker(&this->m_mutex);

    this->m_pipeline = gst_pipeline_new(nullptr);

    this->m_source = gst_element_factory_make("appsrc", nullptr);
    gst_app_src_set_stream_type(GST_APP_SRC(this->m_source), GST_APP_STREAM_TYPE_STREAM);
    g_object_set(G_OBJECT(this->m_source), "format", GST_FORMAT_TIME, nullptr);

    GstElement *audioConvert = gst_element_factory_make("audioconvert", nullptr);
    GstElement *audioResample = gst_element_factory_make("audioresample", nullptr);
    GstElement *audioRate = gst_element_factory_make("audiorate", nullptr);
    this->m_sink = gst_element_factory_make("appsink", nullptr);

    gst_bin_add_many(GST_BIN(this->m_pipeline),
                     this->m_source,
                     audioResample,
                     audioRate,
                     audioConvert,
                     this->m_sink,
                     nullptr);

    gst_element_link_many(this->m_source,
                          audioResample,
                          audioRate,
                          audioConvert,
                          this->m_sink,
                          nullptr);

    // Configure the message bus.
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->m_pipeline));
    this->m_busWatchId = gst_bus_add_watch(bus, this->busCallback, this);
    gst_object_unref(bus);

    this->m_caps = caps;

    return true;
}

AkPacket ConvertAudioGStreamer::convert(const AkAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_pipeline
        || !this->m_source
        || !this->m_sink
        || !this->m_caps)
        return AkPacket();

    QString iFormat = AkAudioCaps::sampleFormatToString(packet.caps().format());
    QString gstIFormat = gstToFF->key(iFormat, "S16");

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    QString fEnd = "LE";
#elif Q_BYTE_ORDER == Q_BIG_ENDIAN
    QString fEnd = "BE";
#endif

    if (packet.caps().bps() > 8 && !gstIFormat.endsWith(fEnd))
        gstIFormat += fEnd;

    const char *gstInLayout =
            AkAudioCaps::isPlanar(packet.caps().format())?
                "non-interleaved": "interleaved";

    GstCaps *inCaps = gst_caps_new_simple("audio/x-raw",
                                          "format", G_TYPE_STRING, gstIFormat.toStdString().c_str(),
                                          "layout", G_TYPE_STRING, gstInLayout,
                                          "rate", G_TYPE_INT, packet.caps().rate(),
                                          "channels", G_TYPE_INT, packet.caps().channels(),
                                          nullptr);

    inCaps = gst_caps_fixate(inCaps);
    GstCaps *sourceCaps = gst_app_src_get_caps(GST_APP_SRC(this->m_source));

    if (!sourceCaps || !gst_caps_is_equal(sourceCaps, inCaps))
        gst_app_src_set_caps(GST_APP_SRC(this->m_source), inCaps);

    gst_caps_unref(inCaps);

    if (sourceCaps)
        gst_caps_unref(sourceCaps);

    QString oFormat = AkAudioCaps::sampleFormatToString(this->m_caps.format());
    QString gstOFormat = gstToFF->key(oFormat, "S16");

    if (this->m_caps.bps() > 8 && !gstOFormat.endsWith(fEnd))
        gstOFormat += fEnd;

    const char *gstOutLayout =
            AkAudioCaps::isPlanar(this->m_caps.format())?
                "non-interleaved": "interleaved";

    GstCaps *outCaps = gst_caps_new_simple("audio/x-raw",
                                           "format", G_TYPE_STRING, gstOFormat.toStdString().c_str(),
                                           "layout", G_TYPE_STRING, gstOutLayout,
                                           "rate", G_TYPE_INT, this->m_caps.rate(),
                                           "channels", G_TYPE_INT, this->m_caps.channels(),
                                           nullptr);

    outCaps = gst_caps_fixate(outCaps);
    GstCaps *sinkCaps = gst_app_sink_get_caps(GST_APP_SINK(this->m_sink));

    if (!sinkCaps || !gst_caps_is_equal(sinkCaps, outCaps))
        gst_app_sink_set_caps(GST_APP_SINK(this->m_sink), outCaps);

    gst_caps_unref(outCaps);

    if (sourceCaps)
        gst_caps_unref(sinkCaps);

    // Start pipeline if it's not it.
    GstState state;
    gst_element_get_state(this->m_pipeline,
                          &state,
                          nullptr,
                          GST_CLOCK_TIME_NONE);

    if (state != GST_STATE_PLAYING) {
        // Run the main GStreamer loop.
        this->m_mainLoop = g_main_loop_new(nullptr, FALSE);
        QtConcurrent::run(&this->m_threadPool, g_main_loop_run, this->m_mainLoop);
        gst_element_set_state(this->m_pipeline, GST_STATE_PLAYING);
    }

    // Write audio frame to the pipeline.
    GstBuffer *buffer = gst_buffer_new_allocate(nullptr,
                                                gsize(packet.buffer().size()),
                                                nullptr);
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);
    memcpy(info.data, packet.buffer().constData(), info.size);
    gst_buffer_unmap(buffer, &info);

    GST_BUFFER_PTS(buffer) = GstClockTime(packet.pts() * packet.timeBase().value() * GST_SECOND);
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
    QByteArray oBuffer(int(info.size), 0);
    memcpy(oBuffer.data(), info.data, info.size);
    gst_buffer_unmap(buffer, &info);
    qint64 pts = qint64(GST_BUFFER_PTS(buffer) / packet.timeBase().value() / GST_SECOND);
    gst_sample_unref(sample);

    // Create a package and return it.
    int nSamples = 8 * int(info.size)
                   / AkAudioCaps::bitsPerSample(this->m_caps.format())
                   / this->m_caps.channels();

    AkAudioPacket oAudioPacket;
    oAudioPacket.caps() = this->m_caps;
    oAudioPacket.caps().samples() = nSamples;
    oAudioPacket.buffer() = oBuffer;
    oAudioPacket.pts() = pts;
    oAudioPacket.timeBase() = AkFrac(1, this->m_caps.rate());
    oAudioPacket.index() = packet.index();
    oAudioPacket.id() = packet.id();

    return oAudioPacket.toPacket();
}

void ConvertAudioGStreamer::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    this->m_caps = AkAudioCaps();

    if (this->m_pipeline) {
        gst_element_set_state(this->m_pipeline, GST_STATE_NULL);
        this->waitState(GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(this->m_pipeline));
        g_source_remove(this->m_busWatchId);
        this->m_pipeline = nullptr;
        this->m_busWatchId = 0;
    }

    if (this->m_mainLoop) {
        g_main_loop_quit(this->m_mainLoop);
        g_main_loop_unref(this->m_mainLoop);
        this->m_mainLoop = nullptr;
    }
}

void ConvertAudioGStreamer::waitState(GstState state)
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

gboolean ConvertAudioGStreamer::busCallback(GstBus *bus,
                                   GstMessage *message,
                                   gpointer userData)
{
    Q_UNUSED(bus)
    ConvertAudioGStreamer *self = static_cast<ConvertAudioGStreamer *>(userData);

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

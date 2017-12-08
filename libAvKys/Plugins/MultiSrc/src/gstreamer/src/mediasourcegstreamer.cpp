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

#include <QDebug>
#include <QFileInfo>
#include <QtConcurrent>
#include <QThreadPool>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/audio/audio.h>
#include <gst/video/video.h>
#include <ak.h>
#include <akaudiopacket.h>
#include <akvideopacket.h>
#include <akaudiocaps.h>
#include <akvideocaps.h>

#include "mediasourcegstreamer.h"
#include "stream.h"

class MediaSourceGStreamerPrivate
{
    public:
        QString m_media;
        QList<int> m_streams;
        bool m_loop;
        bool m_run;
        AkElement::ElementState m_curState;
        qint64 m_maxPacketQueueSize;
        bool m_showLog;
        QThreadPool m_threadPool;
        GstElement *m_pipeline;
        GMainLoop *m_mainLoop;
        guint m_busWatchId;
        qint64 m_audioIndex;
        qint64 m_videoIndex;
        qint64 m_subtitlesIndex;
        qint64 m_audioId;
        qint64 m_videoId;
        qint64 m_subtitlesId;
        QList<Stream> m_streamInfo;

        MediaSourceGStreamerPrivate():
            m_loop(false),
            m_run(false),
            m_curState(AkElement::ElementStateNull),
            m_maxPacketQueueSize(15 * 1024 * 1024),
            m_showLog(false),
            m_pipeline(nullptr),
            m_mainLoop(nullptr),
            m_busWatchId(0),
            m_audioIndex(-1),
            m_videoIndex(-1),
            m_subtitlesIndex(-1),
            m_audioId(-1),
            m_videoId(-1),
            m_subtitlesId(-1)
        {
        }

        inline void waitState(GstState state);
        inline static gboolean busCallback(GstBus *bus,
                                           GstMessage *message,
                                           gpointer userData);
        inline static GstFlowReturn audioBufferCallback(GstElement *audioOutput,
                                                        gpointer userData);
        inline static GstFlowReturn videoBufferCallback(GstElement *videoOutput,
                                                        gpointer userData);
        inline static GstFlowReturn subtitlesBufferCallback(GstElement *subtitlesOutput,
                                                            gpointer userData);
        inline static void aboutToFinish(GstElement *object, gpointer userData);
        inline QStringList languageCodes(const QString &type);
        inline QStringList languageCodes();
};

MediaSourceGStreamer::MediaSourceGStreamer(QObject *parent):
    MediaSource(parent)
{
//    setenv("GST_DEBUG", "2", 1);
    gst_init(nullptr, nullptr);

    this->d = new MediaSourceGStreamerPrivate;
}

MediaSourceGStreamer::~MediaSourceGStreamer()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QStringList MediaSourceGStreamer::medias() const
{
    QStringList medias;

    if (!this->d->m_media.isEmpty())
        medias << this->d->m_media;

    return medias;
}

QString MediaSourceGStreamer::media() const
{
    return this->d->m_media;
}

QList<int> MediaSourceGStreamer::streams() const
{
    return this->d->m_streams;
}

QList<int> MediaSourceGStreamer::listTracks(const QString &mimeType)
{
    bool isRunning = this->d->m_run;

    if (!isRunning)
        this->setState(AkElement::ElementStatePaused);

    QList<int> tracks;

    for (int stream = 0; stream < this->d->m_streamInfo.size(); stream++)
        if (mimeType.isEmpty()
            || this->d->m_streamInfo[stream].caps.mimeType() == mimeType)
            tracks << stream;

    if (!isRunning)
        this->setState(AkElement::ElementStateNull);

    return tracks;
}

QString MediaSourceGStreamer::streamLanguage(int stream)
{
    bool isRunning = this->d->m_run;

    if (!isRunning)
        this->setState(AkElement::ElementStatePaused);

    Stream streamInfo = this->d->m_streamInfo.value(stream, Stream());

    if (!isRunning)
        this->setState(AkElement::ElementStateNull);

    return streamInfo.language;
}

bool MediaSourceGStreamer::loop() const
{
    return this->d->m_loop;
}

int MediaSourceGStreamer::defaultStream(const QString &mimeType)
{
    bool isRunning = this->d->m_run;

    if (!isRunning)
        this->setState(AkElement::ElementStatePaused);

    int defaultStream = -1;

    for (int stream = 0; stream < this->d->m_streamInfo.size(); stream++)
        if (this->d->m_streamInfo[stream].caps.mimeType() == mimeType) {
            defaultStream = stream;

            break;
        }

    if (!isRunning)
        this->setState(AkElement::ElementStateNull);

    return defaultStream;
}

QString MediaSourceGStreamer::description(const QString &media) const
{
    if (this->d->m_media != media)
        return QString();

    return QFileInfo(media).baseName();
}

AkCaps MediaSourceGStreamer::caps(int stream)
{
    bool isRunning = this->d->m_run;

    if (!isRunning)
        this->setState(AkElement::ElementStatePaused);

    Stream streamInfo = this->d->m_streamInfo.value(stream, Stream());

    if (!isRunning)
        this->setState(AkElement::ElementStateNull);

    return streamInfo.caps;
}

qint64 MediaSourceGStreamer::maxPacketQueueSize() const
{
    return this->d->m_maxPacketQueueSize;
}

bool MediaSourceGStreamer::showLog() const
{
    return this->d->m_showLog;
}

void MediaSourceGStreamerPrivate::waitState(GstState state)
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

gboolean MediaSourceGStreamerPrivate::busCallback(GstBus *bus,
                                                  GstMessage *message,
                                                  gpointer userData)
{
    Q_UNUSED(bus)
    MediaSourceGStreamer *self = static_cast<MediaSourceGStreamer *>(userData);

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        gchar *name = gst_object_get_path_string(message->src);
        GError *err = nullptr;
        gchar *debug = nullptr;
        gst_message_parse_error(message, &err, &debug);

        qDebug() << "ERROR: from element"
                 << name
                 << ":"
                 << err->message;

        if (debug)
            qDebug() << "Additional debug info:\n"
                     << debug;

        g_error_free(err);
        g_free(debug);
        g_free(name);
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

GstFlowReturn MediaSourceGStreamerPrivate::audioBufferCallback(GstElement *audioOutput,
                                                               gpointer userData)
{
    MediaSourceGStreamer *self = static_cast<MediaSourceGStreamer *>(userData);

    if (self->d->m_audioIndex < 0)
        return GST_FLOW_OK;

    GstSample *sample = nullptr;
    g_signal_emit_by_name(audioOutput, "pull-sample", &sample);

    if (!sample)
        return GST_FLOW_OK;

    GstCaps *caps = gst_sample_get_caps(sample);
    GstAudioInfo *audioInfo = gst_audio_info_new();
    gst_audio_info_from_caps(audioInfo, caps);

    AkAudioPacket packet;
    packet.caps().isValid() = true;
    packet.caps().format() = AkAudioCaps::SampleFormat_flt;
    packet.caps().bps() = 8 * audioInfo->bpf / audioInfo->channels;
    packet.caps().channels() = audioInfo->channels;
    packet.caps().rate() = audioInfo->rate;
    packet.caps().layout() = AkAudioCaps::Layout_stereo;
    packet.caps().align() = false;

    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    QByteArray oBuffer(int(map.size), 0);
    memcpy(oBuffer.data(), map.data, map.size);

    packet.caps().samples() = gint(map.size) / audioInfo->bpf;
    gst_audio_info_free(audioInfo);

    packet.buffer() = oBuffer;
    packet.pts() = qint64(GST_BUFFER_PTS(buf));
    packet.timeBase() = AkFrac(1, GST_SECOND);
    packet.index() = int(self->d->m_audioIndex);
    packet.id() = self->d->m_audioId;

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);

    emit self->oStream(packet.toPacket());

    return GST_FLOW_OK;
}

GstFlowReturn MediaSourceGStreamerPrivate::videoBufferCallback(GstElement *videoOutput,
                                                               gpointer userData)
{
    auto self = static_cast<MediaSourceGStreamer *>(userData);

    if (self->d->m_videoIndex < 0)
        return GST_FLOW_OK;

    GstSample *sample = nullptr;
    g_signal_emit_by_name(videoOutput, "pull-sample", &sample);

    if (!sample)
        return GST_FLOW_OK;

    GstCaps *caps = gst_sample_get_caps(sample);
    GstVideoInfo *videoInfo = gst_video_info_new();
    gst_video_info_from_caps(videoInfo, caps);

    AkVideoPacket packet;
    packet.caps().isValid() = true;
    packet.caps().format() = AkVideoCaps::Format_rgb24;
    packet.caps().bpp() = AkVideoCaps::bitsPerPixel(packet.caps().format());
    packet.caps().width() = videoInfo->width;
    packet.caps().height() = videoInfo->height;
    packet.caps().fps() = AkFrac(videoInfo->fps_n, videoInfo->fps_d);

    gst_video_info_free(videoInfo);

    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    QByteArray oBuffer(int(map.size), 0);
    memcpy(oBuffer.data(), map.data, map.size);

    packet.buffer() = oBuffer;
    packet.pts() = qint64(GST_BUFFER_PTS(buf));
    packet.timeBase() = AkFrac(1, GST_SECOND);
    packet.index() = int(self->d->m_videoIndex);
    packet.id() = self->d->m_videoId;

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);

    emit self->oStream(packet.toPacket());

    return GST_FLOW_OK;
}

GstFlowReturn MediaSourceGStreamerPrivate::subtitlesBufferCallback(GstElement *subtitlesOutput,
                                                                   gpointer userData)
{
    MediaSourceGStreamer *self = static_cast<MediaSourceGStreamer *>(userData);

    if (self->d->m_subtitlesIndex < 0)
        return GST_FLOW_OK;

    GstSample *sample = nullptr;
    g_signal_emit_by_name(subtitlesOutput, "pull-sample", &sample);

    if (!sample)
        return GST_FLOW_OK;

    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *capsStructure = gst_caps_get_structure(caps, 0);
    const gchar *format = gst_structure_get_string(capsStructure, "format");

    AkPacket packet;
    packet.caps().isValid() = true;
    packet.caps().setMimeType("text/x-raw");
    packet.caps().setProperty("type", format);

    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    QByteArray oBuffer(int(map.size), 0);
    memcpy(oBuffer.data(), map.data, map.size);

    packet.buffer() = oBuffer;
    packet.pts() = qint64(GST_BUFFER_PTS(buf));
    packet.timeBase() = AkFrac(1, GST_SECOND);
    packet.index() = int(self->d->m_subtitlesIndex);
    packet.id() = self->d->m_subtitlesId;

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);

    emit self->oStream(packet);

    return GST_FLOW_OK;
}

void MediaSourceGStreamerPrivate::aboutToFinish(GstElement *object,
                                                gpointer userData)
{
    MediaSourceGStreamer *self = static_cast<MediaSourceGStreamer *>(userData);

    if (!self->d->m_loop)
        return;

    // Set the media file to play.
    if (gst_uri_is_valid(self->d->m_media.toStdString().c_str())) {
        g_object_set(G_OBJECT(object),
                     "uri",
                     self->d->m_media.toStdString().c_str(),
                     nullptr);
    } else {
        auto uri = gst_filename_to_uri(self->d->m_media.toStdString().c_str(),
                                       nullptr);
        g_object_set(G_OBJECT(object), "uri", uri, nullptr);
        g_free(uri);
    }

}

QStringList MediaSourceGStreamerPrivate::languageCodes(const QString &type)
{
    QStringList languages;

    int nStreams = 0;
    g_object_get(G_OBJECT(this->m_pipeline),
                 QString("n-%1").arg(type).toStdString().c_str(),
                 &nStreams,
                 nullptr);

    for (int stream = 0; stream < nStreams; stream++) {
        GstTagList *tags = nullptr;
        g_signal_emit_by_name(this->m_pipeline,
                              QString("get-%1-tags").arg(type).toStdString().c_str(),
                              stream,
                              &tags);

        if (!tags) {
            languages << QString();

            continue;
        }

        gchar *str = nullptr;

        if (gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &str)) {
          languages << QString(str);
          g_free(str);
        } else
            languages << QString();

        gst_tag_list_free(tags);
    }

    return languages;
}

QStringList MediaSourceGStreamerPrivate::languageCodes()
{
    QStringList languages;
    languages << languageCodes("audio");
    languages << languageCodes("video");
    languages << languageCodes("text");

    return languages;
}

void MediaSourceGStreamer::setMedia(const QString &media)
{
    if (media == this->d->m_media)
        return;

    bool isRunning = this->d->m_run;
    this->setState(AkElement::ElementStateNull);
    this->d->m_media = media;

    if (isRunning && !this->d->m_media.isEmpty())
        this->setState(AkElement::ElementStatePlaying);

    emit this->mediaChanged(media);
    emit this->mediasChanged(this->medias());
}

void MediaSourceGStreamer::setStreams(const QList<int> &streams)
{
    if (this->d->m_streams == streams)
        return;

    this->d->m_streams = streams;

    if (this->d->m_run)
        this->updateStreams();

    emit this->streamsChanged(streams);
}

void MediaSourceGStreamer::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->d->m_maxPacketQueueSize == maxPacketQueueSize)
        return;

    this->d->m_maxPacketQueueSize = maxPacketQueueSize;
    emit this->maxPacketQueueSizeChanged(maxPacketQueueSize);
}

void MediaSourceGStreamer::setShowLog(bool showLog)
{
    if (this->d->m_showLog == showLog)
        return;

    this->d->m_showLog = showLog;
    emit this->showLogChanged(showLog);
}

void MediaSourceGStreamer::setLoop(bool loop)
{
    if (this->d->m_loop == loop)
        return;

    this->d->m_loop = loop;
    emit this->loopChanged(loop);
}

void MediaSourceGStreamer::resetMedia()
{
    this->setMedia("");
}

void MediaSourceGStreamer::resetStreams()
{
    if  (this->d->m_streams.isEmpty())
        return;

    this->d->m_streams.clear();
    emit this->streamsChanged(this->d->m_streams);
}

void MediaSourceGStreamer::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void MediaSourceGStreamer::resetShowLog()
{
    this->setShowLog(false);
}

void MediaSourceGStreamer::resetLoop()
{
    this->setLoop(false);
}

bool MediaSourceGStreamer::setState(AkElement::ElementState state)
{
    switch (this->d->m_curState) {
    case AkElement::ElementStateNull: {
        if (state == AkElement::ElementStatePaused
            || state == AkElement::ElementStatePlaying) {
            // Create pipeline.
            this->d->m_pipeline =
                    gst_element_factory_make("playbin", "mediaBin");

            // Else, try to open it anyway.

            // Set the media file to play.
            if (gst_uri_is_valid(this->d->m_media.toStdString().c_str())) {
                g_object_set(G_OBJECT(this->d->m_pipeline),
                             "uri",
                             this->d->m_media.toStdString().c_str(),
                             nullptr);
            } else {
                auto uri = gst_filename_to_uri(this->d->m_media.toStdString().c_str(),
                                               nullptr);
                g_object_set(G_OBJECT(this->d->m_pipeline), "uri", uri, nullptr);
                g_free(uri);
            }

            g_object_set(G_OBJECT(this->d->m_pipeline),
                         "buffer-size", this->d->m_maxPacketQueueSize, nullptr);

            // Append the appsinks to grab audio and video frames, and subtitles.
            GstElement *audioOutput = gst_element_factory_make("appsink",
                                                               "audioOutput");
            g_object_set(G_OBJECT(this->d->m_pipeline), "audio-sink", audioOutput, nullptr);
            GstElement *videoOutput = gst_element_factory_make("appsink", "videoOutput");
            g_object_set(G_OBJECT(this->d->m_pipeline), "video-sink", videoOutput, nullptr);
            GstElement *subtitlesOutput = gst_element_factory_make("appsink",
                                                                   "subtitlesOutput");
            g_object_set(G_OBJECT(this->d->m_pipeline), "text-sink", subtitlesOutput, nullptr);

            // Emmit the signals when a frame is available.
            g_object_set(G_OBJECT(audioOutput), "emit-signals", TRUE, nullptr);
            g_object_set(G_OBJECT(videoOutput), "emit-signals", TRUE, nullptr);
            g_object_set(G_OBJECT(subtitlesOutput), "emit-signals", TRUE, nullptr);

            // Convert audio to a standard format.
            GstCaps *audioCaps =
                    gst_caps_new_simple("audio/x-raw",
                                        "format", G_TYPE_STRING, "F32LE",
                                        "channels", G_TYPE_INT, 2,
                                        "layout", G_TYPE_STRING, "interleaved",
                                        nullptr);

            gst_app_sink_set_caps(GST_APP_SINK(audioOutput), audioCaps);
            gst_caps_unref(audioCaps);

            // Convert video to a standard format.
            GstCaps *videoCaps =
                    gst_caps_new_simple("video/x-raw",
                                         "format", G_TYPE_STRING, "RGB",
                                         nullptr);

            gst_app_sink_set_caps(GST_APP_SINK(videoOutput), videoCaps);
            gst_caps_unref(videoCaps);

            // Connect signals
            g_signal_connect(this->d->m_pipeline,
                             "about-to-finish",
                             G_CALLBACK(this->d->aboutToFinish),
                             this);
            g_signal_connect(audioOutput,
                             "new-sample",
                             G_CALLBACK(this->d->audioBufferCallback),
                             this);
            g_signal_connect(videoOutput,
                             "new-sample",
                             G_CALLBACK(this->d->videoBufferCallback),
                             this);
            g_signal_connect(subtitlesOutput,
                             "new-sample",
                             G_CALLBACK(this->d->subtitlesBufferCallback),
                             this);

            // Configure the message bus.
            GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->d->m_pipeline));
            this->d->m_busWatchId = gst_bus_add_watch(bus, this->d->busCallback, this);
            gst_object_unref(bus);

            // Run the main GStreamer loop.
            this->d->m_mainLoop = g_main_loop_new(nullptr, FALSE);
            QtConcurrent::run(&this->d->m_threadPool,
                              g_main_loop_run,
                              this->d->m_mainLoop);
            GstState gstState = state == AkElement::ElementStatePaused?
                                 GST_STATE_PAUSED: GST_STATE_PLAYING;
            gst_element_set_state(this->d->m_pipeline, gstState);
            this->d->m_run = true;

            // Wait until paused/playing state is reached.
            this->d->waitState(gstState);

            // Read the number of tracks in the file.
            int audioTracks = 0;
            g_object_get(G_OBJECT(this->d->m_pipeline),
                         "n-audio",
                         &audioTracks,
                         nullptr);
            int videoTracks = 0;
            g_object_get(G_OBJECT(this->d->m_pipeline),
                         "n-video",
                         &videoTracks,
                         nullptr);
            int subtitlesTracks = 0;
            g_object_get(G_OBJECT(this->d->m_pipeline),
                         "n-text",
                         &subtitlesTracks,
                         nullptr);

            int totalTracks = audioTracks + videoTracks + subtitlesTracks;

            // Read info of all possible streams.
            GstPad *pad = nullptr;
            int curStream = -1;
            this->d->m_streamInfo.clear();

            auto languages = this->d->languageCodes();

            for (int stream = 0; stream < totalTracks; stream++) {
                if (stream < audioTracks) {
                    g_object_get(G_OBJECT(this->d->m_pipeline),
                                 "current-audio",
                                 &curStream,
                                 nullptr);

                    int streamId = stream;

                    g_object_set(G_OBJECT(this->d->m_pipeline),
                                 "current-audio",
                                 streamId,
                                 nullptr);

                    g_signal_emit_by_name(this->d->m_pipeline,
                                          "get-audio-pad",
                                          streamId,
                                          &pad);

                    if (pad) {
                        GstCaps *caps = gst_pad_get_current_caps(pad);
                        GstAudioInfo *audioInfo = gst_audio_info_new();
                        gst_audio_info_from_caps(audioInfo, caps);

                        AkAudioCaps audioCaps;
                        audioCaps.isValid() = true;
                        audioCaps.format() = AkAudioCaps::SampleFormat_flt;
                        audioCaps.bps() = 8 * audioInfo->bpf / audioInfo->channels;
                        audioCaps.channels() = audioInfo->channels;
                        audioCaps.rate() = audioInfo->rate;
                        audioCaps.layout() = AkAudioCaps::Layout_stereo;
                        audioCaps.align() = false;
                        this->d->m_streamInfo << Stream(audioCaps.toCaps(),
                                                        languages[stream]);

                        gst_audio_info_free(audioInfo);
                    }

                    g_object_set(G_OBJECT(this->d->m_pipeline),
                                 "current-audio",
                                 curStream,
                                 nullptr);
                } else if (stream < audioTracks + videoTracks) {
                    g_object_get(G_OBJECT(this->d->m_pipeline),
                                 "current-video",
                                 &curStream,
                                 nullptr);

                    int streamId = stream - audioTracks;

                    g_object_set(G_OBJECT(this->d->m_pipeline),
                                 "current-video",
                                 streamId,
                                 nullptr);

                    g_signal_emit_by_name(this->d->m_pipeline,
                                          "get-video-pad",
                                          streamId,
                                          &pad);

                    if (pad) {
                        GstCaps *caps = gst_pad_get_current_caps(pad);
                        GstVideoInfo *videoInfo = gst_video_info_new();
                        gst_video_info_from_caps(videoInfo, caps);

                        AkVideoCaps videoCaps;
                        videoCaps.isValid() = true;
                        videoCaps.format() = AkVideoCaps::Format_rgb24;
                        videoCaps.bpp() = AkVideoCaps::bitsPerPixel(videoCaps.format());
                        videoCaps.width() = videoInfo->width;
                        videoCaps.height() = videoInfo->height;
                        videoCaps.fps() = AkFrac(videoInfo->fps_n, videoInfo->fps_d);
                        this->d->m_streamInfo << Stream(videoCaps.toCaps(),
                                                        languages[stream]);

                        gst_video_info_free(videoInfo);
                    }

                    g_object_set(G_OBJECT(this->d->m_pipeline),
                                 "current-video",
                                 curStream,
                                 nullptr);
                } else {
                    g_object_get(G_OBJECT(this->d->m_pipeline),
                                 "current-text",
                                 &curStream,
                                 nullptr);

                    int streamId = stream - audioTracks - videoTracks;

                    g_object_set(G_OBJECT(this->d->m_pipeline),
                                 "current-text",
                                 streamId,
                                 nullptr);

                    g_signal_emit_by_name(this->d->m_pipeline,
                                          "get-text-pad",
                                          streamId,
                                          &pad);

                    if (pad) {
                        GstCaps *caps = gst_pad_get_current_caps(pad);
                        GstStructure *capsStructure = gst_caps_get_structure(caps, 0);
                        const gchar *format = gst_structure_get_string(capsStructure, "format");

                        AkCaps subtitlesCaps;
                        subtitlesCaps.isValid() = true;
                        subtitlesCaps.setMimeType("text/x-raw");
                        subtitlesCaps.setProperty("type", format);
                        this->d->m_streamInfo << Stream(subtitlesCaps,
                                                        languages[stream]);
                    }

                    g_object_set(G_OBJECT(this->d->m_pipeline),
                                 "current-text",
                                 curStream,
                                 nullptr);
                }
            }

            this->updateStreams();

            this->d->m_audioId = Ak::id();
            this->d->m_videoId = Ak::id();
            this->d->m_subtitlesId = Ak::id();
            this->d->m_curState = state;

            return true;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull: {
            this->d->m_run = false;

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

            this->d->m_curState = state;

            return true;
        }
        case AkElement::ElementStatePlaying: {
            gst_element_set_state(this->d->m_pipeline, GST_STATE_PLAYING);
            this->d->waitState(GST_STATE_PLAYING);
            this->d->m_curState = state;

            return true;
        }
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull: {
            this->d->m_run = false;

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

            this->d->m_curState = state;

            return true;
        }
        case AkElement::ElementStatePaused: {
            gst_element_set_state(this->d->m_pipeline, GST_STATE_PAUSED);
            this->d->waitState(GST_STATE_PAUSED);
            this->d->m_curState = state;

            return true;
        }
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

void MediaSourceGStreamer::updateStreams()
{
    // Read the number of tracks in the file.
    int audioTracks = 0;
    g_object_get(G_OBJECT(this->d->m_pipeline),
                 "n-audio",
                 &audioTracks,
                 nullptr);
    int videoTracks = 0;
    g_object_get(G_OBJECT(this->d->m_pipeline),
                 "n-video",
                 &videoTracks,
                 nullptr);
    int subtitlesTracks = 0;
    g_object_get(G_OBJECT(this->d->m_pipeline),
                 "n-text",
                 &subtitlesTracks,
                 nullptr);

    // Set default streams
    this->d->m_audioIndex = -1;
    this->d->m_videoIndex = -1;
    this->d->m_subtitlesIndex = -1;

    if (this->d->m_streams.isEmpty()) {
        if (audioTracks > 0) {
            this->d->m_audioIndex = 0;
            g_object_set(G_OBJECT(this->d->m_pipeline),
                         "current-audio",
                         0,
                         nullptr);
        }

        if (videoTracks > 0) {
            this->d->m_videoIndex = audioTracks;
            g_object_set(G_OBJECT(this->d->m_pipeline),
                         "current-video",
                         0,
                         nullptr);
        }

    } else {
        for (const int &stream: this->d->m_streams) {
            if (stream < audioTracks) {
                this->d->m_audioIndex = stream;

                g_object_set(G_OBJECT(this->d->m_pipeline),
                             "current-audio",
                             stream,
                             nullptr);
            } else if (stream < audioTracks + videoTracks) {
                this->d->m_videoIndex = stream;

                g_object_set(G_OBJECT(this->d->m_pipeline),
                             "current-video",
                             stream - audioTracks,
                             nullptr);
            } else {
                this->d->m_subtitlesIndex = stream;

                g_object_set(G_OBJECT(this->d->m_pipeline),
                             "current-text",
                             stream - audioTracks - videoTracks,
                             nullptr);
            }
        }
    }
}

#include "moc_mediasourcegstreamer.cpp"

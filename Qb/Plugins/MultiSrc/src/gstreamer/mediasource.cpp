/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#include <gst/app/gstappsink.h>
#include <gst/audio/audio.h>
#include <gst/video/video.h>

#include "mediasource.h"

MediaSource::MediaSource(QObject *parent): QObject(parent)
{
    gst_init(NULL, NULL);

    this->m_maxPacketQueueSize = 15 * 1024 * 1024;
    this->m_showLog = false;
    this->m_loop = false;
    this->m_run = false;

    this->m_pipeline = NULL;
    this->m_mainLoop = NULL;
    this->m_busWatchId = 0;
}

MediaSource::~MediaSource()
{
    this->uninit();
}

QStringList MediaSource::medias() const
{
    QStringList medias;

    if (!this->m_media.isEmpty())
        medias << this->m_media;

    return medias;
}

QString MediaSource::media() const
{
    return this->m_media;
}

QList<int> MediaSource::streams() const
{
    return this->m_streams;
}

QList<int> MediaSource::listTracks(const QString &mimeType)
{
    QList<int> tracks;

    bool isRunning = this->m_run;

    if (!isRunning)
        this->init(true);

    for (int stream = 0; stream < this->m_streamInfo.size(); stream++)
        if (mimeType.isEmpty()
            || this->m_streamInfo[stream].caps.mimeType() == mimeType)
            tracks << stream;

    if (!isRunning)
        this->uninit();

    return tracks;
}

QString MediaSource::streamLanguage(int stream)
{
    bool isRunning = this->m_run;

    if (!isRunning)
        this->init(true);

    Stream streamInfo = this->m_streamInfo.value(stream, Stream());

    if (!isRunning)
        this->uninit();

    return streamInfo.language;
}

bool MediaSource::loop() const
{
    return this->m_loop;
}

int MediaSource::defaultStream(const QString &mimeType)
{
    bool isRunning = this->m_run;

    if (!isRunning)
        this->init(true);

    int defaultStream = -1;

    for (int stream = 0; stream < this->m_streamInfo.size(); stream++)
        if (this->m_streamInfo[stream].caps.mimeType() == mimeType) {
            defaultStream = stream;

            break;
        }

    if (!isRunning)
        this->uninit();

    return defaultStream;
}

QString MediaSource::description(const QString &media) const
{
    if (this->m_media != media)
        return QString();

    return QFileInfo(media).baseName();
}

QbCaps MediaSource::caps(int stream)
{
    bool isRunning = this->m_run;

    if (!isRunning)
        this->init(true);

    Stream streamInfo = this->m_streamInfo.value(stream, Stream());

    if (!isRunning)
        this->uninit();

    return streamInfo.caps;
}

qint64 MediaSource::maxPacketQueueSize() const
{
    return this->m_maxPacketQueueSize;
}

bool MediaSource::showLog() const
{
    return this->m_showLog;
}

void MediaSource::waitState(GstState state)
{
    forever {
        GstState curState;

        if (gst_element_get_state(this->m_pipeline,
                                  &curState,
                                  NULL,
                                  GST_CLOCK_TIME_NONE) == GST_STATE_CHANGE_SUCCESS
            && curState == state)
            break;
    }
}

gboolean MediaSource::busCallback(GstBus *bus, GstMessage *message,
                                  gpointer userData)
{
    Q_UNUSED(bus)
    MediaSource *self = static_cast<MediaSource *>(userData);

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;
            gst_message_parse_error(message, &err, &debug);
            qDebug() << "Error: " << err->message;
            g_error_free(err);
            g_free(debug);
            g_main_loop_quit(self->m_mainLoop);

            break;
        }

        case GST_MESSAGE_EOS:
            g_main_loop_quit(self->m_mainLoop);
        break;

        default:
        break;
    }

    return TRUE;
}

GstFlowReturn MediaSource::audioBufferCallback(GstElement *audioOutput,
                                               gpointer userData)
{
    MediaSource *self = static_cast<MediaSource *>(userData);

    if (self->m_audioIndex < 0)
        return GST_FLOW_OK;

    GstSample *sample = NULL;
    g_signal_emit_by_name(audioOutput, "pull-sample", &sample);

    if (!sample)
        return GST_FLOW_OK;

    GstCaps *caps = gst_sample_get_caps(sample);
    GstAudioInfo *audioInfo = gst_audio_info_new();
    gst_audio_info_from_caps(audioInfo, caps);

    QbAudioPacket packet;
    packet.caps().isValid() = true;
    packet.caps().format() = QbAudioCaps::SampleFormat_flt;
    packet.caps().bps() = audioInfo->bpf / audioInfo->channels;
    packet.caps().channels() = audioInfo->channels;
    packet.caps().rate() = audioInfo->rate;
    packet.caps().layout() = QbAudioCaps::Layout_stereo;
    packet.caps().align() = false;

    gst_audio_info_free(audioInfo);

    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    QbBufferPtr oBuffer(new char[map.size]);
    memcpy(oBuffer.data(), map.data, map.size);

    packet.caps().samples() = map.size / audioInfo->bpf;

    packet.buffer() = oBuffer;
    packet.bufferSize() = map.size;
    packet.pts() = GST_BUFFER_PTS(buf);
    packet.timeBase() = QbFrac(1, 1e9);
    packet.index() = self->m_audioIndex;
    packet.id() = self->m_audioId;

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);

    emit self->oStream(packet.toPacket());

    return GST_FLOW_OK;
}

GstFlowReturn MediaSource::videoBufferCallback(GstElement *videoOutput,
                                               gpointer userData)
{
    MediaSource *self = static_cast<MediaSource *>(userData);

    if (self->m_videoIndex < 0)
        return GST_FLOW_OK;

    GstSample *sample = NULL;
    g_signal_emit_by_name(videoOutput, "pull-sample", &sample);

    if (!sample)
        return GST_FLOW_OK;

    GstCaps *caps = gst_sample_get_caps(sample);
    GstVideoInfo *videoInfo = gst_video_info_new();
    gst_video_info_from_caps(videoInfo, caps);

    QbVideoPacket packet;
    packet.caps().isValid() = true;
    packet.caps().format() = QbVideoCaps::Format_bgra;
    packet.caps().width() = videoInfo->width;
    packet.caps().height() = videoInfo->height;
    packet.caps().fps() = QbFrac(videoInfo->fps_n, videoInfo->fps_d);

    gst_video_info_free(videoInfo);

    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    QbBufferPtr oBuffer(new char[map.size]);
    memcpy(oBuffer.data(), map.data, map.size);

    packet.buffer() = oBuffer;
    packet.bufferSize() = map.size;
    packet.pts() = GST_BUFFER_PTS(buf);
    packet.timeBase() = QbFrac(1, 1e9);
    packet.index() = self->m_videoIndex;
    packet.id() = self->m_videoId;

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);

    emit self->oStream(packet.toPacket());

    return GST_FLOW_OK;
}

GstFlowReturn MediaSource::subtitlesBufferCallback(GstElement *subtitlesOutput,
                                                   gpointer userData)
{
    MediaSource *self = static_cast<MediaSource *>(userData);

    if (self->m_subtitlesIndex < 0)
        return GST_FLOW_OK;

    GstSample *sample = NULL;
    g_signal_emit_by_name(subtitlesOutput, "pull-sample", &sample);

    if (!sample)
        return GST_FLOW_OK;

    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *capsStructure = gst_caps_get_structure(caps, 0);
    const gchar *format = gst_structure_get_string(capsStructure, "format");

    QbPacket packet;
    packet.caps().isValid() = true;
    packet.caps().setMimeType("text/x-raw");
    packet.caps().setProperty("type", format);

    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    QbBufferPtr oBuffer(new char[map.size]);
    memcpy(oBuffer.data(), map.data, map.size);

    packet.buffer() = oBuffer;
    packet.bufferSize() = map.size;
    packet.pts() = GST_BUFFER_PTS(buf);
    packet.timeBase() = QbFrac(1, 1e9);
    packet.index() = self->m_subtitlesIndex;
    packet.id() = self->m_subtitlesId;

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);

    emit self->oStream(packet);

    return GST_FLOW_OK;
}

void MediaSource::aboutToFinish(GstElement *object, gpointer userData)
{
    MediaSource *self = static_cast<MediaSource *>(userData);

    if (!self->m_loop)
        return;

    QString uri = self->m_media;

    // Check if it's a file, and it hasn't the URI specifier, append it.
    if (QFile::exists(uri) && !uri.startsWith("file://"))
        uri = uri.prepend("file://");

    // Set the media file to play.
    g_object_set(G_OBJECT(object),
                 "uri", uri.toStdString().c_str(), NULL);
}

QStringList MediaSource::languageCodes(const QString &type)
{
    QStringList languages;

    int nStreams = 0;
    g_object_get(G_OBJECT(this->m_pipeline),
                 QString("n-%1").arg(type).toStdString().c_str(),
                 &nStreams,
                 NULL);

    for (int stream = 0; stream < nStreams; stream++) {
        GstTagList *tags = NULL;
        g_signal_emit_by_name(this->m_pipeline,
                              QString("get-%1-tags").arg(type).toStdString().c_str(),
                              stream,
                              &tags);

        if (!tags) {
            languages << QString();

            continue;
        }

        gchar *str = NULL;

        if (gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &str)) {
          languages << QString(str);
          g_free(str);
        } else
            languages << QString();

        gst_tag_list_free(tags);
    }

    return languages;
}

QStringList MediaSource::languageCodes()
{
    QStringList languages;
    languages << languageCodes("audio");
    languages << languageCodes("video");
    languages << languageCodes("text");

    return languages;
}

void MediaSource::setMedia(const QString &media)
{
    if (media == this->m_media)
        return;

    bool isRunning = this->m_run;
    this->uninit();
    this->m_media = media;

    if (isRunning && !this->m_media.isEmpty())
        this->init();

    emit this->mediaChanged(media);
    emit this->mediasChanged(this->medias());
}

void MediaSource::setStreams(const QList<int> &streams)
{
    if (this->m_streams == streams)
        return;

    this->m_streams = streams;

    if (this->m_run)
        this->updateStreams();

    emit this->streamsChanged(streams);
}

void MediaSource::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->m_maxPacketQueueSize == maxPacketQueueSize)
        return;

    this->m_maxPacketQueueSize = maxPacketQueueSize;
    emit this->maxPacketQueueSizeChanged(maxPacketQueueSize);
}

void MediaSource::setShowLog(bool showLog)
{
    if (this->m_showLog == showLog)
        return;

    this->m_showLog = showLog;
    emit this->showLogChanged(showLog);
}

void MediaSource::setLoop(bool loop)
{
    if (this->m_loop == loop)
        return;

    this->m_loop = loop;
    emit this->loopChanged(loop);
}

void MediaSource::resetMedia()
{
    this->setMedia("");
}

void MediaSource::resetStreams()
{
    if  (this->m_streams.isEmpty())
        return;

    this->m_streams.clear();
    emit this->streamsChanged(this->m_streams);
}

void MediaSource::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void MediaSource::resetShowLog()
{
    this->setShowLog(false);
}

void MediaSource::resetLoop()
{
    this->setLoop(false);
}

bool MediaSource::init(bool paused)
{
    // Create pipeline.
    this->m_pipeline = gst_element_factory_make("playbin", "mediaBin");

    QString uri = this->m_media;

    // Check if it's a file, and it hasn't the URI specifier, append it.
    if (QFile::exists(uri) && !uri.startsWith("file://"))
        uri = uri.prepend("file://");

    // Else, try to open it anyway.

    // Set the media file to play.
    g_object_set(G_OBJECT(this->m_pipeline),
                 "uri", uri.toStdString().c_str(), NULL);
    g_object_set(G_OBJECT(this->m_pipeline),
                 "buffer-size", this->m_maxPacketQueueSize, NULL);

    // Append the appsinks to grab audio and video frames, and subtitles.
    GstElement *audioOutput = gst_element_factory_make("appsink",
                                                       "audioOutput");
    g_object_set(G_OBJECT(this->m_pipeline), "audio-sink", audioOutput, NULL);
    GstElement *videoOutput = gst_element_factory_make("appsink", "videoOutput");
    g_object_set(G_OBJECT(this->m_pipeline), "video-sink", videoOutput, NULL);
    GstElement *subtitlesOutput = gst_element_factory_make("appsink",
                                                           "subtitlesOutput");
    g_object_set(G_OBJECT(this->m_pipeline), "text-sink", subtitlesOutput, NULL);

    // Emmit the signals when a frame is available.
    g_object_set(G_OBJECT(audioOutput), "emit-signals", TRUE, NULL);
    g_object_set(G_OBJECT(videoOutput), "emit-signals", TRUE, NULL);
    g_object_set(G_OBJECT(subtitlesOutput), "emit-signals", TRUE, NULL);

    // Convert audio to a standard format.
    GstCaps *audioCaps =
            gst_caps_new_simple("audio/x-raw",
                                "format", G_TYPE_STRING, "F32LE",
                                "channels", G_TYPE_INT, 2,
                                "layout", G_TYPE_STRING, "interleaved",
                                NULL);

    gst_app_sink_set_caps(GST_APP_SINK(audioOutput), audioCaps);
    gst_caps_unref(audioCaps);

    // Convert video to a standard format.
    GstCaps *videoCaps =
            gst_caps_new_simple("video/x-raw",
                                 "format", G_TYPE_STRING, "BGRA",
                                 NULL);

    gst_app_sink_set_caps(GST_APP_SINK(videoOutput), videoCaps);
    gst_caps_unref(videoCaps);

    // Connect signals
    g_signal_connect(this->m_pipeline,
                     "about-to-finish",
                     G_CALLBACK(this->aboutToFinish),
                     this);
    g_signal_connect(audioOutput,
                     "new-sample",
                     G_CALLBACK(this->audioBufferCallback),
                     this);
    g_signal_connect(videoOutput,
                     "new-sample",
                     G_CALLBACK(this->videoBufferCallback),
                     this);
    g_signal_connect(subtitlesOutput,
                     "new-sample",
                     G_CALLBACK(this->subtitlesBufferCallback),
                     this);

    // Configure the message bus.
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->m_pipeline));
    this->m_busWatchId = gst_bus_add_watch(bus, this->busCallback, this);
    gst_object_unref(bus);

    // Run the main GStreamer loop.
    this->m_mainLoop = g_main_loop_new(NULL, FALSE);
    QtConcurrent::run(&this->m_threadPool, g_main_loop_run, this->m_mainLoop);
    GstState state = paused? GST_STATE_PAUSED: GST_STATE_PLAYING;
    gst_element_set_state(this->m_pipeline, state);
    this->m_run = true;

    // Wait until paused/playing state is reached.
    this->waitState(state);

    // Read the number of tracks in the file.
    int audioTracks = 0;
    g_object_get(G_OBJECT(this->m_pipeline), "n-audio", &audioTracks, NULL);
    int videoTracks = 0;
    g_object_get(G_OBJECT(this->m_pipeline), "n-video", &videoTracks, NULL);
    int subtitlesTracks = 0;
    g_object_get(G_OBJECT(this->m_pipeline), "n-text", &subtitlesTracks, NULL);

    int totalTracks = audioTracks + videoTracks + subtitlesTracks;

    // Read info of all possible streams.
    GstPad *pad = NULL;
    int curStream = -1;
    this->m_streamInfo.clear();

    QStringList languages = this->languageCodes();

    for (int stream = 0; stream < totalTracks; stream++) {
        if (stream < audioTracks) {
            g_object_get(G_OBJECT(this->m_pipeline),
                         "current-audio",
                         &curStream,
                         NULL);

            int streamId = stream;

            g_object_set(G_OBJECT(this->m_pipeline),
                         "current-audio",
                         streamId,
                         NULL);

            g_signal_emit_by_name(this->m_pipeline, "get-audio-pad", streamId, &pad);

            if (pad) {
                GstCaps *caps = gst_pad_get_current_caps(pad);
                GstAudioInfo *audioInfo = gst_audio_info_new();
                gst_audio_info_from_caps(audioInfo, caps);

                QbAudioCaps audioCaps;
                audioCaps.isValid() = true;
                audioCaps.format() = QbAudioCaps::SampleFormat_flt;
                audioCaps.bps() = audioInfo->bpf / audioInfo->channels;
                audioCaps.channels() = audioInfo->channels;
                audioCaps.rate() = audioInfo->rate;
                audioCaps.layout() = QbAudioCaps::Layout_stereo;
                audioCaps.align() = false;
                this->m_streamInfo << Stream(audioCaps.toCaps(),
                                             languages[stream]);

                gst_audio_info_free(audioInfo);
            }

            g_object_set(G_OBJECT(this->m_pipeline),
                         "current-audio",
                         curStream,
                         NULL);
        } else if (stream < audioTracks + videoTracks) {
            g_object_get(G_OBJECT(this->m_pipeline),
                         "current-video",
                         &curStream,
                         NULL);

            int streamId = stream - audioTracks;

            g_object_set(G_OBJECT(this->m_pipeline),
                         "current-video",
                         streamId,
                         NULL);

            g_signal_emit_by_name(this->m_pipeline, "get-video-pad", streamId, &pad);

            if (pad) {
                GstCaps *caps = gst_pad_get_current_caps(pad);
                GstVideoInfo *videoInfo = gst_video_info_new();
                gst_video_info_from_caps(videoInfo, caps);

                QbVideoCaps videoCaps;
                videoCaps.isValid() = true;
                videoCaps.format() = QbVideoCaps::Format_bgra;
                videoCaps.width() = videoInfo->width;
                videoCaps.height() = videoInfo->height;
                videoCaps.fps() = QbFrac(videoInfo->fps_n, videoInfo->fps_d);
                this->m_streamInfo << Stream(videoCaps.toCaps(),
                                             languages[stream]);

                gst_video_info_free(videoInfo);
            }

            g_object_set(G_OBJECT(this->m_pipeline),
                         "current-video",
                         curStream,
                         NULL);
        } else {
            g_object_get(G_OBJECT(this->m_pipeline),
                         "current-text",
                         &curStream,
                         NULL);

            int streamId = stream - audioTracks - videoTracks;

            g_object_set(G_OBJECT(this->m_pipeline),
                         "current-text",
                         streamId,
                         NULL);

            g_signal_emit_by_name(this->m_pipeline, "get-text-pad", streamId, &pad);

            if (pad) {
                GstCaps *caps = gst_pad_get_current_caps(pad);
                GstStructure *capsStructure = gst_caps_get_structure(caps, 0);
                const gchar *format = gst_structure_get_string(capsStructure, "format");

                QbCaps subtitlesCaps;
                subtitlesCaps.isValid() = true;
                subtitlesCaps.setMimeType("text/x-raw");
                subtitlesCaps.setProperty("type", format);
                this->m_streamInfo << Stream(subtitlesCaps,
                                             languages[stream]);
            }

            g_object_set(G_OBJECT(this->m_pipeline),
                         "current-text",
                         curStream,
                         NULL);
        }
    }

    this->updateStreams();

    this->m_audioId = Qb::id();
    this->m_videoId = Qb::id();
    this->m_subtitlesId = Qb::id();

    return true;
}

void MediaSource::uninit()
{
    this->m_run = false;

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

void MediaSource::updateStreams()
{
    // Read the number of tracks in the file.
    int audioTracks = 0;
    g_object_get(G_OBJECT(this->m_pipeline), "n-audio", &audioTracks, NULL);
    int videoTracks = 0;
    g_object_get(G_OBJECT(this->m_pipeline), "n-video", &videoTracks, NULL);
    int subtitlesTracks = 0;
    g_object_get(G_OBJECT(this->m_pipeline), "n-text", &subtitlesTracks, NULL);

    // Set default streams
    this->m_audioIndex = -1;
    this->m_videoIndex = -1;
    this->m_subtitlesIndex = -1;

    if (this->m_streams.isEmpty()) {
        if (audioTracks > 0) {
            this->m_audioIndex = 0;
            g_object_set(G_OBJECT(this->m_pipeline), "current-audio", 0, NULL);
        }

        if (videoTracks > 0) {
            this->m_videoIndex = audioTracks;
            g_object_set(G_OBJECT(this->m_pipeline), "current-video", 0, NULL);
        }

    } else
        foreach (int stream, this->m_streams) {
            if (stream < audioTracks) {
                this->m_audioIndex = stream;

                g_object_set(G_OBJECT(this->m_pipeline),
                             "current-audio",
                             stream,
                             NULL);
            } else if (stream < audioTracks + videoTracks) {
                this->m_videoIndex = stream;

                g_object_set(G_OBJECT(this->m_pipeline),
                             "current-video",
                             stream - audioTracks,
                             NULL);
            } else {
                this->m_subtitlesIndex = stream;

                g_object_set(G_OBJECT(this->m_pipeline),
                             "current-text",
                             stream - audioTracks - videoTracks,
                             NULL);
            }
        }
}

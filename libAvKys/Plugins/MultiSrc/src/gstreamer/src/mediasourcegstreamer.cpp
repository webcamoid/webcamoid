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

#include <QDebug>
#include <QFileInfo>
#include <QtConcurrent>
#include <QThreadPool>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/audio/audio.h>
#include <gst/video/video.h>
#include <ak.h>
#include <akaudiocaps.h>
#include <akaudiopacket.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <aksubtitlecaps.h>
#include <aksubtitlepacket.h>
#include <akvideocaps.h>
#include <akvideopacket.h>

#include "mediasourcegstreamer.h"

class Stream
{
    public:
        AkCaps caps;
        QString language;

        Stream()
        {
        }

        Stream(const AkCaps &caps,
               const QString &language):
            caps(caps),
            language(language)
        {
        }
};

class MediaSourceGStreamerPrivate
{
    public:
        QString m_media;
        QList<int> m_streams;
        QThreadPool m_threadPool;
        GstElement *m_pipeline {nullptr};
        GMainLoop *m_mainLoop {nullptr};
        QFuture<void> m_mainLoopResult;
        qint64 m_audioIndex {-1};
        qint64 m_videoIndex {-1};
        qint64 m_subtitlesIndex {-1};
        qint64 m_audioId {-1};
        qint64 m_videoId {-1};
        qint64 m_subtitlesId {-1};
        QList<Stream> m_streamInfo;
        qint64 m_maxPacketQueueSize {15 * 1024 * 1024};
        guint m_busWatchId {0};
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        bool m_loop {false};
        bool m_sync {true};
        bool m_run {false};
        bool m_showLog {false};

        void waitState(GstState state);
        static gboolean busCallback(GstBus *bus,
                                    GstMessage *message,
                                    gpointer userData);
        static GstFlowReturn audioBufferCallback(GstElement *audioOutput,
                                                 gpointer userData);
        static GstFlowReturn videoBufferCallback(GstElement *videoOutput,
                                                 gpointer userData);
        static GstFlowReturn subtitlesBufferCallback(GstElement *subtitlesOutput,
                                                     gpointer userData);
        static void aboutToFinish(GstElement *object, gpointer userData);
        QStringList languageCodes(AkCaps::CapsType type);
        QStringList languageCodes();
};

MediaSourceGStreamer::MediaSourceGStreamer(QObject *parent):
    MediaSource(parent)
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

QList<int> MediaSourceGStreamer::listTracks(AkCaps::CapsType type)
{
    bool isRunning = this->d->m_run;

    if (!isRunning)
        this->setState(AkElement::ElementStatePaused);

    QList<int> tracks;
    int i = 0;

    for (auto &streamInfo: this->d->m_streamInfo) {
        if (type == AkCaps::CapsUnknown || streamInfo.caps.type() == type)
            tracks << i;

        i++;
    }

    if (!isRunning)
        this->setState(AkElement::ElementStateNull);

    return tracks;
}

QString MediaSourceGStreamer::streamLanguage(int stream)
{
    bool isRunning = this->d->m_run;

    if (!isRunning)
        this->setState(AkElement::ElementStatePaused);

    auto streamInfo = this->d->m_streamInfo.value(stream, Stream());

    if (!isRunning)
        this->setState(AkElement::ElementStateNull);

    return streamInfo.language;
}

bool MediaSourceGStreamer::loop() const
{
    return this->d->m_loop;
}

bool MediaSourceGStreamer::sync() const
{
    return this->d->m_sync;
}

int MediaSourceGStreamer::defaultStream(AkCaps::CapsType type)
{
    bool isRunning = this->d->m_run;

    if (!isRunning)
        this->setState(AkElement::ElementStatePaused);

    int defaultStream = -1;
    int i = 0;

    for (auto &streamInfo: this->d->m_streamInfo) {
        if (streamInfo.caps.type() == type) {
            defaultStream = i;

            break;
        }

        i++;
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

qint64 MediaSourceGStreamer::durationMSecs()
{
    bool isStopped = this->d->m_state == AkElement::ElementStateNull;

    if (isStopped)
        this->setState(AkElement::ElementStatePaused);

    gint64 duration = 0;
    gst_element_query_duration(this->d->m_pipeline, GST_FORMAT_TIME, &duration);

    if (isStopped)
        this->setState(AkElement::ElementStateNull);

    return duration / 1000000;
}

qint64 MediaSourceGStreamer::currentTimeMSecs()
{
    if (this->d->m_state == AkElement::ElementStateNull)
        return 0;

    gint64 position = 0;
    gst_element_query_position(this->d->m_pipeline, GST_FORMAT_TIME, &position);

    return position / 1000000;
}

qint64 MediaSourceGStreamer::maxPacketQueueSize() const
{
    return this->d->m_maxPacketQueueSize;
}

bool MediaSourceGStreamer::showLog() const
{
    return this->d->m_showLog;
}

AkElement::ElementState MediaSourceGStreamer::state() const
{
    return this->d->m_state;
}

void MediaSourceGStreamer::seek(qint64 mSecs,
                                SeekPosition position)
{
    if (this->d->m_state == AkElement::ElementStateNull)
        return;

    int64_t pts = mSecs;

    switch (position) {
    case SeekCur:
        pts += this->currentTimeMSecs();

        break;

    case SeekEnd:
        pts += this->durationMSecs();

        break;

    default:
        break;
    }

    pts = qBound<qint64>(0, pts, this->durationMSecs()) * 1000000;
    gst_element_seek_simple(this->d->m_pipeline,
                            GST_FORMAT_TIME,
                            GstSeekFlags(GST_SEEK_FLAG_FLUSH
                                         | GST_SEEK_FLAG_KEY_UNIT
                                         | GST_SEEK_FLAG_SNAP_NEAREST),
                            pts);
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
    emit this->durationMSecsChanged(this->durationMSecs());
    emit this->mediaLoaded(media);
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

void MediaSourceGStreamer::setSync(bool sync)
{
    if (this->d->m_sync == sync)
        return;

    this->d->m_sync = sync;
    emit this->syncChanged(sync);
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

void MediaSourceGStreamer::resetSync()
{
    this->setSync(true);
}

bool MediaSourceGStreamer::setState(AkElement::ElementState state)
{
    switch (this->d->m_state) {
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

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            const char gstFormat[] = "S32LE";
#else
            const char gstFormat[] = "S32BE";
#endif

            // Convert audio to a standard format.
            GstCaps *audioCaps =
                    gst_caps_new_simple("audio/x-raw",
                                        "format", G_TYPE_STRING, gstFormat,
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

            // Synchronize streams?
            g_object_set(G_OBJECT(audioOutput),
                         "sync",
                         this->d->m_sync? TRUE: FALSE,
                         nullptr);
            g_object_set(G_OBJECT(videoOutput),
                         "sync",
                         this->d->m_sync? TRUE: FALSE,
                         nullptr);
            g_object_set(G_OBJECT(subtitlesOutput),
                         "sync",
                         this->d->m_sync? TRUE: FALSE,
                         nullptr);

            // Configure the message bus.
            GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->d->m_pipeline));
            this->d->m_busWatchId = gst_bus_add_watch(bus, this->d->busCallback, this);
            gst_object_unref(bus);

            // Run the main GStreamer loop.
            this->d->m_mainLoop = g_main_loop_new(nullptr, FALSE);
            this->d->m_mainLoopResult =
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

                        AkAudioCaps audioCaps(AkAudioCaps::SampleFormat_s32,
                                              AkAudioCaps::Layout_stereo,
                                              false,
                                              audioInfo->rate);
                        this->d->m_streamInfo << Stream(audioCaps,
                                                        languages.value(stream));

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
                        if (auto caps = gst_pad_get_current_caps(pad)) {
                            auto videoInfo = gst_video_info_new();
                            gst_video_info_from_caps(videoInfo, caps);

                            AkVideoCaps videoCaps(AkVideoCaps::Format_rgb24,
                                                  videoInfo->width,
                                                  videoInfo->height,
                                                  AkFrac(videoInfo->fps_n,
                                                         videoInfo->fps_d));
                            this->d->m_streamInfo << Stream(videoCaps,
                                                            languages.value(stream));

                            gst_video_info_free(videoInfo);
                        }
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
                        AkSubtitleCaps subtitlesCaps(AkSubtitleCaps::SubtitleFormat_text);
                        this->d->m_streamInfo << Stream(subtitlesCaps,
                                                        languages.value(stream));
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
            this->d->m_state = state;
            emit this->stateChanged(state);

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

            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }
        case AkElement::ElementStatePlaying: {
            gst_element_set_state(this->d->m_pipeline, GST_STATE_PLAYING);
            this->d->waitState(GST_STATE_PLAYING);
            this->d->m_state = state;
            emit this->stateChanged(state);

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

            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }
        case AkElement::ElementStatePaused: {
            gst_element_set_state(this->d->m_pipeline, GST_STATE_PAUSED);
            this->d->waitState(GST_STATE_PAUSED);
            this->d->m_state = state;
            emit this->stateChanged(state);

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
        for (auto &stream: this->d->m_streams) {
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
    auto self = static_cast<MediaSourceGStreamer *>(userData);

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

GstFlowReturn MediaSourceGStreamerPrivate::audioBufferCallback(GstElement *audioOutput,
                                                               gpointer userData)
{
    auto self = static_cast<MediaSourceGStreamer *>(userData);

    if (self->d->m_audioIndex < 0)
        return GST_FLOW_OK;

    GstSample *sample = nullptr;
    g_signal_emit_by_name(audioOutput, "pull-sample", &sample);

    if (!sample)
        return GST_FLOW_OK;

    auto caps = gst_sample_get_caps(sample);
    auto audioInfo = gst_audio_info_new();
    gst_audio_info_from_caps(audioInfo, caps);

    auto buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);
    gint samples = gint(map.size) / audioInfo->bpf;

    AkAudioCaps audioCaps(AkAudioCaps::SampleFormat_s32,
                          AkAudioCaps::Layout_stereo,
                          false,
                          audioInfo->rate);
    AkAudioPacket packet(audioCaps, samples);
    memcpy(packet.data(),
           map.data,
           qMin<size_t>(packet.size(), map.size));
    packet.setPts(qint64(GST_BUFFER_PTS(buf)));
    packet.setTimeBase({1, GST_SECOND});
    packet.setIndex(int(self->d->m_audioIndex));
    packet.setId(self->d->m_audioId);

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);
    gst_audio_info_free(audioInfo);

    emit self->oStream(packet);

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

    auto caps = gst_sample_get_caps(sample);
    auto videoInfo = gst_video_info_new();
    gst_video_info_from_caps(videoInfo, caps);

    // Create a package and return it.
    AkVideoCaps videoCaps(AkVideoCaps::Format_rgb24,
                          videoInfo->width,
                          videoInfo->height,
                          AkFrac(videoInfo->fps_n, videoInfo->fps_d));
    AkVideoPacket packet(videoCaps);
    auto buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    for (int plane = 0; plane < GST_VIDEO_INFO_N_PLANES(videoInfo); ++plane) {
        auto planeData = map.data + GST_VIDEO_INFO_PLANE_OFFSET(videoInfo, plane);
        auto oLineSize = GST_VIDEO_INFO_PLANE_STRIDE(videoInfo, plane);
        auto lineSize = qMin<size_t>(packet.lineSize(plane), oLineSize);
        auto heightDiv = packet.heightDiv(plane);

        for (int y = 0; y < videoInfo->height; ++y) {
            auto ys = y >> heightDiv;
            memcpy(packet.line(plane, y),
                   planeData + ys * oLineSize,
                   lineSize);
        }
    }

    packet.setPts(qint64(GST_BUFFER_PTS(buf)));
    packet.setTimeBase({1, GST_SECOND});
    packet.setIndex(int(self->d->m_videoIndex));
    packet.setId(self->d->m_videoId);

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);
    gst_video_info_free(videoInfo);

    emit self->oStream(packet);

    return GST_FLOW_OK;
}

GstFlowReturn MediaSourceGStreamerPrivate::subtitlesBufferCallback(GstElement *subtitlesOutput,
                                                                   gpointer userData)
{
    auto self = static_cast<MediaSourceGStreamer *>(userData);

    if (self->d->m_subtitlesIndex < 0)
        return GST_FLOW_OK;

    GstSample *sample = nullptr;
    g_signal_emit_by_name(subtitlesOutput, "pull-sample", &sample);

    if (!sample)
        return GST_FLOW_OK;

    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    AkSubtitleCaps oCaps(AkSubtitleCaps::SubtitleFormat_text);
    AkSubtitlePacket packet(oCaps, map.size);
    memcpy(packet.data(), map.data, map.size);
    packet.setPts(qint64(GST_BUFFER_PTS(buf)));
    packet.setTimeBase({1, GST_SECOND});
    packet.setIndex(int(self->d->m_subtitlesIndex));
    packet.setId(self->d->m_subtitlesId);

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);

    emit self->oStream(packet);

    return GST_FLOW_OK;
}

void MediaSourceGStreamerPrivate::aboutToFinish(GstElement *object,
                                                gpointer userData)
{
    auto self = static_cast<MediaSourceGStreamer *>(userData);

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

QStringList MediaSourceGStreamerPrivate::languageCodes(AkCaps::CapsType type)
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
    languages << languageCodes(AkCaps::CapsAudio);
    languages << languageCodes(AkCaps::CapsVideo);
    languages << languageCodes(AkCaps::CapsSubtitle);

    return languages;
}

#include "moc_mediasourcegstreamer.cpp"

/* Webcamoid, webcam capture application.
 * Copyright (C) 2021  Gonzalo Exequiel Pedone
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

#include <QFileInfo>
#include <QMutex>
#include <QThreadPool>
#include <QWaitCondition>
#include <QtConcurrent>
#include <vlc/vlc.h>
#include <ak.h>
#include <akaudiocaps.h>
#include <akaudiopacket.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <aksubtitlecaps.h>
#include <akvideocaps.h>
#include <akvideopacket.h>

#include "mediasourcevlc.h"

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

class MediaSourceVLCPrivate
{
    public:
        MediaSourceVLC *self;
        QString m_media;
        QList<int> m_streams;
        QThreadPool m_threadPool;
        QList<Stream> m_streamInfo;
        qint64 m_maxPacketQueueSize {15 * 1024 * 1024};
        libvlc_instance_t *m_vlcInstance {nullptr};
        libvlc_media_player_t *m_mediaPlayer {nullptr};
        QMutex m_mutex;
        QWaitCondition m_mediaParsed;
        AkAudioCaps m_audioCaps;
        AkVideoPacket m_videoFrame;
        AkFrac m_fps;
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        qint64 m_pts {-1};
        qint64 m_duration {0};
        qint64 m_audioId {-1};
        qint64 m_videoId {-1};
        qint64 m_subtitlesId {-1};
        qint64 m_audioIndex {-1};
        qint64 m_videoIndex {-1};
        bool m_loop {false};
        bool m_sync {true};
        bool m_showLog {false};

        explicit MediaSourceVLCPrivate(MediaSourceVLC *self);
        void doLoop();
        static void mediaParsedChangedCallback(const struct libvlc_event_t *event,
                                               void *userData);
        static void mediaPlayerEndReachedCallback(const struct libvlc_event_t *event,
                                                  void *userData);
        static void mediaPlayerTimeChanged(const struct libvlc_event_t *event,
                                           void *userData);
        static void *videoLockCallback(void *userData, void **planes);
        static void videoDisplayCallback(void *userData, void *picture);
        static void audioPlayCallback(void *userData,
                                      const void *samples,
                                      unsigned count,
                                      int64_t pts);
        static unsigned videoFormatCallback(void **userData,
                                            char *chroma,
                                            unsigned *width,
                                            unsigned *height,
                                            unsigned *pitches,
                                            unsigned *lines);
        static int audioSetupCallback(void **userData,
                                      char *format,
                                      unsigned *rate,
                                      unsigned *channels);
};

MediaSourceVLC::MediaSourceVLC(QObject *parent):
    MediaSource(parent)
{
    this->d = new MediaSourceVLCPrivate(this);

    if (this->d->m_threadPool.maxThreadCount() < 4)
        this->d->m_threadPool.setMaxThreadCount(4);

    //qputenv("VLC_VERBOSE", "3");

    auto binDir = QDir(BINDIR).absolutePath();
    auto vlcPluginsDir = QDir(VLC_PLUGINS_PATH).absolutePath();
    auto relVlcPluginsDir = QDir(binDir).relativeFilePath(vlcPluginsDir);
    QDir appDir = QCoreApplication::applicationDirPath();

    if (appDir.cd(relVlcPluginsDir)) {
        auto path = appDir.absolutePath();
        path.replace("/", QDir::separator());

        if (QFileInfo::exists(path + "/plugins.dat")) {
            if (qEnvironmentVariableIsEmpty("VLC_PLUGIN_PATH"))
                qputenv("VLC_PLUGIN_PATH", path.toLocal8Bit());
        } else {
#ifdef Q_OS_OSX
            QString path;
            QString vlcDir;
            QString vlcInstallPath = VLC_INSTALL_PATH;

            // Check user defined VLC path first.

            if (!vlcInstallPath.isEmpty()
                && QFileInfo::exists(vlcInstallPath))
                vlcDir = vlcInstallPath;

            // Check VLC installed by homebrew.

            if (vlcDir.isEmpty()) {
                QString vlcBin = "/usr/local/bin/vlc";

                if (QFileInfo::exists(vlcBin)) {
                    auto vlcBinPath = QFileInfo(vlcBin).canonicalFilePath();
                    auto vlcInstallDir = QFileInfo(vlcBinPath).canonicalPath()
                                         + "/VLC.app";

                    if (QFileInfo::exists(vlcInstallDir))
                        vlcDir = vlcInstallDir;
                }
            }

            // Check VLC manually installed by the user.

            if (vlcDir.isEmpty()) {
                QString vlcInstallDir = "/Applications/VLC.app";

                if (QFileInfo::exists(vlcInstallDir))
                    vlcDir = vlcInstallDir;
            }

            if (!vlcDir.isEmpty()) {
                auto vlcPluginsDirs = vlcDir + "/Contents/MacOS/plugins";

                if (QFileInfo::exists(vlcPluginsDirs))
                    path = vlcPluginsDirs;
            }

            if (!path.isEmpty()
                && QFileInfo::exists(path + "/plugins.dat")
                && qEnvironmentVariableIsEmpty("VLC_PLUGIN_PATH")) {
                qputenv("VLC_PLUGIN_PATH", path.toLocal8Bit());
            }
#endif
        }
    }

    this->d->m_vlcInstance = libvlc_new(0, nullptr);

    if (this->d->m_vlcInstance) {
        this->d->m_mediaPlayer = libvlc_media_player_new(this->d->m_vlcInstance);
        libvlc_event_attach(libvlc_media_player_event_manager(this->d->m_mediaPlayer),
                            libvlc_MediaPlayerEndReached,
                            &MediaSourceVLCPrivate::mediaPlayerEndReachedCallback,
                            this);
        libvlc_event_attach(libvlc_media_player_event_manager(this->d->m_mediaPlayer),
                            libvlc_MediaPlayerTimeChanged,
                            &MediaSourceVLCPrivate::mediaPlayerTimeChanged,
                            this);
        libvlc_video_set_callbacks(this->d->m_mediaPlayer,
                                   &MediaSourceVLCPrivate::videoLockCallback,
                                   nullptr,
                                   &MediaSourceVLCPrivate::videoDisplayCallback,
                                   this);
        libvlc_audio_set_callbacks(this->d->m_mediaPlayer,
                                   &MediaSourceVLCPrivate::audioPlayCallback,
                                   nullptr,
                                   nullptr,
                                   nullptr,
                                   nullptr,
                                   this);
        libvlc_video_set_format_callbacks(this->d->m_mediaPlayer,
                                          &MediaSourceVLCPrivate::videoFormatCallback,
                                          nullptr);
        libvlc_audio_set_format_callbacks(this->d->m_mediaPlayer,
                                          &MediaSourceVLCPrivate::audioSetupCallback,
                                          nullptr);
    }
}

MediaSourceVLC::~MediaSourceVLC()
{
    this->setState(AkElement::ElementStateNull);

    if (this->d->m_mediaPlayer)
        libvlc_media_player_release(this->d->m_mediaPlayer);

    if (this->d->m_vlcInstance)
        libvlc_release(this->d->m_vlcInstance);

    delete this->d;
}

QStringList MediaSourceVLC::medias() const
{
    QStringList medias;

    if (!this->d->m_media.isEmpty())
        medias << this->d->m_media;

    return medias;
}

QString MediaSourceVLC::media() const
{
    return this->d->m_media;
}

QList<int> MediaSourceVLC::streams() const
{
    return this->d->m_streams;
}

QList<int> MediaSourceVLC::listTracks(AkCaps::CapsType type)
{
    QList<int> tracks;
    int i = 0;

    for (auto &streamInfo: this->d->m_streamInfo) {
        if (type != AkCaps::CapsUnknown || streamInfo.caps.type() == type)
            tracks << i;

        i++;
    }

    return tracks;
}

QString MediaSourceVLC::streamLanguage(int stream)
{
    auto streamInfo = this->d->m_streamInfo.value(stream, Stream());

    return streamInfo.language;
}

bool MediaSourceVLC::loop() const
{
    return this->d->m_loop;
}

bool MediaSourceVLC::sync() const
{
    return this->d->m_sync;
}

int MediaSourceVLC::defaultStream(AkCaps::CapsType type)
{
    int defaultStream = -1;
    int i = 0;

    for (auto &streamInfo: this->d->m_streamInfo) {
        if (streamInfo.caps.type() == type) {
            defaultStream = i;

            break;
        }

        i++;
    }

    return defaultStream;
}

QString MediaSourceVLC::description(const QString &media) const
{
    if (this->d->m_media != media)
        return {};

    return QFileInfo(media).baseName();
}

AkCaps MediaSourceVLC::caps(int stream)
{
    Stream streamInfo = this->d->m_streamInfo.value(stream, Stream());

    return streamInfo.caps;
}

qint64 MediaSourceVLC::durationMSecs()
{
    return this->d->m_duration;
}

qint64 MediaSourceVLC::currentTimeMSecs()
{
    if (this->d->m_state == AkElement::ElementStateNull)
        return 0;

    qint64 currentTime = 0;
    this->d->m_mutex.lock();

    if (this->d->m_mediaPlayer)
        currentTime = libvlc_media_player_get_time(this->d->m_mediaPlayer);

    this->d->m_mutex.unlock();

    return std::max<qint64>(0, currentTime);
}

qint64 MediaSourceVLC::maxPacketQueueSize() const
{
    return this->d->m_maxPacketQueueSize;
}

bool MediaSourceVLC::showLog() const
{
    return this->d->m_showLog;
}

AkElement::ElementState MediaSourceVLC::state() const
{
    return this->d->m_state;
}

void MediaSourceVLC::seek(qint64 mSecs,
                          SeekPosition position)
{
    if (this->d->m_state == AkElement::ElementStateNull)
        return;

    int64_t pts = mSecs;
    auto duration = this->durationMSecs();

    switch (position) {
    case SeekCur:
        pts += this->currentTimeMSecs();

        break;

    case SeekEnd:
        pts += duration;

        break;

    default:
        break;
    }

    pts = qBound<qint64>(0, pts, duration);
    libvlc_media_player_set_position(this->d->m_mediaPlayer,
                                     float(pts) / float(duration));
}

void MediaSourceVLC::setMedia(const QString &media)
{
    if (media == this->d->m_media)
        return;

    auto state = this->d->m_state;
    this->setState(AkElement::ElementStateNull);
    this->d->m_media = media;

    if (!this->d->m_media.isEmpty()) {
        libvlc_media_t *vlcMedia = nullptr;

        if (this->d->m_vlcInstance) {
            if (QFileInfo(media).isFile()
                && QFileInfo::exists(media)) {
                auto media_ = media;
                media_.replace("/", QDir::separator());
                vlcMedia = libvlc_media_new_path(this->d->m_vlcInstance,
                                                 media_.toStdString().c_str());
            } else {
                vlcMedia = libvlc_media_new_location(this->d->m_vlcInstance,
                                                     media.toStdString().c_str());
            }
        }

        if (vlcMedia) {
            this->d->m_mutex.lock();
            libvlc_media_player_set_media(this->d->m_mediaPlayer, vlcMedia);
            this->d->m_mutex.unlock();
            libvlc_event_attach(libvlc_media_event_manager(vlcMedia),
                                libvlc_MediaParsedChanged,
                                &MediaSourceVLCPrivate::mediaParsedChangedCallback,
                                this);
            libvlc_media_parse_with_options(vlcMedia,
                                            libvlc_media_parse_network,
                                            3000);
            libvlc_media_release(vlcMedia);

            this->d->m_mutex.lock();
            this->d->m_mediaParsed.wait(&this->d->m_mutex);
            this->d->m_mutex.unlock();
        } else {
            this->d->m_mutex.lock();

            if (this->d->m_mediaPlayer)
                libvlc_media_player_set_media(this->d->m_mediaPlayer, nullptr);

            this->d->m_mutex.unlock();
            this->d->m_duration = 0;
            this->d->m_media = "";
        }

        this->setState(state);
    } else {
        this->d->m_mutex.lock();
        libvlc_media_player_set_media(this->d->m_mediaPlayer, nullptr);
        this->d->m_mutex.unlock();
        this->d->m_duration = 0;
    }

    emit this->mediaChanged(this->d->m_media);
    emit this->mediasChanged(this->medias());
    emit this->durationMSecsChanged(this->durationMSecs());
}

void MediaSourceVLC::setStreams(const QList<int> &streams)
{
    if (this->d->m_streams == streams)
        return;

    this->d->m_streams = streams;
    emit this->streamsChanged(streams);
}

void MediaSourceVLC::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->d->m_maxPacketQueueSize == maxPacketQueueSize)
        return;

    this->d->m_maxPacketQueueSize = maxPacketQueueSize;
    emit this->maxPacketQueueSizeChanged(maxPacketQueueSize);
}

void MediaSourceVLC::setShowLog(bool showLog)
{
    if (this->d->m_showLog == showLog)
        return;

    this->d->m_showLog = showLog;
    emit this->showLogChanged(showLog);
}

void MediaSourceVLC::setLoop(bool loop)
{
    if (this->d->m_loop == loop)
        return;

    this->d->m_loop = loop;
    emit this->loopChanged(loop);
}

void MediaSourceVLC::setSync(bool sync)
{
    if (this->d->m_sync == sync)
        return;

    this->d->m_sync = sync;
    emit this->syncChanged(sync);
}

void MediaSourceVLC::resetMedia()
{
    this->setMedia("");
}

void MediaSourceVLC::resetStreams()
{
    if  (this->d->m_streams.isEmpty())
        return;

    this->d->m_streams.clear();
    emit this->streamsChanged(this->d->m_streams);
}

void MediaSourceVLC::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void MediaSourceVLC::resetShowLog()
{
    this->setShowLog(false);
}

void MediaSourceVLC::resetLoop()
{
    this->setLoop(false);
}

void MediaSourceVLC::resetSync()
{
    this->setSync(true);
}

bool MediaSourceVLC::setState(AkElement::ElementState state)
{
    switch (this->d->m_state) {
    case AkElement::ElementStateNull: {
        if (state == AkElement::ElementStatePaused
            || state == AkElement::ElementStatePlaying) {
            if (this->d->m_media.isEmpty())
                return false;

            QList<int> filterStreams;

            if (this->d->m_streams.isEmpty())
                filterStreams << this->defaultStream(AkCaps::CapsAudio)
                              << this->defaultStream(AkCaps::CapsVideo);
            else
                filterStreams = this->d->m_streams;

            this->d->m_mutex.lock();

            for (int &i: filterStreams) {
                auto caps = this->caps(i);

                switch (caps.type()) {
                case AkCaps::CapsAudio:
                    libvlc_audio_set_track(this->d->m_mediaPlayer, i);
                    this->d->m_audioIndex = i;

                    break;
                case AkCaps::CapsVideo:
                    libvlc_video_set_track(this->d->m_mediaPlayer, i);
                    this->d->m_fps = AkVideoCaps(caps).fps();
                    this->d->m_videoIndex = i;
                    break;
                default:
                    break;
                }
            }

            if (libvlc_media_player_play(this->d->m_mediaPlayer) != 0) {
                this->d->m_mutex.unlock();

                return false;
            }

            if (state == AkElement::ElementStatePaused)
                libvlc_media_player_set_pause(this->d->m_mediaPlayer, true);

            this->d->m_mutex.unlock();
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
            this->d->m_mutex.lock();
            libvlc_media_player_stop(this->d->m_mediaPlayer);
            this->d->m_mutex.unlock();
            this->d->m_videoFrame = AkVideoPacket();
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }
        case AkElement::ElementStatePlaying: {
            this->d->m_mutex.lock();
            libvlc_media_player_set_pause(this->d->m_mediaPlayer, false);
            this->d->m_mutex.unlock();
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
            this->d->m_mutex.lock();
            libvlc_media_player_stop(this->d->m_mediaPlayer);
            this->d->m_mutex.unlock();
            this->d->m_videoFrame = AkVideoPacket();
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        }
        case AkElement::ElementStatePaused: {
            this->d->m_mutex.lock();
            libvlc_media_player_set_pause(this->d->m_mediaPlayer, true);
            this->d->m_mutex.unlock();
            this->d->m_state = state;
            emit this->stateChanged(state);

            break;
        }
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

MediaSourceVLCPrivate::MediaSourceVLCPrivate(MediaSourceVLC *self):
    self(self)
{
}

void MediaSourceVLCPrivate::doLoop()
{
    if (self->d->m_loop) {
        self->setState(AkElement::ElementStateNull);
        self->setState(AkElement::ElementStatePlaying);
    }
}

void MediaSourceVLCPrivate::mediaParsedChangedCallback(const libvlc_event_t *event,
                                                       void *userData)
{
    auto self = reinterpret_cast<MediaSourceVLC *>(userData);
    self->d->m_mutex.lock();

    if (event->u.media_parsed_changed.new_status != libvlc_media_parsed_status_done) {
        self->d->m_mediaParsed.wakeAll();
        self->d->m_mutex.unlock();

        return;
    }

    bool streamsChanged = false;
    qint64 duration = 0;

    if (self->d->m_mediaPlayer) {
        auto vlcMedia = libvlc_media_player_get_media(self->d->m_mediaPlayer);
        duration = std::max<qint64>(0,
                                    libvlc_media_get_duration(vlcMedia));

        QList<Stream> streamInfo;
        libvlc_media_track_t **tracks = nullptr;
        auto ntracks = libvlc_media_tracks_get(vlcMedia, &tracks);

        for (uint i = 0; i < ntracks; i++) {
            switch (tracks[i]->i_type) {
            case libvlc_track_audio: {
                AkAudioCaps audioCaps(AkAudioCaps::SampleFormat_s16,
                                      AkAudioCaps::defaultChannelLayout(int(tracks[i]->audio->i_channels)),
                                      false,
                                      int(tracks[i]->audio->i_rate));
                streamInfo << Stream(audioCaps,
                                     tracks[i]->psz_language);

                break;
            }

            case libvlc_track_video: {
                AkVideoCaps videoCaps(AkVideoCaps::Format_rgb24,
                                      int(tracks[i]->video->i_width),
                                      int(tracks[i]->video->i_height),
                                      AkFrac(tracks[i]->video->i_frame_rate_num,
                                             tracks[i]->video->i_frame_rate_den));
                streamInfo << Stream(videoCaps,
                                     tracks[i]->psz_language);

                break;
            }

            case libvlc_track_text: {
                AkSubtitleCaps subtitlesCaps(AkSubtitleCaps::SubtitleFormat_text);
                streamInfo << Stream(subtitlesCaps,
                                     tracks[i]->psz_language);

                break;
            }

            default:
                break;
            }
        }

        if (ntracks > 0 && tracks)
            libvlc_media_tracks_release(tracks, ntracks);

        self->d->m_streamInfo = streamInfo;
        streamsChanged = true;
    }

    self->d->m_mediaParsed.wakeAll();
    self->d->m_mutex.unlock();

    if (self->d->m_duration != duration) {
        self->d->m_duration = duration;
        emit self->durationMSecsChanged(duration);
    }

    if (streamsChanged) {
        emit self->streamsChanged(self->d->m_streams);
        emit self->mediaLoaded(self->d->m_media);
    }
}

void MediaSourceVLCPrivate::mediaPlayerEndReachedCallback(const libvlc_event_t *event,
                                                          void *userData)
{
    Q_UNUSED(event)
    auto self = reinterpret_cast<MediaSourceVLC *>(userData);
    auto result = QtConcurrent::run(&self->d->m_threadPool, [self] () {
        self->d->doLoop();
    });
    Q_UNUSED(result)
}

void MediaSourceVLCPrivate::mediaPlayerTimeChanged(const libvlc_event_t *event, void *userData)
{
    auto self = reinterpret_cast<MediaSourceVLC *>(userData);
    self->d->m_pts = event->u.media_player_time_changed.new_time;
}

void *MediaSourceVLCPrivate::videoLockCallback(void *userData, void **planes)
{
    auto self = reinterpret_cast<MediaSourceVLC *>(userData);
    planes[0] = self->d->m_videoFrame.data();

    return self;
}

void MediaSourceVLCPrivate::videoDisplayCallback(void *userData, void *picture)
{
    Q_UNUSED(picture)
    auto self = reinterpret_cast<MediaSourceVLC *>(userData);
    self->d->m_videoFrame.setPts(self->d->m_pts);
    emit self->oStream(self->d->m_videoFrame);
}

void MediaSourceVLCPrivate::audioPlayCallback(void *userData,
                                              const void *samples,
                                              unsigned count,
                                              int64_t pts)
{
    auto self = reinterpret_cast<MediaSourceVLC *>(userData);

    AkAudioPacket packet(self->d->m_audioCaps, count);
    memcpy(packet.data(), samples, packet.size());
    packet.setPts(pts);
    packet.setTimeBase({1, 1000});
    packet.setIndex(int(self->d->m_audioIndex));
    packet.setId(self->d->m_audioId);

    emit self->oStream(packet);
}

unsigned MediaSourceVLCPrivate::videoFormatCallback(void **userData,
                                                    char *chroma,
                                                    unsigned *width,
                                                    unsigned *height,
                                                    unsigned *pitches,
                                                    unsigned *lines)
{
    auto self = reinterpret_cast<MediaSourceVLC *>(*userData);

    AkVideoCaps caps(AkVideoCaps::Format_rgb24,
                     int(*width),
                     int(*height),
                     self->d->m_fps);
    self->d->m_videoFrame = AkVideoPacket(caps);
    self->d->m_videoFrame.setTimeBase(AkFrac(1, 1000));
    self->d->m_videoFrame.setIndex(int(self->d->m_videoIndex));
    self->d->m_videoFrame.setId(self->d->m_videoId);
    snprintf(chroma, 5, "%s", "RV24");
    *pitches = unsigned(self->d->m_videoFrame.lineSize(0));
    *lines = *height;

    return 1;
}

int MediaSourceVLCPrivate::audioSetupCallback(void **userData,
                                              char *format,
                                              unsigned *rate,
                                              unsigned *channels)
{
    auto self = reinterpret_cast<MediaSourceVLC *>(*userData);

    /* Force audio playback in stereo mode to make it compatible with any of
     * the available audio converters.
     */
    *channels = 2;

    self->d->m_audioCaps = {AkAudioCaps::SampleFormat_s16,
                            AkAudioCaps::defaultChannelLayout(int(*channels)),
                            false,
                            int(*rate)};
    snprintf(format, 5, "%s", "S16N");

    return 0;
}

#include "moc_mediasourcevlc.cpp"

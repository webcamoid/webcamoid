/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QList>
#include <QMediaPlayer>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>

#include "videothumbnailer.h"
#include "thumbnailresponse.h"

#define THUMBNAIL_MAX_SIZE 512
#define THUMBNAIL_TIMEOUT_MS 15000

struct ThumbnailJob
{
    QPointer<ThumbnailResponse> response;
    QUrl source;
    QString cachePath;
};

class VideoThumbnailerPrivate
{
    public:
        VideoThumbnailer *self;
        QMediaPlayer *m_player {nullptr};
        QVideoSink *m_sink {nullptr};
        QTimer *m_timeout {nullptr};
        QList<ThumbnailJob> m_queue;
        ThumbnailJob m_current;
        bool m_busy {false};
        bool m_started {false};

        explicit VideoThumbnailerPrivate(VideoThumbnailer *self);
        void ensurePlayer();
        void next();
        void finishCurrent(const QImage &image, const QString &error);
        void onMediaLoaded();
        void onVideoFrame(const QVideoFrame &frame);
};

VideoThumbnailer::VideoThumbnailer(QObject *parent):
    QObject(parent)
{
    this->d = new VideoThumbnailerPrivate(this);
}

VideoThumbnailer::~VideoThumbnailer()
{
    if (this->d->m_player) {
        this->d->m_player->stop();
        this->d->m_player->setSource({});
    }

    delete this->d;
}

void VideoThumbnailer::enqueue(const QPointer<ThumbnailResponse> &response,
                               const QUrl &source,
                               const QString &cachePath)
{
    this->d->m_queue << ThumbnailJob {response, source, cachePath};
    this->d->next();
}

VideoThumbnailerPrivate::VideoThumbnailerPrivate(VideoThumbnailer *self):
    self(self)
{
}

void VideoThumbnailerPrivate::ensurePlayer()
{
    if (this->m_player)
        return;

    // Created on first use to not load the multimedia backend at startup.
    this->m_player = new QMediaPlayer(this->self);
    this->m_sink = new QVideoSink(this->self);
    this->m_player->setVideoSink(this->m_sink);

    this->m_timeout = new QTimer(this->self);
    this->m_timeout->setSingleShot(true);
    this->m_timeout->setInterval(THUMBNAIL_TIMEOUT_MS);

    QObject::connect(this->m_player,
                     &QMediaPlayer::mediaStatusChanged,
                     this->self,
                     [this] (QMediaPlayer::MediaStatus status) {
        if (!this->m_busy)
            return;

        if (status == QMediaPlayer::LoadedMedia)
            this->onMediaLoaded();
        else if (status == QMediaPlayer::InvalidMedia)
            this->finishCurrent({}, "Invalid media");
    });
    QObject::connect(this->m_player,
                     &QMediaPlayer::errorOccurred,
                     this->self,
                     [this] (QMediaPlayer::Error error,
                             const QString &errorString) {
        Q_UNUSED(error)

        if (this->m_busy)
            this->finishCurrent({}, errorString);
    });
    QObject::connect(this->m_sink,
                     &QVideoSink::videoFrameChanged,
                     this->self,
                     [this] (const QVideoFrame &frame) {
        this->onVideoFrame(frame);
    });
    QObject::connect(this->m_timeout,
                     &QTimer::timeout,
                     this->self,
                     [this] () {
        if (this->m_busy)
            this->finishCurrent({}, "Timeout");
    });
}

void VideoThumbnailerPrivate::next()
{
    while (!this->m_busy && !this->m_queue.isEmpty()) {
        this->m_current = this->m_queue.takeFirst();

        if (!this->m_current.response
            || this->m_current.response->isCanceled()) {
            this->m_current = {};

            continue;
        }

        this->ensurePlayer();
        this->m_busy = true;
        this->m_started = false;
        this->m_timeout->start();
        this->m_player->setSource(this->m_current.source);
    }
}

void VideoThumbnailerPrivate::finishCurrent(const QImage &image,
                                            const QString &error)
{
    this->m_timeout->stop();
    this->m_player->stop();

    // Reset the source so the same video can be loaded again.
    this->m_player->setSource({});

    auto response = this->m_current.response;
    this->m_current = {};
    this->m_busy = false;
    this->m_started = false;

    if (response) {
        QMetaObject::invokeMethod(response.data(),
                                  [response, image, error] () {
            if (response)
                response->finish(image, error);
        }, Qt::QueuedConnection);
    }

    QMetaObject::invokeMethod(this->self, [this] () {
        this->next();
    }, Qt::QueuedConnection);
}

void VideoThumbnailerPrivate::onMediaLoaded()
{
    if (this->m_started)
        return;

    this->m_started = true;
    auto duration = this->m_player->duration();

    // Seek some time forward to get a more representative frame
    // (10% of the video).
    if (duration > 0)
        this->m_player->setPosition(qMin<qint64>(5000, duration / 10));

    this->m_player->play();
}

void VideoThumbnailerPrivate::onVideoFrame(const QVideoFrame &frame)
{
    if (!this->m_busy || !this->m_started || !frame.isValid())
        return;

    auto image = frame.toImage();

    if (image.isNull())
        return;

    if (image.width() > THUMBNAIL_MAX_SIZE
        || image.height() > THUMBNAIL_MAX_SIZE) {
        image = image.scaled(THUMBNAIL_MAX_SIZE,
                             THUMBNAIL_MAX_SIZE,
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }

    if (!this->m_current.cachePath.isEmpty()
        && QDir().mkpath(QFileInfo(this->m_current.cachePath).absolutePath())) {
        image.save(this->m_current.cachePath, "PNG");
    }

    this->finishCurrent(image, {});
}

#include "moc_videothumbnailer.cpp"

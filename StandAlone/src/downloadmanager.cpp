/* Webcamoid, camera capture application.
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

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQmlContext>
#include <QSharedPointer>
#include <QtQml>

#include "downloadmanager.h"

class DownloadInfo
{
    public:
        QString title;
        QString url;
        // QFile is not copyable, and QVector copies/assigns its elements when
        // it grows or erases items. A shared pointer keeps the open file (and
        // its state) alive across those copies.
        QSharedPointer<QFile> file {new QFile};
        qint64 size {0};
        qint64 downloaded {0};
        DownloadManager::DownloadStatus status {DownloadManager::DownloadStatusFinished};
        bool abort {false};
        bool writeFailed {false};
        QElapsedTimer timeElapsed;
        QString errorString;

        DownloadInfo();
        DownloadInfo(const QString &title,
                     const QString &url,
                     const QString &file);
        DownloadInfo(const DownloadInfo &other);
        DownloadInfo &operator =(const DownloadInfo &other);
        bool operator ==(const DownloadInfo &other) const;
};

class DownloadManagerPrivate
{
    public:
        DownloadManager *self;
        QQmlApplicationEngine *m_engine {nullptr};
        QNetworkAccessManager m_manager;
        QVector<DownloadInfo> m_downloads;
        QMutex m_mutex;

        explicit DownloadManagerPrivate(DownloadManager *self);
        void updateProgress(const QString &url,
                            qint64 bytesReceived,
                            qint64 bytesTotal);
        void downloadFinished(const QString &url, QNetworkReply *reply);
        void downloadFile(const QString &url, QNetworkReply *reply);
        bool abort(const QString &url);
        void setWriteError(const QString &url, const QString &error);
};

DownloadManager::DownloadManager(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new DownloadManagerPrivate(this);
    this->setQmlEngine(engine);
    this->d->m_manager.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
}

DownloadManager::~DownloadManager()
{
    delete this->d;
}

QStringList DownloadManager::downloads() const
{
    QMutexLocker locker(&this->d->m_mutex);
    QStringList downloads;

    for (auto &info: this->d->m_downloads)
        downloads << info.url;

    return downloads;
}

QString DownloadManager::downloadTitle(const QString &url) const
{
    QMutexLocker locker(&this->d->m_mutex);
    QString title;

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            title = info.title;

    return title;
}

QString DownloadManager::downloadFile(const QString &url) const
{
    QMutexLocker locker(&this->d->m_mutex);
    QString file;

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            file = info.file->fileName();

    return file;
}

qint64 DownloadManager::downloadSize(const QString &url) const
{
    QMutexLocker locker(&this->d->m_mutex);
    qint64 size = 0;

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            size = info.size;

    return size;
}

qint64 DownloadManager::downloadedBytes(const QString &url) const
{
    QMutexLocker locker(&this->d->m_mutex);
    qint64 downloaded = 0;

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            downloaded = info.downloaded;

    return downloaded;
}

DownloadManager::DownloadStatus DownloadManager::downloadStatus(const QString &url) const
{
    QMutexLocker locker(&this->d->m_mutex);
    DownloadStatus status = DownloadStatusFinished;

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            status = info.status;

    return status;
}

quint64 DownloadManager::downloadTimeElapsed(const QString &url) const
{
    QMutexLocker locker(&this->d->m_mutex);
    quint64 elapsed = 0;

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            elapsed = info.timeElapsed.elapsed();

    return elapsed;
}

QString DownloadManager::downloadErrorString(const QString &url) const
{
    QMutexLocker locker(&this->d->m_mutex);
    QString errorString = 0;

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            errorString = info.errorString;

    return errorString;
}

inline bool DownloadManager::enqueue(const QString &title,
                                     const QString &fromUrl,
                                     const QString &toFile)
{
    auto downloads = this->downloads();

    if (downloads.contains(fromUrl))
        return false;

    DownloadInfo info(title, fromUrl, toFile);
    info.status = DownloadManager::DownloadStatusStarted;

    auto reply = this->d->m_manager.get(QNetworkRequest(QUrl(fromUrl)));
    QObject::connect(reply,
                     &QNetworkReply::downloadProgress,
                     this,
                     [this, fromUrl] (qint64 bytesReceived, qint64 bytesTotal) {
        this->d->updateProgress(fromUrl, bytesReceived, bytesTotal);
    });
    QObject::connect(reply,
                     &QNetworkReply::finished,
                     this,
                     [this, fromUrl, reply] () {
        this->d->downloadFinished(fromUrl, reply);
    });
    QObject::connect(reply,
                     &QNetworkReply::readyRead,
                     this,
                     [this, fromUrl, reply] () {
        this->d->downloadFile(fromUrl, reply);
    });

    qDebug() << "Downloading file from "
             << reply->url()
             << "to"
             << info.file->fileName();
    info.timeElapsed.start();
    
    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_downloads << info;
    }
    
    emit this->downloadsChanged(this->downloads());
    emit this->downloadChanged(fromUrl);

    return true;
}

void DownloadManager::cancel()
{
    QMutexLocker locker(&this->d->m_mutex);

    for (auto &info: this->d->m_downloads)
        if (info.status == DownloadManager::DownloadStatusStarted
            || info.status == DownloadManager::DownloadStatusInProgress) {
            info.abort = true;
        }
}

void DownloadManager::cancel(const QString &url)
{
    QMutexLocker locker(&this->d->m_mutex);

    for (auto &info: this->d->m_downloads)
        if (info.url == url) {
            if (info.status == DownloadManager::DownloadStatusStarted
                || info.status == DownloadManager::DownloadStatusInProgress) {
                info.abort = true;
            }

            break;
        }
}

void DownloadManager::remove(const QString &url)
{
    bool changed = false;

    {
        QMutexLocker locker(&this->d->m_mutex);

        auto it = std::find_if(this->d->m_downloads.begin(),
                               this->d->m_downloads.end(),
                               [url] (const DownloadInfo &info) {
            return info.url == url
                    && info.status != DownloadStatusStarted
                    && info.status != DownloadStatusInProgress;
        });

        if (it != this->d->m_downloads.end()) {
            this->d->m_downloads.erase(it);
            changed = true;
        }
    }

    if (changed)
        emit this->downloadsChanged(this->downloads());
}

void DownloadManager::clear()
{
    bool changed = false;

    {
        QMutexLocker locker(&this->d->m_mutex);
        QVector<DownloadInfo> downloads;

        for (auto &info: this->d->m_downloads)
            if (info.status == DownloadStatusStarted
                || info.status == DownloadStatusInProgress) {
                downloads << info;
            }

        if (this->d->m_downloads != downloads) {
            this->d->m_downloads = downloads;
            changed = true;
        }
    }

    if (changed)
        emit this->downloadsChanged(this->downloads());
}

void DownloadManager::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine) {
        engine->rootContext()->setContextProperty("downloadManager", this);
        qmlRegisterType<DownloadManager>("Webcamoid", 1, 0, "DownloadManager");
    }
}

DownloadManagerPrivate::DownloadManagerPrivate(DownloadManager *self):
    self(self)
{
}

void DownloadManagerPrivate::updateProgress(const QString &url,
                                            qint64 bytesReceived,
                                            qint64 bytesTotal)
{
    bool emitSignal = false;

    {
        QMutexLocker locker(&this->m_mutex);

        for (auto &info: this->m_downloads)
            if (info.url == url) {
                info.downloaded = bytesReceived;
                info.size = bytesTotal;
                emitSignal = true;

                break;
            }
    }

    if (emitSignal)
        emit self->downloadChanged(url);
}

void DownloadManagerPrivate::downloadFinished(const QString &url,
                                              QNetworkReply *reply)
{
    bool emitSignals = false;

    {
        QMutexLocker locker(&this->m_mutex);

        for (auto &info: this->m_downloads)
            if (info.url == url) {
                bool wasOpen = info.file->isOpen();

                // Data still buffered by QFile is written to disk here, so
                // a full disk or an exhausted quota can show up only now.
                if (wasOpen) {
                    if (!info.file->flush() && !info.writeFailed) {
                        info.writeFailed = true;
                        info.errorString = info.file->errorString();
                    }

                    info.file->close();

                    if (info.file->error() != QFileDevice::NoError
                        && !info.writeFailed) {
                        info.writeFailed = true;
                        info.errorString = info.file->errorString();
                    }
                }

                if (info.writeFailed) {
                    // The saved file is incomplete: never report success.
                    info.status = DownloadManager::DownloadStatusFailed;

                    // Remove the partial file, but only if it was us who
                    // created it (not when open() itself failed).
                    if (wasOpen)
                        info.file->remove();
                } else if (info.abort) {
                    info.status = DownloadManager::DownloadStatusCanceled;
                    info.errorString = reply->errorString();
                } else {
                    info.status = reply->error() == QNetworkReply::NoError?
                                      DownloadManager::DownloadStatusFinished:
                                      DownloadManager::DownloadStatusFailed;
                    info.errorString = reply->errorString();
                }

                info.timeElapsed = {};
                emitSignals = true;

                break;
            }
    }

    if (emitSignals) {
        emit self->downloadChanged(url);
        emit self->finished(url);
    }

    reply->deleteLater();
}

void DownloadManagerPrivate::downloadFile(const QString &url,
                                          QNetworkReply *reply)
{
    bool isOpen = false;
    QString openError;

    {
        QMutexLocker locker(&this->m_mutex);

        for (auto &info: this->m_downloads)
            if (info.url == url) {
                isOpen = info.file->isOpen();

                if (!isOpen) {
                    isOpen = info.file->open(QIODevice::WriteOnly);

                    if (!isOpen)
                        openError = info.file->errorString();
                }

                break;
            }
    }

    if (!isOpen)
        this->setWriteError(url, openError);

    // reply->abort() emits finished(), so it must be called without
    // holding the mutex.
    if (!isOpen || this->abort(url)) {
        reply->abort();

        return;
    }

    bool emitSignal = false;
    bool writeFailed = false;

    {
        QMutexLocker locker(&this->m_mutex);

        for (auto &info: this->m_downloads)
            if (info.url == url) {
                auto data = reply->readAll();
                auto written = info.file->write(data);

                // QFile is buffered, so flush to detect errors (no space
                // left, quota exceeded, etc.) now instead of at the end.
                if (written != data.size() || !info.file->flush()) {
                    info.writeFailed = true;
                    info.errorString = info.file->errorString();
                    writeFailed = true;
                } else {
                    info.status = DownloadManager::DownloadStatusInProgress;
                    emitSignal = true;
                }

                break;
            }
    }

    if (writeFailed) {
        // Stop downloading; downloadFinished() will mark it as failed.
        reply->abort();

        return;
    }

    if (emitSignal)
        emit self->downloadChanged(url);
}

bool DownloadManagerPrivate::abort(const QString &url)
{
    QMutexLocker locker(&this->m_mutex);
    bool abort = true;

    for (auto &info: this->m_downloads)
        if (info.url == url)
            abort = info.abort;

    return abort;
}

void DownloadManagerPrivate::setWriteError(const QString &url,
                                           const QString &error)
{
    QMutexLocker locker(&this->m_mutex);

    for (auto &info: this->m_downloads)
        if (info.url == url) {
            info.writeFailed = true;
            info.errorString = error;

            break;
        }
}

DownloadInfo::DownloadInfo()
{

}

DownloadInfo::DownloadInfo(const QString &title,
                           const QString &url,
                           const QString &file):
    title(title),
    url(url),
    file(new QFile(file))
{

}

DownloadInfo::DownloadInfo(const DownloadInfo &other):
    title(other.title),
    url(other.url),
    file(other.file),
    size(other.size),
    downloaded(other.downloaded),
    status(other.status),
    abort(other.abort),
    writeFailed(other.writeFailed),
    timeElapsed(other.timeElapsed),
    errorString(other.errorString)
{
}

DownloadInfo &DownloadInfo::operator =(const DownloadInfo &other)
{
    if (this != &other) {
        this->title = other.title;
        this->url = other.url;
        this->size = other.size;
        this->downloaded = other.downloaded;
        this->status = other.status;
        this->abort = other.abort;
        this->writeFailed = other.writeFailed;
        this->timeElapsed = other.timeElapsed;
        this->errorString = other.errorString;
        this->file = other.file;
    }

    return *this;
}

bool DownloadInfo::operator ==(const DownloadInfo &other) const
{
    return this->url == other.url;
}

#include "moc_downloadmanager.cpp"

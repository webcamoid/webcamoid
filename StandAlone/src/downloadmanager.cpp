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

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQmlContext>
#include <QtQml>

#include "downloadmanager.h"

class DownloadInfo
{
    public:
        QString title;
        QString url;
        QFile file;
        qint64 size {0};
        qint64 downloaded {0};
        DownloadManager::DownloadStatus status {DownloadManager::DownloadStatusFinished};
        bool abort {false};
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
    QStringList downloads;
    this->d->m_mutex.lock();

    for (auto &info: this->d->m_downloads)
        downloads << info.url;

    this->d->m_mutex.unlock();

    return downloads;
}

QString DownloadManager::downloadTitle(const QString &url) const
{
    QString title;
    this->d->m_mutex.lock();

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            title = info.title;

    this->d->m_mutex.unlock();

    return title;
}

QString DownloadManager::downloadFile(const QString &url) const
{
    QString file;
    this->d->m_mutex.lock();

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            file = info.file.fileName();

    this->d->m_mutex.unlock();

    return file;
}

qint64 DownloadManager::downloadSize(const QString &url) const
{
    qint64 size = 0;
    this->d->m_mutex.lock();

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            size = info.size;

    this->d->m_mutex.unlock();

    return size;
}

qint64 DownloadManager::downloadedBytes(const QString &url) const
{
    qint64 downloaded = 0;
    this->d->m_mutex.lock();

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            downloaded = info.downloaded;

    this->d->m_mutex.unlock();

    return downloaded;
}

DownloadManager::DownloadStatus DownloadManager::downloadStatus(const QString &url) const
{
    DownloadStatus status = DownloadStatusFinished;
    this->d->m_mutex.lock();

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            status = info.status;

    this->d->m_mutex.unlock();

    return status;
}

quint64 DownloadManager::downloadTimeElapsed(const QString &url) const
{
    quint64 elapsed = 0;
    this->d->m_mutex.lock();

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            elapsed = info.timeElapsed.elapsed();

    this->d->m_mutex.unlock();

    return elapsed;
}

QString DownloadManager::downloadErrorString(const QString &url) const
{
    QString errorString = 0;
    this->d->m_mutex.lock();

    for (auto &info: this->d->m_downloads)
        if (info.url == url)
            errorString = info.errorString;

    this->d->m_mutex.unlock();

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
    info.file.setFileName(toFile);

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
             << info.file.fileName();
    info.timeElapsed.start();
    this->d->m_mutex.lock();
    this->d->m_downloads << info;
    this->d->m_mutex.unlock();
    emit this->downloadsChanged(this->downloads());
    emit this->downloadChanged(fromUrl);

    return true;
}

void DownloadManager::cancel()
{
    this->d->m_mutex.lock();

    for (auto &info: this->d->m_downloads)
        if (info.status == DownloadManager::DownloadStatusStarted
            || info.status == DownloadManager::DownloadStatusInProgress) {
            info.abort = true;
        }

    this->d->m_mutex.unlock();
}

void DownloadManager::cancel(const QString &url)
{
    this->d->m_mutex.lock();

    for (auto &info: this->d->m_downloads)
        if (info.url == url) {
            if (info.status == DownloadManager::DownloadStatusStarted
                || info.status == DownloadManager::DownloadStatusInProgress) {
                info.abort = true;
            }

            break;
        }

    this->d->m_mutex.unlock();
}

void DownloadManager::remove(const QString &url)
{
    this->d->m_mutex.lock();

    auto it = std::find_if(this->d->m_downloads.begin(),
                           this->d->m_downloads.end(),
                           [url] (const DownloadInfo &info) {
        return info.url == url
                && info.status != DownloadStatusStarted
                && info.status != DownloadStatusInProgress;
    });

    bool changed = false;

    if (it != this->d->m_downloads.end()) {
        this->d->m_downloads.erase(it);
        changed = true;
    }

    this->d->m_mutex.unlock();

    if (changed)
        emit this->downloadsChanged(this->downloads());
}

void DownloadManager::clear()
{
    QVector<DownloadInfo> downloads;
    this->d->m_mutex.lock();

    for (auto &info: this->d->m_downloads)
        if (info.status == DownloadStatusStarted
            || info.status == DownloadStatusInProgress) {
            downloads << info;
        }

    bool changed = false;

    if (this->d->m_downloads != downloads) {
        this->d->m_downloads = downloads;
        changed = true;
    }

    this->d->m_mutex.unlock();

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
    this->m_mutex.lock();

    for (auto &info: this->m_downloads)
        if (info.url == url) {
            info.downloaded = bytesReceived;
            info.size = bytesTotal;
            emitSignal = true;

            break;
        }

    this->m_mutex.unlock();

    if (emitSignal)
        emit self->downloadChanged(url);
}

void DownloadManagerPrivate::downloadFinished(const QString &url,
                                              QNetworkReply *reply)
{
    bool emitSignals = false;
    this->m_mutex.lock();

    for (auto &info: this->m_downloads)
        if (info.url == url) {
            info.file.close();

            if (info.abort)
                info.status = DownloadManager::DownloadStatusCanceled;
            else
                info.status = reply->error() == QNetworkReply::NoError?
                                  DownloadManager::DownloadStatusFinished:
                                  DownloadManager::DownloadStatusFailed;

            info.timeElapsed = {};
            info.errorString = reply->errorString();
            emitSignals = true;

            break;
        }

    this->m_mutex.unlock();

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

    this->m_mutex.lock();

    for (auto &info: this->m_downloads)
        if (info.url == url) {
            isOpen = info.file.isOpen();

            if (!isOpen)
                isOpen = info.file.open(QIODevice::WriteOnly);

            break;
        }

    this->m_mutex.unlock();

    if (!isOpen || this->abort(url)) {
        reply->abort();

        return;
    }

    bool emitSignal = false;
    this->m_mutex.lock();

    for (auto &info: this->m_downloads)
        if (info.url == url) {
            info.file.write(reply->readAll());
            info.status = DownloadManager::DownloadStatusInProgress;
            emitSignal = true;

            break;
        }

    this->m_mutex.unlock();

    if (emitSignal)
        emit self->downloadChanged(url);
}

bool DownloadManagerPrivate::abort(const QString &url)
{
    bool abort = true;
    this->m_mutex.lock();

    for (auto &info: this->m_downloads)
        if (info.url == url)
            abort = info.abort;

    this->m_mutex.unlock();

    return abort;
}

DownloadInfo::DownloadInfo()
{

}

DownloadInfo::DownloadInfo(const QString &title,
                           const QString &url,
                           const QString &file):
    title(title),
    url(url),
    file(file)
{

}

DownloadInfo::DownloadInfo(const DownloadInfo &other):
    title(other.title),
    url(other.url),
    size(other.size),
    downloaded(other.downloaded),
    status(other.status),
    abort(other.abort),
    timeElapsed(other.timeElapsed),
    errorString(other.errorString)
{
    this->file.setFileName(other.file.fileName());
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
        this->timeElapsed = other.timeElapsed;
        this->errorString = other.errorString;
        this->file.close();
        this->file.setFileName(other.file.fileName());
    }

    return *this;
}

bool DownloadInfo::operator ==(const DownloadInfo &other) const
{
    return this->url == other.url;
}

#include "moc_downloadmanager.cpp"

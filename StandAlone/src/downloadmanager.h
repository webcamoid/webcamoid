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

#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>

class DownloadManagerPrivate;
class DownloadManager;
class QQmlApplicationEngine;
class QNetworkReply;

using DownloadManagerPtr = QSharedPointer<DownloadManager>;

class DownloadManager: public QObject
{
    Q_OBJECT
    Q_ENUMS(DownloadStatus)
    Q_PROPERTY(QStringList downloads
               READ downloads
               NOTIFY downloadsChanged)

    public:
        enum DownloadStatus {
            DownloadStatusStarted,
            DownloadStatusInProgress,
            DownloadStatusFinished,
            DownloadStatusCanceled,
            DownloadStatusFailed
        };

        DownloadManager(QQmlApplicationEngine *engine=nullptr,
                        QObject *parent=nullptr);
        ~DownloadManager();

        Q_INVOKABLE QStringList downloads() const;
        Q_INVOKABLE QString downloadTitle(const QString &url) const;
        Q_INVOKABLE QString downloadFile(const QString &url) const;
        Q_INVOKABLE qint64 downloadSize(const QString &url) const;
        Q_INVOKABLE qint64 downloadedBytes(const QString &url) const;
        Q_INVOKABLE DownloadStatus downloadStatus(const QString &url) const;
        Q_INVOKABLE quint64 downloadTimeElapsed(const QString &url) const;
        Q_INVOKABLE QString downloadErrorString(const QString &url) const;

    private:
        DownloadManagerPrivate *d;

    signals:
        void finished(const QString &url);
        void downloadChanged(const QString &url);
        void downloadsChanged(const QStringList &downloads);

    public slots:
        bool enqueue(const QString &title,
                     const QString &fromUrl,
                     const QString &toFile);
        void cancel();
        void cancel(const QString &id);
        void remove(const QString &url);
        void clear();
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);

    friend class DownloadManagerPrivate;
};

Q_DECLARE_METATYPE(DownloadManager::DownloadStatus)

#endif // DOWNLOADMANAGER_H

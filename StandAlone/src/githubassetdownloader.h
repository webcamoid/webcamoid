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

#ifndef GITHUBASSETDOWNLOADER_H
#define GITHUBASSETDOWNLOADER_H

#include <QObject>
#include <QSharedPointer>

class GitHubAssetDownloader;
class GitHubAssetDownloaderPrivate;
class GitHubAuth;
class QByteArray;
class QNetworkAccessManager;
class QNetworkReply;
class QUrl;

using GitHubAssetDownloaderPtr = QSharedPointer<GitHubAssetDownloader>;

/* Downloads an asset of a private release using its API URL
 * (https://api.github.com/repos/.../releases/assets/<id>).
 *
 * - The first request carries the token. GitHub answers with a redirection to
 *   a signed URL; that second request is made WITHOUT the token (the URL is
 *   already signed and sending other credentials can make it fail).
 * - The file is written to "<destination>.part" and renamed to the
 *   destination only when the size and the SHA-256 match.
 */
class GitHubAssetDownloader: public QObject
{
    Q_OBJECT

    public:
        enum Error {
            NotAuthorized,
            NetworkError,
            FileError,
            SizeMismatch,
            ChecksumMismatch,
            Canceled
        };
        Q_ENUM(Error)

        GitHubAssetDownloader(GitHubAuth *auth,
                              QNetworkAccessManager *manager,
                              QObject *parent=nullptr);
        ~GitHubAssetDownloader();

        Q_INVOKABLE bool busy() const;

    private:
        GitHubAssetDownloaderPrivate *d;

    signals:
        void progress(qint64 received, qint64 total);
        void finishedFile(const QString &path);
        void finishedMemory(const QByteArray &data);
        void failed(GitHubAssetDownloader::Error error, const QString &message);

    public slots:
        // expectedSize and expectedSha256 are optional (0 / empty: not verified).
        void downloadToFile(const QUrl &apiUrl,
                            const QString &path,
                            qint64 expectedSize=0,
                            const QString &expectedSha256={});
        void downloadToMemory(const QUrl &apiUrl); // small files (<1 MiB)
        void cancel();
};

#endif // GITHUBASSETDOWNLOADER_H

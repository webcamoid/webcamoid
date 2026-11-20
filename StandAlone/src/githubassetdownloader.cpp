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

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "githubassetdownloader.h"
#include "githubauth.h"

#define MAX_REDIRECTS       5
#define MAX_MEMORY_DOWNLOAD (1024 * 1024)

class GitHubAssetDownloaderPrivate
{
    public:
        GitHubAssetDownloader *self;
        GitHubAuth *m_auth;
        QNetworkAccessManager *m_manager;
        QNetworkReply *m_reply {nullptr};
        QFile m_file;
        QByteArray m_buffer;
        QCryptographicHash m_hash {QCryptographicHash::Sha256};
        QString m_path;
        QString m_expectedSha256;
        qint64 m_expectedSize {0};
        qint64 m_received {0};
        int m_redirects {0};
        bool m_toFile {false};
        bool m_active {false};

        explicit GitHubAssetDownloaderPrivate(GitHubAssetDownloader *self);
        static int httpStatus(QNetworkReply *reply);
        static bool isRedirect(int status);
        bool begin(const QUrl &apiUrl);
        void request(const QUrl &url, bool withAuth);
        void consume(QNetworkReply *reply);
        void onFinished(QNetworkReply *reply);
        void fail(GitHubAssetDownloader::Error error,
                  const QString &message);
        void failLater(GitHubAssetDownloader::Error error,
                       const QString &message);
        void cleanup();
};

GitHubAssetDownloader::GitHubAssetDownloader(GitHubAuth *auth,
                                             QNetworkAccessManager *manager,
                                             QObject *parent):
    QObject(parent)
{
    this->d = new GitHubAssetDownloaderPrivate(this);
    this->d->m_auth = auth;
    this->d->m_manager = manager;
}

GitHubAssetDownloader::~GitHubAssetDownloader()
{
    delete this->d;
}

bool GitHubAssetDownloader::busy() const
{
    return this->d->m_active;
}

void GitHubAssetDownloader::downloadToFile(const QUrl &apiUrl,
                                           const QString &path,
                                           qint64 expectedSize,
                                           const QString &expectedSha256)
{
    if (this->d->m_active)
        return;

    if (!this->d->begin(apiUrl))
        return;

    QDir().mkpath(QFileInfo(path).absolutePath());
    this->d->m_path = path;
    this->d->m_expectedSize = expectedSize;
    this->d->m_expectedSha256 = expectedSha256.toLower();
    this->d->m_file.setFileName(path + ".part");

    if (!this->d->m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        this->d->failLater(FileError,
                           tr("Cannot write %1: %2")
                               .arg(this->d->m_file.fileName(),
                                    this->d->m_file.errorString()));

        return;
    }

    this->d->m_toFile = true;
    this->d->m_active = true;
    this->d->request(apiUrl, true);
}

void GitHubAssetDownloader::downloadToMemory(const QUrl &apiUrl)
{
    if (this->d->m_active)
        return;

    if (!this->d->begin(apiUrl))
        return;

    this->d->m_toFile = false;
    this->d->m_expectedSize = 0;
    this->d->m_expectedSha256.clear();
    this->d->m_active = true;
    this->d->request(apiUrl, true);
}

void GitHubAssetDownloader::cancel()
{
    if (this->d->m_active)
        this->d->fail(Canceled, tr("Download canceled"));
}

GitHubAssetDownloaderPrivate::GitHubAssetDownloaderPrivate(GitHubAssetDownloader *self):
    self(self )
{

}

int GitHubAssetDownloaderPrivate::httpStatus(QNetworkReply *reply)
{
    return reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
}

bool GitHubAssetDownloaderPrivate::isRedirect(int status)
{
    return status == 301
            || status == 302
            || status == 303
            || status == 307
            || status == 308;
}

bool GitHubAssetDownloaderPrivate::begin(const QUrl &apiUrl)
{
    this->m_hash.reset();
    this->m_received = 0;
    this->m_redirects = 0;
    this->m_buffer.clear();

    if (!apiUrl.isValid() || apiUrl.isEmpty()) {
        this->failLater(GitHubAssetDownloader::NetworkError,
                        QObject::tr("Invalid download address"));

        return false;
    }

    if (this->m_auth->accessToken().isEmpty()) {
        this->failLater(GitHubAssetDownloader::NotAuthorized,
                        QObject::tr("You are not signed in"));

        return false;
    }

    return true;
}

void GitHubAssetDownloaderPrivate::request(const QUrl &url, bool withAuth)
{
    QNetworkRequest request(url);
    // The redirection is followed manually to not resend the token.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setRawHeader("User-Agent", "Webcamoid");

    if (withAuth) {
        request.setRawHeader("Accept", "application/octet-stream");
        request.setRawHeader("Authorization",
                             "Bearer " + this->m_auth->accessToken().toUtf8());
        request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    }

    auto reply = this->m_manager->get(request);
    this->m_reply = reply;
    QObject::connect(reply,
                     &QNetworkReply::readyRead,
                     self,
                     [this, reply] () { this->consume(reply); });
    QObject::connect(reply,
                     &QNetworkReply::downloadProgress,
                     self,
                     [this, reply] (qint64 received, qint64 total) {
        if (reply == this->m_reply && httpStatus(reply) == 200)
            emit self->progress(received, total);
    });
    QObject::connect(reply,
                     &QNetworkReply::finished,
                     self,
                     [this, reply] () { this->onFinished(reply); });
}

void GitHubAssetDownloaderPrivate::consume(QNetworkReply *reply)
{
    if (reply != this->m_reply)
        return;

    auto data = reply->readAll();

    // The bodies of redirections and errors are discarded.
    if (httpStatus(reply) != 200 || data.isEmpty())
        return;

    this->m_hash.addData(data);
    this->m_received += data.size();

    if (this->m_toFile) {
        if (this->m_file.write(data) != data.size())
            this->fail(GitHubAssetDownloader::FileError,
                       QObject::tr("Cannot write the downloaded file: %1")
                       .arg(this->m_file.errorString()));
    } else {
        this->m_buffer += data;

        if (this->m_buffer.size() > MAX_MEMORY_DOWNLOAD)
            this->fail(GitHubAssetDownloader::NetworkError,
                       QObject::tr("The file is too large"));
    }
}

void GitHubAssetDownloaderPrivate::onFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply != this->m_reply)
        return; // canceled or already failed

    this->consume(reply);

    if (!this->m_active)
        return; // consume() failed

    this->m_reply = nullptr;
    auto status = httpStatus(reply);

    if (isRedirect(status)) {
        // Read the raw header: the URL can be relative to the request.
        auto location = QUrl::fromEncoded(reply->rawHeader("Location"));
        auto target = location.isEmpty()? QUrl(): reply->url().resolved(location);

        if (++this->m_redirects > MAX_REDIRECTS || !target.isValid()) {
            this->fail(GitHubAssetDownloader::NetworkError,
                       QObject::tr("Invalid redirection"));

            return;
        }

        if (target.scheme() != "https") {
            this->fail(GitHubAssetDownloader::NetworkError,
                       QObject::tr("Refusing an insecure redirection"));

            return;
        }

        // The target URL is already signed: no credentials.
        this->request(target, false);

        return;
    }

    if (status == 401 || status == 403 || status == 404) {
        this->fail(GitHubAssetDownloader::NotAuthorized,
                   QObject::tr("Your account has no access to this build"));

        return;
    }

    if (status != 200 || reply->error() != QNetworkReply::NoError) {
        this->fail(GitHubAssetDownloader::NetworkError, reply->errorString());

        return;
    }

    if (this->m_expectedSize > 0 && this->m_received != this->m_expectedSize) {
        this->fail(GitHubAssetDownloader::SizeMismatch,
                   QObject::tr("The downloaded file has an unexpected size"));

        return;
    }

    auto sha256 = QString::fromLatin1(this->m_hash.result().toHex());

    if (!this->m_expectedSha256.isEmpty()
        && sha256 != this->m_expectedSha256) {
        this->fail(GitHubAssetDownloader::ChecksumMismatch,
                   QObject::tr("The downloaded file is corrupt (checksum mismatch)"));

        return;
    }

    if (this->m_toFile) {
        this->m_file.close();
        auto partPath = this->m_file.fileName();
        QFile::remove(this->m_path);

        if (!QFile::rename(partPath, this->m_path)) {
            this->fail(GitHubAssetDownloader::FileError,
                       QObject::tr("Cannot save the file as %1").arg(this->m_path));

            return;
        }

        this->m_active = false;
        emit self->finishedFile(this->m_path);
    } else {
        auto data = this->m_buffer;
        this->m_buffer.clear();
        this->m_active = false;
        emit self->finishedMemory(data);
    }
}

void GitHubAssetDownloaderPrivate::fail(GitHubAssetDownloader::Error error,
                                        const QString &message)
{
    auto reply = this->m_reply;
    this->m_reply = nullptr;

    if (reply)
        reply->abort(); // onFinished() ignores it: it is no longer m_reply

    this->cleanup();
    emit self->failed(error, message);
}

void GitHubAssetDownloaderPrivate::failLater(GitHubAssetDownloader::Error error,
                                             const QString &message)
{
    // Errors detected before starting are emitted in the next event loop
    // iteration, so the signal never arrives before the caller returns.
    QMetaObject::invokeMethod(self,
                              [this, error, message] () {
                                  emit self->failed(error, message);
                              },
                              Qt::QueuedConnection);
}

void GitHubAssetDownloaderPrivate::cleanup()
{
    if (this->m_file.isOpen())
        this->m_file.close();

    if (this->m_toFile)
        QFile::remove(this->m_file.fileName()); // removes the .part file

    this->m_buffer.clear();
    this->m_active = false;
}

#include "moc_githubassetdownloader.cpp"

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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSysInfo>

#include "githubdailybuilds.h"
#include "githubauth.h"

#define RELEASE_PATH "/repos/webcamoid/WebcamoidPrivate/releases/tags/daily-build"

class GitHubDailyBuildsPrivate
{
    public:
        GitHubDailyBuilds *self;
        GitHubAuth *m_auth;
        QNetworkAccessManager *m_manager;
        QNetworkReply *m_reply {nullptr};
        QString m_os;
        QString m_arch;
        QByteArray m_etag;
        GitHubDailyBuilds::BuildInfo m_last;
        GitHubAssetDownloaderPtr m_sidecar;
        GitHubDailyBuilds::BuildInfo m_pending;
        QString m_apiBase {"https://api.github.com"};
        bool m_waitingSidecar {false};
        bool m_waitingRefresh {false};
        bool m_retried {false};

        explicit GitHubDailyBuildsPrivate(GitHubDailyBuilds *self);
        void failLater(GitHubDailyBuilds::Error error, const QString &message);
        void requestRelease();
        void onReply(QNetworkReply *reply);
        void finishCheck(const GitHubDailyBuilds::BuildInfo &build);
        void onSidecarData(const QByteArray &data);
        void onSidecarFailed();
        void onTokenRefreshed();
        void onRefreshFailed(const QString &error);
};

GitHubDailyBuilds::GitHubDailyBuilds(GitHubAuth *auth,
                                     QNetworkAccessManager *manager,
                                     QObject *parent):
    QObject(parent)
{
    this->d = new GitHubDailyBuildsPrivate(this);
    this->d->m_auth = auth;
    this->d->m_manager = manager;
    this->d->m_os = currentOs();
    this->d->m_arch = currentArch();
    this->d->m_sidecar = GitHubAssetDownloaderPtr::create(auth, manager);

    QObject::connect(this->d->m_sidecar.data(),
                     &GitHubAssetDownloader::finishedMemory,
                     this,
                     [this] (const QByteArray &data) {
                        this->d->onSidecarData(data);
                     });
    QObject::connect(this->d->m_sidecar.data(),
                     &GitHubAssetDownloader::failed,
                     this,
                     [this] () { this->d->onSidecarFailed(); });
    QObject::connect(auth,
                     &GitHubAuth::tokenRefreshed,
                     this,
                     [this] () { this->d->onTokenRefreshed(); });
    QObject::connect(auth,
                     &GitHubAuth::refreshFailed,
                     this,
                     [this] (const QString &error) {
                        this->d->onRefreshFailed(error);
                     });
}

GitHubDailyBuilds::~GitHubDailyBuilds()
{
    delete this->d;
}

QString GitHubDailyBuilds::currentOs()
{
    // Android also defines Q_OS_LINUX, so it must be checked first.
#if defined(Q_OS_ANDROID)
    return "android";
#elif defined(Q_OS_WIN)
    return "windows";
#elif defined(Q_OS_MACOS)
    return "macos";
#elif defined(Q_OS_LINUX)
    return "linux";
#else
    return {};
#endif
}

QString GitHubDailyBuilds::currentArch()
{
    return QSysInfo::buildCpuArchitecture();
}

QString GitHubDailyBuilds::assetNameForPlatform(const QString &os,
                                                const QString &arch)
{
    if (os == "windows" && arch == "x86_64")
        return "webcamoid-installer-windows-daily-master-win64.exe";

    if (os == "linux" && arch == "x86_64")
        return "webcamoid-portable-linux-daily-master-x64.AppImage";

    if (os == "linux" && arch == "arm64")
        return "webcamoid-portable-linux-daily-master-arm64.AppImage";

    if (os == "macos")
        return "webcamoid-installer-mac-daily-master.run";

    if (os == "android")
        return "webcamoid-android-daily-master.apk";

    return {};
}

bool GitHubDailyBuilds::parseRelease(const QJsonObject &release,
                                     const QString &assetName,
                                     BuildInfo *info)
{
    auto assets = release.value("assets").toArray();

    for (auto asset: assets) {
        auto obj = asset.toObject();

        if (obj.value("name").toString() != assetName)
            continue;

        // Ignore incomplete uploads (the assets are replaced on every build).
        if (obj.value("state").toString() != "uploaded")
            continue;

        BuildInfo build;
        build.name = assetName;
        build.assetId = obj.value("id").toInteger();
        build.size = obj.value("size").toInteger();

        auto digest = obj.value("digest").toString();

        if (digest.startsWith("sha256:"))
            build.sha256 = digest.mid(7);

        build.updatedAt =
            QDateTime::fromString(obj.value("updated_at").toString(),
                                  Qt::ISODate);
        build.apiUrl = QUrl(obj.value("url").toString());

        if (!build.isValid())
            return false;

        for (auto other: assets) {
            auto otherObj = other.toObject();

            if (otherObj.value("name").toString() == assetName + ".json"
                && otherObj.value("state").toString() == "uploaded") {
                build.sidecarUrl = QUrl(otherObj.value("url").toString());

                break;
            }
        }

        if (info)
            *info = build;

        return true;
    }

    return false;
}

bool GitHubDailyBuilds::applySidecar(const QByteArray &json, BuildInfo *info)
{
    auto doc = QJsonDocument::fromJson(json);

    if (!doc.isObject())
        return false;

    auto obj = doc.object();
    auto commit = obj.value("commit").toString();
    static const QRegularExpression hex {"^[0-9a-fA-F]{7,64}$"};

    if (!hex.match(commit).hasMatch())
        return false;

    info->commit = commit.toLower();
    info->builtAt = QDateTime::fromString(obj.value("built_at").toString(),
                                          Qt::ISODate);

    return true;
}

GitHubDailyBuilds::UpdateStatus
GitHubDailyBuilds::updateStatus(const BuildInfo &build,
                                const QString &runningCommit,
                                bool runningIsDailyBuild)
{
    // It can only tell "up to date" or "another build" if the running binary is
    // a daily build with a known commit and the asset has its commit. The commits
    // are not ordered: different means "a build is available".
    if (!runningIsDailyBuild
        || runningCommit.size() < 7
        || build.commit.size() < 7)
        return Unknown;

    auto a = runningCommit.toLower();
    auto b = build.commit.toLower();
    auto n = qMin(a.size(), b.size());

    return a.left(n) == b.left(n)? UpToDate: UpdateAvailable;
}

QString GitHubDailyBuilds::assetName() const
{
    return assetNameForPlatform(this->d->m_os, this->d->m_arch);
}

void GitHubDailyBuilds::check()
{
    if (this->d->m_reply
        || this->d->m_waitingRefresh
        || this->d->m_waitingSidecar)
        return; // a request is already in progress

    this->d->m_retried = false;

    if (this->assetName().isEmpty()) {
        this->d->failLater(UnsupportedPlatform,
                           tr("Daily builds are not available for this platform"));

        return;
    }

    if (!this->d->m_auth->accessTokenValid()) {
        if (this->d->m_auth->canRefresh()) {
            this->d->m_waitingRefresh = true;
            this->d->m_auth->refresh();
        } else {
            this->d->failLater(NotSignedIn, tr("You are not signed in"));
        }

        return;
    }

    this->d->requestRelease();
}

GitHubDailyBuildsPrivate::GitHubDailyBuildsPrivate(GitHubDailyBuilds *self):
    self(self)
{

}

void GitHubDailyBuildsPrivate::failLater(GitHubDailyBuilds::Error error,
                                         const QString &message)
{
    // Errors detected inside check() are emitted in the next event loop
    // iteration, so the signal never arrives before check() returns.
    QMetaObject::invokeMethod(self,
                              [this, error, message] () {
                                  emit self->failed(error, message);
                              },
                              Qt::QueuedConnection);
}

void GitHubDailyBuildsPrivate::requestRelease()
{
    QNetworkRequest request {QUrl(this->m_apiBase + RELEASE_PATH)};
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("Authorization",
                         "Bearer " + this->m_auth->accessToken().toUtf8());
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setRawHeader("User-Agent", "Webcamoid");

    // With an ETag, if nothing changed GitHub answers 304 and the request does
    // not count against the rate limit.
    if (!this->m_etag.isEmpty() && this->m_last.isValid())
        request.setRawHeader("If-None-Match", this->m_etag);

    auto reply = this->m_manager->get(request);
    this->m_reply = reply;
    QObject::connect(reply,
                     &QNetworkReply::finished,
                     self,
                     [this, reply] () { this->onReply(reply); });
}

void GitHubDailyBuildsPrivate::onReply(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply != this->m_reply)
        return;

    this->m_reply = nullptr;
    auto status =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (status == 304 && this->m_last.isValid()) {
        emit self->buildAvailable(this->m_last);

        return;
    }

    if (status == 200) {
        auto doc = QJsonDocument::fromJson(reply->readAll());
        GitHubDailyBuilds::BuildInfo build;

        if (!doc.isObject()) {
            emit self->failed(GitHubDailyBuilds::InvalidResponse,
                              QObject::tr("Invalid response from GitHub"));

            return;
        }

        if (!self->parseRelease(doc.object(), self->assetName(), &build)) {
            emit self->failed(GitHubDailyBuilds::AssetNotFound,
                              QObject::tr("The daily build for this platform is not available right now"));

            return;
        }

        this->m_etag = reply->rawHeader("ETag");

        if (build.sidecarUrl.isValid()) {
            this->m_pending = build;
            this->m_waitingSidecar = true;
            this->m_sidecar->downloadToMemory(build.sidecarUrl);
        } else {
            this->finishCheck(build);
        }

        return;
    }

    if (status == 401) {
        // Token rejected (revoked or expired): try to renew it once.
        this->m_etag.clear();
        this->m_last = {};

        if (!this->m_retried && this->m_auth->canRefresh()) {
            this->m_retried = true;
            this->m_waitingRefresh = true;
            this->m_auth->refresh();
        } else {
            emit self->failed(GitHubDailyBuilds::SessionExpired,
                              QObject::tr("Your GitHub session expired, please sign in again"));
        }

        return;
    }

    if (status == 403 || status == 429) {
        if (reply->rawHeader("X-RateLimit-Remaining") == "0")
            emit self->failed(GitHubDailyBuilds::RateLimited,
                              QObject::tr("GitHub rate limit reached, try again later"));
        else
            emit self->failed(GitHubDailyBuilds::NoAccess,
                              QObject::tr("Your account has no access to the daily builds"));

        return;
    }

    if (status == 404) {
        // GitHub answers 404 (not 403) to who cannot see a private repository.
        emit self->failed(GitHubDailyBuilds::NoAccess,
                          QObject::tr("Your account has no access to the daily builds"));

        return;
    }

    emit self->failed(GitHubDailyBuilds::NetworkError, reply->errorString());
}

void GitHubDailyBuildsPrivate::finishCheck(const GitHubDailyBuilds::BuildInfo &build)
{
    this->m_last = build;
    emit self->buildAvailable(build);
}

void GitHubDailyBuildsPrivate::onSidecarData(const QByteArray &data)
{
    if (!this->m_waitingSidecar)
        return;

    this->m_waitingSidecar = false;
    self->applySidecar(data, &this->m_pending); // if invalid, the build has no commit
    this->finishCheck(this->m_pending);
}

void GitHubDailyBuildsPrivate::onSidecarFailed()
{
    if (!this->m_waitingSidecar)
        return;

    this->m_waitingSidecar = false;
    // Without a commit the status is Unknown; the ETag is cleared to request the
    // file again in the next check.
    this->m_etag.clear();
    this->finishCheck(this->m_pending);
}

void GitHubDailyBuildsPrivate::onTokenRefreshed()
{
    if (!this->m_waitingRefresh)
        return;

    this->m_waitingRefresh = false;
    this->requestRelease();
}

void GitHubDailyBuildsPrivate::onRefreshFailed(const QString &error)
{
    if (!this->m_waitingRefresh)
        return;

    this->m_waitingRefresh = false;

    // If GitHub rejected the refresh token, GitHubAuth removed the tokens and the
    // user must sign in again; on a network failure, retry later.
    if (!this->m_auth->canRefresh() && !this->m_auth->accessTokenValid())
        emit self->failed(GitHubDailyBuilds::SessionExpired,
                          QObject::tr("Your GitHub session expired, please sign in again"));
    else
        emit self->failed(GitHubDailyBuilds::NetworkError, error);
}

#include "moc_githubdailybuilds.cpp"

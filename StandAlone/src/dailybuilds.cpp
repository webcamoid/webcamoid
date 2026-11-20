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

#include <QDateTime>
#include <QDir>
#include <QNetworkAccessManager>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QtQml>

#include "dailybuilds.h"
#include "dailybuildinstaller.h"
#include "githubassetdownloader.h"
#include "githubauth.h"
#include "githubdailybuilds.h"
#include "updates.h"

#define CHECK_TIMER_MS (10 * 60 * 1000) // same as Updates

class DailyBuildsPrivate
{
    public:
        DailyBuilds *self;
        Updates *m_updates {nullptr};
        QQmlApplicationEngine *m_engine {nullptr};
        QNetworkAccessManager m_manager;
        GitHubAuth m_auth;
        GitHubDailyBuilds m_builds;
        GitHubAssetDownloader m_downloader;
        QTimer m_timer;
        DailyBuilds::Channel m_channel {DailyBuilds::Stable};
        DailyBuilds::State m_state {DailyBuilds::SignedOut};
        DailyBuilds::Error m_error {DailyBuilds::NoError};
        QString m_errorMessage;
        QString m_userCode;
        QUrl m_verificationUri;
        QDateTime m_codeExpiresAt;
        GitHubDailyBuilds::BuildInfo m_build;
        DailyBuilds::BuildStatus m_status {DailyBuilds::Unknown};
        QString m_skipped;
        QString m_announced;
        QDateTime m_lastCheck;
        qreal m_progress {0.0};
        QString m_downloadedFile;
        bool m_downloadAfterRefresh {false};

        DailyBuildsPrivate(DailyBuilds *self, Updates *updates);
        static QString appClientId();
        static QString runningCommit();
        static bool runningIsDailyBuild();
        bool available() const;
        bool notifyEnabled() const;
        int checkIntervalDays() const;
        QString skipKey(const GitHubDailyBuilds::BuildInfo &build) const;
        void setState(DailyBuilds::State state);
        void setError(DailyBuilds::Error error, const QString &message);
        void clearError();
        void loadSettings();
        void saveChannel() const;
        void saveSkipped() const;
        void saveLastCheck() const;
        void autoCheck();
        void startDownload();
        void onBuildAvailable(const GitHubDailyBuilds::BuildInfo &build);
        void onCheckFailed(GitHubDailyBuilds::Error error,
                           const QString &message);
        void onDownloadFailed(GitHubAssetDownloader::Error error,
                              const QString &message);
};

DailyBuilds::DailyBuilds(Updates *updates,
                         QQmlApplicationEngine *engine,
                         QObject *parent):
    QObject(parent)
{
    this->d = new DailyBuildsPrivate(this, updates);
    this->setQmlEngine(engine);
}

DailyBuilds::~DailyBuilds()
{
    delete this->d;
}

bool DailyBuilds::isAvailable() const
{
    return this->d->available();
}

DailyBuilds::Channel DailyBuilds::channel() const
{
    return this->d->m_channel;
}

DailyBuilds::State DailyBuilds::state() const
{
    return this->d->m_state;
}

QString DailyBuilds::userCode() const
{
    return this->d->m_userCode;
}

QUrl DailyBuilds::verificationUri() const
{
    return this->d->m_verificationUri;
}

QDateTime DailyBuilds::codeExpiresAt() const
{
    return this->d->m_codeExpiresAt;
}

bool DailyBuilds::hasBuild() const
{
    return this->d->m_build.isValid();
}

QString DailyBuilds::buildName() const
{
    return this->d->m_build.name;
}

QString DailyBuilds::buildCommit() const
{
    return this->d->m_build.commit.left(7);
}

QDateTime DailyBuilds::buildDate() const
{
    return this->d->m_build.builtAt.isValid()?
                this->d->m_build.builtAt:
                this->d->m_build.updatedAt;
}

qint64 DailyBuilds::buildSize() const
{
    return this->d->m_build.size;
}

DailyBuilds::BuildStatus DailyBuilds::buildStatus() const
{
    return this->d->m_status;
}

bool DailyBuilds::buildSkipped() const
{
    return this->d->m_build.isValid()
           && !this->d->m_skipped.isEmpty()
           && this->d->m_skipped == this->d->skipKey(this->d->m_build);
}

qreal DailyBuilds::progress() const
{
    return this->d->m_progress;
}

QString DailyBuilds::downloadedFile() const
{
    return this->d->m_downloadedFile;
}

QUrl DailyBuilds::downloadFolder() const
{
    return QUrl::fromLocalFile(DailyBuildInstaller::downloadDir());
}

DailyBuilds::InstallKind DailyBuilds::installKind() const
{
    switch (DailyBuildInstaller::kindForOs(GitHubDailyBuilds::currentOs())) {
    case DailyBuildInstaller::RunsInstaller:
        return InstallRunsInstaller;
    case DailyBuildInstaller::StandaloneExecutable:
        return InstallStandalone;
    default:
        return InstallUnsupported;
    }
}

DailyBuilds::Error DailyBuilds::error() const
{
    return this->d->m_error;
}

QString DailyBuilds::errorMessage() const
{
    return this->d->m_errorMessage;
}

QDateTime DailyBuilds::lastCheck() const
{
    return this->d->m_lastCheck;
}

void DailyBuilds::setChannel(Channel channel)
{
    if (this->d->m_channel == channel)
        return;

    this->d->m_channel = channel;
    this->d->saveChannel();
    emit this->channelChanged(channel);

    // When selecting daily while signed in, check right away.
    if (channel == Daily
        && this->d->m_state == SignedIn
        && !this->d->m_build.isValid())
        this->checkNow();
}

void DailyBuilds::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine) {
        engine->rootContext()->setContextProperty("dailyBuilds", this);
        qmlRegisterType<DailyBuilds>("Webcamoid", 1, 0, "DailyBuilds");
    }
}

void DailyBuilds::start()
{
    if (!this->d->available())
        return;

    this->d->m_timer.start();
    QTimer::singleShot(0, this, [this] () { this->d->autoCheck(); });
}

void DailyBuilds::signIn()
{
    if (!this->d->available()
        || this->d->m_state == SigningIn
        || this->d->m_state == SignedIn)
        return;

    this->d->clearError();
    this->d->m_userCode.clear();
    this->d->m_auth.login();
    this->d->setState(SigningIn);
}

void DailyBuilds::cancelSignIn()
{
    if (this->d->m_state != SigningIn)
        return;

    this->d->m_auth.cancelLogin();
    this->d->m_userCode.clear();
    emit this->userCodeChanged();
    this->d->setState(SignedOut);
}

void DailyBuilds::signOut()
{
    this->d->m_downloader.cancel();
    this->d->m_auth.logout();
    this->d->m_build = {};
    this->d->m_status = Unknown;
    this->d->m_downloadAfterRefresh = false;
    this->d->m_userCode.clear();
    this->d->m_downloadedFile.clear();
    this->d->m_progress = 0.0;
    this->d->clearError();
    emit this->buildChanged();
    emit this->userCodeChanged();
    emit this->downloadedFileChanged({});
    emit this->progressChanged(0.0);
    this->d->setState(SignedOut);
}

void DailyBuilds::checkNow()
{
    if (!this->d->available() || this->d->m_state != SignedIn)
        return;

    this->d->clearError();
    this->d->setState(Checking);
    this->d->m_builds.check();
}

void DailyBuilds::download()
{
    if (this->d->m_state != SignedIn || !this->d->m_build.isValid())
        return;

    this->d->clearError();
    this->d->m_progress = 0.0;
    emit this->progressChanged(0.0);
    this->d->setState(Downloading);

    if (this->d->m_auth.accessTokenValid()) {
        this->d->startDownload();
    } else if (this->d->m_auth.canRefresh()) {
        // The token may have expired since the last check.
        this->d->m_downloadAfterRefresh = true;
        this->d->m_auth.refresh();
    } else {
        this->d->setError(SessionExpired,
                          tr("Your GitHub session expired, please sign in again"));
        this->d->setState(SignedOut);
    }
}

void DailyBuilds::cancelDownload()
{
    this->d->m_downloadAfterRefresh = false;

    if (this->d->m_downloader.busy())
        this->d->m_downloader.cancel();
    else if (this->d->m_state == Downloading)
        this->d->setState(SignedIn);
}

void DailyBuilds::skipBuild()
{
    if (!this->d->m_build.isValid())
        return;

    this->d->m_skipped = this->d->skipKey(this->d->m_build);
    this->d->saveSkipped();
    emit this->buildChanged();
}

bool DailyBuilds::install()
{
    if (this->d->m_state != Downloaded || this->d->m_downloadedFile.isEmpty())
        return false;

    QString error;

    if (!DailyBuildInstaller::startInstaller(this->d->m_downloadedFile,
                                             &error)) {
        this->d->setError(DownloadFailed, error);

        return false;
    }

    emit this->installerStarted();

    return true;
}

void DailyBuilds::resetError()
{
    this->d->clearError();
}

DailyBuildsPrivate::DailyBuildsPrivate(DailyBuilds *self, Updates *updates):
    self(self),
    m_updates(updates),
    m_auth(appClientId(), &this->m_manager),
    m_builds(&this->m_auth, &this->m_manager),
    m_downloader(&this->m_auth, &this->m_manager)
{
    this->m_timer.setInterval(CHECK_TIMER_MS);
    QObject::connect(&this->m_timer,
                     &QTimer::timeout,
                     this->self,
                     [this] () { this->autoCheck(); });

    // Sign in
    QObject::connect(&this->m_auth,
                     &GitHubAuth::deviceCodeReady,
                     this->self,
                     [this] (const QString &userCode,
                             const QUrl &verificationUri,
                             int expiresInSecs) {
        this->m_userCode = userCode;
        this->m_verificationUri = verificationUri;
        this->m_codeExpiresAt =
            QDateTime::currentDateTime().addSecs(expiresInSecs);
        emit this->self->userCodeChanged();
    });
    QObject::connect(&this->m_auth,
                     &GitHubAuth::loggedIn,
                     this->self,
                     [this] () {
        this->m_userCode.clear();
        emit this->self->userCodeChanged();
        this->setState(DailyBuilds::SignedIn);

        if (this->m_channel == DailyBuilds::Daily)
            this->self->checkNow();
    });
    QObject::connect(&this->m_auth,
                     &GitHubAuth::loginFailed,
                     this->self,
                     [this] (const QString &error) {
        this->m_userCode.clear();
        emit this->self->userCodeChanged();
        this->setError(DailyBuilds::SignInFailed, error);
        this->setState(DailyBuilds::SignedOut);
    });

    // Token renewal before downloading
    QObject::connect(&this->m_auth,
                     &GitHubAuth::tokenRefreshed,
                     this->self,
                     [this] () {
        if (this->m_downloadAfterRefresh) {
            this->m_downloadAfterRefresh = false;
            this->startDownload();
        }
    });
    QObject::connect(&this->m_auth,
                     &GitHubAuth::refreshFailed,
                     this->self,
                     [this] (const QString &error) {
        if (!this->m_downloadAfterRefresh)
            return;

        this->m_downloadAfterRefresh = false;

        if (this->m_auth.canRefresh() || this->m_auth.accessTokenValid()) {
            this->setError(DailyBuilds::NetworkError, error);
            this->setState(DailyBuilds::SignedIn);
        } else {
            this->setError(DailyBuilds::SessionExpired,
                           DailyBuilds::tr("Your GitHub session expired, please sign in again"));
            this->setState(DailyBuilds::SignedOut);
        }
    });

    // Check
    QObject::connect(&this->m_builds,
                     &GitHubDailyBuilds::buildAvailable,
                     this->self,
                     [this] (const GitHubDailyBuilds::BuildInfo &build) {
        this->onBuildAvailable(build);
    });
    QObject::connect(&this->m_builds,
                     &GitHubDailyBuilds::failed,
                     this->self,
                     [this] (GitHubDailyBuilds::Error error,
                             const QString &message) {
        this->onCheckFailed(error, message);
    });

    // Download
    QObject::connect(&this->m_downloader,
                     &GitHubAssetDownloader::progress,
                     this->self,
                     [this] (qint64 received, qint64 total) {
        this->m_progress = total > 0? qreal(received) / qreal(total): 0.0;
        emit this->self->progressChanged(this->m_progress);
    });
    QObject::connect(&this->m_downloader,
                     &GitHubAssetDownloader::finishedFile,
                     this->self,
                     [this] (const QString &path) {
        // The AppImage and the macOS installers must be executable.
        QString error;
        DailyBuildInstaller::makeExecutable(path, &error);

        // Set the state first so that the handlers of the next signals (for example
        // signing out) are not overwritten afterwards.
        this->m_downloadedFile = path;
        this->m_progress = 1.0;
        this->setState(DailyBuilds::Downloaded);
        emit this->self->progressChanged(1.0);
        emit this->self->downloadedFileChanged(path);
    });
    QObject::connect(&this->m_downloader,
                     &GitHubAssetDownloader::failed,
                     this->self,
                     [this] (GitHubAssetDownloader::Error error,
                             const QString &message) {
        this->onDownloadFailed(error, message);
    });

    this->loadSettings();
    this->m_auth.loadStoredTokens();

    if (this->available()
        && (this->m_auth.accessTokenValid() || this->m_auth.canRefresh()))
        this->m_state = DailyBuilds::SignedIn;
}

QString DailyBuildsPrivate::appClientId()
{
#ifdef GITHUB_APP_CLIENT_ID
    return QStringLiteral(GITHUB_APP_CLIENT_ID);
#else
    return {};
#endif
}

QString DailyBuildsPrivate::runningCommit()
{
#ifdef GIT_COMMIT_HASH
    return QStringLiteral(GIT_COMMIT_HASH);
#else
    return {};
#endif
}

bool DailyBuildsPrivate::runningIsDailyBuild()
{
#ifdef DAILY_BUILD
    return true;
#else
    return false;
#endif
}

bool DailyBuildsPrivate::available() const
{
    return !appClientId().isEmpty()
           && !this->m_builds.assetName().isEmpty();
}

bool DailyBuildsPrivate::notifyEnabled() const
{
    return this->m_updates? this->m_updates->notifyNewVersion(): true;
}

int DailyBuildsPrivate::checkIntervalDays() const
{
    return this->m_updates? this->m_updates->checkInterval(): 1;
}

QString DailyBuildsPrivate::skipKey(const GitHubDailyBuilds::BuildInfo &build) const
{
    return build.commit.isEmpty()? build.sha256: build.commit;
}

void DailyBuildsPrivate::setState(DailyBuilds::State state)
{
    if (this->m_state == state)
        return;

    this->m_state = state;
    emit this->self->stateChanged(state);
}

void DailyBuildsPrivate::setError(DailyBuilds::Error error,
                                  const QString &message)
{
    this->m_error = error;
    this->m_errorMessage = message;
    emit this->self->errorChanged();
}

void DailyBuildsPrivate::clearError()
{
    if (this->m_error == DailyBuilds::NoError
        && this->m_errorMessage.isEmpty())
        return;

    this->m_error = DailyBuilds::NoError;
    this->m_errorMessage.clear();
    emit this->self->errorChanged();
}

void DailyBuildsPrivate::loadSettings()
{
    QSettings config;
    config.beginGroup("DailyBuilds");
    // Daily builds follow the daily channel by default.
    auto defaultChannel = runningIsDailyBuild()?
                              DailyBuilds::Daily:
                              DailyBuilds::Stable;
    this->m_channel =
        config.value("channel", int(defaultChannel)).toInt() == DailyBuilds::Daily?
            DailyBuilds::Daily:
            DailyBuilds::Stable;
    this->m_skipped = config.value("skipped").toString();
    this->m_lastCheck = config.value("lastCheck").toDateTime();
    config.endGroup();
}

void DailyBuildsPrivate::saveChannel() const
{
    QSettings config;
    config.beginGroup("DailyBuilds");
    config.setValue("channel", int(this->m_channel));
    config.endGroup();
}

void DailyBuildsPrivate::saveSkipped() const
{
    QSettings config;
    config.beginGroup("DailyBuilds");
    config.setValue("skipped", this->m_skipped);
    config.endGroup();
}

void DailyBuildsPrivate::saveLastCheck() const
{
    QSettings config;
    config.beginGroup("DailyBuilds");
    config.setValue("lastCheck", this->m_lastCheck);
    config.endGroup();
}

void DailyBuildsPrivate::autoCheck()
{
    auto days = this->checkIntervalDays();

    if (!this->available()
        || (this->m_updates && !this->m_updates->isEnabled())
        || this->m_channel != DailyBuilds::Daily
        || this->m_state != DailyBuilds::SignedIn
        || !this->notifyEnabled()
        || days < 1)
        return;

    if (!this->m_lastCheck.isValid()
        || this->m_lastCheck.daysTo(QDateTime::currentDateTime()) >= days)
        this->self->checkNow();
}

void DailyBuildsPrivate::startDownload()
{
    auto destination =
        QDir(DailyBuildInstaller::downloadDir()).filePath(this->m_build.name);
    this->m_downloader.downloadToFile(this->m_build.apiUrl,
                                      destination,
                                      this->m_build.size,
                                      this->m_build.sha256);
}

void DailyBuildsPrivate::onBuildAvailable(const GitHubDailyBuilds::BuildInfo &build)
{
    this->m_build = build;
    this->m_status =
        DailyBuilds::BuildStatus(GitHubDailyBuilds::updateStatus(build,
                                                                 runningCommit(),
                                                                 runningIsDailyBuild()));
    this->m_lastCheck = QDateTime::currentDateTime();
    this->saveLastCheck();

    if (this->m_state == DailyBuilds::Checking)
        this->setState(DailyBuilds::SignedIn);

    emit this->self->buildChanged();
    emit this->self->lastCheckChanged();

    // Notify only when it is certain that there is another build, the user did
    // not skip it and it was not already notified.
    auto key = this->skipKey(build);

    if (this->m_status == DailyBuilds::UpdateAvailable
        && !this->self->buildSkipped()
        && key != this->m_announced
        && this->notifyEnabled()) {
        this->m_announced = key;
        emit this->self->newBuildAvailable(build.commit.left(7));
    }
}

void DailyBuildsPrivate::onCheckFailed(GitHubDailyBuilds::Error error,
                                       const QString &message)
{
    DailyBuilds::Error code = DailyBuilds::NetworkError;
    bool signedOut = false;

    switch (error) {
    case GitHubDailyBuilds::NotSignedIn:
        code = DailyBuilds::NoError;
        signedOut = true;

        break;
    case GitHubDailyBuilds::SessionExpired:
        code = DailyBuilds::SessionExpired;
        signedOut = true;
        // The stored tokens are useless: remove them to not repeat the error on startup.
        this->m_auth.logout();

        break;
    case GitHubDailyBuilds::NoAccess:
        code = DailyBuilds::NoAccess;

        break;
    case GitHubDailyBuilds::RateLimited:
        code = DailyBuilds::RateLimited;

        break;
    case GitHubDailyBuilds::UnsupportedPlatform:
        code = DailyBuilds::UnsupportedPlatform;

        break;
    case GitHubDailyBuilds::AssetNotFound:
        code = DailyBuilds::BuildNotAvailable;

        break;
    default:
        break;
    }

    if (code != DailyBuilds::NoError)
        this->setError(code, message);

    if (this->m_state == DailyBuilds::Checking
        || this->m_state == DailyBuilds::SignedIn
        || signedOut)
        this->setState(signedOut?
                           DailyBuilds::SignedOut:
                           DailyBuilds::SignedIn);
}

void DailyBuildsPrivate::onDownloadFailed(GitHubAssetDownloader::Error error,
                                          const QString &message)
{
    this->m_progress = 0.0;
    emit this->self->progressChanged(0.0);

    switch (error) {
    case GitHubAssetDownloader::Canceled:
        break;
    case GitHubAssetDownloader::NotAuthorized:
        this->setError(DailyBuilds::NoAccess, message);

        break;
    case GitHubAssetDownloader::SizeMismatch:
    case GitHubAssetDownloader::ChecksumMismatch:
        this->setError(DailyBuilds::DownloadCorrupt, message);

        break;
    case GitHubAssetDownloader::FileError:
        this->setError(DailyBuilds::DownloadFailed, message);

        break;
    default:
        this->setError(DailyBuilds::NetworkError, message);

        break;
    }

    this->setState(DailyBuilds::SignedIn);
}

#include "moc_dailybuilds.cpp"

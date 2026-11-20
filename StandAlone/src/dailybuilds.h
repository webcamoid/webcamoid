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

#ifndef DAILYBUILDS_H
#define DAILYBUILDS_H

#include <QObject>
#include <QSharedPointer>

class DailyBuilds;
class DailyBuildsPrivate;
class QQmlApplicationEngine;
class Updates;
class QDateTime;
class QUrl;

using DailyBuildsPtr = QSharedPointer<DailyBuilds>;

/* QML controller of the daily builds channel.
 *
 * It joins the GitHub sign-in, the release query, the verified download and
 * the final action of each platform. QML only binds to its properties.
 */
class DailyBuilds: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available
               READ isAvailable
               CONSTANT)
    Q_PROPERTY(Channel channel
               READ channel
               WRITE setChannel
               NOTIFY channelChanged)
    Q_PROPERTY(State state
               READ state
               NOTIFY stateChanged)
    Q_PROPERTY(QString userCode
               READ userCode
               NOTIFY userCodeChanged)
    Q_PROPERTY(QUrl verificationUri
               READ verificationUri
               NOTIFY userCodeChanged)
    Q_PROPERTY(QDateTime codeExpiresAt
               READ codeExpiresAt
               NOTIFY userCodeChanged)
    Q_PROPERTY(bool hasBuild
               READ hasBuild
               NOTIFY buildChanged)
    Q_PROPERTY(QString buildName
               READ buildName
               NOTIFY buildChanged)
    Q_PROPERTY(QString buildCommit
               READ buildCommit
               NOTIFY buildChanged)
    Q_PROPERTY(QDateTime buildDate
               READ buildDate
               NOTIFY buildChanged)
    Q_PROPERTY(qint64 buildSize
               READ buildSize
               NOTIFY buildChanged)
    Q_PROPERTY(BuildStatus buildStatus
               READ buildStatus
               NOTIFY buildChanged)
    Q_PROPERTY(bool buildSkipped
               READ buildSkipped
               NOTIFY buildChanged)
    Q_PROPERTY(qreal progress
               READ progress
               NOTIFY progressChanged)
    Q_PROPERTY(QString downloadedFile
               READ downloadedFile
               NOTIFY downloadedFileChanged)
    Q_PROPERTY(QUrl downloadFolder
               READ downloadFolder
               CONSTANT)
    Q_PROPERTY(InstallKind installKind
               READ installKind
               CONSTANT)
    Q_PROPERTY(Error error
               READ error
               NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage
               READ errorMessage
               NOTIFY errorChanged)
    Q_PROPERTY(QDateTime lastCheck
               READ lastCheck
               NOTIFY lastCheckChanged)

    public:
        enum Channel {
            Stable,
            Daily
        };
        Q_ENUM(Channel)

        enum State {
            SignedOut,
            SigningIn,    // waiting for the user to authorize the code
            SignedIn,     // signed in, no task in progress
            Checking,
            Downloading,
            Downloaded    // verified file, waiting for confirmation
        };
        Q_ENUM(State)

        // Same order as GitHubDailyBuilds::UpdateStatus.
        enum BuildStatus {
            Unknown,          // can only offer to download
            UpToDate,
            UpdateAvailable
        };
        Q_ENUM(BuildStatus)

        enum InstallKind {
            InstallUnsupported,
            InstallRunsInstaller,     // Windows and macOS
            InstallStandalone         // Linux: the AppImage is not installed
        };
        Q_ENUM(InstallKind)

        enum Error {
            NoError,
            SignInFailed,
            SessionExpired,    // the user must sign in again
            NoAccess,          // the account cannot see the daily builds
            RateLimited,
            NetworkError,
            UnsupportedPlatform,
            BuildNotAvailable,
            DownloadCorrupt,
            DownloadFailed
        };
        Q_ENUM(Error)

        DailyBuilds(Updates *updates=nullptr,
                    QQmlApplicationEngine *engine=nullptr,
                    QObject *parent=nullptr);
        ~DailyBuilds();

        Q_INVOKABLE bool isAvailable() const;
        Q_INVOKABLE Channel channel() const;
        Q_INVOKABLE State state() const;
        Q_INVOKABLE QString userCode() const;
        Q_INVOKABLE QUrl verificationUri() const;
        Q_INVOKABLE QDateTime codeExpiresAt() const;
        Q_INVOKABLE bool hasBuild() const;
        Q_INVOKABLE QString buildName() const;
        Q_INVOKABLE QString buildCommit() const;
        Q_INVOKABLE QDateTime buildDate() const;
        Q_INVOKABLE qint64 buildSize() const;
        Q_INVOKABLE BuildStatus buildStatus() const;
        Q_INVOKABLE bool buildSkipped() const;
        Q_INVOKABLE qreal progress() const;
        Q_INVOKABLE QString downloadedFile() const;
        Q_INVOKABLE QUrl downloadFolder() const;
        Q_INVOKABLE InstallKind installKind() const;
        Q_INVOKABLE Error error() const;
        Q_INVOKABLE QString errorMessage() const;
        Q_INVOKABLE QDateTime lastCheck() const;

    private:
        DailyBuildsPrivate *d;

    signals:
        void channelChanged(Channel channel);
        void stateChanged(State state);
        void userCodeChanged();
        void buildChanged();
        void progressChanged(qreal progress);
        void downloadedFileChanged(const QString &downloadedFile);
        void errorChanged();
        void lastCheckChanged();
        // A new build that the user did not skip is available.
        void newBuildAvailable(const QString &commit);
        // The installer was started: the application should close.
        void installerStarted();

    public slots:
        void setChannel(Channel channel);
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);
        void start();
        void signIn();
        void cancelSignIn();
        void signOut();
        void checkNow();
        void download();
        void cancelDownload();
        void skipBuild();
        bool install();      // Windows/macOS: runs the installer; Linux: runs the AppImage
        void resetError();

    friend class DailyBuildsPrivate;
};

#endif // DAILYBUILDS_H

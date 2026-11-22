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

#ifndef GITHUBDAILYBUILDS_H
#define GITHUBDAILYBUILDS_H

#include <QDateTime>
#include <QObject>
#include <QUrl>

#include "githubassetdownloader.h"

class GitHubDailyBuildsPrivate;
class GitHubAuth;
class QNetworkAccessManager;
class QNetworkReply;
class QByteArray;

/* Queries the "daily-build" release of the private sponsors repository,
 * selects the asset that matches the current platform and reads its build
 * information (commit) from the "<asset>.json" file that goes with it.
 *
 * The download is done by GitHubAssetDownloader.
 */
class GitHubDailyBuilds: public QObject
{
    Q_OBJECT

    public:
        enum Error {
            NotSignedIn,
            SessionExpired,
            NoAccess,
            RateLimited,
            UnsupportedPlatform,
            AssetNotFound,
            InvalidResponse,
            NetworkError
        };
        Q_ENUM(Error)

        enum UpdateStatus {
            Unknown,          // cannot be compared: can only offer to download
            UpToDate,
            UpdateAvailable,
            RunningNewer      // the running build is newer than the daily build
        };
        Q_ENUM(UpdateStatus)

        // How the commit of the available build (head) is related to the
        // commit of the running build (base).
        enum CommitOrder {
            OrderUnknown,     // GitHub does not know one of the commits
            OrderFailed,      // the request failed
            Identical,
            HeadAhead,        // the available build is newer
            HeadBehind,       // the running build is newer
            Diverged
        };
        Q_ENUM(CommitOrder)

        struct BuildInfo
        {
            QString name;
            qint64 assetId {0};
            qint64 size {0};
            QString sha256;       // hex; empty if GitHub does not report the digest
            QDateTime updatedAt;
            QUrl apiUrl;          // asset URL in the API (requires a token)
            QUrl sidecarUrl;      // URL of "<asset>.json", if it exists
            QString commit;       // from the .json file; empty if missing
            QDateTime builtAt;    // from the .json file

            bool isValid() const { return this->assetId != 0; }
        };

        GitHubDailyBuilds(GitHubAuth *auth,
                          QNetworkAccessManager *manager,
                          QObject *parent=nullptr);
        ~GitHubDailyBuilds();

        Q_INVOKABLE static QString currentOs();    // android, windows, macos, linux
        Q_INVOKABLE static QString currentArch();  // x86_64, arm64, ...
        Q_INVOKABLE static QString assetNameForPlatform(const QString &os,
                                                        const QString &arch);
        Q_INVOKABLE static bool parseRelease(const QJsonObject &release,
                                             const QString &assetName,
                                             BuildInfo *info);
        Q_INVOKABLE static bool applySidecar(const QByteArray &json,
                                             BuildInfo *info);
        // runningCommit: GIT_COMMIT_HASH of the running binary.
        Q_INVOKABLE static UpdateStatus updateStatus(const BuildInfo &build,
                                                     const QString &runningCommit,
                                                     bool runningIsDailyBuild);
        Q_INVOKABLE QString assetName() const;     // empty if the platform is not supported

    private:
        GitHubDailyBuildsPrivate *d;

    signals:
        void buildAvailable(const GitHubDailyBuilds::BuildInfo &build);
        void failed(GitHubDailyBuilds::Error error, const QString &message);
        void commitsCompared(const QString &base,
                             const QString &head,
                             GitHubDailyBuilds::CommitOrder order);

    public slots:
        void check();
        // Asks GitHub which of the two commits is newer. The commits are not
        // ordered by their hashes.
        void compareCommits(const QString &base, const QString &head);
};

#endif // GITHUBDAILYBUILDS_H

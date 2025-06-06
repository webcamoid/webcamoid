/* Webcamoid, webcam capture application.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
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

#ifndef MEDIATOOLS_H
#define MEDIATOOLS_H

#include <iak/akelement.h>

class MediaToolsPrivate;
class QQmlApplicationEngine;
class AkCaps;
class CliOptions;

class MediaTools: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int windowWidth
               READ windowWidth
               WRITE setWindowWidth
               RESET resetWindowWidth
               NOTIFY windowWidthChanged)
    Q_PROPERTY(int windowHeight
               READ windowHeight
               WRITE setWindowHeight
               RESET resetWindowHeight
               NOTIFY windowHeightChanged)
    Q_PROPERTY(QString applicationName
               READ applicationName
               CONSTANT)
    Q_PROPERTY(QString applicationVersion
               READ applicationVersion
               CONSTANT)
    Q_PROPERTY(bool isDailyBuild
               READ isDailyBuild
               CONSTANT)
    Q_PROPERTY(QString qtVersion
               READ qtVersion
               CONSTANT)
    Q_PROPERTY(QString copyrightNotice
               READ copyrightNotice
               CONSTANT)
    Q_PROPERTY(QString projectUrl
               READ projectUrl
               CONSTANT)
    Q_PROPERTY(QString projectLicenseUrl
               READ projectLicenseUrl
               CONSTANT)
    Q_PROPERTY(QString projectDownloadsUrl
               READ projectDownloadsUrl
               CONSTANT)
    Q_PROPERTY(QString projectIssuesUrl
               READ projectIssuesUrl
               CONSTANT)
    Q_PROPERTY(QString projectGitCommit
               READ projectGitCommit
               CONSTANT)
    Q_PROPERTY(QString projectGitShortCommit
               READ projectGitShortCommit
               CONSTANT)
    Q_PROPERTY(QString projectGitCommitUrl
               READ projectGitCommitUrl
               CONSTANT)
    Q_PROPERTY(QString projectDonationsUrl
               READ projectDonationsUrl
               CONSTANT)
    Q_PROPERTY(QString projectDocumentationUrl
               READ projectDocumentationUrl
               CONSTANT)
    Q_PROPERTY(QString documentsDirectory
               READ documentsDirectory
               WRITE setDocumentsDirectory
               RESET resetDocumentsDirectory
               NOTIFY documentsDirectoryChanged)
    Q_PROPERTY(int adBannerWidth
               READ adBannerWidth
               NOTIFY adBannerWidthChanged)
    Q_PROPERTY(int adBannerHeight
               READ adBannerHeight
               NOTIFY adBannerHeightChanged)

    public:
        enum AdType {
            AdType_Banner,
            AdType_AdaptiveBanner,
            AdType_Appopen,
            AdType_Interstitial,
            AdType_Rewarded,
            AdType_RewardedInterstitial
        };
        Q_ENUM(AdType)

        MediaTools(QObject *parent=nullptr);
        ~MediaTools();

        Q_INVOKABLE int windowWidth() const;
        Q_INVOKABLE int windowHeight() const;
        Q_INVOKABLE QString applicationName() const;
        Q_INVOKABLE QString applicationVersion() const;
        Q_INVOKABLE bool isDailyBuild() const;
        Q_INVOKABLE QString qtVersion() const;
        Q_INVOKABLE QString copyrightNotice() const;
        Q_INVOKABLE QString projectUrl() const;
        Q_INVOKABLE QString projectLicenseUrl() const;
        Q_INVOKABLE QString projectDownloadsUrl() const;
        Q_INVOKABLE QString projectIssuesUrl() const;
        Q_INVOKABLE QString projectGitCommit() const;
        Q_INVOKABLE QString projectGitShortCommit() const;
        Q_INVOKABLE QString projectGitCommitUrl() const;
        Q_INVOKABLE QString projectDonationsUrl() const;
        Q_INVOKABLE QString projectDocumentationUrl() const;
        Q_INVOKABLE QString fileNameFromUri(const QString &uri) const;
        Q_INVOKABLE bool matches(const QString &pattern,
                                 const QStringList &strings) const;
        Q_INVOKABLE QString currentTime() const;
        Q_INVOKABLE QString currentTime(const QString &format) const;
        Q_INVOKABLE QStringList standardLocations(const QString &type) const;
        Q_INVOKABLE static QString readFile(const QString &fileName);
        Q_INVOKABLE QString urlToLocalFile(const QString &urlOrFile) const;
        Q_INVOKABLE QString copyUrlToCache(const QString &urlOrFile) const;
        Q_INVOKABLE static QString convertToAbsolute(const QString &path);
        Q_INVOKABLE static void messageHandler(QtMsgType type,
                                               const QMessageLogContext &context,
                                               const QString &msg);
        Q_INVOKABLE QString readLog(quint64 lineStart=0) const;
        Q_INVOKABLE QString documentsDirectory() const;
        Q_INVOKABLE int adBannerWidth() const;
        Q_INVOKABLE int adBannerHeight() const;

    private:
        MediaToolsPrivate *d;

    signals:
        void windowWidthChanged(int windowWidth);
        void windowHeightChanged(int windowHeight);
        void interfaceLoaded();
        void newInstanceOpened();
        void logUpdated(const QString &messageType, const QString &lastLine);
        void documentsDirectoryChanged(const QString &documentsDirectory);
        void adBannerWidthChanged(int adBannerWidth);
        void adBannerHeightChanged(int adBannerHeight);

    public slots:
        bool init(const CliOptions &cliOptions);
        void setWindowWidth(int windowWidth);
        void setWindowHeight(int windowHeight);
        void setDocumentsDirectory(const QString &documentsDirectory);
        void resetWindowWidth();
        void resetWindowHeight();
        void resetDocumentsDirectory();
        void loadConfigs();
        void saveConfigs();
        void show();
        bool showAd(AdType adType);
        void printLog();
        bool saveLog();
        void makedirs(const QString &path);
        void restartApp();
};

Q_DECLARE_METATYPE(MediaTools::AdType)

#endif // MEDIATOOLS_H

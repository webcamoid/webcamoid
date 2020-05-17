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

#include <akelement.h>

class MediaToolsPrivate;
class QQmlApplicationEngine;
class AkCaps;

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

    public:
        MediaTools(QObject *parent=nullptr);
        ~MediaTools();

        Q_INVOKABLE int windowWidth() const;
        Q_INVOKABLE int windowHeight() const;
        Q_INVOKABLE QString applicationName() const;
        Q_INVOKABLE QString applicationVersion() const;
        Q_INVOKABLE QString qtVersion() const;
        Q_INVOKABLE QString copyrightNotice() const;
        Q_INVOKABLE QString projectUrl() const;
        Q_INVOKABLE QString projectLicenseUrl() const;
        Q_INVOKABLE QString projectDownloadsUrl() const;
        Q_INVOKABLE QString projectIssuesUrl() const;
        Q_INVOKABLE QString fileNameFromUri(const QString &uri) const;
        Q_INVOKABLE bool matches(const QString &pattern,
                                 const QStringList &strings) const;
        Q_INVOKABLE QString currentTime() const;
        Q_INVOKABLE QString currentTime(const QString &format) const;
        Q_INVOKABLE QStringList standardLocations(const QString &type) const;
        Q_INVOKABLE static QString readFile(const QString &fileName);
        Q_INVOKABLE QString urlToLocalFile(const QUrl &url) const;
        Q_INVOKABLE static QString convertToAbsolute(const QString &path);

    private:
        MediaToolsPrivate *d;

    signals:
        void windowWidthChanged(int windowWidth);
        void windowHeightChanged(int windowHeight);
        void interfaceLoaded();

    public slots:
        void setWindowWidth(int windowWidth);
        void setWindowHeight(int windowHeight);
        void resetWindowWidth();
        void resetWindowHeight();
        void loadConfigs();
        void saveConfigs();
        void show();
        void makedirs(const QString &path);
};

#endif // MEDIATOOLS_H

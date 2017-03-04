/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef UPDATES_H
#define UPDATES_H

#include <QTimer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class Updates;

typedef QSharedPointer<Updates> UpdatesPtr;

class Updates: public QObject
{
    Q_OBJECT
    Q_ENUMS(VersionType)
    Q_PROPERTY(bool notifyNewVersion
               READ notifyNewVersion
               WRITE setNotifyNewVersion
               RESET resetNotifyNewVersion
               NOTIFY notifyNewVersionChanged)
    Q_PROPERTY(VersionType versionType
               READ versionType
               NOTIFY versionTypeChanged)
    Q_PROPERTY(QString latestVersion
               READ latestVersion
               NOTIFY latestVersionChanged)
    Q_PROPERTY(int checkInterval
               READ checkInterval
               WRITE setCheckInterval
               RESET resetCheckInterval
               NOTIFY checkIntervalChanged)
    Q_PROPERTY(QDateTime lastUpdate
               READ lastUpdate
               NOTIFY lastUpdateChanged)

    public:
        enum VersionType {
            VersionTypeOld,
            VersionTypeCurrent,
            VersionTypeDevelopment
        };

        explicit Updates(QQmlApplicationEngine *engine=NULL,
                         QObject *parent=NULL);
        ~Updates();

        Q_INVOKABLE bool notifyNewVersion() const;
        Q_INVOKABLE VersionType versionType() const;
        Q_INVOKABLE QString latestVersion() const;
        Q_INVOKABLE int checkInterval() const;
        Q_INVOKABLE QDateTime lastUpdate() const;

    private:
        QQmlApplicationEngine *m_engine;
        QNetworkAccessManager m_manager;
        bool m_notifyNewVersion;
        VersionType m_versionType;
        QString m_latestVersion;
        int m_checkInterval;
        QDateTime m_lastUpdate;
        QTimer m_timer;

        QVariantList vectorize(const QString &version) const;
        void normalize(QVariantList &vector1, QVariantList &vector2) const;
        template<typename Functor>
        inline bool compare(const QString &version1,
                            const QString &version2,
                            Functor func) const {
            auto v1 = this->vectorize(version1);
            auto v2 = this->vectorize(version2);
            this->normalize(v1, v2);
            QString sv1;
            QString sv2;

            for (int i = 0; i < v1.size(); i++) {
                auto fillChar = v1[i].type() == QVariant::String? ' ': '0';
                auto a = v1[i].toString();
                auto b = v2[i].toString();
                auto width = qMax(a.size(), b.size());

                sv1 += QString("%1").arg(a, width, fillChar);
                sv2 += QString("%1").arg(b, width, fillChar);
            }

            return func(sv1, sv2);
        }

    signals:
        void notifyNewVersionChanged(bool notifyNewVersion);
        void versionTypeChanged(VersionType versionType);
        void latestVersionChanged(const QString &latestVersion);
        void checkIntervalChanged(int checkInterval);
        void lastUpdateChanged(const QDateTime &lastUpdate);

    public slots:
        void checkUpdates();
        void setQmlEngine(QQmlApplicationEngine *engine=NULL);
        void setNotifyNewVersion(bool notifyNewVersion);
        void setCheckInterval(int checkInterval);
        void resetNotifyNewVersion();
        void resetCheckInterval();

    private slots:
        void setVersionType(VersionType versionType);
        void setLatestVersion(const QString &latestVersion);
        void setLastUpdate(const QDateTime &lastUpdate);
        void replyFinished(QNetworkReply *reply);
        void loadProperties();
        void saveNotifyNewVersion(bool notifyNewVersion);
        void saveLatestVersion(const QString &latestVersion);
        void saveCheckInterval(int checkInterval);
        void saveLastUpdate(const QDateTime &lastUpdate);
        void saveProperties();
};

Q_DECLARE_METATYPE(Updates::VersionType)

#endif // UPDATES_H

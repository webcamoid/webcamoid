/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#include <QObject>

class UpdatesPrivate;
class Updates;
class QQmlApplicationEngine;

using UpdatesPtr = QSharedPointer<Updates>;

class Updates: public QObject
{
    Q_OBJECT
    Q_ENUMS(ComponentStatus)
    Q_PROPERTY(QStringList components
               READ components
               NOTIFY componentsChanged)
    Q_PROPERTY(bool notifyNewVersion
               READ notifyNewVersion
               WRITE setNotifyNewVersion
               RESET resetNotifyNewVersion
               NOTIFY notifyNewVersionChanged)
    Q_PROPERTY(int checkInterval
               READ checkInterval
               WRITE setCheckInterval
               RESET resetCheckInterval
               NOTIFY checkIntervalChanged)
    Q_PROPERTY(QDateTime lastUpdate
               READ lastUpdate
               NOTIFY lastUpdateChanged)
    Q_PROPERTY(bool isOnline
               READ isOnline
               NOTIFY isOnlineChanged)

    public:
        enum ComponentStatus
        {
            ComponentUpdated,
            ComponentOutdated,
            ComponentNewer
        };

        Updates(QQmlApplicationEngine *engine=nullptr,
                QObject *parent=nullptr);
        ~Updates();

        Q_INVOKABLE QStringList components() const;
        Q_INVOKABLE QString latestVersion(const QString &component) const;
        Q_INVOKABLE ComponentStatus status(const QString &component,
                                           const QString &currentVersion={}) const;
        Q_INVOKABLE bool notifyNewVersion() const;
        Q_INVOKABLE int checkInterval() const;
        Q_INVOKABLE QDateTime lastUpdate() const;
        Q_INVOKABLE static bool isOnline();

    private:
        UpdatesPrivate *d;

    signals:
        void componentsChanged(const QStringList &components);
        void notifyNewVersionChanged(bool notifyNewVersion);
        void checkIntervalChanged(int checkInterval);
        void lastUpdateChanged(const QDateTime &lastUpdate);
        void isOnlineChanged(bool isOnline);
        void newVersionAvailable(const QString &component,
                                 const QString &latestVersion);

    public slots:
        void checkUpdates();
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);
        void setNotifyNewVersion(bool notifyNewVersion);
        void setCheckInterval(int checkInterval);
        void resetNotifyNewVersion();
        void resetCheckInterval();
        void watch(const QString &component,
                   const QString &currentVersion,
                   const QString &url);
        void start();
        void stop();

        friend class UpdatesPrivate;
};

Q_DECLARE_METATYPE(Updates::ComponentStatus)

#endif // UPDATES_H

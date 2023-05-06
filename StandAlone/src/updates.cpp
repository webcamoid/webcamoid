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

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQmlContext>
#include <QSettings>
#include <QTimer>
#include <QtQml>

#include "updates.h"

struct ComponentInfo
{
    public:
        QString component;
        QString currentVersion;
        QString latestVersion;
        QString url;
        QByteArray data;

        ComponentInfo();
        ComponentInfo(const QString &component,
                      const QString &currentVersion,
                      const QString &latestVersion,
                      const QString &url);
        ComponentInfo(const ComponentInfo &other);
        ComponentInfo &operator =(const ComponentInfo &other);
};

class UpdatesPrivate
{
    public:
        Updates *self;
        QQmlApplicationEngine *m_engine {nullptr};
        QNetworkAccessManager m_manager;
        QVector<ComponentInfo> m_componentsInfo;
        QDateTime m_lastUpdate;
        QTimer m_timer;
        int m_checkInterval {0};
        bool m_notifyNewVersion {true};
        QMutex m_mutex;

        explicit UpdatesPrivate(Updates *self);
        void setLastUpdate(const QDateTime &lastUpdate);
        void setLatestVersion(const QString &component, const QString &version);
        void readData(const QString &component, QNetworkReply *reply);
        void readLatestVersion(const QString &component);
        QVariantList vectorize(const QString &version) const;
        void normalize(QVariantList &vector1, QVariantList &vector2) const;
        template<typename Functor>
        bool compare(const QString &version1,
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
        void loadProperties();
        void saveNotifyNewVersion(bool notifyNewVersion);
        void saveCheckInterval(int checkInterval);
        void saveLastUpdate(const QDateTime &lastUpdate);
        void saveLastestVersion();
};

Updates::Updates(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new UpdatesPrivate(this);
    this->setQmlEngine(engine);

    // Check lasUpdate every 10 mins
    this->d->m_timer.setInterval(int(1e3 * 60 * 10));
    QObject::connect(&this->d->m_timer,
                     &QTimer::timeout,
                     this,
                     &Updates::checkUpdates);

    this->d->loadProperties();
}

Updates::~Updates()
{
    delete this->d;
}

QStringList Updates::components() const
{
    QStringList components;

    this->d->m_mutex.lock();

    for (auto &info: this->d->m_componentsInfo)
        components << info.component;

    this->d->m_mutex.unlock();

    return components;
}

QString Updates::latestVersion(const QString &component) const
{
    QString version;

    this->d->m_mutex.lock();

    for (auto &info: this->d->m_componentsInfo)
        if (info.component == component) {
            version = info.latestVersion;

            break;
        }

    this->d->m_mutex.unlock();

    return version;
}

Updates::ComponentStatus Updates::status(const QString &component,
                                         const QString &currentVersion) const
{
    Updates::ComponentStatus status = ComponentUpdated;
    this->d->m_mutex.lock();

    for (auto &info: this->d->m_componentsInfo)
        if (info.component == component) {
            QString curVersion = currentVersion.isEmpty()?
                                    info.currentVersion:
                                    currentVersion;
            auto isNewerVersion =
                    this->d->compare(curVersion,
                                     info.latestVersion,
                                     [] (const QString &a, const QString &b) {
                                        return a > b;
                                     });
            status = isNewerVersion?
                         ComponentNewer:
                     curVersion == info.latestVersion?
                         ComponentUpdated:
                         ComponentOutdated;

            break;
        }

    this->d->m_mutex.unlock();

    return status;
}

bool Updates::notifyNewVersion() const
{
    return this->d->m_notifyNewVersion;
}

int Updates::checkInterval() const
{
    return this->d->m_checkInterval;
}

QDateTime Updates::lastUpdate() const
{
    if (this->d->m_lastUpdate.isValid())
        return this->d->m_lastUpdate;

    return QDateTime::currentDateTime();
}

bool Updates::isOnline()
{
    for (auto &interface: QNetworkInterface::allInterfaces()) {
        if (interface.flags() & QNetworkInterface::IsUp
            && interface.flags() & QNetworkInterface::IsRunning
            && interface.flags() & QNetworkInterface::CanBroadcast
            && interface.flags() & QNetworkInterface::CanMulticast
            && !(interface.flags() & QNetworkInterface::IsLoopBack)) {
            return true;
        }
    }

    return false;
}

void Updates::checkUpdates()
{
    QList<QPair<QNetworkReply *, QString>> replies;

    this->d->m_mutex.lock();

    for (auto &info: this->d->m_componentsInfo) {
        if (info.latestVersion.isEmpty()
            || (this->d->m_checkInterval > 0
                &&  (this->d->m_lastUpdate.isNull()
                     || this->d->m_lastUpdate.daysTo(QDateTime::currentDateTime()) >= this->d->m_checkInterval))) {
            info.data.clear();
            auto reply = this->d->m_manager.get(QNetworkRequest(QUrl(info.url)));
            replies << QPair<QNetworkReply *, QString> {reply, info.component};
        }
    }

    this->d->m_mutex.unlock();

    for (auto &reply: replies) {
        QObject::connect(reply.first,
                         &QNetworkReply::finished,
                         this,
                         [this, reply] () {
            this->d->readLatestVersion(reply.second);
            reply.first->deleteLater();
        });
        QObject::connect(reply.first,
                         &QNetworkReply::readyRead,
                         this,
                         [this, reply] () {
            this->d->readData(reply.second, reply.first);
        });
    }
}

void Updates::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine) {
        engine->rootContext()->setContextProperty("updates", this);
        qmlRegisterType<Updates>("Webcamoid", 1, 0, "Updates");
    }
}

void Updates::setNotifyNewVersion(bool notifyNewVersion)
{
    if (this->d->m_notifyNewVersion == notifyNewVersion)
        return;

    this->d->m_notifyNewVersion = notifyNewVersion;
    emit this->notifyNewVersionChanged(notifyNewVersion);
    this->d->saveNotifyNewVersion(notifyNewVersion);
}

void Updates::setCheckInterval(int checkInterval)
{
    if (this->d->m_checkInterval == checkInterval)
        return;

    this->d->m_checkInterval = checkInterval;
    emit this->checkIntervalChanged(checkInterval);
    this->d->saveCheckInterval(checkInterval);
}

void Updates::resetNotifyNewVersion()
{
    this->setNotifyNewVersion(false);
}

void Updates::resetCheckInterval()
{
    this->setCheckInterval(0);
}

void Updates::watch(const QString &component,
                    const QString &currentVersion,
                    const QString &url)
{
    if (component.isEmpty() || url.isEmpty())
        return;

    this->d->m_mutex.lock();

    for (auto &info: this->d->m_componentsInfo)
        if (info.component == component) {
            info.currentVersion = currentVersion;
            info.url = url;

            if (info.latestVersion.isEmpty())
                info.latestVersion = currentVersion;

            this->d->m_mutex.unlock();

            return;
        }

    this->d->m_mutex.unlock();

    qDebug() << "Added component" << component;
    this->d->m_mutex.lock();
    this->d->m_componentsInfo
            << ComponentInfo {component, currentVersion, currentVersion, url};
    this->d->m_mutex.unlock();
    emit this->componentsChanged(this->components());
}

void Updates::start()
{
    this->checkUpdates();
    this->d->m_timer.start();
}

void Updates::stop()
{
    this->d->m_timer.stop();
}

UpdatesPrivate::UpdatesPrivate(Updates *self):
    self(self)
{

}

void UpdatesPrivate::setLastUpdate(const QDateTime &lastUpdate)
{
    if (this->m_lastUpdate == lastUpdate)
        return;

    this->m_lastUpdate = lastUpdate;
    emit self->lastUpdateChanged(lastUpdate);
    this->saveLastUpdate(lastUpdate);
}

void UpdatesPrivate::setLatestVersion(const QString &component,
                                      const QString &version)
{
    bool emitSignal = false;
    this->m_mutex.lock();

    for (auto &info: this->m_componentsInfo)
        if (info.component == component) {
            info.latestVersion = version;

            if (info.currentVersion != info.latestVersion)
                emitSignal = true;

            break;
        }

    this->m_mutex.unlock();

    if (emitSignal)
        emit self->newVersionAvailable(component, version);
}

void UpdatesPrivate::readData(const QString &component,
                              QNetworkReply *reply)
{
    this->m_mutex.lock();

    for (auto &info: this->m_componentsInfo)
        if (info.component == component) {
            info.data += reply->readAll();

            break;
        }

    this->m_mutex.unlock();
}

void UpdatesPrivate::readLatestVersion(const QString &component)
{
    QByteArray html;
    this->m_mutex.lock();

    for (auto &info: this->m_componentsInfo)
        if (info.component == component) {
            html = info.data;
            info.data.clear();

            break;
        }

    this->m_mutex.unlock();

    if (html.isEmpty())
        return;

    QJsonParseError error {0, QJsonParseError::NoError};
    auto json = QJsonDocument::fromJson(html, &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Error requesting latest version:"
                 << error.errorString();

        return;
    }

    if (!json.isObject())
        return;

    auto jsonObj = json.object();

    if (!jsonObj.contains("tag_name"))
        return;

    auto latestVersion = self->latestVersion(component);
    auto version = jsonObj.value("tag_name").toString(latestVersion);
    this->setLatestVersion(component, version);
    this->saveLastestVersion();
    this->setLastUpdate(QDateTime::currentDateTime());
}

QVariantList UpdatesPrivate::vectorize(const QString &version) const
{
    QVariantList vector;
    QString digs;
    QString alps;

    for (auto &c: version) {
        if (c.isDigit()) {
            digs += c;

            if (!alps.isEmpty()) {
                vector << alps;
                alps.clear();
            }
        } else if (c.isLetter()) {
            alps += c;

            if (!digs.isEmpty()) {
                vector << digs.toUInt();
                digs.clear();
            }
        } else {
            if (!digs.isEmpty()) {
                vector << digs.toUInt();
                digs.clear();
            }

            if (!alps.isEmpty()) {
                vector << alps;
                alps.clear();
            }
        }
    }

    if (!digs.isEmpty()) {
        vector << digs.toUInt();
        digs.clear();
    }

    if (!alps.isEmpty()) {
        vector << alps;
        alps.clear();
    }

    return vector;
}

void UpdatesPrivate::normalize(QVariantList &vector1,
                               QVariantList &vector2) const
{
    auto diff = vector1.size() - vector2.size();

    if (diff > 0) {
        auto offset = vector2.size();

        for (int i = 0; i < diff; i++)
            if (vector1.value(offset + i).type() == QVariant::String)
                vector2 << QString();
            else
                vector2 << 0;
    } else if (diff < 0) {
        diff *= -1;
        auto offset = vector1.size();

        for (int i = 0; i < diff; i++)
            if (vector2.value(offset + i).type() == QVariant::String)
                vector1 << QString();
            else
                vector1 << 0;
    }
}

void UpdatesPrivate::loadProperties()
{
    QSettings config;

    config.beginGroup("Updates");
    this->m_lastUpdate = config.value("lastUpdate").toDateTime();
    this->m_notifyNewVersion = config.value("notify", true).toBool();
    this->m_checkInterval = config.value("checkInterval", 1).toInt();
    int size = config.beginReadArray("updateInfo");

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        auto component = config.value("component").toString();
        auto currentVersion = config.value("currentVersion").toString();
        auto latestVersion = config.value("latestVersion").toString();
        auto url = config.value("url").toString();

        if (currentVersion.isEmpty())
            currentVersion = latestVersion;

        if (latestVersion.isEmpty())
            latestVersion = currentVersion;

        if (!component.isEmpty() && !url.isEmpty() && !currentVersion.isEmpty()) {
            this->m_mutex.lock();
            this->m_componentsInfo << ComponentInfo(component,
                                                    currentVersion,
                                                    latestVersion,
                                                    url);
            this->m_mutex.unlock();
        }
    }

    config.endArray();
    config.endGroup();
}

void UpdatesPrivate::saveNotifyNewVersion(bool notifyNewVersion)
{
    QSettings config;

    config.beginGroup("Updates");
    config.setValue("notify", notifyNewVersion);
    config.endGroup();
}

void UpdatesPrivate::saveCheckInterval(int checkInterval)
{
    QSettings config;

    config.beginGroup("Updates");
    config.setValue("checkInterval", checkInterval);
    config.endGroup();
}

void UpdatesPrivate::saveLastUpdate(const QDateTime &lastUpdate)
{
    QSettings config;

    config.beginGroup("Updates");
    config.setValue("lastUpdate", lastUpdate);
    config.endGroup();
}

void UpdatesPrivate::saveLastestVersion()
{
    QSettings config;

    config.beginGroup("Updates");
    config.beginWriteArray("updateInfo");
    int i = 0;
    this->m_mutex.lock();

    for (auto &info: this->m_componentsInfo) {
        config.setArrayIndex(i);
        config.setValue("component", info.component);
        config.setValue("currentVersion", info.currentVersion);
        config.setValue("latestVersion", info.latestVersion);
        config.setValue("url", info.url);
        i++;
    }

    this->m_mutex.unlock();
    config.endArray();
    config.endGroup();
}

ComponentInfo::ComponentInfo()
{

}

ComponentInfo::ComponentInfo(const QString &component,
                             const QString &currentVersion,
                             const QString &latestVersion,
                             const QString &url):
    component(component),
    currentVersion(currentVersion),
    latestVersion(latestVersion),
    url(url)
{

}

ComponentInfo::ComponentInfo(const ComponentInfo &other):
    component(other.component),
    currentVersion(other.currentVersion),
    latestVersion(other.latestVersion),
    url(other.url),
    data(other.data)
{

}

ComponentInfo &ComponentInfo::operator =(const ComponentInfo &other)
{
    if (this != &other) {
        this->component = other.component;
        this->currentVersion = other.currentVersion;
        this->latestVersion = other.latestVersion;
        this->url = other.url;
        this->data = other.data;
    }

    return *this;
}

#include "moc_updates.cpp"

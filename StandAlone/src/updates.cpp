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

#include <QDebug>
#include <QSettings>
#include <QtQml>
#include <QQmlContext>
#include <QJsonDocument>
#include <QJsonObject>

#include "updates.h"

#define UPDATES_URL "https://api.github.com/repos/webcamoid/webcamoid/releases/latest"

Updates::Updates(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent),
    m_engine(nullptr),
    m_notifyNewVersion(false),
    m_versionType(VersionTypeCurrent),
    m_latestVersion(COMMONS_VERSION),
    m_checkInterval(0)
{
    this->setQmlEngine(engine);

    QObject::connect(&this->m_manager,
                     &QNetworkAccessManager::finished,
                     this,
                     &Updates::replyFinished);

    // Check lasUpdate every 10 mins
    this->m_timer.setInterval(int(1e3 * 60 * 10));

    this->loadProperties();

    QObject::connect(&this->m_timer,
                     &QTimer::timeout,
                     this,
                     &Updates::checkUpdates);
    QObject::connect(this,
                     &Updates::notifyNewVersionChanged,
                     this,
                     &Updates::saveNotifyNewVersion);
    QObject::connect(this,
                     &Updates::latestVersionChanged,
                     this,
                     &Updates::saveLatestVersion);
    QObject::connect(this,
                     &Updates::checkIntervalChanged,
                     this,
                     &Updates::saveCheckInterval);
    QObject::connect(this,
                     &Updates::checkIntervalChanged,
                     [this] (int checkInterval) {
                        if (checkInterval > 0)
                            this->m_timer.start();
                        else
                            this->m_timer.stop();
                     });
    QObject::connect(this,
                     &Updates::lastUpdateChanged,
                     this,
                     &Updates::saveLastUpdate);
}

Updates::~Updates()
{
    this->saveProperties();
}

bool Updates::notifyNewVersion() const
{
    return this->m_notifyNewVersion;
}

Updates::VersionType Updates::versionType() const
{
    return this->m_versionType;
}

QString Updates::latestVersion() const
{
    return this->m_latestVersion;
}

int Updates::checkInterval() const
{
    return this->m_checkInterval;
}

QDateTime Updates::lastUpdate() const
{
    if (this->m_lastUpdate.isValid())
        return this->m_lastUpdate;

    return QDateTime::currentDateTime();
}

QVariantList Updates::vectorize(const QString &version) const
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

void Updates::normalize(QVariantList &vector1, QVariantList &vector2) const
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

void Updates::checkUpdates()
{
    if (this->m_checkInterval > 0
        &&(this->m_lastUpdate.isNull()
           || this->m_lastUpdate.daysTo(QDateTime::currentDateTime()) >= this->m_checkInterval)) {
        this->m_manager.get(QNetworkRequest(QUrl(UPDATES_URL)));
    }
}

void Updates::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->m_engine == engine)
        return;

    this->m_engine = engine;

    if (engine) {
        engine->rootContext()->setContextProperty("Updates", this);
        qmlRegisterType<Updates>("WebcamoidUpdates", 1, 0, "UpdatesT");
    }
}

void Updates::setNotifyNewVersion(bool notifyNewVersion)
{
    if (this->m_notifyNewVersion == notifyNewVersion)
        return;

    this->m_notifyNewVersion = notifyNewVersion;
    emit this->notifyNewVersionChanged(notifyNewVersion);
}

void Updates::setCheckInterval(int checkInterval)
{
    if (this->m_checkInterval == checkInterval)
        return;

    this->m_checkInterval = checkInterval;
    emit this->checkIntervalChanged(checkInterval);
}

void Updates::resetNotifyNewVersion()
{
    this->setNotifyNewVersion(false);
}

void Updates::resetCheckInterval()
{
    this->setCheckInterval(0);
}

void Updates::setVersionType(Updates::VersionType versionType)
{
    if (this->m_versionType == versionType)
        return;

    this->m_versionType = versionType;
    emit this->versionTypeChanged(versionType);
}

void Updates::setLatestVersion(const QString &latestVersion)
{
    if (this->m_latestVersion == latestVersion)
        return;

    this->m_latestVersion = latestVersion;
    emit this->latestVersionChanged(latestVersion);
}

void Updates::setLastUpdate(const QDateTime &lastUpdate)
{
    if (this->m_lastUpdate == lastUpdate)
        return;

    this->m_lastUpdate = lastUpdate;
    emit this->lastUpdateChanged(lastUpdate);
}

void Updates::replyFinished(QNetworkReply *reply)
{
    if (!reply || reply->error() != QNetworkReply::NoError) {
        if (!reply)
            qDebug() << "Error requesting latest version:" << reply->errorString();
        else
            qDebug() << "Error requesting latest version: No response";

        return;
    }

    QString html = reply->readAll();
    QJsonParseError error;
    auto json = QJsonDocument::fromJson(html.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Error requesting latest version:" << error.errorString();

        return;
    }

    if (!json.isObject())
        return;

    auto jsonObj = json.object();

    if (!jsonObj.contains("tag_name"))
        return;

    auto version = jsonObj.value("tag_name").toString(COMMONS_VERSION);

    auto isOldVersion =
            this->compare(version, COMMONS_VERSION,
                          [] (const QVariant &a, const QVariant &b) {
                                return a > b;
                          });

    VersionType versionType = isOldVersion?
                                  VersionTypeOld:
                              version == COMMONS_VERSION?
                                   VersionTypeCurrent:
                                   VersionTypeDevelopment;

    this->setLatestVersion(version);
    this->setVersionType(versionType);
    this->setLastUpdate(QDateTime::currentDateTime());
}

void Updates::loadProperties()
{
    QSettings config;

    config.beginGroup("Updates");
    this->setLastUpdate(config.value("lastUpdate").toDateTime());
    this->setNotifyNewVersion(config.value("notify", true).toBool());
    this->setCheckInterval(config.value("checkInterval", 1).toInt());
    this->setLatestVersion(config.value("latestVersion", COMMONS_VERSION).toString());
    config.endGroup();

   if (this->m_checkInterval > 0) {
       this->m_timer.start();
       this->checkUpdates();
   } else
       this->m_timer.stop();

   auto isOldVersion =
           this->compare(this->m_latestVersion, COMMONS_VERSION,
                         [] (const QVariant &a, const QVariant &b) {
                               return a > b;
                         });

   VersionType versionType = isOldVersion?
                                 VersionTypeOld:
                             this->m_latestVersion == COMMONS_VERSION?
                                  VersionTypeCurrent:
                                  VersionTypeDevelopment;

   this->setVersionType(versionType);
}

void Updates::saveNotifyNewVersion(bool notifyNewVersion)
{
    QSettings config;

    config.beginGroup("Updates");
    config.setValue("notify", notifyNewVersion);
    config.endGroup();
}

void Updates::saveLatestVersion(const QString &latestVersion)
{
    QSettings config;

    config.beginGroup("Updates");
    config.setValue("latestVersion", latestVersion);
    config.endGroup();
}

void Updates::saveCheckInterval(int checkInterval)
{
    QSettings config;

    config.beginGroup("Updates");
    config.setValue("checkInterval", checkInterval);
    config.endGroup();
}

void Updates::saveLastUpdate(const QDateTime &lastUpdate)
{
    QSettings config;

    config.beginGroup("Updates");
    config.setValue("lastUpdate", lastUpdate);
    config.endGroup();
}

void Updates::saveProperties()
{
    QSettings config;

    auto lastUpdate = this->m_lastUpdate;

    if (!lastUpdate.isValid())
        lastUpdate = QDateTime::currentDateTime();

    config.beginGroup("Updates");
    config.setValue("lastUpdate", lastUpdate);
    config.setValue("notify", this->m_notifyNewVersion);
    config.setValue("checkInterval", this->m_checkInterval);
    config.setValue("latestVersion", this->m_latestVersion);
    config.endGroup();
}

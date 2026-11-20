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
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPair>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include "githubauth.h"

#define DEVICE_CODE_PATH   "/login/device/code"
#define TOKEN_PATH         "/login/oauth/access_token"
#define EXPIRY_MARGIN_SECS 120

class GitHubAuthPrivate
{
    public:
        GitHubAuth *self;
        QString m_clientId;
        QString m_oauthBase {"https://github.com"};
        QNetworkAccessManager *m_manager;
        QNetworkReply *m_loginReply {nullptr};
        QNetworkReply *m_refreshReply {nullptr};
        QTimer m_pollTimer;
        QString m_deviceCode;
        QDateTime m_deviceExpires;
        int m_interval {5};

        QString m_accessToken;
        QString m_refreshToken;
        QDateTime m_accessExpires;       // null if the token does not expire
        QDateTime m_refreshExpires;

        using Param = QPair<QString, QString>;

        explicit GitHubAuthPrivate(GitHubAuth *self);
        QNetworkReply *postForm(const QString &url,
                                const QList<Param> &params);
        static QJsonObject parseReply(QNetworkReply *reply);
        void poll();
        void onDeviceCode(QNetworkReply *reply);
        void onPoll(QNetworkReply *reply);
        void onRefresh(QNetworkReply *reply);
        bool storeTokens(const QJsonObject &obj);
        void saveTokens() const;
        void clearTokens();
};

GitHubAuth::GitHubAuth(const QString &clientId,
                       QNetworkAccessManager *manager,
                       QObject *parent):
    QObject(parent)
{
    this->d = new GitHubAuthPrivate(this);
    this->d->m_clientId = clientId;
    this->d->m_manager = manager;
    this->d->m_pollTimer.setSingleShot(true);
    QObject::connect(&this->d->m_pollTimer,
                     &QTimer::timeout,
                     this,
                     [this] () {
                         this->d->poll();
                     });
}

GitHubAuth::~GitHubAuth()
{
    delete this->d;
}

void GitHubAuth::setOAuthBase(const QString &oauthBase)
{
    this->d->m_oauthBase = oauthBase;
}

QString GitHubAuth::accessToken() const
{
    return this->d->m_accessToken;
}

bool GitHubAuth::accessTokenValid() const
{
    if (this->d->m_accessToken.isEmpty())
        return false;

    if (this->d->m_accessExpires.isNull())
        return true;

    return QDateTime::currentDateTimeUtc().addSecs(EXPIRY_MARGIN_SECS)
           < this->d->m_accessExpires;
}

bool GitHubAuth::canRefresh() const
{
    if (this->d->m_refreshToken.isEmpty())
        return false;

    return this->d->m_refreshExpires.isNull()
           || QDateTime::currentDateTimeUtc() < this->d->m_refreshExpires;
}

bool GitHubAuth::loginInProgress() const
{
    return !this->d->m_deviceCode.isEmpty();
}

void GitHubAuth::loadStoredTokens()
{
    QSettings settings;
    settings.beginGroup("GitHubAuth");
    auto data = settings.value("tokens").toByteArray();
    settings.endGroup();

    auto obj = QJsonDocument::fromJson(data).object();
    this->d->m_accessToken = obj.value("access_token").toString();
    this->d->m_refreshToken = obj.value("refresh_token").toString();
    this->d->m_accessExpires =
        QDateTime::fromString(obj.value("access_expires").toString(),
                              Qt::ISODate);
    this->d->m_refreshExpires =
        QDateTime::fromString(obj.value("refresh_expires").toString(),
                              Qt::ISODate);
}

void GitHubAuth::login()
{
    this->cancelLogin();

    if (this->d->m_clientId.isEmpty()) {
        emit this->loginFailed(tr("Missing GitHub client ID"));

        return;
    }

    auto reply = this->d->postForm(this->d->m_oauthBase + DEVICE_CODE_PATH,
                                   {{"client_id", this->d->m_clientId}});
    this->d->m_loginReply = reply;
    QObject::connect(reply,
                     &QNetworkReply::finished,
                     this,
                     [this, reply] () { this->d->onDeviceCode(reply); });
}

void GitHubAuth::cancelLogin()
{
    this->d->m_pollTimer.stop();
    this->d->m_deviceCode.clear();

    auto reply = this->d->m_loginReply;
    this->d->m_loginReply = nullptr;

    if (reply)
        reply->abort();
}

void GitHubAuth::refresh()
{
    if (this->d->m_refreshReply)
        return;

    if (!this->canRefresh()) {
        emit this->refreshFailed(tr("No valid refresh token"));

        return;
    }

    // client_secret is not sent for tokens obtained with the Device Flow.
    auto reply = this->d->postForm(this->d->m_oauthBase + TOKEN_PATH,
                                   {{"client_id", this->d->m_clientId},
                                    {"grant_type", "refresh_token"},
                                    {"refresh_token", this->d->m_refreshToken}});
    this->d->m_refreshReply = reply;
    QObject::connect(reply,
                     &QNetworkReply::finished,
                     this,
                     [this, reply] () { this->d->onRefresh(reply); });
}

void GitHubAuth::logout()
{
    this->cancelLogin();
    this->d->clearTokens();
}

GitHubAuthPrivate::GitHubAuthPrivate(GitHubAuth *self):
    self(self)
{

}

QNetworkReply *GitHubAuthPrivate::postForm(const QString &url,
                                           const QList<Param> &params)
{
    QUrlQuery query;

    for (auto &param: params)
        query.addQueryItem(param.first, param.second);

    QNetworkRequest request {QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "Webcamoid");

    return this->m_manager->post(request,
                                 query.toString(QUrl::FullyEncoded).toUtf8());
}

QJsonObject GitHubAuthPrivate::parseReply(QNetworkReply *reply)
{
    return QJsonDocument::fromJson(reply->readAll()).object();
}

void GitHubAuthPrivate::onDeviceCode(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply != this->m_loginReply)
        return; // canceled

    this->m_loginReply = nullptr;
    auto obj = parseReply(reply);

    if (obj.contains("error")) {
        emit self->loginFailed(obj.value("error_description")
                               .toString(obj.value("error").toString()));

        return;
    }

    if (reply->error() != QNetworkReply::NoError || obj.isEmpty()) {
        emit self->loginFailed(reply->errorString());

        return;
    }

    this->m_deviceCode = obj.value("device_code").toString();
    this->m_interval = qMax(obj.value("interval").toInt(5), 1);
    auto expiresIn = obj.value("expires_in").toInt(900);
    this->m_deviceExpires =
        QDateTime::currentDateTimeUtc().addSecs(expiresIn);

    emit self->deviceCodeReady(obj.value("user_code").toString(),
                               QUrl(obj.value("verification_uri").toString()),
                               expiresIn);

    this->m_pollTimer.start(this->m_interval * 1000);
}

void GitHubAuthPrivate::poll()
{
    if (this->m_deviceCode.isEmpty())
        return;

    if (QDateTime::currentDateTimeUtc() >= this->m_deviceExpires) {
        self->cancelLogin();
        emit self->loginFailed(QObject::tr("The code expired, please try again"));

        return;
    }

    auto reply =
        this->postForm(this->m_oauthBase + TOKEN_PATH,
                       {{"client_id", this->m_clientId},
                        {"device_code", this->m_deviceCode},
                        {"grant_type",
                         "urn:ietf:params:oauth:grant-type:device_code"}});
    this->m_loginReply = reply;
    QObject::connect(reply,
                     &QNetworkReply::finished,
                     self,
                     [this, reply] () { this->onPoll(reply); });
}

void GitHubAuthPrivate::onPoll(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply != this->m_loginReply)
        return; // canceled

    this->m_loginReply = nullptr;
    auto obj = parseReply(reply);
    auto error = obj.value("error").toString();

    if (error.isEmpty() && obj.contains("access_token")) {
        this->m_deviceCode.clear();

        if (this->storeTokens(obj))
            emit self->loggedIn();
        else
            emit self->loginFailed(QObject::tr("Invalid response from GitHub"));

        return;
    }

    if (error.isEmpty()) {
        // Temporary network error: retry until the code expires.
        this->m_pollTimer.start(this->m_interval * 1000);

        return;
    }

    if (error == "authorization_pending") {
        this->m_pollTimer.start(this->m_interval * 1000);
    } else if (error == "slow_down") {
        this->m_interval = qMax(this->m_interval + 5,
                                obj.value("interval").toInt());
        this->m_pollTimer.start(this->m_interval * 1000);
    } else if (error == "expired_token") {
        this->m_deviceCode.clear();
        emit self->loginFailed(QObject::tr("The code expired, please try again"));
    } else if (error == "access_denied") {
        this->m_deviceCode.clear();
        emit self->loginFailed(QObject::tr("Authorization was denied"));
    } else {
        this->m_deviceCode.clear();
        emit self->loginFailed(obj.value("error_description")
                                  .toString(error));
    }
}

void GitHubAuthPrivate::onRefresh(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply == this->m_refreshReply)
        this->m_refreshReply = nullptr;

    auto obj = parseReply(reply);
    auto error = obj.value("error").toString();

    if (error.isEmpty() && obj.contains("access_token")) {
        // The refresh token is rotated on every renewal: store the new one.
        if (this->storeTokens(obj))
            emit self->tokenRefreshed();
        else
            emit self->refreshFailed(QObject::tr("Invalid response from GitHub"));

        return;
    }

    if (error.isEmpty()) {
        // Network failure: keep the tokens and retry later.
        emit self->refreshFailed(reply->errorString());

        return;
    }

    if (error == "bad_refresh_token")
        this->clearTokens(); // the user must sign in again

    emit self->refreshFailed(obj.value("error_description").toString(error));
}

bool GitHubAuthPrivate::storeTokens(const QJsonObject &obj)
{
    auto accessToken = obj.value("access_token").toString();

    if (accessToken.isEmpty())
        return false;

    auto now = QDateTime::currentDateTimeUtc();

    this->m_accessToken = accessToken;
    // These fields are missing if the app does not use expiring tokens.
    this->m_refreshToken = obj.value("refresh_token").toString();
    this->m_accessExpires =
        obj.contains("expires_in")?
            now.addSecs(obj.value("expires_in").toInt()):
            QDateTime();
    this->m_refreshExpires =
        obj.contains("refresh_token_expires_in")?
            now.addSecs(obj.value("refresh_token_expires_in").toInt()):
            QDateTime();
    this->saveTokens();

    return true;
}

void GitHubAuthPrivate::saveTokens() const
{
    QJsonObject obj;
    obj["access_token"] = this->m_accessToken;
    obj["refresh_token"] = this->m_refreshToken;
    obj["access_expires"] = this->m_accessExpires.toString(Qt::ISODate);
    obj["refresh_expires"] = this->m_refreshExpires.toString(Qt::ISODate);

    // TODO: store the tokens in the system keyring instead of QSettings, which
    // keeps them as plain text on disk.
    QSettings settings;
    settings.beginGroup("GitHubAuth");
    settings.setValue("tokens",
                      QJsonDocument(obj).toJson(QJsonDocument::Compact));
    settings.endGroup();
}

void GitHubAuthPrivate::clearTokens()
{
    this->m_accessToken.clear();
    this->m_refreshToken.clear();
    this->m_accessExpires = QDateTime();
    this->m_refreshExpires = QDateTime();

    QSettings settings;
    settings.beginGroup("GitHubAuth");
    settings.remove("tokens");
    settings.endGroup();
}

#include "moc_githubauth.cpp"

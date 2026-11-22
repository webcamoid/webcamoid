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
#include <QDebug>
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
#include "tokenstore.h"

#define OAUTH_BASE         "https://github.com"
#define API_BASE           "https://api.github.com"
#define DEVICE_CODE_PATH   "/login/device/code"
#define TOKEN_PATH         "/login/oauth/access_token"
#define TOKEN_STORE_NAME   "github-tokens"
#define EXPIRY_MARGIN_SECS 120

class GitHubAuthPrivate
{
    public:
        GitHubAuth *self;
        QString m_clientId;
        QNetworkAccessManager *m_manager;
        QNetworkReply *m_loginReply {nullptr};
        QNetworkReply *m_refreshReply {nullptr};
        QNetworkReply *m_userReply {nullptr};
        QTimer m_pollTimer;
        QString m_deviceCode;
        QDateTime m_deviceExpires;
        int m_interval {5};

        QString m_accessToken;
        QString m_refreshToken;
        QDateTime m_accessExpires;       // null if the token does not expire
        QDateTime m_refreshExpires;
        QString m_userName;
        bool m_sessionSaved {false};

        using Param = QPair<QString, QString>;

        explicit GitHubAuthPrivate(GitHubAuth *self);
        QByteArray legacyTokens();
        void clearLegacyTokens();
        QNetworkReply *postForm(const QString &url,
                                const QList<Param> &params);
        static QJsonObject parseReply(QNetworkReply *reply);
        void poll();
        void onDeviceCode(QNetworkReply *reply);
        void onPoll(QNetworkReply *reply);
        void onRefresh(QNetworkReply *reply);
        bool storeTokens(const QJsonObject &obj);
        void saveTokens();
        void clearTokens();
        void setUserName(const QString &userName);
        void onUser(QNetworkReply *reply);
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

QString GitHubAuth::userName() const
{
    return this->d->m_userName;
}

bool GitHubAuth::sessionSaved() const
{
    return this->d->m_sessionSaved;
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
    QByteArray data;
    auto result = TokenStore::read(TOKEN_STORE_NAME, &data);
    this->d->m_sessionSaved = result == TokenStore::Ok;

    if (result != TokenStore::Ok) {
        // A previous version saved the tokens in plain text.
        data = legacyTokens();

        if (!data.isEmpty()) {
            // Move them to the keyring, and if it is not available do not
            // leave them in the disk: they are used until the application
            // closes.
            this->d->m_sessionSaved =
                result == TokenStore::NotFound
                && TokenStore::write(TOKEN_STORE_NAME, data) == TokenStore::Ok;
            clearLegacyTokens();
        }
    }

    QSettings settings;
    settings.beginGroup("GitHubAuth");
    this->d->m_userName = settings.value("login").toString();
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

    auto reply = this->d->postForm(QStringLiteral(OAUTH_BASE DEVICE_CODE_PATH),
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
    auto reply = this->d->postForm(QStringLiteral(OAUTH_BASE TOKEN_PATH),
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

QByteArray GitHubAuthPrivate::legacyTokens()
{
    QSettings settings;
    settings.beginGroup("GitHubAuth");
    auto data = settings.value("tokens").toByteArray();
    settings.endGroup();

    return data;
}

void GitHubAuthPrivate::clearLegacyTokens()
{
    QSettings settings;
    settings.beginGroup("GitHubAuth");
    settings.remove("tokens");
    settings.endGroup();
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
        this->postForm(QStringLiteral(OAUTH_BASE TOKEN_PATH),
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

        if (this->storeTokens(obj)) {
            emit self->loggedIn();
            self->fetchUserName();
        } else
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
        if (this->storeTokens(obj)) {
            emit self->tokenRefreshed();

            if (this->m_userName.isEmpty())
                self->fetchUserName();
        } else
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

void GitHubAuthPrivate::saveTokens()
{
    QJsonObject obj;
    obj["access_token"] = this->m_accessToken;
    obj["refresh_token"] = this->m_refreshToken;
    obj["access_expires"] = this->m_accessExpires.toString(Qt::ISODate);
    obj["refresh_expires"] = this->m_refreshExpires.toString(Qt::ISODate);

    auto data = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    this->m_sessionSaved =
        TokenStore::write(TOKEN_STORE_NAME, data) == TokenStore::Ok;
    clearLegacyTokens();

    // The tokens are never saved in plain text: without a keyring they only
    // live in memory, and the user signs in again when needed.
    if (!this->m_sessionSaved)
        qWarning() << "The GitHub tokens cannot be saved in the system keyring, they will be forgotten when the application closes";
}

void GitHubAuthPrivate::clearTokens()
{
    if (this->m_userReply) {
        this->m_userReply->abort();
        this->m_userReply = nullptr;
    }

    this->setUserName({});
    this->m_sessionSaved = false;
    this->m_accessToken.clear();
    this->m_refreshToken.clear();
    this->m_accessExpires = QDateTime();
    this->m_refreshExpires = QDateTime();

    if (TokenStore::remove(TOKEN_STORE_NAME) == TokenStore::Unavailable)
        qWarning() << "The GitHub tokens cannot be removed from the system keyring";

    clearLegacyTokens();
}

void GitHubAuth::fetchUserName()
{
    if (this->d->m_accessToken.isEmpty() || this->d->m_userReply)
        return;

    QNetworkRequest request {QUrl(QStringLiteral(API_BASE "/user"))};
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("Authorization",
                         "Bearer " + this->d->m_accessToken.toUtf8());
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setRawHeader("User-Agent", "Webcamoid");
    auto reply = this->d->m_manager->get(request);
    this->d->m_userReply = reply;
    QObject::connect(reply,
                     &QNetworkReply::finished,
                     this,
                     [this, reply] () { this->d->onUser(reply); });
}

void GitHubAuthPrivate::onUser(QNetworkReply *reply)
{
    reply->deleteLater();

    // It was canceled, for example because the user signed out.
    if (reply != this->m_userReply)
        return;

    this->m_userReply = nullptr;

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return; // it is only informative, it is not an error

    this->setUserName(parseReply(reply).value("login").toString());
}

void GitHubAuthPrivate::setUserName(const QString &userName)
{
    if (this->m_userName == userName)
        return;

    this->m_userName = userName;

    // It is not a secret, so it is kept to show it on startup.
    QSettings settings;
    settings.beginGroup("GitHubAuth");

    if (userName.isEmpty())
        settings.remove("login");
    else
        settings.setValue("login", userName);

    settings.endGroup();
    emit self->userNameChanged();
}

#include "moc_githubauth.cpp"

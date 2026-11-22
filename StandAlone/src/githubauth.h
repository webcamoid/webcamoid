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

#ifndef GITHUBAUTH_H
#define GITHUBAUTH_H

#include <QObject>

class GitHubAuthPrivate;
class QNetworkAccessManager;
class QNetworkReply;
class QUrl;

/* GitHub sign-in using the Device Flow (GitHub App) and token handling.
 *
 * It only needs the Client ID of the GitHub App: no client secret is used,
 * neither to sign in nor to refresh the token.
 */
class GitHubAuth: public QObject
{
    Q_OBJECT

    public:
        GitHubAuth(const QString &clientId,
                   QNetworkAccessManager *manager,
                   QObject *parent=nullptr);
        ~GitHubAuth();

        Q_INVOKABLE QString accessToken() const;
        Q_INVOKABLE bool accessTokenValid() const;   // exists and is not about to expire
        Q_INVOKABLE bool canRefresh() const;         // there is a refresh token that has not expired
        Q_INVOKABLE bool loginInProgress() const;
        // Login of the GitHub account, empty if it is not known yet.
        Q_INVOKABLE QString userName() const;
        // The tokens are saved in the system keyring. If it is not available,
        // they are only kept in memory and the user must sign in again the
        // next time the application starts.
        Q_INVOKABLE bool sessionSaved() const;

    private:
        GitHubAuthPrivate *d;

    signals:
        // The user must open verificationUri and enter userCode.
        void deviceCodeReady(const QString &userCode,
                             const QUrl &verificationUri,
                             int expiresInSecs);
        void loggedIn();
        void userNameChanged();
        void loginFailed(const QString &error);
        void tokenRefreshed();
        void refreshFailed(const QString &error);

    public slots:
        void loadStoredTokens();
        void login();                    // starts the Device Flow
        void cancelLogin();
        void refresh();                  // renews using the refresh token
        void logout();                   // removes the stored tokens
        void fetchUserName();            // asks GitHub for the login of the account
};

#endif // GITHUBAUTH_H

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

#ifndef TOKENSTORE_H
#define TOKENSTORE_H

#include <QByteArray>
#include <QString>

/* Storage of small secrets in the keyring of the system, using only what the
 * system and Qt provide:
 *
 *   Windows:   Credential Manager
 *   macOS:     Keychain, only if the binary is signed with a certificate
 *   Linux/BSD: Secret Service (GNOME Keyring, KWallet, KeePassXC...) through
 *              D-Bus, if Qt D-Bus is available
 *
 * In any other case, and when the keyring cannot be used, it reports
 * Unavailable and the caller must have a fallback.
 */
class TokenStore
{
    public:
        enum Result {
            Ok,
            NotFound,
            Unavailable    // no keyring, it could not be unlocked or an error
        };

        static Result read(const QString &name, QByteArray *data);
        static Result write(const QString &name, const QByteArray &data);
        static Result remove(const QString &name);
};

#endif // TOKENSTORE_H

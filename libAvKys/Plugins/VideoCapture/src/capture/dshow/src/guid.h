/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef GUID_H
#define GUID_H

#include <QObject>

class GuidPrivate;

class Guid: public QObject
{
    Q_OBJECT

    public:
        Guid(QObject *parent=nullptr);
        Guid(const QByteArray &data);
        Guid(const char *data, size_t len=-1);
        Guid(const Guid &other);
        ~Guid();
        Guid &operator =(const Guid &other);
        bool operator ==(const Guid &other) const;
        bool operator <(const Guid &other) const;
        operator bool() const;

        Q_INVOKABLE operator QString() const;
        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE static Guid fromString(const QString &str);
        Q_INVOKABLE quint8 *data() const;
        Q_INVOKABLE const quint8 *constData() const;

    private:
        GuidPrivate *d;
};

QDebug operator <<(QDebug debug, const Guid &guid);

#endif // GUID_H

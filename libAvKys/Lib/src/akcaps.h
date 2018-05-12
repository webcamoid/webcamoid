/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#ifndef AKCAPS_H
#define AKCAPS_H

#include <QObject>

#include "akcommons.h"

class AkCapsPrivate;
class QDataStream;

class AKCOMMONS_EXPORT AkCaps: public QObject
{
    Q_OBJECT
    Q_ENUMS(CapsType)
    Q_PROPERTY(bool isValid
               READ isValid)
    Q_PROPERTY(QString mimeType
               READ mimeType
               WRITE setMimeType
               RESET resetMimeType
               NOTIFY mimeTypeChanged)

    public:
        enum CapsType
        {
            CapsUnknown = -1,
            CapsAudio,
            CapsVideo,
            CapsSubtitle
        };

        explicit AkCaps(QObject *parent=nullptr);
        AkCaps(const QVariantMap &caps);
        AkCaps(const QString &caps);
        AkCaps(const AkCaps &other);
        virtual ~AkCaps();
        AkCaps &operator =(const AkCaps &other);
        AkCaps &operator =(const QString &other);
        bool operator ==(const AkCaps &other) const;
        bool operator ==(const QString &caps) const;
        bool operator !=(const AkCaps &other) const;
        bool operator !=(const QString &caps) const;
        operator bool() const;

        Q_INVOKABLE virtual bool isValid() const;
        Q_INVOKABLE virtual bool &isValid();
        Q_INVOKABLE virtual QString mimeType() const;
        Q_INVOKABLE AkCaps &fromMap(const QVariantMap &caps);
        Q_INVOKABLE AkCaps &fromString(const QString &caps);
        Q_INVOKABLE QVariantMap toMap() const;
        Q_INVOKABLE virtual QString toString() const;
        Q_INVOKABLE AkCaps &update(const AkCaps &other);
        Q_INVOKABLE bool isCompatible(const AkCaps &other) const;
        Q_INVOKABLE bool contains(const QString &property) const;

    private:
        AkCapsPrivate *d;

    Q_SIGNALS:
        void mimeTypeChanged(const QString &mimeType);

    public Q_SLOTS:
        virtual void setMimeType(const QString &mimeType);
        virtual void resetMimeType();
        void clear();

    friend QDebug operator <<(QDebug debug, const AkCaps &caps);
    friend QDataStream &operator >>(QDataStream &istream, AkCaps &caps);
    friend QDataStream &operator <<(QDataStream &ostream, const AkCaps &caps);
};

QDebug operator <<(QDebug debug, const AkCaps &caps);
QDataStream &operator >>(QDataStream &istream, AkCaps &caps);
QDataStream &operator <<(QDataStream &ostream, const AkCaps &caps);

Q_DECLARE_METATYPE(AkCaps)
Q_DECLARE_METATYPE(AkCaps::CapsType)

#endif // AKCAPS_H

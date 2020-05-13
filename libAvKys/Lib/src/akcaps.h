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

        AkCaps(const QString &mimeType={}, QObject *parent=nullptr);
        AkCaps(const AkCaps &other);
        virtual ~AkCaps();
        AkCaps &operator =(const AkCaps &other);
        bool operator ==(const AkCaps &other) const;
        bool operator !=(const AkCaps &other) const;
        operator bool() const;

        Q_INVOKABLE static QObject *create(const QString &mimeType={});
        Q_INVOKABLE static QObject *create(const AkCaps &caps);
        Q_INVOKABLE QVariant toVariant() const;
        Q_INVOKABLE virtual QString mimeType() const;
        Q_INVOKABLE static AkCaps fromMap(const QVariantMap &caps);
        Q_INVOKABLE QVariantMap toMap() const;
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
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkCaps &caps);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkCaps &caps);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkCaps &caps);

Q_DECLARE_METATYPE(AkCaps)
Q_DECLARE_METATYPE(AkCaps::CapsType)

#endif // AKCAPS_H

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

#ifndef AKCOMPRESSEDCAPS_H
#define AKCOMPRESSEDCAPS_H

#include <QObject>

#include "akcommons.h"

class AkCompressedCapsPrivate;
class AkCompressedAudioCaps;
class AkCompressedVideoCaps;
class QDataStream;

using AkCodecID = quint32;

class AKCOMMONS_EXPORT AkCompressedCaps: public QObject
{
    Q_OBJECT
    Q_PROPERTY(CapsType type
               READ type
               CONSTANT)
    Q_PROPERTY(AkCodecID codecID
               READ codecID
               CONSTANT)

    public:
        enum CapsType
        {
            CapsType_Unknown = -1,
            CapsType_Any = CapsType_Unknown,
            CapsType_Audio,
            CapsType_Video,
        };
        Q_ENUM(CapsType)

        AkCompressedCaps(QObject *parent=nullptr);
        AkCompressedCaps(const AkCompressedCaps &other);
        ~AkCompressedCaps();
        AkCompressedCaps &operator =(const AkCompressedCaps &other);
        bool operator ==(const AkCompressedCaps &other) const;
        bool operator !=(const AkCompressedCaps &other) const;
        operator bool() const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const AkCompressedCaps &caps);
        Q_INVOKABLE QVariant toVariant() const;
        Q_INVOKABLE AkCompressedCaps::CapsType type() const;
        Q_INVOKABLE AkCodecID codecID() const;

    private:
        AkCompressedCapsPrivate *d;

        using DataCopy = void *(*)(void *data);
        using DataDeleter = void (*)(void *data);
        void *privateData() const;
        void setPrivateData(void *data,
                            DataCopy copyFunc,
                            DataDeleter deleterFunc);
        void setType(AkCompressedCaps::CapsType type);
        void setCodecID(AkCodecID codecID);

    public Q_SLOTS:
        static void registerTypes();

    friend QDebug operator <<(QDebug debug, const AkCompressedCaps &caps);
    friend QDataStream &operator >>(QDataStream &istream, AkCompressedCaps &caps);
    friend QDataStream &operator <<(QDataStream &ostream, const AkCompressedCaps &caps);
    friend class AkCompressedCapsPrivate;
    friend class AkCompressedAudioCaps;
    friend class AkCompressedVideoCaps;
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkCompressedCaps &caps);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkCompressedCaps &caps);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkCompressedCaps &caps);

Q_DECLARE_METATYPE(AkCompressedCaps)
Q_DECLARE_METATYPE(AkCompressedCaps::CapsType)

#endif // AKCOMPRESSEDCAPS_H

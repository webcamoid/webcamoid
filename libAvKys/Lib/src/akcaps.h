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
class AkAudioCaps;
class AkCompressedVideoCaps;
class AkSubtitleCaps;
class AkVideoCaps;
class QDataStream;

class AKCOMMONS_EXPORT AkCaps: public QObject
{
    Q_OBJECT
    Q_PROPERTY(CapsType type
               READ type
               CONSTANT)

    public:
        enum CapsType
        {
            CapsUnknown = -1,
            CapsAny = CapsUnknown,
            CapsAudio,
            CapsAudioCompressed,
            CapsVideo,
            CapsVideoCompressed,
            CapsSubtitle
        };
        Q_ENUM(CapsType)

        AkCaps(QObject *parent=nullptr);
        AkCaps(const AkCaps &other);
        ~AkCaps();
        AkCaps &operator =(const AkCaps &other);
        bool operator ==(const AkCaps &other) const;
        bool operator !=(const AkCaps &other) const;
        operator bool() const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const AkCaps &caps);
        Q_INVOKABLE QVariant toVariant() const;
        Q_INVOKABLE AkCaps::CapsType type() const;

    private:
        AkCapsPrivate *d;

        using DataCopy = std::function<void *(void *data)>;
        using DataDeleter = std::function<void (void *data)>;
        void *privateData() const;
        void setPrivateData(void *data,
                            DataCopy copyFunc,
                            DataDeleter deleterFunc);
        void setType(AkCaps::CapsType type);

    public Q_SLOTS:
        static void registerTypes();

    friend QDebug operator <<(QDebug debug, const AkCaps &caps);
    friend QDataStream &operator >>(QDataStream &istream, AkCaps &caps);
    friend QDataStream &operator <<(QDataStream &ostream, const AkCaps &caps);
    friend class AkAudioCaps;
    friend class AkCapsPrivate;
    friend class AkCompressedVideoCaps;
    friend class AkSubtitleCaps;
    friend class AkVideoCaps;
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkCaps &caps);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkCaps &caps);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkCaps &caps);

Q_DECLARE_METATYPE(AkCaps)
Q_DECLARE_METATYPE(AkCaps::CapsType)

#endif // AKCAPS_H

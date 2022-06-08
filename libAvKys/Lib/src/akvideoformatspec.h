/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#ifndef AKVIDEOFORMATSPEC_H
#define AKVIDEOFORMATSPEC_H

#include "akcolorcomponent.h"

class AkVideoFormatSpec;
class AkVideoFormatSpecPrivate;

using AkColorPlanes = QVector<AkColorComponentList>;

class AKCOMMONS_EXPORT AkVideoFormatSpec: public QObject
{
    Q_OBJECT
    Q_PROPERTY(VideoFormatType type
               READ type
               WRITE setType
               RESET resetType
               NOTIFY typeChanged)
    Q_PROPERTY(int endianness
               READ endianness
               WRITE setEndianness
               RESET resetEndianness
               NOTIFY endiannessChanged)
    Q_PROPERTY(AkColorPlanes planes
               READ planes
               WRITE setPlanes
               RESET resetPlanes
               NOTIFY planesChanged)
    Q_PROPERTY(int bpp
               READ bpp
               CONSTANT)

    public:
        enum VideoFormatType
        {
            VFT_Unknown,
            VFT_RGB,
            VFT_YUV,
            VFT_Gray
        };
        Q_ENUM(VideoFormatType)

        AkVideoFormatSpec(QObject *parent=nullptr);
        AkVideoFormatSpec(VideoFormatType type,
                          int endianness,
                          const AkColorPlanes &planes);
        AkVideoFormatSpec(const AkVideoFormatSpec &other);
        ~AkVideoFormatSpec();
        AkVideoFormatSpec &operator =(const AkVideoFormatSpec &other);
        bool operator ==(const AkVideoFormatSpec &other) const;
        bool operator !=(const AkVideoFormatSpec &other) const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const AkVideoFormatSpec &other);
        Q_INVOKABLE static QObject *create(AkVideoFormatSpec::VideoFormatType type,
                                           int endianness,
                                           const AkColorPlanes &planes);
        Q_INVOKABLE QVariant toVariant() const;

        Q_INVOKABLE AkVideoFormatSpec::VideoFormatType type() const;
        Q_INVOKABLE int endianness() const;
        Q_INVOKABLE AkColorPlanes planes() const;
        Q_INVOKABLE int bpp() const;
        Q_INVOKABLE AkColorComponent component(AkColorComponent::ComponentType componentType) const;
        Q_INVOKABLE int componentPlane(AkColorComponent::ComponentType component) const;
        Q_INVOKABLE bool contains(AkColorComponent::ComponentType component) const;
        Q_INVOKABLE size_t byteLength() const;
        Q_INVOKABLE size_t numberOfComponents() const;
        Q_INVOKABLE size_t mainComponents() const;

    private:
        AkVideoFormatSpecPrivate *d;

    Q_SIGNALS:
        void typeChanged(AkVideoFormatSpec::VideoFormatType type);
        void endiannessChanged(int endianness);
        void planesChanged(const AkColorPlanes &planes);

    public Q_SLOTS:
        void setType(AkVideoFormatSpec::VideoFormatType type);
        void setEndianness(int endianness);
        void setPlanes(const AkColorPlanes &planes);
        void resetType();
        void resetEndianness();
        void resetPlanes();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkVideoFormatSpec &caps);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkVideoFormatSpec::VideoFormatType format);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkVideoFormatSpec &caps);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkVideoFormatSpec &caps);

Q_DECLARE_METATYPE(AkVideoFormatSpec)
Q_DECLARE_METATYPE(AkVideoFormatSpec::VideoFormatType)
Q_DECLARE_METATYPE(AkColorPlanes)

#endif // AKVIDEOFORMATSPEC_H

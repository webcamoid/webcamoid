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

#ifndef AKCOLORPLANE_H
#define AKCOLORPLANE_H

#include "akcolorcomponent.h"

class AkColorPlane;
class AkColorPlanePrivate;

using AkColorPlanes = QVector<AkColorPlane>;

class AKCOMMONS_EXPORT AkColorPlane: public QObject
{
    Q_OBJECT
    Q_PROPERTY(size_t components
               READ components
               CONSTANT)
    Q_PROPERTY(size_t bitsSize
               READ bitsSize
               CONSTANT)
    Q_PROPERTY(size_t pixelSize
               READ pixelSize
               CONSTANT)
    Q_PROPERTY(size_t heightDiv
               READ heightDiv
               CONSTANT)

    public:
        AkColorPlane(QObject *parent=nullptr);
        AkColorPlane(const AkColorComponentList &components,
                     size_t bitsSize);
        AkColorPlane(const AkColorPlane &other);
        ~AkColorPlane();
        AkColorPlane &operator =(const AkColorPlane &other);
        bool operator ==(const AkColorPlane &other) const;
        bool operator !=(const AkColorPlane &other) const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const AkColorPlane &colorPlane);
        Q_INVOKABLE static QObject *create(const AkColorComponentList &components,
                                           size_t bitsSize);
        Q_INVOKABLE QVariant toVariant() const;

        Q_INVOKABLE size_t components() const;
        Q_INVOKABLE const AkColorComponent &component(size_t component) const;
        Q_INVOKABLE size_t bitsSize() const;
        Q_INVOKABLE size_t pixelSize() const;
        Q_INVOKABLE size_t widthDiv() const;
        Q_INVOKABLE size_t heightDiv() const;

    private:
        AkColorPlanePrivate *d;

    public Q_SLOTS:
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkColorPlane &plane);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkColorPlane &plane);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkColorPlane &plane);

Q_DECLARE_METATYPE(AkColorPlane)
Q_DECLARE_METATYPE(AkColorPlanes)

#endif // AKCOLORPLANE_H

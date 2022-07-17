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
    Q_PROPERTY(AkColorComponentList components
               READ components
               WRITE setComponents
               RESET resetComponents
               NOTIFY componentsChanged)
    Q_PROPERTY(size_t bitsSize
               READ bitsSize
               WRITE setBitsSize
               RESET resetBitsSize
               NOTIFY bitsSizeChanged)

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

        Q_INVOKABLE AkColorComponentList components() const;
        Q_INVOKABLE size_t bitsSize() const;

    private:
        AkColorPlanePrivate *d;

    Q_SIGNALS:
        void componentsChanged(const AkColorComponentList &components);
        void bitsSizeChanged(size_t bitsSize);

    public Q_SLOTS:
        void setComponents(const AkColorComponentList &components);
        void setBitsSize(size_t bitsSize);
        void resetComponents();
        void resetBitsSize();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkColorPlane &plane);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkColorPlane &plane);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkColorPlane &plane);

Q_DECLARE_METATYPE(AkColorPlane)
Q_DECLARE_METATYPE(AkColorPlanes)

#endif // AKCOLORPLANE_H

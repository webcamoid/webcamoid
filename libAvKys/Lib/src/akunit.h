/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

#ifndef AKUNIT_H
#define AKUNIT_H

#include <QObject>
#include <QVariant>

#include "akcommons.h"

class AkUnitPrivate;
class QWindow;
class QQuickItem;

class AKCOMMONS_EXPORT AkUnit: public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal value
               READ value
               WRITE setValue
               RESET resetValue
               NOTIFY valueChanged)
    Q_PROPERTY(Unit unit
               READ unit
               WRITE setUnit
               RESET resetUnit
               NOTIFY unitChanged)
    Q_PROPERTY(int pixels
               READ pixels
               NOTIFY pixelsChanged)

    public:
        enum Unit
        {
            // Absolute
            cm,
            mm,
            in,
            px,
            pt,
            pc,
            dp,

            // Relative
            vw,
            vh,
            vmin,
            vmax,
        };
        Q_ENUM(Unit)

        AkUnit(qreal value=0.0, Unit unit=px);
        AkUnit(qreal value, const QString &unit);
        AkUnit(qreal value, Unit unit, QWindow *parent);
        AkUnit(qreal value, const QString &unit, QWindow *parent);
        AkUnit(qreal value, Unit unit, QQuickItem *parent);
        AkUnit(qreal value, const QString &unit, QQuickItem *parent);
        AkUnit(const AkUnit &other);
        virtual ~AkUnit();
        AkUnit &operator =(const AkUnit &other);
        bool operator ==(const AkUnit &other) const;
        bool operator !=(const AkUnit &other) const;
        operator int() const;
        operator QString() const;

        Q_INVOKABLE static QObject *create(qreal value=0.0,
                                           AkUnit::Unit unit=AkUnit::px);
        Q_INVOKABLE static QObject *create(qreal value, const QString &unit);
        Q_INVOKABLE static QObject *create(qreal value,
                                           AkUnit::Unit unit,
                                           QObject *parent);
        Q_INVOKABLE static QObject *create(qreal value,
                                           const QString &unit,
                                           QObject *parent);
        Q_INVOKABLE QVariant toVariant() const;

        Q_INVOKABLE qreal value() const;
        Q_INVOKABLE AkUnit::Unit unit() const;
        Q_INVOKABLE int pixels() const;
        Q_INVOKABLE AkUnit convert(AkUnit::Unit unit) const;
        Q_INVOKABLE AkUnit convert(const QString &unit) const;

    private:
        AkUnitPrivate *d;

    signals:
        void valueChanged(qreal value);
        void unitChanged(AkUnit::Unit unit);
        void pixelsChanged(int pixels);

    public slots:
        void setValue(qreal value);
        void setUnit(AkUnit::Unit unit);
        void resetValue();
        void resetUnit();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkUnit &unit);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkUnit &unit);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkUnit &unit);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkUnit::Unit &unit);

Q_DECLARE_METATYPE(AkUnit)
Q_DECLARE_METATYPE(AkUnit::Unit)

#endif // AKUNIT_H

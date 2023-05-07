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

#ifndef AKFRAC_H
#define AKFRAC_H

#include <QObject>

#include "akcommons.h"

class AkFracPrivate;
class QDataStream;

class AKCOMMONS_EXPORT AkFrac: public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 num
               READ num
               WRITE setNum
               RESET resetNum
               NOTIFY numChanged)
    Q_PROPERTY(qint64 den
               READ den
               WRITE setDen
               RESET resetDen
               NOTIFY denChanged)
    Q_PROPERTY(bool isValid
               READ isValid
               NOTIFY isValidChanged)
    Q_PROPERTY(qreal value
               READ value
               NOTIFY valueChanged)
    Q_PROPERTY(QString string
               READ toString
               NOTIFY stringChanged)

    public:
        AkFrac(QObject *parent=nullptr);
        AkFrac(qint64 num, qint64 den);
        AkFrac(const QString &fracString);
        AkFrac(const AkFrac &other);
        virtual ~AkFrac();
        AkFrac &operator =(const AkFrac &other);
        bool operator ==(const AkFrac &other) const;
        bool operator !=(const AkFrac &other) const;
        operator bool() const;
        operator QString() const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(qint64 num, qint64 den);
        Q_INVOKABLE static QObject *create(const QString &frac);
        Q_INVOKABLE static QObject *create(const AkFrac &frac);
        Q_INVOKABLE static QVariant createVariant(qint64 num, qint64 den);
        Q_INVOKABLE QVariant toVariant() const;
        Q_INVOKABLE qint64 num() const;
        Q_INVOKABLE qint64 den() const;
        Q_INVOKABLE qreal value() const;
        Q_INVOKABLE qint64 fastValue() const;
        Q_INVOKABLE bool isValid() const;
        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE AkFrac invert() const;

    private:
        AkFracPrivate *d;

    Q_SIGNALS:
        void numChanged(qint64 num);
        void denChanged(qint64 den);
        void isValidChanged(bool valid);
        void valueChanged(qreal value);
        void stringChanged(const QString &string);

    public Q_SLOTS:
        void setNumDen(qint64 num, qint64 den);
        void setNumDen(const QString &fracString);
        void setNum(qint64 num);
        void setDen(qint64 den);
        void resetNum();
        void resetDen();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkFrac &frac);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkFrac &frac);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkFrac &frac);
AKCOMMONS_EXPORT AkFrac operator *(int number, const AkFrac &frac);
AKCOMMONS_EXPORT AkFrac operator *(qreal number, const AkFrac &frac);
AKCOMMONS_EXPORT AkFrac operator *(const AkFrac &frac1, const AkFrac &frac2);
AKCOMMONS_EXPORT AkFrac operator /(int number, const AkFrac &frac);
AKCOMMONS_EXPORT AkFrac operator /(const AkFrac &fracNum, const AkFrac &fracDen);
AKCOMMONS_EXPORT AkFrac operator +(const AkFrac &frac1, const AkFrac &frac2);
AKCOMMONS_EXPORT AkFrac operator -(const AkFrac &frac1, const AkFrac &frac2);

Q_DECLARE_METATYPE(AkFrac)

#endif // AKFRAC_H

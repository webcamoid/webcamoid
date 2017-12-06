/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

class AKCOMMONS_EXPORT AkFrac: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int num
               READ num
               WRITE setNum
               RESET resetNum
               NOTIFY numChanged)
    Q_PROPERTY(int den
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
        explicit AkFrac(QObject *parent=nullptr);
        AkFrac(qint64 num, qint64 den);
        AkFrac(const QString &fracString);
        AkFrac(const AkFrac &other);
        virtual ~AkFrac();
        AkFrac &operator =(const AkFrac &other);
        bool operator ==(const AkFrac &other) const;
        bool operator !=(const AkFrac &other) const;
        AkFrac operator *(const AkFrac &other) const;

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
        void numChanged();
        void denChanged();
        void isValidChanged();
        void valueChanged();
        void stringChanged();

    public Q_SLOTS:
        void setNumDen(qint64 num, qint64 den);
        void setNumDen(const QString &fracString);
        void setNum(qint64 num);
        void setDen(qint64 den);
        void resetNum();
        void resetDen();

    friend QDebug operator <<(QDebug debug, const AkFrac &frac);
    friend QDataStream &operator >>(QDataStream &istream, AkFrac &frac);
    friend QDataStream &operator <<(QDataStream &ostream, const AkFrac &frac);
    friend AkFrac operator *(int number, const AkFrac &frac);
    friend AkFrac operator /(const AkFrac &fracNum, const AkFrac &fracDen);
    friend AkFrac operator +(const AkFrac &frac1, const AkFrac &frac2);
    friend AkFrac operator -(const AkFrac &frac1, const AkFrac &frac2);
};

QDebug operator <<(QDebug debug, const AkFrac &frac);
QDataStream &operator >>(QDataStream &istream, AkFrac &frac);
QDataStream &operator <<(QDataStream &ostream, const AkFrac &frac);
AkFrac operator *(int number, const AkFrac &frac);
AkFrac operator /(int number, const AkFrac &frac);
AkFrac operator /(const AkFrac &fracNum, const AkFrac &fracDen);
AkFrac operator +(const AkFrac &frac1, const AkFrac &frac2);
AkFrac operator -(const AkFrac &frac1, const AkFrac &frac2);

Q_DECLARE_METATYPE(AkFrac)

#endif // AKFRAC_H

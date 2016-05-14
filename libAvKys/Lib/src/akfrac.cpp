/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include <QRegExp>
#include <QStringList>
#include <QDataStream>

#include "akfrac.h"

#define SIGN(n) ((n < 0)? -1: 1)

class AkFracPrivate
{
    public:
        qint64 m_num;
        qint64 m_den;
        bool m_isValid;

        static inline qint64 gcd(qint64 num, qint64 den)
        {
            num = qAbs(num);
            den = qAbs(den);

            while (num > 0) {
                qint64 tmp = num;
                num = den % num;
                den = tmp;
            }

            return den;
        }

        static inline void reduce(qint64 *num, qint64 *den)
        {
            qint64 gcd = AkFracPrivate::gcd(*num, *den);

            if (gcd) {
                *num /= gcd;
                *den /= gcd;
            }
        }
};

AkFrac::AkFrac(QObject *parent):
    QObject(parent)
{
    this->d = new AkFracPrivate();
    this->d->m_num = 0;
    this->d->m_den = 0;
    this->d->m_isValid = false;
}

AkFrac::AkFrac(qint64 num, qint64 den):
    QObject(NULL)
{
    this->d = new AkFracPrivate();
    this->d->m_num = 0;
    this->d->m_den = 0;
    this->d->m_isValid = false;

    this->setNumDen(num, den);
}

AkFrac::AkFrac(const QString &fracString):
    QObject(NULL)
{
    this->d = new AkFracPrivate();
    this->d->m_num = 0;
    this->d->m_den = 0;
    this->d->m_isValid = false;

    this->setNumDen(fracString);
}

AkFrac::AkFrac(const AkFrac &other):
    QObject()
{
    this->d = new AkFracPrivate();
    this->d->m_num = other.d->m_num;
    this->d->m_den = other.d->m_den;
    this->d->m_isValid = other.d->m_isValid;
}

AkFrac::~AkFrac()
{
    delete this->d;
}

AkFrac &AkFrac::operator =(const AkFrac &other)
{
    if (this != &other) {
        this->d->m_num = other.d->m_num;
        this->d->m_den = other.d->m_den;
        this->d->m_isValid = other.d->m_isValid;
    }

    return *this;
}

bool AkFrac::operator ==(const AkFrac &other) const
{
    if (this->toString() == other.toString())
        return true;

    return false;
}

bool AkFrac::operator !=(const AkFrac &other) const
{
    return !(*this == other);
}

AkFrac AkFrac::operator *(const AkFrac &other) const
{
    return AkFrac(this->d->m_num * other.d->m_num,
                  this->d->m_den * other.d->m_den);
}

qint64 AkFrac::num() const
{
    return this->d->m_num;
}

qint64 AkFrac::den() const
{
    return this->d->m_den;
}

double AkFrac::value() const
{
    return this->d->m_num / double(this->d->m_den);
}

qint64 AkFrac::fastValue() const
{
    return this->d->m_num / this->d->m_den;
}

bool AkFrac::isValid() const
{
    return this->d->m_isValid;
}

QString AkFrac::toString() const
{
    return QString("%1/%2")
            .arg(this->d->m_num)
            .arg(this->d->m_den);
}

AkFrac AkFrac::invert() const
{
    return AkFrac(this->d->m_den,
                  this->d->m_num);
}

void AkFrac::setNumDen(qint64 num, qint64 den)
{
    bool changed = false;

    if (!den) {
        if (this->d->m_num != 0) {
            this->d->m_num = 0;
            changed = true;

            emit this->numChanged();
        }

        if (this->d->m_den != 0) {
            this->d->m_den = 0;
            changed = true;

            emit this->denChanged();
        }

        if (this->d->m_isValid != false) {
            this->d->m_isValid = false;
            changed = true;

            emit this->isValidChanged();
        }

        if (changed) {
            emit this->valueChanged();
            emit this->stringChanged();
        }

        return;
    }

    num = SIGN(den) * num;
    den = qAbs(den);
    AkFracPrivate::reduce(&num, &den);

    if (this->d->m_num != num) {
        this->d->m_num = num;
        changed = true;

        emit this->numChanged();
    }

    if (this->d->m_den != den) {
        this->d->m_den = den;
        changed = true;

        emit this->denChanged();
    }

    if (this->d->m_isValid != true) {
        this->d->m_isValid = true;
        changed = true;

        emit this->isValidChanged();
    }

    if (changed) {
        emit this->valueChanged();
        emit this->stringChanged();
    }
}

void AkFrac::setNumDen(const QString &fracString)
{

    bool match = QRegExp("(\\s*-)?\\s*\\d+\\s*/"
                              "\\s*\\d+\\s*").exactMatch(fracString);

    if (!match) {
        this->setNumDen(0, 0);

        return;
    }

    QStringList fracChunks = fracString.split(QRegExp("\\s*/\\s*"),
                                              QString::SkipEmptyParts);

    qint64 num = fracChunks[0].trimmed().toInt();
    qint64 den = fracChunks[1].trimmed().toInt();

    this->setNumDen(num, den);
}

void AkFrac::setNum(qint64 num)
{
    this->setNumDen(num, this->d->m_den);
}

void AkFrac::setDen(qint64 den)
{
    this->setNumDen(this->d->m_num, den);
}

void AkFrac::resetNum()
{
    this->setNum(0);
}

void AkFrac::resetDen()
{
    this->setDen(0);
}

QDebug operator <<(QDebug debug, const AkFrac &frac)
{
    debug.nospace() << frac.toString();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkFrac &frac)
{
    istream >> frac.d->m_num;
    istream >> frac.d->m_den;
    istream >> frac.d->m_isValid;

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkFrac &frac)
{
    ostream << frac.d->m_num;
    ostream << frac.d->m_den;
    ostream << frac.d->m_isValid;

    return ostream;
}

AkFrac operator *(int number, const AkFrac &frac)
{
    return AkFrac(number * frac.d->m_num, frac.d->m_den);
}

AkFrac operator /(int number, const AkFrac &frac)
{
    return number * frac.invert();
}


AkFrac operator /(const AkFrac &fracNum, const AkFrac &fracDen)
{
    return AkFrac(fracNum.d->m_num * fracDen.d->m_den,
                  fracNum.d->m_den * fracDen.d->m_num);
}

AkFrac operator +(const AkFrac &frac1, const AkFrac &frac2)
{
    return AkFrac(frac1.d->m_num * frac2.d->m_den
                  + frac2.d->m_num * frac1.d->m_den,
                  frac1.d->m_den * frac2.d->m_den);
}

AkFrac operator -(const AkFrac &frac1, const AkFrac &frac2)
{
    return AkFrac(frac1.d->m_num * frac2.d->m_den
                  - frac2.d->m_num * frac1.d->m_den,
                  frac1.d->m_den * frac2.d->m_den);
}

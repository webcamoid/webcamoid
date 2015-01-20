/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include <QRegExp>
#include <QStringList>

#include "qbfrac.h"

#define SIGN(n) ((n < 0)? -1: 1)

class QbFracPrivate
{
    public:
        int m_num;
        int m_den;
        bool m_isValid;

        static inline qint64 gcd(qint64 num, qint64 den)
        {
            num = abs(num);
            den = abs(den);

            while (num > 0) {
                qint64 tmp = num;
                num = den % num;
                den = tmp;
            }

            return den;
        }

        static inline void reduce(qint64 *num, qint64 *den)
        {
            qint64 gcd = QbFracPrivate::gcd(*num, *den);

            if (gcd) {
                *num /= gcd;
                *den /= gcd;
            }
        }
};

QbFrac::QbFrac(QObject *parent):
    QObject(parent)
{
    this->d = new QbFracPrivate();
    this->d->m_num = 0;
    this->d->m_den = 0;
    this->d->m_isValid = false;
}

QbFrac::QbFrac(qint64 num, qint64 den):
    QObject(NULL)
{
    this->d = new QbFracPrivate();
    this->d->m_num = 0;
    this->d->m_den = 0;
    this->d->m_isValid = false;

    this->setNumDen(num, den);
}

QbFrac::QbFrac(const QString &fracString):
    QObject(NULL)
{
    this->d = new QbFracPrivate();
    this->d->m_num = 0;
    this->d->m_den = 0;
    this->d->m_isValid = false;

    this->setNumDen(fracString);
}

QbFrac::QbFrac(const QbFrac &other):
    QObject(other.parent())
{
    this->d = new QbFracPrivate();
    this->d->m_num = other.d->m_num;
    this->d->m_den = other.d->m_den;
    this->d->m_isValid = other.d->m_isValid;
}

QbFrac::~QbFrac()
{
    delete this->d;
}

QbFrac &QbFrac::operator =(const QbFrac &other)
{
    if (this != &other) {
        this->d->m_num = other.d->m_num;
        this->d->m_den = other.d->m_den;
        this->d->m_isValid = other.d->m_isValid;
    }

    return *this;
}

bool QbFrac::operator ==(const QbFrac &other) const
{
    if (this->toString() == other.toString())
        return true;

    return false;
}

bool QbFrac::operator !=(const QbFrac &other) const
{
    return !(*this == other);
}

QbFrac QbFrac::operator *(const QbFrac &other) const
{
    return QbFrac(this->d->m_num * other.d->m_num,
                  this->d->m_den * other.d->m_den);
}

qint64 QbFrac::num() const
{
    return this->d->m_num;
}

qint64 QbFrac::den() const
{
    return this->d->m_den;
}

double QbFrac::value() const
{
    return this->d->m_num / double(this->d->m_den);
}

int QbFrac::fastValue() const
{
    return this->d->m_num / this->d->m_den;
}

bool QbFrac::isValid() const
{
    return this->d->m_isValid;
}

QString QbFrac::toString() const
{
    return QString("%1/%2")
            .arg(this->d->m_num)
            .arg(this->d->m_den);
}

QbFrac QbFrac::invert() const
{
    return QbFrac(this->d->m_den,
                  this->d->m_num);
}

void QbFrac::setNumDen(qint64 num, qint64 den)
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
    den = abs(den);
    QbFracPrivate::reduce(&num, &den);

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

void QbFrac::setNumDen(const QString &fracString)
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

void QbFrac::setNum(qint64 num)
{
    this->setNumDen(num, this->d->m_den);
}

void QbFrac::setDen(qint64 den)
{
    this->setNumDen(this->d->m_num, den);
}

void QbFrac::resetNum()
{
    this->setNum(0);
}

void QbFrac::resetDen()
{
    this->setDen(0);
}

QDebug operator <<(QDebug debug, const QbFrac &frac)
{
    debug.nospace() << frac.toString();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, QbFrac &frac)
{
    istream >> frac.d->m_num;
    istream >> frac.d->m_den;
    istream >> frac.d->m_isValid;

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const QbFrac &frac)
{
    ostream << frac.d->m_num;
    ostream << frac.d->m_den;
    ostream << frac.d->m_isValid;

    return ostream;
}

QbFrac operator *(int number, const QbFrac &frac)
{
    return QbFrac(number * frac.d->m_num, frac.d->m_den);
}

QbFrac operator /(int number, const QbFrac &frac)
{
    return number * frac.invert();
}


QbFrac operator /(const QbFrac &fracNum, const QbFrac &fracDen)
{
    return QbFrac(fracNum.d->m_num * fracDen.d->m_den,
                  fracNum.d->m_den * fracDen.d->m_num);
}

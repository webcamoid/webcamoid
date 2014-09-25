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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include <QRegExp>
#include <QStringList>

#include "qbfrac.h"

QbFrac::QbFrac(QObject *parent): QObject(parent)
{
    this->resetNum();
    this->resetDen();
    this->m_isValid = false;
}

QbFrac::QbFrac(qint64 num, qint64 den):
    QObject(NULL),
    m_num(num),
    m_den(den)
{
    this->m_isValid = den? true: false;
    this->reduce();
}

QbFrac::QbFrac(const QString &fracString):
    QObject(NULL)
{
    this->m_num = 0;
    this->m_den = 0;

    this->m_isValid = QRegExp("\\s*\\d+\\s*/"
                              "\\s*\\d+\\s*").exactMatch(fracString);

    if (!this->m_isValid)
        return;

    QStringList fracChunks = fracString.split(QRegExp("\\s*/\\s*"),
                                              QString::SkipEmptyParts);

    this->m_num = fracChunks[0].trimmed().toInt();
    this->m_den = fracChunks[1].trimmed().toInt();

    if (!this->m_den)
        this->m_isValid = false;
}

QbFrac::QbFrac(const QbFrac &other):
    QObject(other.parent()),
    m_num(other.m_num),
    m_den(other.m_den),
    m_isValid(other.m_isValid)
{
}

QbFrac::~QbFrac()
{
}

QbFrac &QbFrac::operator =(const QbFrac &other)
{
    if (this != &other) {
        this->m_num = other.m_num;
        this->m_den = other.m_den;
        this->m_isValid = other.m_isValid;
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
    return QbFrac(this->num() * other.num(),
                  this->den() * other.den());
}

qint64 QbFrac::num() const
{
    return this->m_num;
}

qint64 QbFrac::den() const
{
    return this->m_den;
}

double QbFrac::value() const
{
    return this->m_num / (double) this->m_den;
}

int QbFrac::fastValue() const
{
    return this->m_num / this->m_den;
}

bool QbFrac::isValid() const
{
    return this->m_isValid;
}

QString QbFrac::toString() const
{
    return QString("%1/%2").arg(this->m_num)
            .arg(this->m_den);
}

QbFrac QbFrac::invert() const
{
    return QbFrac(this->den(),
                  this->num());
}

qint64 QbFrac::gcd() const
{
    qint64 tmp;
    qint64 num = abs(this->m_num);
    qint64 den = abs(this->m_den);

    while (num > 0) {
        tmp = num;
        num = den % num;
        den = tmp;
    }

    return den;
}

void QbFrac::setNum(qint64 num)
{
    this->m_num = num;
}

void QbFrac::reduce()
{
    qint64 gcd = this->gcd();

    if (!gcd)
        return;

    this->m_num /= gcd;
    this->m_den /= gcd;
}

void QbFrac::setDen(qint64 den)
{
    this->m_den = den;

    if (!this->m_den)
        this->m_isValid = false;
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

QbFrac operator *(int number, const QbFrac &frac)
{
    return QbFrac(number * frac.num(),
                  frac.den());
}

QbFrac operator /(int number, const QbFrac &frac)
{
    return number * frac.invert();
}


QbFrac operator /(const QbFrac &fracNum, const QbFrac &fracDen)
{
    return QbFrac(fracNum.num() * fracDen.den(),
                  fracNum.den() * fracDen.num());
}

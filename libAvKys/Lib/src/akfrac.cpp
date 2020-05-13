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

#include <QDataStream>
#include <QDebug>
#include <QRegExp>
#include <QStringList>
#include <QQmlEngine>

#include "akfrac.h"

class AkFracPrivate
{
    public:
        qint64 m_num;
        qint64 m_den;

        template<typename T>
        inline static T sign(T n);
        inline static qint64 gcd(qint64 num, qint64 den);
        inline static void reduce(qint64 *num, qint64 *den);
};

AkFrac::AkFrac(QObject *parent):
    QObject(parent)
{
    this->d = new AkFracPrivate();
    this->d->m_num = 0;
    this->d->m_den = 0;
}

AkFrac::AkFrac(qint64 num, qint64 den):
    QObject(nullptr)
{
    this->d = new AkFracPrivate();
    this->d->m_num = 0;
    this->d->m_den = 0;

    this->setNumDen(num, den);
}

AkFrac::AkFrac(const QString &fracString):
    QObject(nullptr)
{
    this->d = new AkFracPrivate();
    this->d->m_num = 0;
    this->d->m_den = 0;

    this->setNumDen(fracString);
}

AkFrac::AkFrac(const AkFrac &other):
    QObject()
{
    this->d = new AkFracPrivate();
    this->d->m_num = other.d->m_num;
    this->d->m_den = other.d->m_den;
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
    }

    return *this;
}

bool AkFrac::operator ==(const AkFrac &other) const
{
    if (this->d->m_den == 0 && other.d->m_den != 0)
        return false;

    if (this->d->m_den != 0 && other.d->m_den == 0)
        return false;

    return this->d->m_num * other.d->m_den == this->d->m_den * other.d->m_num;
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

QObject *AkFrac::create()
{
    return new AkFrac();
}

QObject *AkFrac::create(qint64 num, qint64 den)
{
    return new AkFrac(num, den);
}

QObject *AkFrac::create(const QString &frac)
{
    return new AkFrac(frac);
}

QObject *AkFrac::create(const AkFrac &frac)
{
    return new AkFrac(frac);
}

QVariant AkFrac::createVariant(qint64 num, qint64 den)
{
    return QVariant::fromValue(AkFrac(num, den));
}

QVariant AkFrac::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkFrac::operator bool() const
{
    return this->d->m_den != 0;
}

AkFrac::operator QString() const
{
    return QString("%1/%2").arg(this->d->m_num).arg(this->d->m_den);
}

qint64 AkFrac::num() const
{
    return this->d->m_num;
}

qint64 AkFrac::den() const
{
    return this->d->m_den;
}

qreal AkFrac::value() const
{
    if (!this->d->m_den)
        return qQNaN();

    return qreal(this->d->m_num) / this->d->m_den;
}

qint64 AkFrac::fastValue() const
{
    if (!this->d->m_den)
        return 0;

    return this->d->m_num / this->d->m_den;
}

bool AkFrac::isValid() const
{
    return *this;
}

QString AkFrac::toString() const
{
    return *this;
}

AkFrac AkFrac::invert() const
{
    return AkFrac(this->d->m_den, this->d->m_num);
}

void AkFrac::setNumDen(qint64 num, qint64 den)
{
    bool changed = false;

    if (!den) {
        if (this->d->m_num != 0) {
            this->d->m_num = 0;
            changed = true;
            emit this->numChanged(0);
        }

        if (this->d->m_den != 0) {
            this->d->m_den = 0;
            changed = true;
            emit this->denChanged(0);
            emit this->isValidChanged(false);
        }

        if (changed) {
            emit this->valueChanged(qQNaN());
            emit this->stringChanged("0/0");
        }

        return;
    }

    num = AkFracPrivate::sign(den) * num;
    den = qAbs(den);
    AkFracPrivate::reduce(&num, &den);

    if (this->d->m_num != num) {
        this->d->m_num = num;
        changed = true;
        emit this->numChanged(num);
    }

    if (this->d->m_den != den) {
        if (!this->d->m_den)
            emit this->isValidChanged(true);

        this->d->m_den = den;
        changed = true;
        emit this->denChanged(den);
    }

    if (changed) {
        emit this->valueChanged(this->value());
        emit this->stringChanged(*this);
    }
}

void AkFrac::setNumDen(const QString &fracString)
{
    bool ok = false;
    auto str = fracString.trimmed();
    auto index = str.indexOf('/');

    if (index < 1) {
        qint64 num = str.toLongLong(&ok);

        if (ok)
            this->setNumDen(num, 1);
        else
            this->setNumDen(0, 0);
    } else {
        qint64 num = str.left(index).trimmed().toLongLong(&ok);

        if (ok) {
            auto n = str.size() - index - 1;

            if (n > 0) {
                qint64 den = str.right(n).trimmed().toLongLong(&ok);

                if (ok && den > 0)
                    this->setNumDen(num, den);
                else
                    this->setNumDen(0, 0);
            } else {
                this->setNumDen(0, 0);
            }
        } else {
            this->setNumDen(0, 0);
        }
    }
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

void AkFrac::registerTypes()
{
    qRegisterMetaType<AkFrac>("AkFrac");
    qRegisterMetaTypeStreamOperators<AkFrac>("AkFrac");
    QMetaType::registerDebugStreamOperator<AkFrac>();
    qmlRegisterSingletonType<AkFrac>("Ak", 1, 0, "AkFrac",
                                     [] (QQmlEngine *qmlEngine,
                                         QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkFrac();
    });
}

QDebug operator <<(QDebug debug, const AkFrac &frac)
{
    debug.nospace() << "AkFrac("
                    << frac.num()
                    << ","
                    << frac.den()
                    << ")";

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkFrac &frac)
{
    qint64 num;
    qint64 den;
    istream >> num;
    istream >> den;
    frac.setNum(num);
    frac.setDen(den);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkFrac &frac)
{
    ostream << frac.num();
    ostream << frac.den();

    return ostream;
}

AkFrac operator *(int number, const AkFrac &frac)
{
    return {number * frac.num(), frac.den()};
}

AkFrac operator /(int number, const AkFrac &frac)
{
    return number * frac.invert();
}

AkFrac operator /(const AkFrac &fracNum, const AkFrac &fracDen)
{
    return {fracNum.num() * fracDen.den(),
            fracNum.den() * fracDen.num()};
}

AkFrac operator +(const AkFrac &frac1, const AkFrac &frac2)
{
    return {frac1.num() * frac2.den() + frac2.num() * frac1.den(),
            frac1.den() * frac2.den()};
}

AkFrac operator -(const AkFrac &frac1, const AkFrac &frac2)
{
    return {frac1.num() * frac2.den() - frac2.num() * frac1.den(),
            frac1.den() * frac2.den()};
}

template<typename T>
T AkFracPrivate::sign(T n)
{
    return (n < 0)? -1: 1;
}

qint64 AkFracPrivate::gcd(qint64 num, qint64 den)
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

void AkFracPrivate::reduce(qint64 *num, qint64 *den)
{
    qint64 gcd = AkFracPrivate::gcd(*num, *den);

    if (gcd) {
        *num /= gcd;
        *den /= gcd;
    }
}

#include "moc_akfrac.cpp"

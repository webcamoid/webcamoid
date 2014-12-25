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

#include <QQmlContext>

#include "qb.h"

Q_GLOBAL_STATIC(Qb, globalQbObject)
Q_GLOBAL_STATIC_WITH_ARGS(qint64, gid, (0))

Qb::Qb(QObject *parent): QObject(parent)
{
}

Qb::Qb(const Qb &other):
    QObject(other.parent())
{
}

void Qb::init()
{
    qRegisterMetaType<Qb>("Qb");
    qRegisterMetaType<QbCaps>("QbCaps");
    qRegisterMetaTypeStreamOperators<QbCaps>("QbCaps");
    qRegisterMetaType<QbElement::ElementState>("QbElement::ElementState");
    qRegisterMetaType<QbElement::ElementState>("ElementState");
    qRegisterMetaTypeStreamOperators<QbElement::ElementState>("QbElement::ElementState");
    qRegisterMetaType<QbFrac>("QbFrac");
    qRegisterMetaTypeStreamOperators<QbFrac>("QbFrac");
    qRegisterMetaType<QbPacket>("QbPacket");
    qRegisterMetaType<QbElementPtr>("QbElementPtr");
}

qint64 Qb::id()
{
    return (*gid)++;
}


bool Qb::qmlRegister(QQmlApplicationEngine *engine)
{
    if (!engine)
        return false;

    engine->rootContext()->setContextProperty("Qb", globalQbObject);

    return true;
}

QObject *Qb::newFrac() const
{
    return new QbFrac();
}

QObject *Qb::newFrac(qint64 num, qint64 den) const
{
    return new QbFrac(num, den);
}

QObject *Qb::newFrac(const QString &fracString) const
{
    return new QbFrac(fracString);
}

QObject *Qb::copy(const QbFrac &frac) const
{
    return new QbFrac(frac);
}

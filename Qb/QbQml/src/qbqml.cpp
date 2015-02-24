/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#include "qbqml.h"

QbQml::QbQml(QQuickItem *parent):
    QQuickItem(parent)
{
    // By default, QQuickItem does not draw anything. If you subclass
    // QQuickItem to create a visual item, you will need to uncomment the
    // following line and re-implement updatePaintNode()

    // setFlag(ItemHasContents, true);
}

QbQml::~QbQml()
{
}

void QbQml::init() const
{
    Qb::init();
}

qint64 QbQml::id() const
{
    return Qb::id();
}

QObject *QbQml::newFrac() const
{
    return new QbFrac();
}

QObject *QbQml::newFrac(qint64 num, qint64 den) const
{
    return new QbFrac(num, den);
}

QObject *QbQml::newFrac(const QString &frac) const
{
    return new QbFrac(frac);
}

QObject *QbQml::newFrac(const QbFrac &frac) const
{
    return new QbFrac(frac);
}

QObject *QbQml::newCaps() const
{
    return new QbCaps();
}

QObject *QbQml::newCaps(const QVariantMap &caps) const
{
    return new QbCaps(caps);
}

QObject *QbQml::newCaps(const QString &caps) const
{
    return new QbCaps(caps);
}

QObject *QbQml::newCaps(const QbCaps &caps) const
{
    return new QbCaps(caps);
}

QVariant QbQml::varFrac(QbFrac *frac) const
{
    return QVariant::fromValue(*frac);
}

QVariant QbQml::varCaps(QbCaps *caps) const
{
    return QVariant::fromValue(*caps);
}

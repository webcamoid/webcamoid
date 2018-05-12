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

#include "akqml.h"
#include "ak.h"
#include "akfrac.h"
#include "akcaps.h"
#include "akvideocaps.h"
#include "akelement.h"

AkQml::AkQml(QQuickItem *parent):
    QQuickItem(parent)
{
    // By default, QQuickItem does not draw anything. If you subclass
    // QQuickItem to create a visual item, you will need to uncomment the
    // following line and re-implement updatePaintNode()

    // setFlag(ItemHasContents, true);
}

AkQml::~AkQml()
{
}

qint64 AkQml::id() const
{
    return Ak::id();
}

QObject *AkQml::newFrac() const
{
    return new AkFrac();
}

QObject *AkQml::newFrac(qint64 num, qint64 den) const
{
    return new AkFrac(num, den);
}

QObject *AkQml::newFrac(const QString &frac) const
{
    return new AkFrac(frac);
}

QObject *AkQml::newFrac(const AkFrac &frac) const
{
    return new AkFrac(frac);
}

QObject *AkQml::newCaps() const
{
    return new AkCaps();
}

QObject *AkQml::newCaps(const QVariantMap &caps) const
{
    return new AkCaps(caps);
}

QObject *AkQml::newCaps(const QString &caps) const
{
    return new AkCaps(caps);
}

QObject *AkQml::newCaps(const AkCaps &caps) const
{
    return new AkCaps(caps);
}

QObject *AkQml::newAudioCaps() const
{
    return new AkAudioCaps();
}

QObject *AkQml::newAudioCaps(const QVariantMap &caps) const
{
    return new AkAudioCaps(caps);
}

QObject *AkQml::newAudioCaps(const QString &caps) const
{
    return new AkAudioCaps(caps);
}

QObject *AkQml::newAudioCaps(const AkCaps &caps) const
{
    return new AkAudioCaps(caps);
}

QObject *AkQml::newAudioCaps(const AkAudioCaps &caps) const
{
    return new AkAudioCaps(caps);
}

QObject *AkQml::newAudioCaps(AkAudioCaps::SampleFormat format,
                             int channels,
                             int rate)
{
    return new AkAudioCaps(format, channels, rate);
}

QObject *AkQml::newVideoCaps() const
{
    return new AkVideoCaps();
}

QObject *AkQml::newVideoCaps(const QVariantMap &caps) const
{
    return new AkVideoCaps(caps);
}

QObject *AkQml::newVideoCaps(const QString &caps) const
{
    return new AkVideoCaps(caps);
}

QObject *AkQml::newVideoCaps(const AkCaps &caps) const
{
    return new AkVideoCaps(caps);
}

QObject *AkQml::newVideoCaps(const AkVideoCaps &caps) const
{
    return new AkVideoCaps(caps);
}

QObject *AkQml::newElement(const QString &pluginId,
                           const QString &elementName) const
{
    return AkElement::createPtr(pluginId, elementName);
}

QVariant AkQml::varFrac(QObject *frac) const
{
    return QVariant::fromValue(*qobject_cast<AkFrac *>(frac));
}

QVariant AkQml::varFrac(AkFrac *frac) const
{
    return QVariant::fromValue(*frac);
}

QVariant AkQml::varFrac(qint64 num, qint64 den) const
{
    return QVariant::fromValue(AkFrac(num, den));
}

QVariant AkQml::varCaps(QObject *caps) const
{
    return QVariant::fromValue(*qobject_cast<AkCaps *>(caps));
}

QVariant AkQml::varCaps(AkCaps *caps) const
{
    return QVariant::fromValue(*caps);
}

#include "moc_akqml.cpp"

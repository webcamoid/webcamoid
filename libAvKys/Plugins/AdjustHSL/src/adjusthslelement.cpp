/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#include <QColor>
#include <QQmlContext>
#include <QVariant>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "adjusthslelement.h"

class AdjustHSLElementPrivate
{
    public:
        int m_hue {0};
        int m_saturation {0};
        int m_luminance {0};
        AkVideoConverter m_videoConverter {AkVideoCaps(AkVideoCaps::Format_argbpack, 0, 0, {})};

        template<typename T>
        inline T mod(T value, T mod)
        {
            return (value % mod + mod) % mod;
        }
};

AdjustHSLElement::AdjustHSLElement(): AkElement()
{
    this->d = new AdjustHSLElementPrivate;
}

AdjustHSLElement::~AdjustHSLElement()
{
    delete this->d;
}

int AdjustHSLElement::hue() const
{
    return this->d->m_hue;
}

int AdjustHSLElement::saturation() const
{
    return this->d->m_saturation;
}

int AdjustHSLElement::luminance() const
{
    return this->d->m_luminance;
}

QString AdjustHSLElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/AdjustHSL/share/qml/main.qml");
}

void AdjustHSLElement::controlInterfaceConfigure(QQmlContext *context,
                                                 const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("AdjustHSL", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket AdjustHSLElement::iVideoStream(const AkVideoPacket &packet)
{
    if (this->d->m_hue == 0
        && this->d->m_saturation == 0
        && this->d->m_luminance == 0) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

   auto hue = this->d->m_hue;
   auto saturation = this->d->m_saturation;
   auto luminance = this->d->m_luminance;

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    for (int y = 0; y < src.caps().height(); ++y) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); ++x) {
            int h;
            int s;
            int l;
            int a;
            QColor(srcLine[x]).getHsl(&h, &s, &l, &a);
            int ht = this->d->mod(h + hue, 360);
            int st = qBound(0, s + saturation, 255);
            int lt = qBound(0, l + luminance, 255);
            QColor color;
            color.setHsl(ht, st, lt, a);
            dstLine[x] = color.rgba();
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void AdjustHSLElement::setHue(int hue)
{
    if (this->d->m_hue == hue)
        return;

    this->d->m_hue = hue;
    emit this->hueChanged(hue);
}

void AdjustHSLElement::setSaturation(int saturation)
{
    if (this->d->m_saturation == saturation)
        return;

    this->d->m_saturation = saturation;
    emit this->saturationChanged(saturation);
}

void AdjustHSLElement::setLuminance(int luminance)
{
    if (this->d->m_luminance == luminance)
        return;

    this->d->m_luminance = luminance;
    emit this->luminanceChanged(luminance);
}

void AdjustHSLElement::resetHue()
{
    this->setHue(0);
}

void AdjustHSLElement::resetSaturation()
{
    this->setSaturation(0);
}

void AdjustHSLElement::resetLuminance()
{
    this->setLuminance(0);
}

#include "moc_adjusthslelement.cpp"

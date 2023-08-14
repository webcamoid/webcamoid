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

#include <QQmlContext>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "blurelement.h"
#include "pixel.h"

class BlurElementPrivate
{
    public:
        int m_radius {5};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        void integralImage(const AkVideoPacket &src, PixelU32 *integral);
};

BlurElement::BlurElement():
    AkElement()
{
    this->d = new BlurElementPrivate;
}

BlurElement::~BlurElement()
{
    delete this->d;
}

int BlurElement::radius() const
{
    return this->d->m_radius;
}

QString BlurElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Blur/share/qml/main.qml");
}

void BlurElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Blur", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket BlurElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    int oWidth = src.caps().width() + 1;
    int oHeight = src.caps().height() + 1;
    auto integral = new PixelU32[oWidth * oHeight];
    this->d->integralImage(src, integral);

    int radius = this->d->m_radius;

    for (int y = 0; y < src.caps().height(); ++y) {
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));
        int yp = qMax(y - radius, 0);
        int kh = qMin(y + radius, src.caps().height() - 1) - yp + 1;

        for (int x = 0; x < src.caps().width(); ++x) {
            int xp = qMax(x - radius, 0);
            int kw = qMin(x + radius, src.caps().width() - 1) - xp + 1;

            PixelU32 sum = integralSum(integral, oWidth, xp, yp, kw, kh);
            PixelU32 mean = sum / quint32(kw * kh);

            oLine[x] = qRgba(int(mean.r), int(mean.g), int(mean.b), int(mean.a));
        }
    }

    delete [] integral;

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void BlurElement::setRadius(int radius)
{
    if (this->d->m_radius == radius)
        return;

    this->d->m_radius = radius;
    emit this->radiusChanged(radius);
}

void BlurElement::resetRadius()
{
    this->setRadius(5);
}

void BlurElementPrivate::integralImage(const AkVideoPacket &src,
                                       PixelU32 *integral)
{
    int oWidth = src.caps().width() + 1;
    int oHeight = src.caps().height() + 1;

    auto integralLine = integral + oWidth;
    auto prevIntegralLine = integral;

    for (int y = 1; y < oHeight; ++y) {
        auto line = reinterpret_cast<const QRgb *>(src.constLine(0, y - 1));

        // Reset current line summation.
        PixelU32 sum;

        for (int x = 1; x < oWidth; ++x) {
            // Accumulate pixels in current line.
            sum += line[x - 1];

            // Accumulate current line and previous line.
            integralLine[x] = sum + prevIntegralLine[x];
        }

        integralLine += oWidth;
        prevIntegralLine += oWidth;
    }
}

#include "moc_blurelement.cpp"

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

#include <QImage>
#include <QQmlContext>
#include <akvideopacket.h>

#include "blurelement.h"
#include "pixel.h"

class BlurElementPrivate
{
    public:
        int m_radius {5};

        void integralImage(const QImage &image,
                           int oWidth, int oHeight,
                           PixelU32 *integral);
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

void BlurElementPrivate::integralImage(const QImage &image,
                                       int oWidth, int oHeight,
                                       PixelU32 *integral)
{
    for (int y = 1; y < oHeight; y++) {
        auto line = reinterpret_cast<const QRgb *>(image.constScanLine(y - 1));

        // Reset current line summation.
        PixelU32 sum;

        for (int x = 1; x < oWidth; x++) {
            QRgb pixel = line[x - 1];

            // Accumulate pixels in current line.
            sum += pixel;

            // Offset to the current line.
            int offset = x + y * oWidth;

            // Offset to the previous line.
            // equivalent to x + (y - 1) * oWidth;
            int offsetPrevious = offset - oWidth;

            // Accumulate current line and previous line.
            integral[offset] = sum + integral[offsetPrevious];
        }
    }
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

AkPacket BlurElement::iStream(const AkPacket &packet)
{
    AkVideoPacket videoPacket(packet);
    auto src = videoPacket.toImage();

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    int oWidth = src.width() + 1;
    int oHeight = src.height() + 1;
    auto integral = new PixelU32[oWidth * oHeight];
    this->d->integralImage(src, oWidth, oHeight, integral);

    int radius = this->d->m_radius;

    for (int y = 0; y < src.height(); y++) {
        QRgb *oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));
        int yp = qMax(y - radius, 0);
        int kh = qMin(y + radius, src.height() - 1) - yp + 1;

        for (int x = 0; x < src.width(); x++) {
            int xp = qMax(x - radius, 0);
            int kw = qMin(x + radius, src.width() - 1) - xp + 1;

            PixelU32 sum = integralSum(integral, oWidth, xp, yp, kw, kh);
            PixelU32 mean = sum / quint32(kw * kh);

            oLine[x] = qRgba(int(mean.r), int(mean.g), int(mean.b), int(mean.a));
        }
    }

    delete [] integral;

    auto oPacket = AkVideoPacket::fromImage(oFrame, videoPacket).toPacket();
    akSend(oPacket)
}

#include "moc_blurelement.cpp"

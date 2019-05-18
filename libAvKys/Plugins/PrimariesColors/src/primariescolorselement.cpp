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
#include <akpacket.h>
#include <akvideopacket.h>

#include "primariescolorselement.h"

class PrimariesColorsElementPrivate
{
    public:
        int m_factor {2};
};

PrimariesColorsElement::PrimariesColorsElement(): AkElement()
{
    this->d = new PrimariesColorsElementPrivate;
}

PrimariesColorsElement::~PrimariesColorsElement()
{
    delete this->d;
}

int PrimariesColorsElement::factor() const
{
    return this->d->m_factor;
}

QString PrimariesColorsElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/PrimariesColors/share/qml/main.qml");
}

void PrimariesColorsElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("PrimariesColors", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket PrimariesColorsElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = packet.toImage();

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    int f = this->d->m_factor + 1;
    int factor127 = (f * f - 3) * 127;
    int factorTot = f * f;

    if (factor127 < 0) {
        factor127 = 0;
        factorTot = 3;
    }

    for (int y = 0; y < src.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto destLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            QRgb pixel = srcLine[x];
            int ri = qRed(pixel);
            int gi = qGreen(pixel);
            int bi = qBlue(pixel);
            int mean;

            if (f > 32)
                mean = 127;
            else
                mean = (ri + gi + bi + factor127) / factorTot;

            int r = ri > mean? 255: 0;
            int g = gi > mean? 255: 0;
            int b = bi > mean? 255: 0;

            destLine[x] = qRgba(r, g, b, qAlpha(pixel));
        }
    }

    auto oPacket = AkVideoPacket::fromImage(oFrame, packet);
    akSend(oPacket)
}

void PrimariesColorsElement::setFactor(int factor)
{
    if (this->d->m_factor == factor)
        return;

    this->d->m_factor = factor;
    emit this->factorChanged(factor);
}

void PrimariesColorsElement::resetFactor()
{
    this->setFactor(2);
}

#include "moc_primariescolorselement.cpp"

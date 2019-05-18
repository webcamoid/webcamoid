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
#include <QtMath>
#include <QQmlContext>
#include <akpacket.h>
#include <akvideopacket.h>

#include "implodeelement.h"

class ImplodeElementPrivate
{
    public:
        qreal m_amount {1.0};
};

ImplodeElement::ImplodeElement(): AkElement()
{
    this->d = new ImplodeElementPrivate;
}

ImplodeElement::~ImplodeElement()
{
    delete this->d;
}

qreal ImplodeElement::amount() const
{
    return this->d->m_amount;
}

QString ImplodeElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Implode/share/qml/main.qml");
}

void ImplodeElement::controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Implode", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket ImplodeElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = packet.toImage();

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    int xc = src.width() >> 1;
    int yc = src.height() >> 1;
    int radius = qMin(xc, yc);

    for (int y = 0; y < src.height(); y++) {
        auto iLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));
        int yDiff = y - yc;

        for (int x = 0; x < src.width(); x++) {
            int xDiff = x - xc;
            qreal distance = sqrt(xDiff * xDiff + yDiff * yDiff);

            if (distance >= radius)
                oLine[x] = iLine[x];
            else {
                qreal factor = pow(distance / radius, this->d->m_amount);

                int xp = int(factor * xDiff + xc);
                int yp = int(factor * yDiff + yc);

                xp = qBound(0, xp, oFrame.width() - 1);
                yp = qBound(0, yp, oFrame.height() - 1);

                auto line = reinterpret_cast<const QRgb *>(src.constScanLine(yp));
                oLine[x] = line[xp];
            }
        }
    }

    auto oPacket = AkVideoPacket::fromImage(oFrame, packet);
    akSend(oPacket)
}

void ImplodeElement::setAmount(qreal amount)
{
    if (qFuzzyCompare(this->d->m_amount, amount))
        return;

    this->d->m_amount = amount;
    emit this->amountChanged(amount);
}

void ImplodeElement::resetAmount()
{
    this->setAmount(1.0);
}

#include "moc_implodeelement.cpp"

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
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "swirlelement.h"

class SwirlElementPrivate
{
    public:
        qreal m_degrees {60.0};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argb, 0, 0, {}}};
};

SwirlElement::SwirlElement(): AkElement()
{
    this->d = new SwirlElementPrivate;
}

SwirlElement::~SwirlElement()
{
    delete this->d;
}

qreal SwirlElement::degrees() const
{
    return this->d->m_degrees;
}

QString SwirlElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Swirl/share/qml/main.qml");
}

void SwirlElement::controlInterfaceConfigure(QQmlContext *context,
                                             const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Swirl", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket SwirlElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = this->d->m_videoConverter.convertToImage(packet);

    if (src.isNull())
        return AkPacket();

    QImage oFrame(src.size(), src.format());

    qreal xScale = 1.0;
    qreal yScale = 1.0;
    qreal xCenter = src.width() >> 1;
    qreal yCenter = src.height() >> 1;
    qreal radius = qMax(xCenter, yCenter);

    if (src.width() > src.height())
        yScale = qreal(src.width()) / src.height();
    else if (src.width() < src.height())
        xScale = qreal(src.height()) / src.width();

    auto degrees = qDegreesToRadians(this->d->m_degrees);

    for (int y = 0; y < src.height(); y++) {
        auto iLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));
        qreal yDistance = yScale * (y - yCenter);

        for (int x = 0; x < src.width(); x++) {
            qreal xDistance = xScale * (x - xCenter);
            qreal distance = xDistance * xDistance + yDistance * yDistance;

            if (distance >= radius * radius)
                oLine[x] = iLine[x];
            else {
                qreal factor = 1.0 - sqrt(distance) / radius;
                qreal sine = sin(degrees * factor * factor);
                qreal cosine = cos(degrees * factor * factor);

                int xp = int((cosine * xDistance - sine * yDistance) / xScale + xCenter);
                int yp = int((sine * xDistance + cosine * yDistance) / yScale + yCenter);

                if (!oFrame.rect().contains(xp, yp))
                    continue;

                auto line = reinterpret_cast<const QRgb *>(src.constScanLine(yp));
                oLine[x] = line[xp];
            }
        }
    }

    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

void SwirlElement::setDegrees(qreal degrees)
{
    if (qFuzzyCompare(this->d->m_degrees, degrees))
        return;

    this->d->m_degrees = degrees;
    emit this->degreesChanged(degrees);
}

void SwirlElement::resetDegrees()
{
    this->setDegrees(60);
}

#include "moc_swirlelement.cpp"

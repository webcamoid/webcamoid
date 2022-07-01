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
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "cinemaelement.h"

class CinemaElementPrivate
{
    public:
        qreal m_stripSize {0.5};
        QRgb m_stripColor {qRgb(0, 0, 0)};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argb, 0, 0, {}}};
};

CinemaElement::CinemaElement(): AkElement()
{
    this->d = new CinemaElementPrivate;
}

CinemaElement::~CinemaElement()
{
    delete this->d;
}

qreal CinemaElement::stripSize() const
{
    return this->d->m_stripSize;
}

QRgb CinemaElement::stripColor() const
{
    return this->d->m_stripColor;
}

QString CinemaElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Cinema/share/qml/main.qml");
}

void CinemaElement::controlInterfaceConfigure(QQmlContext *context,
                                              const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Cinema", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket CinemaElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = this->d->m_videoConverter.convertToImage(packet);

    if (src.isNull())
        return AkPacket();

    QImage oFrame(src.size(), src.format());
    int cy = src.height() >> 1;

    for (int y = 0; y < src.height(); y++) {
        qreal k = 1.0 - qAbs(y - cy) / qreal(cy);
        auto iLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        if (k > this->d->m_stripSize)
            memcpy(oLine, iLine, size_t(src.bytesPerLine()));
        else
            for (int x = 0; x < src.width(); x++) {
                qreal a = qAlpha(this->d->m_stripColor) / 255.0;

                int r = int(a * (qRed(this->d->m_stripColor) - qRed(iLine[x])) + qRed(iLine[x]));
                int g = int(a * (qGreen(this->d->m_stripColor) - qGreen(iLine[x])) + qGreen(iLine[x]));
                int b = int(a * (qBlue(this->d->m_stripColor) - qBlue(iLine[x])) + qBlue(iLine[x]));

                oLine[x] = qRgba(r, g, b, qAlpha(iLine[x]));
            }
    }

    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

void CinemaElement::setStripSize(qreal stripSize)
{
    if (qFuzzyCompare(this->d->m_stripSize, stripSize))
        return;

    this->d->m_stripSize = stripSize;
    emit this->stripSizeChanged(stripSize);
}

void CinemaElement::setStripColor(QRgb hideColor)
{
    if (this->d->m_stripColor == hideColor)
        return;

    this->d->m_stripColor = hideColor;
    emit this->stripColorChanged(hideColor);
}

void CinemaElement::resetStripSize()
{
    this->setStripSize(0.5);
}

void CinemaElement::resetStripColor()
{
    this->setStripColor(qRgb(0, 0, 0));
}

#include "moc_cinemaelement.cpp"

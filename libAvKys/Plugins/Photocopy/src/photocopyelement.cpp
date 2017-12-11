/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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
#include <akutils.h>
#include <akpacket.h>

#include "photocopyelement.h"

PhotocopyElement::PhotocopyElement(): AkElement()
{
    this->m_brightness = 0.75;
    this->m_contrast = 20;
}

qreal PhotocopyElement::brightness() const
{
    return this->m_brightness;
}

qreal PhotocopyElement::contrast() const
{
    return this->m_contrast;
}

QString PhotocopyElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Photocopy/share/qml/main.qml");
}

void PhotocopyElement::controlInterfaceConfigure(QQmlContext *context,
                                                 const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Photocopy", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void PhotocopyElement::setBrightness(qreal brightness)
{
    if (qFuzzyCompare(this->m_brightness, brightness))
        return;

    this->m_brightness = brightness;
    emit this->brightnessChanged(brightness);
}

void PhotocopyElement::setContrast(qreal contrast)
{
    if (qFuzzyCompare(this->m_contrast, contrast))
        return;

    this->m_contrast = contrast;
    emit this->contrastChanged(contrast);
}

void PhotocopyElement::resetBrightness()
{
    this->setBrightness(0.75);
}

void PhotocopyElement::resetContrast()
{
    this->setContrast(20);
}

AkPacket PhotocopyElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    for (int y = 0; y < src.height(); y++) {
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *dstLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int r = qRed(srcLine[x]);
            int g = qGreen(srcLine[x]);
            int b = qBlue(srcLine[x]);

            //desaturate
            int luma = this->rgbToLuma(r, g, b);

            //compute sigmoidal transfer
            qreal val = luma / 255.0;
            val = 255.0 / (1 + exp(this->m_contrast * (0.5 - val)));
            val = val * this->m_brightness;
            luma = int(qBound(0.0, val, 255.0));

            dstLine[x] = qRgba(luma, luma, luma, qAlpha(srcLine[x]));
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

#include "moc_photocopyelement.cpp"

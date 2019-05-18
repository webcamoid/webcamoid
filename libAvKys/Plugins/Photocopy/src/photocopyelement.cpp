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
#include <akpacket.h>
#include <akvideopacket.h>

#include "photocopyelement.h"

class PhotocopyElementPrivate
{
    public:
        qreal m_brightness {0.75};
        qreal m_contrast {20.0};

        inline static int rgbToLuma(int red, int green, int blue);
};

PhotocopyElement::PhotocopyElement(): AkElement()
{
    this->d = new PhotocopyElementPrivate;
}

PhotocopyElement::~PhotocopyElement()
{
    delete this->d;
}

qreal PhotocopyElement::brightness() const
{
    return this->d->m_brightness;
}

qreal PhotocopyElement::contrast() const
{
    return this->d->m_contrast;
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

AkPacket PhotocopyElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = packet.toImage();

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
            int luma = PhotocopyElementPrivate::rgbToLuma(r, g, b);

            //compute sigmoidal transfer
            qreal val = luma / 255.0;
            val = 255.0 / (1 + exp(this->d->m_contrast * (0.5 - val)));
            val = val * this->d->m_brightness;
            luma = int(qBound(0.0, val, 255.0));

            dstLine[x] = qRgba(luma, luma, luma, qAlpha(srcLine[x]));
        }
    }

    auto oPacket = AkVideoPacket::fromImage(oFrame, packet);
    akSend(oPacket)
}

void PhotocopyElement::setBrightness(qreal brightness)
{
    if (qFuzzyCompare(this->d->m_brightness, brightness))
        return;

    this->d->m_brightness = brightness;
    emit this->brightnessChanged(brightness);
}

void PhotocopyElement::setContrast(qreal contrast)
{
    if (qFuzzyCompare(this->d->m_contrast, contrast))
        return;

    this->d->m_contrast = contrast;
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

int PhotocopyElementPrivate::rgbToLuma(int red, int green, int blue)
{
    int min;
    int max;

    if (red > green) {
        max = qMax(red, blue);
        min = qMin(green, blue);
    } else {
        max = qMax(green, blue);
        min = qMin(red, blue);
    }

    return qRound((max + min) / 2.0);
}

#include "moc_photocopyelement.cpp"

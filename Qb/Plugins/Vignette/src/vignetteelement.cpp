/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "vignetteelement.h"

VignetteElement::VignetteElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    this->resetAspect();
    this->resetClearCenter();
    this->resetSoftness();
}

QObject *VignetteElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Vignette/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Vignette", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

qreal VignetteElement::aspect() const
{
    return this->m_aspect;
}

qreal VignetteElement::clearCenter() const
{
    return this->m_clearCenter;
}

qreal VignetteElement::softness() const
{
    return this->m_softness;
}

QVector<qreal> VignetteElement::updateVignette(int width, int height)
{
    qreal soft = 5 * pow(1.0 - this->m_softness, 2) + 0.01;
    qreal scaleX = 1;
    qreal scaleY = 1;

    // Distance from 0.5 (\in [0,0.5]) scaled to [0,1]
    qreal scale = fabs(this->m_aspect - 0.5) * 2;

    // Map scale to [0,5] in a way that values near 0 can be adjusted more precisely
    scale = 1 + 4 * pow(scale, 3);

    // Scale either x or y, depending on the aspect value being above or below 0.5
    if (this->m_aspect > 0.5)
        scaleX = scale;
    else
        scaleY = scale;

    int cx = width >> 1;
    int cy = height >> 1;
    qreal rmax = sqrt(cx * cx + cy * cy);

    QVector<qreal> vignette(width * height);

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {
            // Euclidian distance to the center, normalized to [0,1]
            qreal tx = scaleX * (x - cx);
            qreal ty = scaleY * (y - cy);
            qreal r = sqrt(tx * tx + ty * ty) / rmax;

            // Subtract the clear center
            r -= this->m_clearCenter;

            if (r <= 0)
                // Clear center: Do not modify the brightness here
                vignette[y * width + x] = 1;
            else {
                r *= soft;

                if (r > M_PI_2)
                    vignette[y * width + x] = 0;
                else
                    vignette[y * width + x] = pow(cos(r), 4);
            }
        }

    return vignette;
}

void VignetteElement::setAspect(qreal aspect)
{
    if (aspect != this->m_aspect) {
        this->m_aspect = aspect;
        emit this->aspectChanged();
    }
}

void VignetteElement::setClearCenter(qreal clearCenter)
{
    if (clearCenter != this->m_clearCenter) {
        this->m_clearCenter = clearCenter;
        emit this->clearCenterChanged();
    }
}

void VignetteElement::setSoftness(qreal softness)
{
    if (softness != this->m_softness) {
        this->m_softness = softness;
        emit this->softnessChanged();
    }
}

void VignetteElement::resetAspect()
{
    this->setAspect(0.5);
}

void VignetteElement::resetClearCenter()
{
    this->setClearCenter(0);
}

void VignetteElement::resetSoftness()
{
    this->setSoftness(0.6);
}

QbPacket VignetteElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int videoArea = src.width() * src.height();

    QImage oFrame = QImage(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    static qreal aspect = qQNaN();
    static qreal clearCenter = qQNaN();
    static qreal softness = qQNaN();

    if (packet.caps() != this->m_caps
        || this->m_aspect != aspect
        || this->m_clearCenter != clearCenter
        || this->m_softness != softness) {
        this->m_vignette = this->updateVignette(src.width(), src.height());

        this->m_caps = packet.caps();
        aspect = this->m_aspect;
        clearCenter = this->m_clearCenter;
        softness = this->m_softness;
    }

    // Darken the pixels by multiplying with the vignette's factor
    for (int i = 0; i < videoArea; i++) {
        int r = this->m_vignette[i] * qRed(srcBits[i]);
        int g = this->m_vignette[i] * qGreen(srcBits[i]);
        int b = this->m_vignette[i] * qBlue(srcBits[i]);

        r = qBound(0, r, 255);
        g = qBound(0, g, 255);
        b = qBound(0, b, 255);

        destBits[i] = qRgba(r, g, b, qAlpha(srcBits[i]));
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}

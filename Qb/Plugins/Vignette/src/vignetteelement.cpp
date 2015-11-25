/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <QPainter>

#include "vignetteelement.h"

VignetteElement::VignetteElement(): QbElement()
{
    this->m_color = qRgb(0, 0, 0);
    this->m_aspect = 3.0 / 7.0;
    this->m_scale = 0.5;
    this->m_softness = 0.5;

    QObject::connect(this,
                     &VignetteElement::colorChanged,
                     this,
                     &VignetteElement::updateVignette);
    QObject::connect(this,
                     &VignetteElement::aspectChanged,
                     this,
                     &VignetteElement::updateVignette);
    QObject::connect(this,
                     &VignetteElement::scaleChanged,
                     this,
                     &VignetteElement::updateVignette);
    QObject::connect(this,
                     &VignetteElement::softnessChanged,
                     this,
                     &VignetteElement::updateVignette);
    QObject::connect(this,
                     &VignetteElement::curSizeChanged,
                     this,
                     &VignetteElement::updateVignette);
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

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

QRgb VignetteElement::color() const
{
    return this->m_color;
}

qreal VignetteElement::aspect() const
{
    return this->m_aspect;
}

qreal VignetteElement::scale() const
{
    return this->m_scale;
}

qreal VignetteElement::softness() const
{
    return this->m_softness;
}

void VignetteElement::setColor(QRgb color)
{
    if (this->m_color == color)
        return;

    this->m_color = color;
    emit this->colorChanged(color);
}

void VignetteElement::setAspect(qreal aspect)
{
    if (this->m_aspect == aspect)
        return;

    this->m_aspect = aspect;
    emit this->aspectChanged(aspect);
}

void VignetteElement::setScale(qreal scale)
{
    if (this->m_scale == scale)
        return;

    this->m_scale = scale;
    emit this->scaleChanged(scale);
}

void VignetteElement::setSoftness(qreal softness)
{
    if (this->m_softness == softness)
        return;

    this->m_softness = softness;
    emit this->softnessChanged(softness);
}

void VignetteElement::resetColor()
{
    this->setColor(qRgb(0, 0, 0));
}

void VignetteElement::resetAspect()
{
    this->setAspect(3.0 / 7.0);
}

void VignetteElement::resetScale()
{
    this->setScale(0.5);
}

void VignetteElement::resetSoftness()
{
    this->setSoftness(0.5);
}

QbPacket VignetteElement::iStream(const QbPacket &packet)
{
    QImage src = QbUtils::packetToImage(packet);

    if (src.isNull())
        return QbPacket();

    QImage oFrame = src.convertToFormat(QImage::Format_ARGB32);

    if (src.size() != this->m_curSize) {
        this->m_curSize = src.size();
        emit this->curSizeChanged(this->m_curSize);
    }

    this->m_mutex.lock();
    QImage vignette = this->m_vignette;
    this->m_mutex.unlock();

    QPainter painter;
    painter.begin(&oFrame);
    painter.drawImage(0, 0, vignette);
    painter.end();

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, packet);
    qbSend(oPacket)
}

void VignetteElement::updateVignette()
{
    this->m_mutex.lock();

    QSize curSize = this->m_curSize;
    QImage vignette(curSize, QImage::Format_ARGB32);

    // Center of the ellipse.
    int xc = vignette.width() / 2;
    int yc = vignette.height() / 2;

    qreal aspect = qBound(0.0, this->m_aspect, 1.0);
    qreal rho = qBound(0.01, this->m_aspect, 0.99);

    // Calculate the maximum scale to clear the vignette.
    qreal scale = this->m_scale * sqrt(1.0 / pow(rho, 2)
                                       + 1.0 / pow(1.0 - rho, 2));

    // Calculate radius.
    qreal a = scale * aspect * xc;
    qreal b = scale * (1.0 - aspect) * yc;

    // Prevent divide by zero.
    if (a < 0.01)
        a = 0.01;

    if (b < 0.01)
        b = 0.01;

    qreal qa = a * a;
    qreal qb = b * b;
    qreal qab = qa * qb;

    int softness = 255.0 * (2.0 * this->m_softness - 1.0);

    int red = qRed(this->m_color);
    int green = qGreen(this->m_color);
    int blue = qBlue(this->m_color);
    int alpha = qAlpha(this->m_color);

    // Get the radius to a corner.
    qreal dwa = xc / a;
    qreal dhb = yc / b;
    qreal maxRadius = this->radius(dwa, dhb);

    this->m_mutex.unlock();

    for (int y = 0; y < vignette.height(); y++) {
        QRgb *line = (QRgb *) vignette.scanLine(y);
        int dy = y - yc;
        qreal qdy = dy * dy;
        qreal dyb = dy / b;

        for (int x = 0; x < vignette.width(); x++) {
            int dx = x - xc;
            qreal qdx = dx * dx;
            qreal dxa = qreal(dx) / a;

            if (qb * qdx + qa * qdy < qab
                && a != 0 && b != 0)
                // If the point is inside the ellipse,
                // show the original pixel.
                line[x] = qRgba(0, 0, 0, 0);
            else {
                // The opacity of the pixel depends on the relation between
                // it's radius and the corner radius.
                qreal k = this->radius(dxa, dyb) / maxRadius;
                int opacity = k * alpha - softness;
                opacity = qBound(0, opacity, 255);
                line[x] = qRgba(red, green, blue, opacity);
            }
        }
    }

    this->m_mutex.lock();
    this->m_vignette = vignette;
    this->m_mutex.unlock();
}

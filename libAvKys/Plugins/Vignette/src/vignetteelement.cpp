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

#include <QMutex>
#include <QQmlContext>
#include <QSize>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "vignetteelement.h"

class VignetteElementPrivate
{
    public:
        QRgb m_color {qRgb(0, 0, 0)};
        qreal m_aspect {3.0 / 7.0};
        qreal m_scale {0.5};
        qreal m_softness {0.5};
        QSize m_curSize;
        AkVideoPacket m_vignette;
        QMutex m_mutex;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        AkVideoMixer m_videoMixer;

        inline qreal radius(qreal x, qreal y) const;
        void updateVignette();
};

VignetteElement::VignetteElement(): AkElement()
{
    this->d = new VignetteElementPrivate;
}

VignetteElement::~VignetteElement()
{
    delete this->d;
}

QRgb VignetteElement::color() const
{
    return this->d->m_color;
}

qreal VignetteElement::aspect() const
{
    return this->d->m_aspect;
}

qreal VignetteElement::scale() const
{
    return this->d->m_scale;
}

qreal VignetteElement::softness() const
{
    return this->d->m_softness;
}

QString VignetteElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Vignette/share/qml/main.qml");
}

void VignetteElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Vignette", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket VignetteElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto dst = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!dst)
        return {};

    this->d->m_mutex.lock();

    QSize frameSize(dst.caps().width(), dst.caps().height());

    if (frameSize != this->d->m_curSize) {
        this->d->m_curSize = frameSize;
        this->d->updateVignette();
    }

    this->d->m_videoMixer.begin(&dst);
    this->d->m_videoMixer.draw(this->d->m_vignette);
    this->d->m_videoMixer.end();

    this->d->m_mutex.unlock();

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void VignetteElement::setColor(QRgb color)
{
    if (this->d->m_color == color)
        return;

    this->d->m_color = color;
    emit this->colorChanged(color);
    this->d->m_mutex.lock();
    this->d->updateVignette();
    this->d->m_mutex.unlock();
}

void VignetteElement::setAspect(qreal aspect)
{
    if (qFuzzyCompare(this->d->m_aspect, aspect))
        return;

    this->d->m_aspect = aspect;
    emit this->aspectChanged(aspect);
    this->d->m_mutex.lock();
    this->d->updateVignette();
    this->d->m_mutex.unlock();
}

void VignetteElement::setScale(qreal scale)
{
    if (qFuzzyCompare(this->d->m_scale, scale))
        return;

    this->d->m_scale = scale;
    emit this->scaleChanged(scale);
    this->d->m_mutex.lock();
    this->d->updateVignette();
    this->d->m_mutex.unlock();
}

void VignetteElement::setSoftness(qreal softness)
{
    if (qFuzzyCompare(this->d->m_softness, softness))
        return;

    this->d->m_softness = softness;
    emit this->softnessChanged(softness);
    this->d->m_mutex.lock();
    this->d->updateVignette();
    this->d->m_mutex.unlock();
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

qreal VignetteElementPrivate::radius(qreal x, qreal y) const
{
    return qSqrt(x * x + y * y);
}

void VignetteElementPrivate::updateVignette()
{
    AkVideoPacket vignette({AkVideoCaps::Format_argbpack,
                           this->m_curSize.width(),
                           this->m_curSize.height(),
                           {}});

    // Center of the ellipse.
    int xc = vignette.caps().width() / 2;
    int yc = vignette.caps().height() / 2;

    qreal aspect = qBound(0.0, this->m_aspect, 1.0);
    qreal rho = qBound(0.01, this->m_aspect, 0.99);

    // Calculate the maximum scale to clear the vignette.
    qreal scale = this->m_scale * qSqrt(1.0 / qPow(rho, 2)
                                        + 1.0 / qPow(1.0 - rho, 2));

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

    qreal softness = 255.0 * (2.0 * this->m_softness - 1.0);

    int red = qRed(this->m_color);
    int green = qGreen(this->m_color);
    int blue = qBlue(this->m_color);
    int alpha = qAlpha(this->m_color);

    // Get the radius to a corner.
    qreal dwa = xc / a;
    qreal dhb = yc / b;
    qreal maxRadius = this->radius(dwa, dhb);

    for (int y = 0; y < vignette.caps().height(); y++) {
        auto line = reinterpret_cast<QRgb *>(vignette.line(0, y));
        int dy = y - yc;
        qreal qdy = dy * dy;
        qreal dyb = dy / b;

        for (int x = 0; x < vignette.caps().width(); x++) {
            int dx = x - xc;
            qreal qdx = dx * dx;
            qreal dxa = qreal(dx) / a;

            if (qb * qdx + qa * qdy < qab
                && !qIsNull(a) && !qIsNull(b)) {
                // If the point is inside the ellipse,
                // show the original pixel.
                line[x] = qRgba(0, 0, 0, 0);
            } else {
                // The opacity of the pixel depends on the relation between
                // it's radius and the corner radius.
                qreal k = this->radius(dxa, dyb) / maxRadius;
                int opacity = int(k * alpha - softness);
                opacity = qBound(0, opacity, 255);
                line[x] = qRgba(red, green, blue, opacity);
            }
        }
    }

    this->m_vignette = vignette;
}

#include "moc_vignetteelement.cpp"

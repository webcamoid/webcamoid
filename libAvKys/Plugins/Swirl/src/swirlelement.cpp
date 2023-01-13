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
#include <QSize>
#include <QtMath>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "swirlelement.h"

class SwirlElementPrivate
{
    public:
        qreal m_degrees {60.0};
        QSize m_frameSize;
        qreal m_currentDegrees {0.0};
        int *m_rotationX {nullptr};
        int *m_rotationY {nullptr};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        void createRotationMap(int width, int height, qreal degrees);
};

SwirlElement::SwirlElement(): AkElement()
{
    this->d = new SwirlElementPrivate;
}

SwirlElement::~SwirlElement()
{
    if (this->d->m_rotationX)
        delete [] this->d->m_rotationX;

    if (this->d->m_rotationY)
        delete [] this->d->m_rotationY;

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
    auto degrees = this->d->m_degrees;

    if (qFuzzyCompare(degrees, 0.0)) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_frameSize
        || !qFuzzyCompare(degrees, this->d->m_currentDegrees)) {
        this->d->createRotationMap(src.caps().width(),
                                   src.caps().height(),
                                   degrees);
        this->d->m_frameSize = frameSize;
        this->d->m_currentDegrees = degrees;
    }

    for (int y = 0; y < src.caps().height(); y++) {
        auto yOffset = y * src.caps().width();
        auto xLine = this->d->m_rotationX + yOffset;
        auto yLine = this->d->m_rotationY + yOffset;
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++)
            oLine[x] = src.pixel<QRgb>(0, xLine[x], yLine[x]);
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
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

void SwirlElementPrivate::createRotationMap(int width,
                                            int height,
                                            qreal degrees)
{
    if (this->m_rotationX)
        delete [] this->m_rotationX;

    if (this->m_rotationY)
        delete [] this->m_rotationY;

    auto mapSize = size_t(width) * size_t(height);
    this->m_rotationX = new int [mapSize];
    this->m_rotationY = new int [mapSize];

    qreal xScale = 1.0;
    qreal yScale = 1.0;
    qreal xCenter = width >> 1;
    qreal yCenter = height >> 1;
    qreal radius = qMax(xCenter, yCenter);

    if (width > height)
        yScale = qreal(width) / height;
    else if (width < height)
        xScale = qreal(height) / width;

    auto radians = qDegreesToRadians(degrees);

    for (int y = 0; y < height; y++) {
        auto yOffset = y * width;
        auto xLine = this->m_rotationX + yOffset;
        auto yLine = this->m_rotationY + yOffset;
        qreal yDistance = yScale * (y - yCenter);
        auto yDistance2 = yDistance * yDistance;

        for (int x = 0; x < width; x++) {
            qreal xDistance = xScale * (x - xCenter);
            qreal distance = xDistance * xDistance + yDistance2;

            if (distance >= radius * radius) {
                xLine[x] = x;
                yLine[x] = y;
            } else {
                qreal factor = 1.0 - sqrt(distance) / radius;
                qreal sine = qSin(radians * factor * factor);
                qreal cosine = qCos(radians * factor * factor);

                int xp = int((cosine * xDistance - sine * yDistance) / xScale + xCenter);
                int yp = int((sine * xDistance + cosine * yDistance) / yScale + yCenter);

                if (xp >= 0 && xp < width && yp >= 0 && yp < height) {
                    xLine[x] = xp;
                    yLine[x] = yp;
                } else {
                    xLine[x] = x;
                    yLine[x] = y;
                }
            }
        }
    }
}

#include "moc_swirlelement.cpp"

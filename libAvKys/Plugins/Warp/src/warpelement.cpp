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

#include "warpelement.h"

class WarpElementPrivate
{
    public:
        qreal m_ripples {4};
        int m_duration {20};
        QSize m_frameSize;
        qreal *m_phiTable {nullptr};
        int m_t {0};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        void updatePhyTable(int width, int height);
};

WarpElement::WarpElement(): AkElement()
{
    this->d = new WarpElementPrivate;
}

WarpElement::~WarpElement()
{
    if (this->d->m_phiTable)
        delete [] this->d->m_phiTable;

    delete this->d;
}

qreal WarpElement::ripples() const
{
    return this->d->m_ripples;
}

int WarpElement::duration() const
{
    return this->d->m_duration;
}

QString WarpElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Warp/share/qml/main.qml");
}

void WarpElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Warp", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket WarpElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_frameSize) {
        this->d->updatePhyTable(src.caps().width(), src.caps().height());
        this->d->m_frameSize = frameSize;
    }

    qreal fps = 30.0;

    if (src.caps().fps().num() != 0
        && src.caps().fps().den() != 0
        && src.caps().fps().value() > 0.0)
        fps = src.caps().fps().value();

    int framesDuration = qRound(this->d->m_duration * fps);

    if (framesDuration < 1)
        framesDuration = 1;

    qreal dx = 30 * qSin(4 * M_PI * (this->d->m_t + 100) / framesDuration)
               + 40 * qSin(M_PI * (this->d->m_t - 10) / framesDuration);
    qreal dy = -35 * qSin(2 * M_PI * this->d->m_t / framesDuration)
               + 40 * qSin(M_PI * (this->d->m_t + 30) / framesDuration);
    qreal ripples = this->d->m_ripples * qSin(8 * M_PI * (this->d->m_t - 70) / framesDuration);

    this->d->m_t = (this->d->m_t + 1) % framesDuration;

    for (int y = 0; y < src.caps().height(); y++) {
        auto phyLine = this->d->m_phiTable + y * src.caps().width();
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            qreal phi = ripples * phyLine[x];

            int xOrig = int(dx * qCos(phi) + x);
            int yOrig = int(dy * qSin(phi) + y);

            xOrig = qBound(0, xOrig, src.caps().width() - 1);
            yOrig = qBound(0, yOrig, src.caps().height() - 1);
            dstLine[x] = src.pixel<QRgb>(0, xOrig, yOrig);
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void WarpElement::setRipples(qreal ripples)
{
    if (qFuzzyCompare(this->d->m_ripples, ripples))
        return;

    this->d->m_ripples = ripples;
    emit this->ripplesChanged(ripples);
}

void WarpElement::setDuration(int duration)
{
    if (this->d->m_duration == duration)
        return;

    this->d->m_duration = duration;
    emit this->durationChanged(duration);
}

void WarpElement::resetRipples()
{
    this->setRipples(4);
}

void WarpElement::resetDuration()
{
    this->setDuration(20);
}

void WarpElementPrivate::updatePhyTable(int width, int height)
{
    int cx = width >> 1;
    int cy = height >> 1;

    qreal k = 2.0 * M_PI / qSqrt(cx * cx + cy * cy);

    if (this->m_phiTable)
        delete [] this->m_phiTable;

    this->m_phiTable = new qreal [width * height];

    for (int y = 0; y < height; y++) {
        auto phyLine = this->m_phiTable + y * width;
        auto diffY = y - cy;
        auto diffY2 = diffY * diffY;

        for (int x = 0; x < width; x++) {
            auto diffX = x - cx;
            auto diffX2 = diffX * diffX;
            phyLine[x] = k * qSqrt(diffX2 + diffY2);
        }
    }
}

#include "moc_warpelement.cpp"

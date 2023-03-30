/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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
#include <QVariant>
#include <QtMath>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "rotateelement.h"

#define VALUE_SHIFT 8

class RotateElementPrivate
{
    public:
        qreal m_angle {0.0};
        bool m_keep {false};
        qint64 m_kernel[4];
        qint64 m_frameKernel[4];
        bool m_clampBounds {false};
        QMutex m_mutex;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        inline void updateMatrix(qreal angle);
};

RotateElement::RotateElement(): AkElement()
{
    this->d = new RotateElementPrivate;
    this->d->updateMatrix(this->d->m_angle);
}

RotateElement::~RotateElement()
{
    delete this->d;
}

qreal RotateElement::angle() const
{
    return this->d->m_angle;
}

bool RotateElement::keep() const
{
    return this->d->m_keep;
}

QString RotateElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Rotate/share/qml/main.qml");
}

void RotateElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Rotate", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket RotateElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    this->d->m_mutex.lock();

    int oWidth = 0;
    int oHeight = 0;

    if (this->d->m_keep) {
        oWidth = src.caps().width();
        oHeight = src.caps().height();
    } else {
        oWidth  = (src.caps().width() * this->d->m_frameKernel[0] + src.caps().height() * this->d->m_frameKernel[1]) >> VALUE_SHIFT;
        oHeight = (src.caps().width() * this->d->m_frameKernel[2] + src.caps().height() * this->d->m_frameKernel[3]) >> VALUE_SHIFT;
    }

    auto caps = src.caps();
    caps.setWidth(oWidth);
    caps.setHeight(oHeight);
    AkVideoPacket dst(caps);
    dst.copyMetadata(src);

    int scx = src.caps().width() >> 1;
    int scy = src.caps().height() >> 1;
    int dcx = dst.caps().width() >> 1;
    int dcy = dst.caps().height() >> 1;

    for (int y = 0; y < dst.caps().height(); y++) {
        int dy = y - dcy;
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < dst.caps().width(); x++) {
            int dx = x - dcx;
            int xp = int(((dx * this->d->m_kernel[0] + dy * this->d->m_kernel[1]) >> VALUE_SHIFT) + scx);
            int yp = int(((dx * this->d->m_kernel[2] + dy * this->d->m_kernel[3]) >> VALUE_SHIFT) + scy);

            if (this->d->m_clampBounds) {
                xp = qBound(0, xp, src.caps().width() - 1);
                yp = qBound(0, yp, src.caps().height() - 1);
            }

            if (xp >= 0 && xp < src.caps().width()
                && yp >= 0 && yp < src.caps().height()) {
                auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, yp));
                oLine[x] = iLine[xp];
            } else {
                oLine[x] = qRgba(0, 0, 0, 0);
            }
        }
    }

    this->d->m_mutex.unlock();

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void RotateElement::setAngle(qreal angle)
{
    if (this->d->m_angle == angle)
        return;

    this->d->m_angle = angle;
    emit this->angleChanged(angle);
    this->d->updateMatrix(angle);
}

void RotateElement::setKeep(bool keep)
{
    if (this->d->m_keep == keep)
        return;

    this->d->m_keep = keep;
    emit this->keepChanged(keep);
}

void RotateElement::resetAngle()
{
    this->setAngle(0.0);
}

void RotateElement::resetKeep()
{
    this->setKeep(false);
}

void RotateElementPrivate::updateMatrix(qreal angle)
{
    int mult = 1 << VALUE_SHIFT;
    auto radians = angle * M_PI / 180.0f;
    auto c = qRound64(mult * qCos(radians));
    auto s = qRound64(mult * qSin(radians));
    auto ca = qAbs(c);
    auto sa = qAbs(s);

    this->m_mutex.lock();

    this->m_kernel[0] =  c;
    this->m_kernel[1] = -s;
    this->m_kernel[2] =  s;
    this->m_kernel[3] =  c;

    this->m_frameKernel[0] = ca;
    this->m_frameKernel[1] = sa;
    this->m_frameKernel[2] = sa;
    this->m_frameKernel[3] = ca;

    this->m_mutex.unlock();

    this->m_clampBounds =
            this->m_frameKernel[0] == 0 || this->m_frameKernel[0] == mult;
}

#include "moc_rotateelement.cpp"

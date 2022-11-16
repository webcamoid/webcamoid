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
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "embosselement.h"

class EmbossElementPrivate
{
    public:
        qreal m_factor {1.0};
        qreal m_bias {128.0};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_graya8pack, 0, 0, {}}};
};

EmbossElement::EmbossElement(): AkElement()
{
    this->d = new EmbossElementPrivate;
}

EmbossElement::~EmbossElement()
{
    delete this->d;
}

qreal EmbossElement::factor() const
{
    return this->d->m_factor;
}

qreal EmbossElement::bias() const
{
    return this->d->m_bias;
}

QString EmbossElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Emboss/share/qml/main.qml");
}

void EmbossElement::controlInterfaceConfigure(QQmlContext *context,
                                              const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Emboss", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket EmbossElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    auto width_1 = src.caps().width() - 1;
    auto height_1 = src.caps().height() - 1;

    auto factor = this->d->m_factor;
    auto bias = this->d->m_bias;

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const quint16 *>(src.constLine(0, y));
        auto srcLine_m1 = reinterpret_cast<const quint16 *>(src.constLine(0, qMax(y - 1, 0)));
        auto srcLine_p1 = reinterpret_cast<const quint16 *>(src.constLine(0, qMin(y + 1, height_1)));

        auto dstLine  = reinterpret_cast<quint16 *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            int x_m1 = qMax(x - 1, 0);
            int x_p1 = qMin(x + 1,  width_1);

            int gray = 2 * (srcLine_m1[x_m1] >> 8)
                     + (srcLine_m1[x] >> 8)
                     + (srcLine[x_m1] >> 8)
                     - (srcLine[x_p1] >> 8)
                     - (srcLine_p1[x] >> 8)
                     - (2 * srcLine_p1[x_p1] >> 8);

            gray = qRound(factor * gray + bias);
            quint16 y = quint8(qBound(0, gray, 255));
            quint16 a = srcLine[x] & 0xff;
            dstLine[x] = y << 8 | a;
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void EmbossElement::setFactor(qreal factor)
{
    if (qFuzzyCompare(this->d->m_factor, factor))
        return;

    this->d->m_factor = factor;
    emit this->factorChanged(factor);
}

void EmbossElement::setBias(qreal bias)
{
    if (qFuzzyCompare(this->d->m_bias, bias))
        return;

    this->d->m_bias = bias;
    emit this->biasChanged(bias);
}

void EmbossElement::resetFactor()
{
    this->setFactor(1);
}

void EmbossElement::resetBias()
{
    this->setBias(128);
}

#include "moc_embosselement.cpp"

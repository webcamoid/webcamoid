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

#include <QQmlContext>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "opacityelement.h"

class OpacityElementPrivate
{
    public:
        qreal m_opacity {1.0};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
};

OpacityElement::OpacityElement(): AkElement()
{
    this->d = new OpacityElementPrivate;
}

OpacityElement::~OpacityElement()
{
    delete this->d;
}

qreal OpacityElement::opacity() const
{
    return this->d->m_opacity;
}

QString OpacityElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Opacity/share/qml/main.qml");
}

void OpacityElement::controlInterfaceConfigure(QQmlContext *context,
                                              const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Opacity", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket OpacityElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    auto opacity = qRound(256 * this->d->m_opacity);

    for (int y = 0; y < src.caps().height(); y++) {
        auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = iLine[x];

            auto r = qRed(pixel);
            auto g = qGreen(pixel);
            auto b = qBlue(pixel);
            auto a = (opacity * qAlpha(pixel)) >> 8;

            oLine[x] = qRgba(r, g, b, a);
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void OpacityElement::setOpacity(qreal opacity)
{
    if (qFuzzyCompare(this->d->m_opacity, opacity))
        return;

    this->d->m_opacity = opacity;
    emit this->opacityChanged(opacity);
}

void OpacityElement::resetOpacity()
{
    this->setOpacity(1.0);
}

#include "moc_opacityelement.cpp"

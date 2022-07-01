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

#include <QImage>
#include <QQmlContext>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "aspectratioelement.h"

class AspectRatioElementPrivate
{
    public:
        int m_width {16};
        int m_height {9};
        AkVideoConverter m_videoConverter {AkVideoCaps(AkVideoCaps::Format_argb, 0, 0, {})};
};

AspectRatioElement::AspectRatioElement(): AkElement()
{
    this->d = new AspectRatioElementPrivate;
}

AspectRatioElement::~AspectRatioElement()
{
    delete this->d;
}

int AspectRatioElement::width() const
{
    return this->d->m_width;
}

int AspectRatioElement::height() const
{
    return this->d->m_height;
}

QString AspectRatioElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/AspectRatio/share/qml/main.qml");
}

void AspectRatioElement::controlInterfaceConfigure(QQmlContext *context,
                                              const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("AspectRatio", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket AspectRatioElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = this->d->m_videoConverter.convertToImage(packet);

    if (src.isNull())
        return AkPacket();

    int oWidth = qRound(qreal(src.height())
                         * qMax(this->d->m_width, 1)
                         / qMax(this->d->m_height, 1));
    oWidth = qMin(oWidth, src.width());
    int oHeight = qRound(qreal(src.width())
                         * qMax(this->d->m_height, 1)
                         / qMax(this->d->m_width, 1));
    oHeight = qMin(oHeight, src.height());
    int x = (src.width() - oWidth) / 2;
    int y = (src.height() - oHeight) / 2;
    auto oFrame = src.copy(x, y, oWidth, oHeight);
    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

void AspectRatioElement::setWidth(int width)
{
    if (this->d->m_width == width)
        return;

    this->d->m_width = width;
    emit this->widthChanged(width);
}

void AspectRatioElement::setHeight(int height)
{
    if (this->d->m_height == height)
        return;

    this->d->m_height = height;
    emit this->heightChanged(height);
}

void AspectRatioElement::resetWidth()
{
    this->setWidth(16);
}

void AspectRatioElement::resetHeight()
{
    this->setHeight(9);
}

#include "moc_aspectratioelement.cpp"

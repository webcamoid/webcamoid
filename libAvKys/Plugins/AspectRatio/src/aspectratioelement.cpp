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
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "aspectratioelement.h"

class AspectRatioElementPrivate
{
    public:
        int m_width {16};
        int m_height {9};
        AkVideoConverter m_videoConverter;
};

AspectRatioElement::AspectRatioElement(): AkElement()
{
    this->d = new AspectRatioElementPrivate;
    this->d->m_videoConverter.setAspectRatioMode(AkVideoConverter::AspectRatioMode_Expanding);
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
    if (!packet)
        return {};

    int oWidth = qRound(qreal(packet.caps().height())
                         * qMax(this->d->m_width, 1)
                         / qMax(this->d->m_height, 1));
    oWidth = qMin(oWidth, packet.caps().width());
    int oHeight = qRound(qreal(packet.caps().width())
                         * qMax(this->d->m_height, 1)
                         / qMax(this->d->m_width, 1));
    oHeight = qMin(oHeight, packet.caps().height());

    AkVideoCaps oCaps(packet.caps());
    oCaps.setWidth(oWidth);
    oCaps.setHeight(oHeight);
    this->d->m_videoConverter.setOutputCaps(oCaps);

    this->d->m_videoConverter.begin();
    auto dst = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (dst)
        emit this->oStream(dst);

    return dst;
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

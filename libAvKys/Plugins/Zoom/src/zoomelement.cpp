/* Webcamoid, webcam capture application. Zoom Plug-in.
 * Copyright (C) 2022  Tj <hacker@iam.tj>
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

#include <cmath>
#include <QDebug>
#include <QRect>
#include <QImage>
#include <QQmlContext>
#include <akpacket.h>
#include <akvideopacket.h>

#include "zoomelement.h"

class ZoomElementPrivate
{
    public:
        double m_zoom = 1.00;
        double m_maxZoom = 20.0;
};

ZoomElement::ZoomElement(): AkElement()
{
    d = new ZoomElementPrivate;
}

ZoomElement::~ZoomElement()
{
    delete d;
}

QString ZoomElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId);
    return QString("qrc:/Zoom/share/qml/main.qml");
}

void ZoomElement::controlInterfaceConfigure(QQmlContext *context, const QString &controlId) const
{
    Q_UNUSED(controlId)
    context->setContextProperty("Zoom", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

// getters
double ZoomElement::zoom() const
{
    return d->m_zoom;
}
double ZoomElement::maxZoom() const
{
    return d->m_maxZoom;
}

// setters
void ZoomElement::setZoom(double newZoom)
{
    if ( newZoom >= 1.0 && newZoom <= d->m_maxZoom)
    {
        d->m_zoom = newZoom;
        emit zoomChanged(d->m_zoom);
    }
}
void ZoomElement::setMaxZoom(double newMaxZoom)
{
    if ( newMaxZoom >= 1.0)
    {
        d->m_maxZoom = newMaxZoom;
        emit maxZoomChanged(d->m_maxZoom);
    }
}

// internal

AkPacket ZoomElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = packet.toImage();
    QRect box;

    // canot test for being exactly 1.0 since it might be 1.000000001
    if (d->m_zoom < 1.01)
        akSend(packet)
    box.setWidth( static_cast<int>( lround( static_cast<double>( src.width() ) / d->m_zoom ) ) );
    box.setHeight( static_cast<int>( lround( static_cast<double>( src.height() ) / d->m_zoom ) ) );
    // do NOT use set{Left,Right} they alter the width! (lost 6 hours doubting my math on that!)
    box.moveLeft((src.width() / 2) - (box.width() / 2));
    box.moveTop((src.height() / 2) - (box.height() / 2));
    auto oPacket = AkVideoPacket::fromImage(box.isValid() ? src.copy(box) : src, packet);
    akSend(oPacket)
}

#include "moc_zoomelement.cpp"

/* Webcamoid, webcam capture application. Crop Plug-in.
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
#include <QRect>
#include <QImage>
#include <QQmlContext>
#include <akpacket.h>
#include <akvideopacket.h>

#include "cropelement.h"

class CropElementPrivate
{
    public:
        QRect m_box {}; // satisfies QRect::isNull()
        QRect m_source {}; // original source size
        QRect m_maximum {}; // maximum given m_box left,top values
        QRect m_minimum {}; // smallest acceptable
        double m_aspectRatio = 0;
};

CropElement::CropElement(): AkElement()
{
    d = new CropElementPrivate;
}

CropElement::~CropElement()
{
    delete d;
}

QString CropElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId);
    return QString("qrc:/Crop/share/qml/main.qml");
}

void CropElement::controlInterfaceConfigure(QQmlContext *context, const QString &controlId) const
{
    Q_UNUSED(controlId)
    context->setContextProperty("Cropping", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}
int CropElement::recalcHeight(const int width)
{
    return static_cast<int>(lround(width / d->m_aspectRatio));
}
int CropElement::recalcWidth(const int height)
{
    return static_cast<int>(lround(height * d->m_aspectRatio));
}

// getters (using QRectF to enable reading from QML)
QRectF CropElement::box() const
{
    return QRectF(d->m_box);
}
QRectF CropElement::minimum() const
{
    return QRectF(d->m_minimum);
}
QRectF CropElement::maximum() const
{
    return QRectF(d->m_maximum);
}
double CropElement::aspectRatio() const
{
    return d->m_aspectRatio;
}

// setters (using QRectF to enable setting from QML)
void CropElement::setMinimum(QRectF qfMin)
{
    QRect newMin = qfMin.toRect();
    if (!newMin.isNull() && newMin.isValid()
      && d->m_maximum.width() >= newMin.width()
      && d->m_maximum.height() >= newMin.height()
      && aspectRatioGood(newMin))
    {
        d->m_minimum = newMin;
        emit minimumChanged(d->m_minimum);
    }
}

void CropElement::setBox(QRectF qfBox)
{
    bool updateWidth = true;
    bool updateHeight = true;
    bool updateMax = false;

    QRect newBox = qfBox.toRect();
    if (newBox.isNull() || ! newBox.isValid() || newBox == d->m_box)
        return;

    int maxX = newBox.width() + newBox.left();
    if (maxX > d->m_source.width()) {
        // need to shrink width of new box
        int newW = d->m_source.width() - newBox.left();
        if (newW < d->m_minimum.width()) {
            updateWidth = false;
        } else {
            newBox.setWidth(newW);
            newBox.setHeight(recalcHeight(newW));
            d->m_maximum.setWidth(newBox.width());
            d->m_maximum.setHeight(newBox.height());
            updateWidth = true;
            updateMax = true;
        }
    } else {
        updateWidth = false;
        if (newBox.left() + newBox.width() < d->m_source.width()) {
            d->m_maximum.setWidth(d->m_source.width() - newBox.left());
            d->m_maximum.setHeight(recalcHeight(d->m_maximum.width()));
            updateMax = true;
        }
    }

    int maxY = newBox.height() + newBox.top();
    if (maxY > d->m_source.height()) {
        // need to shrink height
        int newH = d->m_source.height() - newBox.top();
        if (newH < d->m_minimum.width()) {
            updateHeight = false;
        } else {
            newBox.setHeight(newH);
            newBox.setWidth(recalcWidth(newH));
            d->m_maximum.setHeight(newBox.height());
            d->m_maximum.setWidth(newBox.width());
            updateHeight = true;
            updateMax = true;
        }
    } else {
        updateHeight = false;
        if (!updateMax && (newBox.top() + newBox.height() < d->m_source.height())) {
            d->m_maximum.setHeight(d->m_source.height() - newBox.top());
            d->m_maximum.setWidth(recalcWidth(d->m_maximum.height()));
            updateMax = true;
        }
    }

    if (updateMax)
        emit maximumChanged(d->m_maximum);

    if (newBox.isValid()) {
        d->m_box = newBox;
        emit boxChanged(d->m_box);
    }
}
void CropElement::resetMinimum()
{
    if (d->m_source.isValid())
    {
        d->m_minimum.setWidth(d->m_source.width() / 10);
        d->m_minimum.setHeight(d->m_source.height() / 10);
        emit minimumChanged(d->m_minimum);
    }
}
void CropElement::resetBox()
{
    if (d->m_source.isValid())
    {
        d->m_maximum = d->m_source;
        d->m_box = d->m_maximum;
        emit maximumChanged(d->m_maximum);
        emit boxChanged(d->m_box);
    }
}

// allow external to intialise effect (when no source is active) - usually done dynamically in iVideoStream()
void CropElement::initLimits()
{
    // initialise to Full HD size
    d->m_maximum.setWidth(1920);
    d->m_maximum.setHeight(1080);
    emit maximumChanged(d->m_maximum);
    d->m_aspectRatio = static_cast<double>(d->m_maximum.width()) / d->m_maximum.height();
    emit aspectRatioChanged(d->m_aspectRatio);
    // set box size so QML compoments don't reset their .value if it is out of range of .from <> .to
    d->m_box = d->m_maximum;
    emit boxChanged(d->m_box);
    d->m_minimum.setWidth(d->m_maximum.width() / 10);
    d->m_minimum.setHeight(d->m_maximum.height() / 10);
    emit minimumChanged(d->m_minimum);
}

// internal

bool CropElement::aspectRatioGood(QRect rectangle)
{
    double ar = 0;
    if (rectangle.isValid())
        ar = static_cast<double>(rectangle.width()) / rectangle.height();
    return ar == d->m_aspectRatio ? true : false;
}


AkPacket CropElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = packet.toImage();

    if (src.isNull())
    {
        d->m_source = {};
        d->m_box = {};
        d->m_maximum = {};
        d->m_minimum = {};
        d->m_aspectRatio = 0;
        // don't emit signals else GUI might stutter between values rapidly
        akSend(packet)
    }

    // previous filter, or the source, may have changed size
    if (d->m_source.width()  != src.width()
      || d->m_source.height() != src.height())
    {
        d->m_source.setWidth(src.width());
        d->m_source.setHeight(src.height());
        d->m_maximum = d->m_source;
        emit maximumChanged(d->m_maximum);
        d->m_aspectRatio = static_cast<double>(src.width()) / src.height();
        emit aspectRatioChanged(d->m_aspectRatio);
        // and that affects minimum size
        d->m_minimum.setWidth(src.width() / 10);
        d->m_minimum.setHeight(src.height() / 10);
        emit minimumChanged(d->m_minimum);
    }

    // need to initialise crop area to match the frame; should only happen once
    if (d->m_box.isNull())
    {
        d->m_box.setLeft(0);
        d->m_box.setTop(0);
        d->m_box.setWidth(src.width());
        d->m_box.setHeight(src.height());
        emit boxChanged(d->m_maximum);
    }
    auto oPacket = AkVideoPacket::fromImage(d->m_box.isValid() ? src.copy(d->m_box) : src, packet);
    akSend(oPacket)
}

#include "moc_cropelement.cpp"

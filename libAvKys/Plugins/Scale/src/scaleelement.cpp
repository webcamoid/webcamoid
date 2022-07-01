/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "scaleelement.h"

class ScaleElementPrivate
{
    public:
        int m_width {-1};
        int m_height {-1};
        ScaleElement::ScalingMode m_scaling {ScaleElement::Fast};
        ScaleElement::AspectRatioMode m_aspectRatio {ScaleElement::Ignore};
        AkVideoConverter m_videoConverter;
};

ScaleElement::ScaleElement(): AkElement()
{
    this->d = new ScaleElementPrivate;
}

ScaleElement::~ScaleElement()
{
    delete this->d;
}

int ScaleElement::width() const
{
    return this->d->m_width;
}

int ScaleElement::height() const
{
    return this->d->m_height;
}

ScaleElement::ScalingMode ScaleElement::scaling() const
{
    return this->d->m_scaling;
}

ScaleElement::AspectRatioMode ScaleElement::aspectRatio() const
{
    return this->d->m_aspectRatio;
}

QString ScaleElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Scale/share/qml/main.qml");
}

void ScaleElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Scaling", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket ScaleElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = this->d->m_videoConverter.convertToImage(packet);

    if (src.isNull())
        return {};

    int width = this->d->m_width < 1? src.width(): this->d->m_width;
    int height = this->d->m_height < 1? src.height(): this->d->m_height;
    Qt::AspectRatioMode aspectMode = Qt::IgnoreAspectRatio;
    Qt::TransformationMode mode = Qt::FastTransformation;

    switch (this->d->m_aspectRatio) {
    case Keep:
        aspectMode = Qt::KeepAspectRatio;

        break;

    case Expanding:
        aspectMode = Qt::KeepAspectRatioByExpanding;

        break;

    default:
        break;
    }

    if (this->d->m_scaling == Linear)
        mode = Qt::SmoothTransformation;

    auto oFrame = src.scaled(width, height, aspectMode, mode);
    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

void ScaleElement::setWidth(int width)
{
    if (this->d->m_width == width)
        return;

    this->d->m_width = width;
    emit this->widthChanged(this->d->m_width);
}

void ScaleElement::setHeight(int height)
{
    if (this->d->m_height == height)
        return;

    this->d->m_height = height;
    emit this->heightChanged(this->d->m_height);
}

void ScaleElement::setScaling(ScalingMode scaling)
{
    if (this->d->m_scaling == scaling)
        return;

    this->d->m_scaling = scaling;
    emit this->scalingChanged(this->d->m_scaling);
}

void ScaleElement::setAspectRatio(AspectRatioMode aspectRatio)
{
    if (this->d->m_aspectRatio == aspectRatio)
        return;

    this->d->m_aspectRatio = aspectRatio;
    emit this->aspectRatioChanged(this->d->m_aspectRatio);
}

void ScaleElement::resetWidth()
{
    this->setWidth(-1);
}

void ScaleElement::resetHeight()
{
    this->setHeight(-1);
}

void ScaleElement::resetScaling()
{
    this->setScaling(Fast);
}

void ScaleElement::resetAspectRatio()
{
    this->setAspectRatio(Ignore);
}

QDataStream &operator >>(QDataStream &istream, ScaleElement::ScalingMode &scaling)
{
    int scalingInt;
    istream >> scalingInt;
    scaling = static_cast<ScaleElement::ScalingMode>(scalingInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, ScaleElement::ScalingMode scaling)
{
    ostream << static_cast<int>(scaling);

    return ostream;
}

QDataStream &operator >>(QDataStream &istream, ScaleElement::AspectRatioMode &aspectRatioMode)
{
    int aspectRatioModeInt;
    istream >> aspectRatioModeInt;
    aspectRatioMode = static_cast<ScaleElement::AspectRatioMode>(aspectRatioModeInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, ScaleElement::AspectRatioMode aspectRatioMode)
{
    ostream << static_cast<int>(aspectRatioMode);

    return ostream;
}

#include "moc_scaleelement.cpp"

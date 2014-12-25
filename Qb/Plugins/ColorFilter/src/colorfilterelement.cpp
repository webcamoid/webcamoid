/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "colorfilterelement.h"

ColorFilterElement::ColorFilterElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    qRegisterMetaType<QRgb>("QRgb");

    this->resetColor();
    this->resetRadius();
    this->resetSoft();
    this->resetDisable();
}

QObject *ColorFilterElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/ColorFilter/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("ColorFilter", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

QRgb ColorFilterElement::color() const
{
    return this->m_color;
}

float ColorFilterElement::radius() const
{
    return this->m_radius;
}

bool ColorFilterElement::soft() const
{
    return this->m_soft;
}

bool ColorFilterElement::disable() const
{
    return this->m_disable;
}

void ColorFilterElement::setColor(QRgb color)
{
    if (color != this->m_color) {
        this->m_color = color;
        emit this->colorChanged();
    }
}

void ColorFilterElement::setRadius(float radius)
{
    if (radius != this->m_radius) {
        this->m_radius = radius;
        emit this->radiusChanged();
    }
}

void ColorFilterElement::setSoft(bool soft)
{
    if (soft != this->m_soft) {
        this->m_soft = soft;
        emit this->softChanged();
    }
}

void ColorFilterElement::setDisable(bool disable)
{
    if (disable != this->m_disable) {
        this->m_disable = disable;
        emit this->disableChanged();
    }
}

void ColorFilterElement::resetColor()
{
    this->setColor(qRgb(0, 0, 0));
}

void ColorFilterElement::resetRadius()
{
    this->setRadius(1.0);
}

void ColorFilterElement::resetSoft()
{
    this->setSoft(false);
}

void ColorFilterElement::resetDisable()
{
    this->setDisable(false);
}

QbPacket ColorFilterElement::iStream(const QbPacket &packet)
{
    if (this->m_disable)
        qbSend(packet)

    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    for (int i = 0; i < videoArea; i++) {
        int r = qRed(srcBits[i]);
        int g = qGreen(srcBits[i]);
        int b = qBlue(srcBits[i]);

        int rf = qRed(this->m_color);
        int gf = qGreen(this->m_color);
        int bf = qBlue(this->m_color);

        int rd = r - rf;
        int gd = g - gf;
        int bd = b - bf;

        float k = sqrt(rd * rd + gd * gd + bd * bd);

        if (k <= this->m_radius) {
            if (this->m_soft) {
                float p = k / this->m_radius;

                int gray = qGray(srcBits[i]);

                r = p * (gray - r) + r;
                g = p * (gray - g) + g;
                b = p * (gray - b) + b;

                destBits[i] = qRgba(r, g, b, qAlpha(srcBits[i]));
            }
            else
                destBits[i] = srcBits[i];
        }
        else {
            int gray = qGray(srcBits[i]);
            destBits[i] = qRgba(gray, gray, gray, qAlpha(srcBits[i]));
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}

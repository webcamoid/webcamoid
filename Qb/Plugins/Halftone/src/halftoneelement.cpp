/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <QStandardPaths>

#include "halftoneelement.h"

HalftoneElement::HalftoneElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetPattern();
    this->resetPatternSize();
    this->resetLightness();
    this->resetSlope();
    this->resetIntercept();
}

QObject *HalftoneElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Halftone/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Halftone", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    QStringList picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    context->setContextProperty("picturesPath", picturesPath[0]);

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

QString HalftoneElement::pattern() const
{
    return this->m_pattern;
}

QSize HalftoneElement::patternSize() const
{
    return this->m_patternSize;
}

qreal HalftoneElement::lightness() const
{
    return this->m_lightness;
}

qreal HalftoneElement::slope() const
{
    return this->m_slope;
}

qreal HalftoneElement::intercept() const
{
    return this->m_intercept;
}

QImage HalftoneElement::loadPattern(const QString &patternFile, const QSize &size) const
{
    QImage image(patternFile);
    QSize patternSize = size.isEmpty()? image.size(): size;
    QImage pattern(patternSize, QImage::Format_Indexed8);

    for (int i = 0; i < 256; i++)
        pattern.setColor(i, qRgb(i, i, i));

    image = image.scaled(patternSize).convertToFormat(QImage::Format_RGB32);
    const QRgb *bits = (const QRgb *) image.constBits();
    quint8 *patternBits = (quint8 *) pattern.bits();
    int videoArea = patternSize.width() * patternSize.height();

    for (int i = 0; i < videoArea; i++)
        patternBits[i] = qGray(bits[i]);

    return pattern;
}

void HalftoneElement::setPattern(const QString &pattern)
{
    if (pattern != this->m_pattern) {
        this->m_pattern = pattern;
        emit this->patternChanged();
    }
}

void HalftoneElement::setPatternSize(const QSize &patternSize)
{
    if (patternSize != this->m_patternSize) {
        this->m_patternSize = patternSize;
        emit this->patternSizeChanged();
    }
}

void HalftoneElement::setLightness(qreal lightness)
{
    if (lightness != this->m_lightness) {
        this->m_lightness = lightness;
        emit this->lightnessChanged();
    }
}

void HalftoneElement::setSlope(qreal slope)
{
    if (slope != this->m_slope) {
        this->m_slope = slope;
        emit this->slopeChanged();
    }
}

void HalftoneElement::setIntercept(qreal intercept)
{
    if (intercept != this->m_intercept) {
        this->m_intercept = intercept;
        emit this->interceptChanged();
    }
}

void HalftoneElement::resetPattern()
{
    this->setPattern(":/Halftone/share/patterns/ditherCluster8Matrix.bmp");
}

void HalftoneElement::resetPatternSize()
{
    this->setPatternSize(QSize());
}

void HalftoneElement::resetLightness()
{
    this->setLightness(0.5);
}

void HalftoneElement::resetSlope()
{
    this->setSlope(1.0);
}

void HalftoneElement::resetIntercept()
{
    this->setIntercept(0.0);
}

QbPacket HalftoneElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    static QbCaps caps;
    static QString pattern;
    static QSize patternSize;

    if (packet.caps() != caps
        || this->m_pattern != pattern
        || this->m_patternSize != patternSize) {
        this->m_patternImage = this->loadPattern(this->m_pattern, this->m_patternSize);
        caps = packet.caps();
    }

    quint8 *patternBits = (quint8 *) this->m_patternImage.bits();

    // filter image
    for (int i = 0, y = 0; y < src.height(); y++)
        for (int x = 0; x < src.width(); i++, x++) {
            int col = x % this->m_patternImage.width();
            int row = y % this->m_patternImage.height();

            int gray = qGray(srcBits[i]);
            int threshold = patternBits[row * this->m_patternImage.width() + col];
            threshold = this->m_slope * threshold + this->m_intercept;
            threshold = qBound(0, threshold, 255);

            if (gray > threshold)
                destBits[i] = srcBits[i];
            else {
                QColor color(srcBits[i]);

                color.setHsl(color.hue(),
                             color.saturation(),
                             this->m_lightness * color.lightness(),
                             color.alpha());

                destBits[i] = color.rgba();
            }
        }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}

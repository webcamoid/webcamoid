/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <QColor>
#include <QStandardPaths>

#include "halftoneelement.h"

HalftoneElement::HalftoneElement(): AkElement()
{
    this->m_pattern = ":/Halftone/share/patterns/ditherCluster8Matrix.bmp";
    this->m_lightness = 0.5;
    this->m_slope = 1.0;
    this->m_intercept = 0.0;

    this->updatePattern();

    QObject::connect(this,
                     &HalftoneElement::patternChanged,
                     this,
                     &HalftoneElement::updatePattern);
    QObject::connect(this,
                     &HalftoneElement::patternSizeChanged,
                     this,
                     &HalftoneElement::updatePattern);
}

QObject *HalftoneElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Halftone/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Halftone", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    QStringList picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    context->setContextProperty("picturesPath", picturesPath[0]);

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

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

void HalftoneElement::updatePattern()
{
    if (this->m_pattern.isEmpty()) {
        this->m_mutex.lock();
        this->m_patternImage = QImage();
        this->m_mutex.unlock();

        return;
    }

    QImage image(this->m_pattern);

    if (image.isNull()) {
        this->m_mutex.lock();
        this->m_patternImage = QImage();
        this->m_mutex.unlock();

        return;
    }

    QSize patternSize = this->m_patternSize.isEmpty()?
                            image.size(): this->m_patternSize;
    QImage pattern(patternSize, QImage::Format_Indexed8);

    for (int i = 0; i < 256; i++)
        pattern.setColor(i, qRgb(i, i, i));

    image = image.scaled(patternSize).convertToFormat(QImage::Format_RGB32);
    const QRgb *bits = (const QRgb *) image.constBits();
    quint8 *patternBits = (quint8 *) pattern.bits();
    int videoArea = patternSize.width() * patternSize.height();

    for (int i = 0; i < videoArea; i++)
        patternBits[i] = qGray(bits[i]);

    this->m_mutex.lock();
    this->m_patternImage = pattern;
    this->m_mutex.unlock();
}

void HalftoneElement::setPattern(const QString &pattern)
{
    if (this->m_pattern == pattern)
        return;

    this->m_pattern = pattern;
    emit this->patternChanged(pattern);
}

void HalftoneElement::setPatternSize(const QSize &patternSize)
{
    if (this->m_patternSize == patternSize)
        return;

    this->m_patternSize = patternSize;
    emit this->patternSizeChanged(patternSize);
}

void HalftoneElement::setLightness(qreal lightness)
{
    if (this->m_lightness == lightness)
        return;

    this->m_lightness = lightness;
    emit this->lightnessChanged(lightness);
}

void HalftoneElement::setSlope(qreal slope)
{
    if (this->m_slope == slope)
        return;

    this->m_slope = slope;
    emit this->slopeChanged(slope);
}

void HalftoneElement::setIntercept(qreal intercept)
{
    if (this->m_intercept == intercept)
        return;

    this->m_intercept = intercept;
    emit this->interceptChanged(intercept);
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

AkPacket HalftoneElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    this->m_mutex.lock();

    if (this->m_patternImage.isNull()) {
        this->m_mutex.unlock();
        akSend(packet)
    }

    QImage patternImage = this->m_patternImage.copy();
    this->m_mutex.unlock();

    // filter image
    for (int y = 0; y < src.height(); y++) {
        const QRgb *iLine = (const QRgb *) src.constScanLine(y);
        QRgb *oLine = (QRgb *) oFrame.scanLine(y);

        for (int x = 0; x < src.width(); x++) {
            int col = x % patternImage.width();
            int row = y % patternImage.height();

            int gray = qGray(iLine[x]);
            const quint8 *pattern = (const quint8 *) patternImage.constScanLine(row);
            int threshold = pattern[col];
            threshold = this->m_slope * threshold + this->m_intercept;
            threshold = qBound(0, threshold, 255);

            if (gray > threshold)
                oLine[x] = iLine[x];
            else {
                QColor color(iLine[x]);

                color.setHsl(color.hue(),
                             color.saturation(),
                             this->m_lightness * color.lightness(),
                             color.alpha());

                oLine[x] = color.rgba();
            }
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

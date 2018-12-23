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

#include <QImage>
#include <QQmlContext>
#include <QMutex>
#include <QStandardPaths>
#include <akutils.h>
#include <akpacket.h>

#include "halftoneelement.h"

class HalftoneElementPrivate
{
    public:
        QString m_pattern {":/Halftone/share/patterns/ditherCluster8Matrix.bmp"};
        QSize m_patternSize;
        qreal m_lightness {0.5};
        qreal m_slope {1.0};
        qreal m_intercept {0.0};
        QMutex m_mutex;
        QSize m_frameSize;
        QImage m_patternImage;
};

HalftoneElement::HalftoneElement(): AkElement()
{
    this->d = new HalftoneElementPrivate;
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

HalftoneElement::~HalftoneElement()
{
    delete this->d;
}

QString HalftoneElement::pattern() const
{
    return this->d->m_pattern;
}

QSize HalftoneElement::patternSize() const
{
    return this->d->m_patternSize;
}

qreal HalftoneElement::lightness() const
{
    return this->d->m_lightness;
}

qreal HalftoneElement::slope() const
{
    return this->d->m_slope;
}

qreal HalftoneElement::intercept() const
{
    return this->d->m_intercept;
}

void HalftoneElement::updatePattern()
{
    if (this->d->m_pattern.isEmpty()) {
        this->d->m_mutex.lock();
        this->d->m_patternImage = QImage();
        this->d->m_mutex.unlock();

        return;
    }

    QImage image(this->d->m_pattern);

    if (image.isNull()) {
        this->d->m_mutex.lock();
        this->d->m_patternImage = QImage();
        this->d->m_mutex.unlock();

        return;
    }

    QSize patternSize = this->d->m_patternSize.isEmpty()?
                            image.size(): this->d->m_patternSize;
    QImage pattern(patternSize, QImage::Format_Indexed8);

    for (int i = 0; i < 256; i++)
        pattern.setColor(i, qRgb(i, i, i));

    image = image.scaled(patternSize).convertToFormat(QImage::Format_RGB32);

    for (int y = 0; y < patternSize.height(); y++) {
        auto srcLine = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        auto dstLine = pattern.scanLine(y);

        for (int x = 0; x < patternSize.width(); x++)
            dstLine[x] = quint8(qGray(srcLine[x]));
    }

    this->d->m_mutex.lock();
    this->d->m_patternImage = pattern;
    this->d->m_mutex.unlock();
}

QString HalftoneElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Halftone/share/qml/main.qml");
}

void HalftoneElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Halftone", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    QStringList picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    context->setContextProperty("picturesPath", picturesPath[0]);
}

void HalftoneElement::setPattern(const QString &pattern)
{
    if (this->d->m_pattern == pattern)
        return;

    this->d->m_pattern = pattern;
    emit this->patternChanged(pattern);
}

void HalftoneElement::setPatternSize(const QSize &patternSize)
{
    if (this->d->m_patternSize == patternSize)
        return;

    this->d->m_patternSize = patternSize;
    emit this->patternSizeChanged(patternSize);
}

void HalftoneElement::setLightness(qreal lightness)
{
    if (qFuzzyCompare(this->d->m_lightness, lightness))
        return;

    this->d->m_lightness = lightness;
    emit this->lightnessChanged(lightness);
}

void HalftoneElement::setSlope(qreal slope)
{
    if (qFuzzyCompare(this->d->m_slope, slope))
        return;

    this->d->m_slope = slope;
    emit this->slopeChanged(slope);
}

void HalftoneElement::setIntercept(qreal intercept)
{
    if (qFuzzyCompare(this->d->m_intercept, intercept))
        return;

    this->d->m_intercept = intercept;
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

    this->d->m_mutex.lock();

    if (this->d->m_patternImage.isNull()) {
        this->d->m_mutex.unlock();
        akSend(packet)
    }

    QImage patternImage = this->d->m_patternImage.copy();
    this->d->m_mutex.unlock();

    // filter image
    for (int y = 0; y < src.height(); y++) {
        const QRgb *iLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < src.width(); x++) {
            int col = x % patternImage.width();
            int row = y % patternImage.height();

            int gray = qGray(iLine[x]);
            auto pattern = reinterpret_cast<const quint8 *>(patternImage.constScanLine(row));
            int threshold = pattern[col];
            threshold = int(this->d->m_slope
                            * threshold
                            + this->d->m_intercept);
            threshold = qBound(0, threshold, 255);

            if (gray > threshold)
                oLine[x] = iLine[x];
            else {
                QColor color(iLine[x]);

                color.setHsl(color.hue(),
                             color.saturation(),
                             int(this->d->m_lightness * color.lightness()),
                             color.alpha());

                oLine[x] = color.rgba();
            }
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

#include "moc_halftoneelement.cpp"

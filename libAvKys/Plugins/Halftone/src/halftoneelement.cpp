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
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "halftoneelement.h"

class HalftoneElementPrivate
{
    public:
        QString m_pattern {":/Halftone/share/patterns/ditherCluster8Matrix.bmp"};
        QSize m_patternSize;
        int m_lightning {32};
        qreal m_slope {1.0};
        qreal m_interception {0.0};
        QMutex m_mutex;
        QSize m_frameSize;
        QImage m_patternImage;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        void updatePattern();
};

HalftoneElement::HalftoneElement(): AkElement()
{
    this->d = new HalftoneElementPrivate;
    this->d->updatePattern();

    QObject::connect(this,
                     &HalftoneElement::patternChanged,
                     [this] () {
                         this->d->updatePattern();
                     });
    QObject::connect(this,
                     &HalftoneElement::patternSizeChanged,
                     [this] () {
                         this->d->updatePattern();
                     });
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

int HalftoneElement::lightning() const
{
    return this->d->m_lightning;
}

qreal HalftoneElement::slope() const
{
    return this->d->m_slope;
}

qreal HalftoneElement::interception() const
{
    return this->d->m_interception;
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

AkPacket HalftoneElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    this->d->m_mutex.lock();

    if (this->d->m_patternImage.isNull()) {
        this->d->m_mutex.unlock();

        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    auto lightning = this->d->m_lightning;
    auto slope = this->d->m_slope;
    auto interception = this->d->m_interception;

    // filter image
    for (int y = 0; y < src.caps().height(); y++) {
        auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));
        int row = y % this->d->m_patternImage.height();
        auto pattern = reinterpret_cast<const quint8 *>(this->d->m_patternImage.constScanLine(row));

        for (int x = 0; x < src.caps().width(); x++) {
            int col = x % this->d->m_patternImage.width();
            auto &pixel = iLine[x];

            int gray = qGray(pixel);
            int threshold = pattern[col];
            threshold = int(slope * threshold + interception);
            threshold = qBound(0, threshold, 255);

            if (gray > threshold) {
                oLine[x] = pixel;
            } else {
                int r = qBound(0, qRed(pixel) + lightning, 255);
                int g = qBound(0, qGreen(pixel) + lightning, 255);
                int b = qBound(0, qBlue(pixel) + lightning, 255);
                oLine[x] = qRgba(r, g, b, qAlpha(pixel));
            }
        }
    }

    this->d->m_mutex.unlock();

    if (dst)
        emit this->oStream(dst);

    return dst;
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

void HalftoneElement::setLightning(int lightning)
{
    if (this->d->m_lightning == lightning)
        return;

    this->d->m_lightning = lightning;
    emit this->lightningChanged(lightning);
}

void HalftoneElement::setSlope(qreal slope)
{
    if (qFuzzyCompare(this->d->m_slope, slope))
        return;

    this->d->m_slope = slope;
    emit this->slopeChanged(slope);
}

void HalftoneElement::setInterception(qreal interception)
{
    if (qFuzzyCompare(this->d->m_interception, interception))
        return;

    this->d->m_interception = interception;
    emit this->interceptionChanged(interception);
}

void HalftoneElement::resetPattern()
{
    this->setPattern(":/Halftone/share/patterns/ditherCluster8Matrix.bmp");
}

void HalftoneElement::resetPatternSize()
{
    this->setPatternSize(QSize());
}

void HalftoneElement::resetLightning()
{
    this->setLightning(32);
}

void HalftoneElement::resetSlope()
{
    this->setSlope(1.0);
}

void HalftoneElement::resetInterception()
{
    this->setInterception(0.0);
}

void HalftoneElementPrivate::updatePattern()
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

    auto pattern = image.convertToFormat(QImage::Format_Grayscale8);

    if (!this->m_patternSize.isEmpty() && this->m_patternSize != image.size())
        pattern = image.scaled(this->m_patternSize);

    this->m_mutex.lock();
    this->m_patternImage = pattern;
    this->m_mutex.unlock();
}

#include "moc_halftoneelement.cpp"

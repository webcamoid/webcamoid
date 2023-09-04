/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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
#include <QPainter>
#include <QQmlContext>
#include <QReadWriteLock>
#include <QStandardPaths>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "colorkeyelement.h"

class ColorKeyElementPrivate
{
    public:
        QRgb m_colorKey {qRgb(0, 0, 0)};
        int m_colorDiff {32};
        int m_smoothness {0};
        bool m_normalize {false};
        ColorKeyElement::BackgroundType m_backgroundType {ColorKeyElement::BackgroundTypeNoBackground};
        QRgb m_backgroundColor {qRgba(0, 0, 0, 0)};
        QString m_background;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        AkElementPtr m_normalizeFilter {akPluginManager->create<AkElement>("VideoFilter/Normalize")};
        QImage m_backgroundImage;
        QReadWriteLock m_mutex;
};

ColorKeyElement::ColorKeyElement():
    AkElement()
{
    this->d = new ColorKeyElementPrivate;
}

ColorKeyElement::~ColorKeyElement()
{
    delete this->d;
}

QRgb ColorKeyElement::colorKey() const
{
    return this->d->m_colorKey;
}

int ColorKeyElement::colorDiff() const
{
    return this->d->m_colorDiff;
}

int ColorKeyElement::smoothness() const
{
    return this->d->m_smoothness;
}

bool ColorKeyElement::normalize() const
{
    return this->d->m_normalize;
}

ColorKeyElement::BackgroundType ColorKeyElement::backgroundType() const
{
    return this->d->m_backgroundType;
}

QRgb ColorKeyElement::backgroundColor() const
{
    return this->d->m_backgroundColor;
}

QString ColorKeyElement::background() const
{
    return this->d->m_background;
}

QString ColorKeyElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ColorKey/share/qml/main.qml");
}

void ColorKeyElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ColorKey", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    QStringList picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    context->setContextProperty("picturesPath", picturesPath[0]);
}

AkPacket ColorKeyElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket *srcPtr = nullptr;
    AkVideoPacket normalized;

    if (this->d->m_normalize) {
        normalized = this->d->m_normalizeFilter->iStream(src);
        srcPtr = &normalized;
    } else {
        srcPtr = &src;
    }

    auto colorKey = this->d->m_colorKey;
    auto ckR = qRed(colorKey);
    auto ckG = qGreen(colorKey);
    auto ckB = qBlue(colorKey);

    auto colorDiff = this->d->m_colorDiff;
    auto colorDiff2 = colorDiff * colorDiff;

    auto alphaContrast = 128 - this->d->m_smoothness;

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    for (int y = 0; y < srcPtr->caps().height(); ++y) {
        auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto nLine = reinterpret_cast<const QRgb *>(srcPtr->constLine(0, y));
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < srcPtr->caps().width(); ++x) {
            auto &npixel = nLine[x];
            auto diffR = ckR - qRed(npixel);
            auto diffG = ckG - qGreen(npixel);
            auto diffB = ckB - qBlue(npixel);

            auto diffR2 = diffR * diffR;
            auto diffG2 = diffG * diffG;
            auto diffB2 = diffB * diffB;

            auto k = diffR2 + diffG2 + diffB2;
            int alpha = 255;

            if (colorDiff > 0)
                alpha = qMin(255 * k / colorDiff2, 255);

            if (alphaContrast > 0) {
                if (alpha < 127)
                    alpha = qMax(alpha - alphaContrast, 0);
                else
                    alpha = qMin(alpha + alphaContrast, 255);
            }

            auto &pixel = iLine[x];
            oLine[x] = qRgba(qRed(pixel),
                             qGreen(pixel),
                             qBlue(pixel),
                             (alpha * qAlpha(pixel)) >> 8);
        }
    }

    if (this->d->m_backgroundType == BackgroundTypeImage
        && !this->d->m_background.isEmpty()) {
        QImage image(dst.caps().width(),
                     dst.caps().height(),
                     QImage::Format_ARGB32);
        auto lineSize = qMin<size_t>(dst.lineSize(0), image.bytesPerLine());

        for (int y = 0; y < dst.caps().height(); y++) {
            auto srcLine = dst.constLine(0, y);
            auto dstLine = image.scanLine(y);
            memcpy(dstLine, srcLine, lineSize);
        }

        this->d->m_mutex.lockForRead();

        if (!this->d->m_backgroundImage.isNull()) {
            auto scaled =
                this->d->m_backgroundImage.scaled(src.caps().width(),
                                                  src.caps().height(),
                                                  Qt::KeepAspectRatioByExpanding);

            QPainter painter;
            painter.begin(&scaled);
            painter.drawImage(0, 0, image);
            painter.end();

            lineSize = qMin<size_t>(scaled.bytesPerLine(), dst.lineSize(0));

            for (int y = 0; y < dst.caps().height(); y++) {
                auto srcLine = scaled.constScanLine(y);
                auto dstLine = dst.line(0, y);
                memcpy(dstLine, srcLine, lineSize);
            }
        }

        this->d->m_mutex.unlock();
    } else if (this->d->m_backgroundType == BackgroundTypeColor
             && qAlpha(this->d->m_backgroundColor) != 0) {
        QImage background(dst.caps().width(),
                          dst.caps().height(),
                          QImage::Format_ARGB32);
        background.fill(this->d->m_backgroundColor);

        QImage image(dst.caps().width(),
                     dst.caps().height(),
                     QImage::Format_ARGB32);
        auto lineSize = qMin<size_t>(dst.lineSize(0), image.bytesPerLine());

        for (int y = 0; y < dst.caps().height(); y++) {
            auto srcLine = dst.constLine(0, y);
            auto dstLine = image.scanLine(y);
            memcpy(dstLine, srcLine, lineSize);
        }

        QPainter painter;
        painter.begin(&background);
        painter.drawImage(0, 0, image);
        painter.end();

        lineSize = qMin<size_t>(background.bytesPerLine(), dst.lineSize(0));

        for (int y = 0; y < dst.caps().height(); y++) {
            auto srcLine = background.constScanLine(y);
            auto dstLine = dst.line(0, y);
            memcpy(dstLine, srcLine, lineSize);
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void ColorKeyElement::setColorKey(QRgb color)
{
    if (this->d->m_colorKey == color)
        return;

    this->d->m_colorKey = color;
    emit this->colorKeyChanged(color);
}

void ColorKeyElement::setColorDiff(int colorDiff)
{
    if (this->d->m_colorDiff == colorDiff)
        return;

    this->d->m_colorDiff = colorDiff;
    emit this->colorDiffChanged(colorDiff);
}

void ColorKeyElement::setSmoothness(int smoothness)
{
    if (this->d->m_smoothness == smoothness)
        return;

    this->d->m_smoothness = smoothness;
    emit this->smoothnessChanged(smoothness);
}

void ColorKeyElement::setNormalize(bool normalize)
{
    if (this->d->m_normalize == normalize)
        return;

    this->d->m_normalize = normalize;
    emit this->normalizeChanged(normalize);
}

void ColorKeyElement::setBackgroundType(BackgroundType backgroundType)
{
    if (this->d->m_backgroundType == backgroundType)
        return;

    this->d->m_backgroundType = backgroundType;
    emit this->backgroundTypeChanged(backgroundType);
}

void ColorKeyElement::setBackgroundColor(QRgb backgroundColor)
{
    if (this->d->m_backgroundColor == backgroundColor)
        return;

    this->d->m_backgroundColor = backgroundColor;
    emit this->backgroundColorChanged(backgroundColor);
}

void ColorKeyElement::setBackground(const QString &background)
{
    if (this->d->m_background == background)
        return;

    this->d->m_background = background;
    this->d->m_mutex.lockForWrite();

    if (background.isEmpty())
        this->d->m_backgroundImage = {};
    else
        this->d->m_backgroundImage =
            QImage(background).convertedTo(QImage::Format_ARGB32);

    this->d->m_mutex.unlock();
    emit this->backgroundChanged(background);
}

void ColorKeyElement::resetColorKey()
{
    this->setColorKey(qRgb(0, 0, 0));
}

void ColorKeyElement::resetColorDiff()
{
    this->setColorDiff(32);
}

void ColorKeyElement::resetSmoothness()
{
    this->setSmoothness(0);
}

void ColorKeyElement::resetNormalize()
{
    this->setNormalize(false);
}

void ColorKeyElement::resetBackgroundType()
{
    this->setBackgroundType(BackgroundTypeNoBackground);
}

void ColorKeyElement::resetBackgroundColor()
{
    this->setBackgroundColor(qRgba(0, 0, 0, 0));
}

void ColorKeyElement::resetBackground()
{
    this->setBackground({});
}

QDataStream &operator >>(QDataStream &istream, ColorKeyElement::BackgroundType &mode)
{
    int modeInt;
    istream >> modeInt;
    mode = static_cast<ColorKeyElement::BackgroundType>(modeInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, ColorKeyElement::BackgroundType mode)
{
    ostream << static_cast<int>(mode);

    return ostream;
}

#include "moc_colorkeyelement.cpp"

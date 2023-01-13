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

#include <QQmlContext>
#include <QSize>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "lifeelement.h"

class LifeElementPrivate
{
    public:
        QSize m_frameSize;
        AkVideoPacket m_prevFrame;
        AkVideoPacket m_lifeBuffer;
        QRgb m_lifeColor {qRgb(255, 255, 255)};
        int m_threshold {15};
        int m_lumaThreshold {15};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        inline AkVideoPacket imageDiff(const AkVideoPacket &img1,
                                       const AkVideoPacket &img2,
                                       int threshold,
                                       int lumaThreshold) const;
        inline void updateLife();
};

LifeElement::LifeElement(): AkElement()
{
    this->d = new LifeElementPrivate;
}

LifeElement::~LifeElement()
{
    delete this->d;
}

QRgb LifeElement::lifeColor() const
{
    return this->d->m_lifeColor;
}

int LifeElement::threshold() const
{
    return this->d->m_threshold;
}

int LifeElement::lumaThreshold() const
{
    return this->d->m_lumaThreshold;
}

QString LifeElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Life/share/qml/main.qml");
}

void LifeElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Life", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket LifeElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    auto dst = src;
    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_frameSize) {
        this->d->m_lifeBuffer = AkVideoPacket();
        this->d->m_prevFrame = AkVideoPacket();
        this->d->m_frameSize = frameSize;
    }

    if (!this->d->m_prevFrame) {
        this->d->m_lifeBuffer = AkVideoPacket({AkVideoCaps::Format_gray8,
                                               src.caps().width(),
                                               src.caps().height(),
                                               {}}, true);
    }
    else {
        // Compute the difference between previous and current frame,
        // and save it to the buffer.
        auto diff = this->d->imageDiff(this->d->m_prevFrame,
                                       src,
                                       this->d->m_threshold,
                                       this->d->m_lumaThreshold);

        for (int y = 0; y < this->d->m_lifeBuffer.caps().height(); y++) {
            auto diffLine = diff.constLine(0, y);
            auto lifeBufferLine = this->d->m_lifeBuffer.line(0, y);

            for (int x = 0; x < this->d->m_lifeBuffer.caps().width(); x++)
                lifeBufferLine[x] |= diffLine[x];
        }

        this->d->updateLife();

        auto lifeColor = qRgba(qRed(this->d->m_lifeColor),
                               qGreen(this->d->m_lifeColor),
                               qBlue(this->d->m_lifeColor),
                               255);

        for (int y = 0; y < src.caps().height(); y++) {
            auto iLine = this->d->m_lifeBuffer.constLine(0, y);
            auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));

            for (int x = 0; x < src.caps().width(); x++) {
                if (iLine[x])
                    oLine[x] = lifeColor;
            }
        }
    }

    this->d->m_prevFrame = src;

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void LifeElement::setLifeColor(QRgb lifeColor)
{
    if (this->d->m_lifeColor == lifeColor)
        return;

    this->d->m_lifeColor = lifeColor;
    emit this->lifeColorChanged(lifeColor);
}

void LifeElement::setThreshold(int threshold)
{
    if (this->d->m_threshold == threshold)
        return;

    this->d->m_threshold = threshold;
    emit this->thresholdChanged(threshold);
}

void LifeElement::setLumaThreshold(int lumaThreshold)
{
    if (this->d->m_lumaThreshold == lumaThreshold)
        return;

    this->d->m_lumaThreshold = lumaThreshold;
    emit this->lumaThresholdChanged(lumaThreshold);
}

void LifeElement::resetLifeColor()
{
    this->setLifeColor(qRgb(255, 255, 255));
}

void LifeElement::resetThreshold()
{
    this->setThreshold(15);
}

void LifeElement::resetLumaThreshold()
{
    this->setLumaThreshold(15);
}

AkVideoPacket LifeElementPrivate::imageDiff(const AkVideoPacket &img1,
                                            const AkVideoPacket &img2,
                                            int threshold,
                                            int lumaThreshold) const
{
    int width = qMin(img1.caps().width(), img2.caps().width());
    int height = qMin(img1.caps().height(), img2.caps().height());
    AkVideoPacket diff({AkVideoCaps::Format_gray8, width, height, {}});

    for (int y = 0; y < height; y++) {
        auto line1 = reinterpret_cast<const QRgb *>(img1.constLine(0, y));
        auto line2 = reinterpret_cast<const QRgb *>(img2.constLine(0, y));
        auto lineDiff = diff.line(0, y);

        for (int x = 0; x < width; x++) {
            int r1 = qRed(line1[x]);
            int g1 = qGreen(line1[x]);
            int b1 = qBlue(line1[x]);

            int r2 = qRed(line2[x]);
            int g2 = qGreen(line2[x]);
            int b2 = qBlue(line2[x]);

            int dr = r1 - r2;
            int dg = g1 - g2;
            int db = b1 - b2;

            int colorDiff = dr * dr + dg * dg + db * db;

            lineDiff[x] = qSqrt(colorDiff / 3.0) >= threshold
                          && qGray(line2[x]) >= lumaThreshold? 1: 0;
        }
    }

    return diff;
}

void LifeElementPrivate::updateLife()
{
    AkVideoPacket lifeBuffer(this->m_lifeBuffer.caps(), true);

    for (int y = 1; y < lifeBuffer.caps().height() - 1; y++) {
        auto iLine = this->m_lifeBuffer.constLine(0, y);
        auto oLine = lifeBuffer.line(0, y);

        for (int x = 1; x < lifeBuffer.caps().width() - 1; x++) {
            int count = 0;

            for (int j = -1; j < 2; j++) {
                auto line = this->m_lifeBuffer.constLine(0, y + j);

                for (int i = -1; i < 2; i++)
                    count += line[x + i];
            }

            auto &ipixel = iLine[x];
            count -= ipixel;

            if ((ipixel && count == 2) || count == 3)
                oLine[x] = 1;
        }
    }

    this->m_lifeBuffer = lifeBuffer;
}

#include "moc_lifeelement.cpp"

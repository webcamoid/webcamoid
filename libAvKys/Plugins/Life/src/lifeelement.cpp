/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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
#include <QtMath>
#include <QPainter>
#include <akutils.h>
#include <akpacket.h>

#include "lifeelement.h"

class LifeElementPrivate
{
    public:
        QRgb m_lifeColor;
        int m_threshold;
        int m_lumaThreshold;
        QSize m_frameSize;
        QImage m_prevFrame;
        QImage m_lifeBuffer;

        LifeElementPrivate():
            m_lifeColor(qRgb(255, 255, 255)),
            m_threshold(15),
            m_lumaThreshold(15)
        {
        }
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

QImage LifeElement::imageDiff(const QImage &img1,
                              const QImage &img2,
                              int threshold,
                              int lumaThreshold)
{
    int width = qMin(img1.width(), img2.width());
    int height = qMin(img1.height(), img2.height());
    QImage diff(width, height, QImage::Format_Indexed8);

    for (int y = 0; y < height; y++) {
        const QRgb *line1 = reinterpret_cast<const QRgb *>(img1.constScanLine(y));
        const QRgb *line2 = reinterpret_cast<const QRgb *>(img2.constScanLine(y));
        quint8 *lineDiff = diff.scanLine(y);

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

            lineDiff[x] = sqrt(colorDiff / 3) >= threshold
                          && qGray(line2[x]) >= lumaThreshold? 1: 0;
        }
    }

    return diff;
}

void LifeElement::updateLife()
{
    QImage lifeBuffer(this->d->m_lifeBuffer.size(),
                      this->d->m_lifeBuffer.format());
    lifeBuffer.fill(0);

    for (int y = 1; y < lifeBuffer.height() - 1; y++) {
        auto iLine = this->d->m_lifeBuffer.constScanLine(y);
        auto oLine = lifeBuffer.scanLine(y);

        for (int x = 1; x < lifeBuffer.width() - 1; x++) {
            int count = 0;

            for (int j = -1; j < 2; j++) {
                auto line = this->d->m_lifeBuffer.constScanLine(y + j);

                for (int i = -1; i < 2; i++)
                    count += line[x + i];
            }

            count -= iLine[x];

            if ((iLine[x] && count == 2) || count == 3)
                oLine[x] = 1;
        }
    }

    memcpy(this->d->m_lifeBuffer.bits(),
           lifeBuffer.constBits(),
           size_t(lifeBuffer.byteCount()));
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

AkPacket LifeElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame = src;

    if (src.size() != this->d->m_frameSize) {
        this->d->m_lifeBuffer = QImage();
        this->d->m_prevFrame = QImage();
        this->d->m_frameSize = src.size();
    }

    if (this->d->m_prevFrame.isNull()) {
        this->d->m_lifeBuffer = QImage(src.size(), QImage::Format_Indexed8);
        this->d->m_lifeBuffer.setColor(0, 0);
        this->d->m_lifeBuffer.setColor(1, this->d->m_lifeColor);
        this->d->m_lifeBuffer.fill(0);
    }
    else {
        // Compute the difference between previous and current frame,
        // and save it to the buffer.
        QImage diff =
                this->imageDiff(this->d->m_prevFrame,
                                src,
                                this->d->m_threshold,
                                this->d->m_lumaThreshold);

        this->d->m_lifeBuffer.setColor(1, this->d->m_lifeColor);

        for (int y = 0; y < this->d->m_lifeBuffer.height(); y++) {
            auto diffLine = diff.constScanLine(y);
            auto lifeBufferLine = this->d->m_lifeBuffer.scanLine(y);

            for (int x = 0; x < this->d->m_lifeBuffer.width(); x++)
                lifeBufferLine[x] |= diffLine[x];
        }

        this->updateLife();

        QPainter painter;
        painter.begin(&oFrame);
        painter.drawImage(0, 0, this->d->m_lifeBuffer);
        painter.end();
    }

    this->d->m_prevFrame = src.copy();

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

#include "moc_lifeelement.cpp"

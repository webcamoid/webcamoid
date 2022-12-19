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

#include <QDataStream>
#include <QQmlContext>
#include <QSize>
#include <QtMath>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "hypnoticelement.h"

class HypnoticElementPrivate
{
    public:
        HypnoticElement::OpticMode m_mode {HypnoticElement::OpticModeSpiral1};
        HypnoticElement::OpticMode m_currentMode {HypnoticElement::OpticModeSpiral1};
        int m_speedInc {0};
        int m_threshold {127};
        QSize m_frameSize;
        QRgb m_palette[256];
        AkVideoPacket m_opticalMap;
        quint8 m_speed {16};
        quint8 m_phase {0};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        inline void createPalette();
        inline AkVideoPacket createOpticalMap(const QSize &size,
                                              HypnoticElement::OpticMode mode) const;
        inline AkVideoPacket imageThreshold(const AkVideoPacket &src,
                                            int threshold) const;
};

HypnoticElement::HypnoticElement(): AkElement()
{
    this->d = new HypnoticElementPrivate;
    this->d->createPalette();
}

HypnoticElement::~HypnoticElement()
{
    delete this->d;
}

HypnoticElement::OpticMode HypnoticElement::mode() const
{
    return this->d->m_mode;
}

int HypnoticElement::speedInc() const
{
    return this->d->m_speedInc;
}

int HypnoticElement::threshold() const
{
    return this->d->m_threshold;
}

QString HypnoticElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Hypnotic/share/qml/main.qml");
}

void HypnoticElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Hypnotic", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket HypnoticElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_frameSize
        || this->d->m_currentMode != this->d->m_mode) {
        this->d->m_speed = 16;
        this->d->m_phase = 0;
        this->d->m_opticalMap = this->d->createOpticalMap(frameSize, this->d->m_mode);
        this->d->m_frameSize = frameSize;
        this->d->m_currentMode = this->d->m_mode;
    }

    this->d->m_speed += this->d->m_speedInc;
    this->d->m_phase -= this->d->m_speed;
    auto diff = this->d->imageThreshold(src, this->d->m_threshold);

    for (int i = 0, y = 0; y < src.caps().height(); y++) {
        auto optLine = this->d->m_opticalMap.constLine(0, y);
        auto diffLine = diff.constLine(0, y);
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); i++, x++)
            dstLine[x] = this->d->m_palette[(quint8(int(optLine[x]) + int(this->d->m_phase)) ^ diffLine[x]) & 255];
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void HypnoticElement::setMode(OpticMode mode)
{
    if (this->d->m_mode == mode)
        return;

    this->d->m_mode = mode;
    emit this->modeChanged(mode);
}

void HypnoticElement::setSpeedInc(int speedInc)
{
    if (this->d->m_speedInc == speedInc)
        return;

    this->d->m_speedInc = speedInc;
    emit this->speedIncChanged(speedInc);
}

void HypnoticElement::setThreshold(int threshold)
{
    if (this->d->m_threshold == threshold)
        return;

    this->d->m_threshold = threshold;
    emit this->thresholdChanged(threshold);
}

void HypnoticElement::resetMode()
{
    this->setMode(HypnoticElement::OpticModeSpiral1);
}

void HypnoticElement::resetSpeedInc()
{
    this->setSpeedInc(0);
}

void HypnoticElement::resetThreshold()
{
    this->setThreshold(127);
}

QDataStream &operator >>(QDataStream &istream, HypnoticElement::OpticMode &mode)
{
    int modeInt;
    istream >> modeInt;
    mode = static_cast<HypnoticElement::OpticMode>(modeInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, HypnoticElement::OpticMode mode)
{
    ostream << static_cast<int>(mode);

    return ostream;
}

void HypnoticElementPrivate::createPalette()
{
    for (int i = 0; i < 112; i++) {
        this->m_palette[i] = qRgb(0, 0, 0);
        this->m_palette[i + 128] = qRgb(255, 255, 255);
    }

    for (int i = 0; i < 16; i++) {
        auto color = QRgb(16 * (i + 1) - 1);
        this->m_palette[i + 112] = qRgb(qRed(color), qGreen(color), qBlue(color));
        color = 255 - color;
        this->m_palette[i + 240] = qRgb(qRed(color), qGreen(color), qBlue(color));
    }
}

AkVideoPacket HypnoticElementPrivate::createOpticalMap(const QSize &size,
                                                       HypnoticElement::OpticMode mode) const
{
    AkVideoPacket opticalMap({AkVideoCaps::Format_gray8,
                              size.width(),
                              size.height(),
                              {}});

    for (int y = 0; y < size.height(); y++) {
        auto line = opticalMap.line(0, y);
        auto yy = qreal(2 * y - size.height()) / (2 * size.width());

        for (int x = 0; x < size.width(); x++) {
            auto xx = qreal(2 * x - size.width()) / (2 * size.width());
            qreal r = sqrt(xx * xx + yy * yy);
            qreal at = atan2(xx, yy);

            switch (mode) {
            case HypnoticElement::OpticModeSpiral1:
                line[x] = quint8(256.0 * at / M_PI + 4000.0 * r);
                break;
            case HypnoticElement::OpticModeSpiral2: {
                int j = int(300.0 * r / 32.0);
                qreal rr = 300.0 * r - 32.0 * j;

                j *= 64;
                j += rr > 28.0? qRound(16.0 * (rr - 28.0)): 0;

                line[x] = quint8(4096.0 * at / M_PI + 1600.0 * r - j);

                break;
            }
            case HypnoticElement::OpticModeParabola:
                line[x] = quint8(400.0 * yy / (0.3 * xx * xx + 0.1));
                break;
            case HypnoticElement::OpticModeHorizontalStripe:
                line[x] = quint8(5120 * x / size.width());
                break;
            }
        }
    }

    return opticalMap;
}

AkVideoPacket HypnoticElementPrivate::imageThreshold(const AkVideoPacket &src,
                                                     int threshold) const
{
    auto ocaps = src.caps();
    ocaps.setFormat(AkVideoCaps::Format_gray8);
    AkVideoPacket diff(ocaps);
    diff.copyMetadata(src);

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcBits = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto diffBits = diff.line(0, y);

        for (int x = 0; x < src.caps().width(); x++)
            diffBits[x] = qGray(srcBits[x]) >= threshold? 255: 0;
    }

    return diff;
}

#include "moc_hypnoticelement.cpp"

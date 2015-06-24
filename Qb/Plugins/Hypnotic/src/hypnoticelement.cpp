/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#include "hypnoticelement.h"

HypnoticElement::HypnoticElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetMode();
    this->resetSpeedInc();
    this->resetThreshold();

    this->m_palette = this->createPalette();
}

QObject *HypnoticElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Hypnotic/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Hypnotic", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

QString HypnoticElement::mode() const
{
    return this->m_mode;
}

int HypnoticElement::speedInc() const
{
    return this->m_speedInc;
}

int HypnoticElement::threshold() const
{
    return this->m_threshold;
}

QVector<QRgb> HypnoticElement::createPalette()
{
    QVector<QRgb> palette(256);

    for (int i = 0; i < 112; i++) {
        palette[i] = qRgb(0, 0, 0);
        palette[i + 128] = qRgb(255, 255, 255);
    }

    for (int i = 0; i < 16; i++) {
        QRgb color = 16 * (i + 1) - 1;
        palette[i + 112] = qRgb(qRed(color), qGreen(color), qBlue(color));
        color = 255 - color;
        palette[i + 240] = qRgb(qRed(color), qGreen(color), qBlue(color));
    }

    return palette;
}

OpticalMap HypnoticElement::createOpticalMap(int width, int height)
{
    OpticalMap opticalMap;
    int sci = 640 / width;
    int i = 0;

    opticalMap["spiral1"] = QImage(width, height, QImage::Format_Indexed8);
    opticalMap["spiral2"] = QImage(width, height, QImage::Format_Indexed8);
    opticalMap["parabola"] = QImage(width, height, QImage::Format_Indexed8);
    opticalMap["horizontalStripe"] = QImage(width, height, QImage::Format_Indexed8);

    QMap<QString, quint8 *> opticalMapPtr;

    opticalMapPtr["spiral1"] = (quint8 *) opticalMap["spiral1"].bits();
    opticalMapPtr["spiral2"] = (quint8 *) opticalMap["spiral2"].bits();
    opticalMapPtr["parabola"] = (quint8 *) opticalMap["parabola"].bits();
    opticalMapPtr["horizontalStripe"] = (quint8 *) opticalMap["horizontalStripe"].bits();

    for (int y = 0; y < height; y++) {
        qreal yy = (qreal) (y - height / 2) / width;

        for (int x = 0; x < width; x++) {
            qreal xx = (qreal) x / width - 0.5;

            qreal r = sqrt(xx * xx + yy * yy);
            qreal at = atan2(xx, yy);

            opticalMapPtr["spiral1"][i] = ((unsigned int)
                ((at / M_PI * 256) + (r * 4000))) & 255;

            int j = r * 300 / 32;
            qreal rr = r * 300 - j * 32;

            j *= 64;
            j += (rr > 28)? (rr - 28) * 16: 0;

            opticalMapPtr["spiral2"][i] = ((unsigned int)
                ((at / M_PI * 4096) + (r * 1600) - j)) & 255;

            opticalMapPtr["parabola"][i] = ((unsigned int) (yy / (xx * xx * 0.3 + 0.1) * 400)) & 255;
            opticalMapPtr["horizontalStripe"][i] = x * 8 * sci;
            i++;
        }
    }

    return opticalMap;
}

QImage HypnoticElement::imageYOver(const QImage &src, int threshold)
{
    int yThreshold = 7 * threshold;
    QRgb *srcBits = (QRgb *) src.bits();
    QImage diff(src.size(), QImage::Format_Indexed8);
    quint8 *diffBits = diff.bits();
    int videoArea = src.width() * src.height();

    for (int i = 0; i < videoArea; i++) {
        QRgb pixel = srcBits[i];
        int r = (pixel & 0xff0000) >> (16 - 1);
        int g = (pixel & 0xff00) >> (8 - 2);
        int b = pixel & 0xff;
        int v = yThreshold - (r + g + b);

        diffBits[i] = v >> 24;
    }

    return diff;
}

void HypnoticElement::setMode(const QString &mode)
{
    if (mode != this->m_mode) {
        this->m_mode = mode;
        emit this->modeChanged();
    }
}

void HypnoticElement::setSpeedInc(int speedInc)
{
    if (speedInc != this->m_speedInc) {
        this->m_speedInc = speedInc;
        emit this->speedIncChanged();
    }
}

void HypnoticElement::setThreshold(int threshold)
{
    if (threshold != this->m_threshold) {
        this->m_threshold = threshold;
        emit this->thresholdChanged();
    }
}

void HypnoticElement::resetMode()
{
    this->setMode("spiral1");
}

void HypnoticElement::resetSpeedInc()
{
    this->setSpeedInc(0);
}

void HypnoticElement::resetThreshold()
{
    this->setThreshold(50);
}

QbPacket HypnoticElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());
    QRgb *destBits = (QRgb *) oFrame.bits();

    static QbCaps caps;

    if (packet.caps() != caps) {
        this->m_speed = 16;
        this->m_phase = 0;

        this->m_opticalMap = this->createOpticalMap(src.width(),
                                                    src.height());

        caps = packet.caps();
    }

    quint8 *opticalMap;

    if (this->m_opticalMap.contains(this->m_mode))
        opticalMap = (quint8 *) this->m_opticalMap[this->m_mode].bits();
    else
        opticalMap = (quint8 *) this->m_opticalMap["spiral1"].bits();

    this->m_speed += this->m_speedInc;
    this->m_phase -= this->m_speed;

    QImage diff = this->imageYOver(src, this->m_threshold);
    quint8 *diffBits = (quint8 *) diff.bits();

    for (int i = 0, y = 0; y < src.height(); y++)
        for (int x = 0; x < src.width(); i++, x++)
            destBits[i] = this->m_palette[(((char) (opticalMap[i] + this->m_phase)) ^ diffBits[i]) & 255];

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}

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

#include "dizzyelement.h"

DizzyElement::DizzyElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetPhaseIncrement();
    this->resetZoomRate();
}

QObject *DizzyElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Dizzy/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Dizzy", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

float DizzyElement::phaseIncrement() const
{
    return this->m_phaseIncrement;
}

float DizzyElement::zoomRate() const
{
    return this->m_zoomRate;
}

void DizzyElement::setParams(int *dx, int *dy,
                             int *sx, int *sy,
                             int width, int height,
                             float phase, float zoomRate)
{
    float dizz = 10 * sin(phase)
                 + 5 * sin(1.9 * phase + 5);

    int x = width >> 1;
    int y = height >> 1;
    float t = zoomRate * (x * x + y * y);

    float vx;
    float vy;

    if (width > height) {
        if (dizz >= 0) {
            if(dizz > x)
                dizz = x;

            vx = (x * (x - dizz) + y * y) / t;
        }
        else {
            if(dizz < -x)
                dizz = -x;

            vx = (x * (x + dizz) + y * y) / t;
        }

        vy = dizz * y / t;
    }
    else {
        if (dizz >= 0) {
            if (dizz > y)
                dizz = y;

            vx = (x * x + y * (y - dizz)) / t;
        }
        else {
            if (dizz < -y)
                dizz = -y;

            vx = (x * x + y * (y + dizz)) / t;
        }

        vy = dizz * x / t;
    }

    *dx = 65536 * vx;
    *dy = 65536 * vy;
    *sx = 65536 * (-vx * x + vy * y + x + 2 * cos(5 * phase));
    *sy = 65536 * (-vx * y - vy * x + y + 2 * sin(6 * phase));
}

void DizzyElement::setPhaseIncrement(float phaseIncrement)
{
    if (phaseIncrement != this->m_phaseIncrement) {
        this->m_phaseIncrement = phaseIncrement;
        emit this->phaseIncrementChanged();
    }
}

void DizzyElement::setZoomRate(float zoomRate)
{
    if (zoomRate != this->m_zoomRate) {
        this->m_zoomRate = zoomRate;
        emit this->zoomRateChanged();
    }
}

void DizzyElement::resetPhaseIncrement()
{
    this->setPhaseIncrement(0.02);
}

void DizzyElement::resetZoomRate()
{
    this->setZoomRate(1.01);
}

QbPacket DizzyElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    if (packet.caps() != this->m_caps) {
        this->m_prevFrame = QImage();
        this->m_phase = 0;

        this->m_caps = packet.caps();
    }

    if (this->m_prevFrame.isNull())
        oFrame = src;
    else {
        int dx;
        int dy;
        int sx;
        int sy;

        this->setParams(&dx, &dy, &sx, &sy,
                        src.width(), src.height(),
                        this->m_phase, this->m_zoomRate);

        this->m_phase += this->m_phaseIncrement;

        if (this->m_phase > 5700000)
            this->m_phase = 0;

        QRgb *prevFrameBits = (QRgb *) this->m_prevFrame.bits();

        for (int y = 0, i = 0; y < src.height(); y++) {
            int ox = sx;
            int oy = sy;

            for (int x = 0; x < src.width(); x++, i++) {
                int j = (oy >> 16) * src.width() + (ox >> 16);

                if (j < 0)
                    j = 0;

                if (j >= videoArea)
                    j = videoArea;

                QRgb v = prevFrameBits[j] & 0xfcfcff;
                v = 3 * v + (srcBits[i] & 0xfcfcff);
                destBits[i] = (v >> 2) | 0xff000000;
                ox += dx;
                oy += dy;
            }

            sx -= dy;
            sy += dx;
        }
    }

    this->m_prevFrame = oFrame.copy();

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}

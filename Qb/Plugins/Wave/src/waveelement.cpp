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

#include "waveelement.h"

WaveElement::WaveElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetAmplitude();
    this->resetPhases();
    this->resetBackground();
}

QObject *WaveElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Wave/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Wave", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

qreal WaveElement::amplitude() const
{
    return this->m_amplitude;
}

qreal WaveElement::phases() const
{
    return this->m_phases;
}

QRgb WaveElement::background() const
{
    return this->m_background;
}

void WaveElement::setAmplitude(qreal amplitude)
{
    if (amplitude != this->m_amplitude) {
        this->m_amplitude = amplitude;
        emit this->amplitudeChanged();
    }
}

void WaveElement::setPhases(qreal phases)
{
    if (phases != this->m_phases) {
        this->m_phases = phases;
        emit this->phasesChanged();
    }
}

void WaveElement::setBackground(QRgb background)
{
    if (background != this->m_background) {
        this->m_background = background;
        emit this->backgroundChanged();
    }
}

void WaveElement::resetAmplitude()
{
    this->setAmplitude(16);
}

void WaveElement::resetPhases()
{
    this->setPhases(8);
}

void WaveElement::resetBackground()
{
    this->setBackground(0);
}

QbPacket WaveElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.width(),
                  src.height() + 2 * qAbs(this->m_amplitude),
                  src.format());

    qreal sineMap[oFrame.width()];

    for (int x = 0; x < oFrame.width(); x++)
        sineMap[x] = qAbs(this->m_amplitude)
                      + this->m_amplitude
                      * sin((this->m_phases * 2.0 * M_PI * x) / oFrame.width());

    for (int y = 0; y < oFrame.height(); y++) {
        QRgb *dest = (QRgb *) oFrame.scanLine(y);

        for (int x = 0; x < oFrame.width(); x++)
            *dest++ = this->interpolateBackground(src,
                                                  x,
                                                  y - sineMap[x],
                                                  this->m_background);
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}

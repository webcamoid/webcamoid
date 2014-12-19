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

#include "cartoonelement.h"

CartoonElement::CartoonElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetThreshold();
    this->resetDiffSpace();
}

QObject *CartoonElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Cartoon/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Cartoon", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

int CartoonElement::threshold() const
{
    return this->m_threshold;
}

int CartoonElement::diffSpace() const
{
    return this->m_diffSpace;
}

int CartoonElement::getMaxContrast(const QRgb *src, int x, int y)
{
    long max = 0;

    int xMin = qBound(0, x - this->m_diffSpace, this->m_width);
    int xMax = qBound(0, x + this->m_diffSpace, this->m_width);
    int yMin = qBound(0, y - this->m_diffSpace, this->m_height);
    int yMax = qBound(0, y + this->m_diffSpace, this->m_height);

    // Assumes PrePixelModify has been run.
    QRgb c1 = this->pixelate(src, xMin, y);
    QRgb c2 = this->pixelate(src, xMax, y);
    long error = this->gmError(c1, c2);

    if (error > max)
        max = error;

    c1 = this->pixelate(src, x, yMin);
    c2 = this->pixelate(src, x, yMax);
    error = this->gmError(c1, c2);

    if (error > max)
        max = error;

    c1 = this->pixelate(src, xMin, yMin);
    c2 = this->pixelate(src, xMax, yMax);
    error = this->gmError(c1,  c2);

    if (error > max)
        max = error;

    c1 = this->pixelate(src, xMax, yMin);
    c2 = this->pixelate(src, xMin, yMax);
    error = this->gmError(c1, c2);

    if (error > max)
        max = error;

    return sqrt(max);
}

void CartoonElement::setThreshold(int threshold)
{
    if (threshold != this->m_threshold) {
        this->m_threshold = threshold;
        emit this->thresholdChange();
    }
}

void CartoonElement::setDiffSpace(int diffspace)
{
    if (diffspace != this->m_diffSpace) {
        this->m_diffSpace = diffspace;
        emit this->diffSpaceChange();
    }
}

void CartoonElement::resetThreshold()
{
    this->setThreshold(191);
}

void CartoonElement::resetDiffSpace()
{
    this->setDiffSpace(4);
}

QbPacket CartoonElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    if (packet.caps() != this->m_caps) {
        this->m_width = src.width();
        this->m_height = src.height();
        this->m_yprecal.clear();

        for (int x = 0; x < 2 * src.height(); x++)
            this->m_yprecal << src.width() * x;

        this->m_caps = packet.caps();
    }

    // Cartoonify picture, do a form of edge detect
    for (int y = 0; y < src.height(); y++) {
        for (int x = 0; x < src.width(); x++) {
            int t = this->getMaxContrast(srcBits, x, y);
            int offset = x + this->m_yprecal[y];

            if (t > this->m_threshold)
                // Make a border pixel
                destBits[offset] = qRgb(0, 0, 0);
            else
                // Copy original color
                destBits[offset] = this->flattenColor(srcBits[offset]);
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}

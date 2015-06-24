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

#include "scanlineselement.h"

ScanLinesElement::ScanLinesElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetShowSize();
    this->resetHideSize();
    this->resetHideColor();
}

QObject *ScanLinesElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/ScanLines/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("ScanLines", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

int ScanLinesElement::showSize() const
{
    return this->m_showSize;
}

int ScanLinesElement::hideSize() const
{
    return this->m_hideSize;
}

QRgb ScanLinesElement::hideColor() const
{
    return this->m_hideColor;
}

void ScanLinesElement::setShowSize(int showSize)
{
    if (showSize != this->m_showSize) {
        this->m_showSize = showSize;
        emit this->showSizeChanged();
    }
}

void ScanLinesElement::setHideSize(int hideSize)
{
    if (hideSize != this->m_hideSize) {
        this->m_hideSize = hideSize;
        emit this->hideSizeChanged();
    }
}

void ScanLinesElement::setHideColor(QRgb hideColor)
{
    if (hideColor != this->m_hideColor) {
        this->m_hideColor = hideColor;
        emit this->hideColorChanged();
    }
}

void ScanLinesElement::resetShowSize()
{
    this->setShowSize(1);
}

void ScanLinesElement::resetHideSize()
{
    this->setHideSize(4);
}

void ScanLinesElement::resetHideColor()
{
    this->setHideColor(qRgb(0, 0, 0));
}

QbPacket ScanLinesElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());

    int showSize = this->m_showSize;
    int hideSize = this->m_hideSize;

    if (showSize < 1 && hideSize < 1)
        qbSend(packet)

    for (int y = 0; y < src.height(); y++) {
        for (int i = 0; i < showSize && y < src.height(); i++, y++)
            memcpy(oFrame.scanLine(y), src.scanLine(y), src.bytesPerLine());

        for (int j = 0; j < hideSize && y < src.height(); j++, y++) {
            QRgb *line = (QRgb *) oFrame.scanLine(y);

            for (int x = 0; x < src.width(); x++)
                line[x] = this->m_hideColor;
        }

        y--;
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}

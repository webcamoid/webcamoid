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

#include <QImage>
#include <QQmlContext>
#include <akutils.h>
#include <akpacket.h>

#include "scanlineselement.h"

ScanLinesElement::ScanLinesElement(): AkElement()
{
    this->m_showSize = 1;
    this->m_hideSize = 4;
    this->m_hideColor = qRgb(0, 0, 0);
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

QString ScanLinesElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ScanLines/share/qml/main.qml");
}

void ScanLinesElement::controlInterfaceConfigure(QQmlContext *context,
                                                 const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ScanLines", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void ScanLinesElement::setShowSize(int showSize)
{
    if (this->m_showSize == showSize)
        return;

    this->m_showSize = showSize;
    emit this->showSizeChanged(showSize);
}

void ScanLinesElement::setHideSize(int hideSize)
{
    if (this->m_hideSize == hideSize)
        return;

    this->m_hideSize = hideSize;
    emit this->hideSizeChanged(hideSize);
}

void ScanLinesElement::setHideColor(QRgb hideColor)
{
    if (this->m_hideColor == hideColor)
        return;

    this->m_hideColor = hideColor;
    emit this->hideColorChanged(hideColor);
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

AkPacket ScanLinesElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());

    int showSize = this->m_showSize;
    int hideSize = this->m_hideSize;

    if (showSize < 1 && hideSize < 1)
        akSend(packet)

    for (int y = 0; y < src.height(); y++) {
        for (int i = 0; i < showSize && y < src.height(); i++, y++)
            memcpy(oFrame.scanLine(y), src.scanLine(y), size_t(src.bytesPerLine()));

        for (int j = 0; j < hideSize && y < src.height(); j++, y++) {
            QRgb *line = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

            for (int x = 0; x < src.width(); x++)
                line[x] = this->m_hideColor;
        }

        y--;
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

#include "moc_scanlineselement.cpp"

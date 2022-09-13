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

#include <QImage>
#include <QQmlContext>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "scanlineselement.h"

class ScanLinesElementPrivate
{
    public:
        int m_showSize {1};
        int m_hideSize {4};
        QRgb m_hideColor {qRgb(0, 0, 0)};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argb, 0, 0, {}}};
};

ScanLinesElement::ScanLinesElement(): AkElement()
{
    this->d = new ScanLinesElementPrivate;
}

ScanLinesElement::~ScanLinesElement()
{
    delete this->d;
}

int ScanLinesElement::showSize() const
{
    return this->d->m_showSize;
}

int ScanLinesElement::hideSize() const
{
    return this->d->m_hideSize;
}

QRgb ScanLinesElement::hideColor() const
{
    return this->d->m_hideColor;
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

AkPacket ScanLinesElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = this->d->m_videoConverter.convertToImage(packet);

    if (src.isNull())
        return AkPacket();

    QImage oFrame(src.size(), src.format());

    int showSize = this->d->m_showSize;
    int hideSize = this->d->m_hideSize;

    if (showSize < 1 && hideSize < 1) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    for (int y = 0; y < src.height(); y++) {
        for (int i = 0; i < showSize && y < src.height(); i++, y++)
            memcpy(oFrame.scanLine(y), src.scanLine(y), size_t(src.bytesPerLine()));

        for (int j = 0; j < hideSize && y < src.height(); j++, y++) {
            auto line = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

            for (int x = 0; x < src.width(); x++)
                line[x] = this->d->m_hideColor;
        }

        y--;
    }

    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

void ScanLinesElement::setShowSize(int showSize)
{
    if (this->d->m_showSize == showSize)
        return;

    this->d->m_showSize = showSize;
    emit this->showSizeChanged(showSize);
}

void ScanLinesElement::setHideSize(int hideSize)
{
    if (this->d->m_hideSize == hideSize)
        return;

    this->d->m_hideSize = hideSize;
    emit this->hideSizeChanged(hideSize);
}

void ScanLinesElement::setHideColor(QRgb hideColor)
{
    if (this->d->m_hideColor == hideColor)
        return;

    this->d->m_hideColor = hideColor;
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

#include "moc_scanlineselement.cpp"

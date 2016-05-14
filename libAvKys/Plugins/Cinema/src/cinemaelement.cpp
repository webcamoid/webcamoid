/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include <QtMath>

#include "cinemaelement.h"

CinemaElement::CinemaElement(): AkElement()
{
    this->m_stripSize = 0.5;
    this->m_stripColor = qRgb(0, 0, 0);
}

QObject *CinemaElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Cinema/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Cinema", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

qreal CinemaElement::stripSize() const
{
    return this->m_stripSize;
}

QRgb CinemaElement::stripColor() const
{
    return this->m_stripColor;
}

void CinemaElement::setStripSize(qreal stripSize)
{
    if (qFuzzyCompare(this->m_stripSize, stripSize))
        return;

    this->m_stripSize = stripSize;
    emit this->stripSizeChanged(stripSize);
}

void CinemaElement::setStripColor(QRgb hideColor)
{
    if (this->m_stripColor == hideColor)
        return;

    this->m_stripColor = hideColor;
    emit this->stripColorChanged(hideColor);
}

void CinemaElement::resetStripSize()
{
    this->setStripSize(0.5);
}

void CinemaElement::resetStripColor()
{
    this->setStripColor(qRgb(0, 0, 0));
}

AkPacket CinemaElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame(src.size(), src.format());
    int cy = src.height() >> 1;

    for (int y = 0; y < src.height(); y++) {
        qreal k = 1.0 - qAbs(y - cy) / qreal(cy);
        const QRgb *iLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        if (k > this->m_stripSize)
            memcpy(oLine, iLine, size_t(src.bytesPerLine()));
        else
            for (int x = 0; x < src.width(); x++) {
                qreal a = qAlpha(this->m_stripColor) / 255.0;

                int r = int(a * (qRed(this->m_stripColor) - qRed(iLine[x])) + qRed(iLine[x]));
                int g = int(a * (qGreen(this->m_stripColor) - qGreen(iLine[x])) + qGreen(iLine[x]));
                int b = int(a * (qBlue(this->m_stripColor) - qBlue(iLine[x])) + qBlue(iLine[x]));

                oLine[x] = qRgba(r, g, b, qAlpha(iLine[x]));
            }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

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

#include "edgeelement.h"

EdgeElement::EdgeElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=gray");

    this->resetEqualize();
    this->resetInvert();
}

EdgeElement::~EdgeElement()
{
}

QObject *EdgeElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Edge/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Edge", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

bool EdgeElement::equalize() const
{
    return this->m_equalize;
}

bool EdgeElement::invert() const
{
    return this->m_invert;
}

void EdgeElement::setEqualize(bool equalize)
{
    if (equalize != this->m_equalize) {
        this->m_equalize = equalize;
        emit this->equalizeChanged();
    }
}

void EdgeElement::setInvert(bool invert)
{
    if (invert != this->m_invert) {
        this->m_invert = invert;
        emit this->invertChanged();
    }
}

void EdgeElement::resetEqualize()
{
    this->setEqualize(false);
}

void EdgeElement::resetInvert()
{
    this->setInvert(false);
}

QbPacket EdgeElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());

    quint8 *srcBits = (quint8 *) src.bits();

    if (this->m_equalize) {
        int videoArea = src.width() * src.height();
        int minGray = 255;
        int maxGray = 0;

        for (int i = 0; i < videoArea; i++) {
            if (srcBits[i] < minGray)
                minGray = srcBits[i];

            if (srcBits[i] > maxGray)
                maxGray = srcBits[i];
        }

        if (maxGray == minGray)
            memset(srcBits, minGray, videoArea);
        else {
            int diffGray = maxGray - minGray;

            for (int i = 0; i < videoArea; i++)
                srcBits[i] = 255 * (srcBits[i] - minGray) / diffGray;
        }
    }

    for (int y = 0; y < src.height(); y++) {
        quint8 *iLine = (quint8 *) src.scanLine(y);

        quint8 *iLine_m1 = (y < 1)?
                               iLine:
                               (quint8 *) src.scanLine(y - 1);

        quint8 *iLine_p1 = (y >= src.height() - 1)?
                               iLine:
                               (quint8 *) src.scanLine(y + 1);

        quint8 *oLine = (quint8 *) oFrame.scanLine(y);

        for (int x = 0; x < src.width(); x++) {
            int x_m1 = (x < 1)? x: x - 1;
            int x_p1 = (x >= src.width() - 1)? x: x + 1;

            int grayX = iLine_m1[x_p1]
                      + 2 * iLine[x_p1]
                      + iLine_p1[x_p1]
                      - iLine_m1[x_m1]
                      - 2 * iLine[x_m1]
                      - iLine_p1[x_m1];

            int grayY = iLine_m1[x_m1]
                      + 2 * iLine_m1[x]
                      + iLine_m1[x_p1]
                      - iLine_p1[x_m1]
                      - 2 * iLine_p1[x]
                      - iLine_p1[x_p1];

            int gray = qAbs(grayX) + qAbs(grayY);
            gray = qBound(0, gray, 255);

            if (this->m_invert)
                oLine[x] = 255 - gray;
            else
                oLine[x] = gray;
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}

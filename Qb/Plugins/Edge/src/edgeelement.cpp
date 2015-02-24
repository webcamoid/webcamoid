/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#include "edgeelement.h"

EdgeElement::EdgeElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=gray");

    this->resetEqualize();
    this->resetInvert();
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
    quint8 *destBits = (quint8 *) oFrame.bits();

    int widthMin = src.width() - 1;
    int widthMax = src.width() + 1;
    int heightMin = src.height() - 1;

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

    memset(oFrame.scanLine(0), 0, src.width());
    memset(oFrame.scanLine(heightMin), 0, src.width());

    for (int y = 0; y < src.height(); y++) {
        int xOffset = y * src.width();

        destBits[xOffset] = 0;
        destBits[xOffset + widthMin] = 0;
    }

    for (int y = 1; y < heightMin; y++) {
        int xOffset = y * src.width();

        for (int x = 1; x < widthMin; x++) {
            int pixel = x + xOffset;

            int grayX =   srcBits[pixel - widthMax]
                        + srcBits[pixel - 1]
                        + srcBits[pixel + widthMin]
                        - srcBits[pixel - widthMin]
                        - srcBits[pixel + 1]
                        - srcBits[pixel + widthMax];

            int grayY =   srcBits[pixel - widthMax]
                        + srcBits[pixel - src.width()]
                        + srcBits[pixel - widthMin]
                        - srcBits[pixel + widthMin]
                        - srcBits[pixel + src.width()]
                        - srcBits[pixel + widthMax];

            int gray = sqrt(grayX * grayX + grayY * grayY);

            if (this->m_invert)
                destBits[pixel] = 255 - qBound(0, gray, 255);
            else
                destBits[pixel] = qBound(0, gray, 255);
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}

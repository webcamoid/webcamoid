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

#include "delaygrabelement.h"

DelayGrabElement::DelayGrabElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    this->m_modeToStr[DelayGrabModeRandomSquare] = "RandomSquare";
    this->m_modeToStr[DelayGrabModeVerticalIncrease] = "VerticalIncrease";
    this->m_modeToStr[DelayGrabModeHorizontalIncrease] = "HorizontalIncrease";
    this->m_modeToStr[DelayGrabModeRingsIncrease] = "RingsIncrease";

    this->resetMode();
    this->resetBlockSize();
    this->resetNFrames();
}

QObject *DelayGrabElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/DelayGrab/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("DelayGrab", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

QString DelayGrabElement::mode() const
{
    return this->m_modeToStr[this->m_mode];
}

int DelayGrabElement::blockSize() const
{
    return this->m_blockSize;
}

int DelayGrabElement::nFrames() const
{
    return this->m_nFrames;
}

QVector<int> DelayGrabElement::createDelaymap(DelayGrabMode mode)
{
    QVector<int> delayMap(this->m_delayMapHeight * this->m_delayMapWidth);

    for (int i = 0, y = this->m_delayMapHeight; y > 0; y--) {
        for (int x = this->m_delayMapWidth; x > 0; i++, x--) {
            // Random delay with square distribution
            if (mode == DelayGrabModeRandomSquare) {
                qreal d = (qreal) qrand() / RAND_MAX;
                delayMap[i] = 16.0 * d * d;
            }
            // Vertical stripes of increasing delay outward from center
            else if (mode == DelayGrabModeVerticalIncrease) {
                int k = this->m_delayMapWidth >> 1;
                int v;

                if (x < k)
                    v = k - x;
                else if (x > k)
                    v = x - k;
                else
                    v = 0;

                delayMap[i] = v >> 1;
            }
            // Horizontal stripes of increasing delay outward from center
            else if (mode == DelayGrabModeHorizontalIncrease) {
                int k = this->m_delayMapHeight >> 1;
                int v;

                if (y < k)
                    v = k - y;
                else if (y > k)
                    v = y - k;
                else
                    v = 0;

                delayMap[i] = v >> 1;
            }
            // Rings of increasing delay outward from center
            else {
                int dx = x - (this->m_delayMapWidth >> 1);
                int dy = y - (this->m_delayMapHeight >> 1);
                int v = sqrt(dx * dx + dy * dy);

                delayMap[i] = v >> 1;
            }

            // Clip values
            delayMap[i] = qBound(0, delayMap[i], this->m_nFrames - 1);
        }
    }

    return delayMap;
}

void DelayGrabElement::setMode(const QString &mode)
{
    DelayGrabMode modeEnum = this->m_modeToStr.values().contains(mode)?
                                 this->m_modeToStr.key(mode):
                                 DelayGrabModeRingsIncrease;

    if (modeEnum != this->m_mode) {
        this->m_mode = modeEnum;
        emit this->modeChanged();
    }
}

void DelayGrabElement::setBlockSize(int blockSize)
{
    if (blockSize < 1)
        blockSize = 1;

    if (blockSize != this->m_blockSize) {
        this->m_blockSize = blockSize;
        emit this->blockSizeChanged();
    }
}

void DelayGrabElement::setNFrames(int nFrames)
{
    if (nFrames < 0)
        nFrames = 0;

    if (nFrames != this->m_nFrames) {
        this->m_nFrames = nFrames;
        emit this->nFramesChanged();
    }
}

void DelayGrabElement::resetMode()
{
    this->setMode("RingsIncrease");
}

void DelayGrabElement::resetBlockSize()
{
    this->setBlockSize(2);
}

void DelayGrabElement::resetNFrames()
{
    this->setNFrames(71);
}

QbPacket DelayGrabElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame = QImage(src.size(), src.format());
    QRgb *destBits = (QRgb *) oFrame.bits();

    static DelayGrabMode mode = this->m_mode;
    static int blockSize = this->m_blockSize;

    if (packet.caps() != this->m_caps
        || mode != this->m_mode
        || blockSize != this->m_blockSize) {
        this->m_delayMapWidth = src.width() / this->m_blockSize;
        this->m_delayMapHeight = src.height() / this->m_blockSize;
        this->m_delayMap = this->createDelaymap(this->m_mode);
        this->m_frames.clear();

        this->m_caps = packet.caps();
        mode = this->m_mode;
        blockSize = this->m_blockSize;
    }

    this->m_frames << src.copy();
    int diff = this->m_frames.size() - this->m_nFrames;

    for (int i = 0; i < diff; i++)
        this->m_frames.takeFirst();

    if (this->m_frames.isEmpty())
        qbSend(packet)

    int delayMapWidth = this->m_delayMapWidth;
    int delayMapHeight = this->m_delayMapHeight;

    // Copy image blockwise to screenbuffer
    for (int i = 0, y = 0; y < delayMapHeight; y++) {
        for (int x = 0; x < delayMapWidth ; i++, x++) {
            int curFrame = qAbs(this->m_frames.size() - 1 - this->m_delayMap[i]) % this->m_frames.size();
            int curFrameWidth = this->m_frames[curFrame].width();
            int xyoff = blockSize * (x + y * curFrameWidth);

            // source
            QRgb *source = (QRgb *) this->m_frames[curFrame].bits();
            source += xyoff;

            // target
            QRgb *dest = destBits;
            dest += xyoff;

            // copy block
            for (int j = 0; j < blockSize; j++) {
                memcpy(dest, source, 4 * blockSize);
                source += curFrameWidth;
                dest += curFrameWidth;
            }
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}

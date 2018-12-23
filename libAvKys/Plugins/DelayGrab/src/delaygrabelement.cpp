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

#include <cstdlib>
#include <QMap>
#include <QVector>
#include <QMutex>
#include <QImage>
#include <QQmlContext>
#include <QtMath>
#include <akutils.h>
#include <akpacket.h>

#include "delaygrabelement.h"

using DelayGrabModeMap = QMap<DelayGrabElement::DelayGrabMode, QString>;

inline DelayGrabModeMap initDelayGrabModeMap()
{
    DelayGrabModeMap modeToStr {
        {DelayGrabElement::DelayGrabModeRandomSquare      , "RandomSquare"      },
        {DelayGrabElement::DelayGrabModeVerticalIncrease  , "VerticalIncrease"  },
        {DelayGrabElement::DelayGrabModeHorizontalIncrease, "HorizontalIncrease"},
        {DelayGrabElement::DelayGrabModeRingsIncrease     , "RingsIncrease"     }
    };

    return modeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(DelayGrabModeMap, modeToStr, (initDelayGrabModeMap()))


class DelayGrabElementPrivate
{
    public:
        DelayGrabElement::DelayGrabMode m_mode {DelayGrabElement::DelayGrabModeRingsIncrease};
        int m_blockSize {2};
        int m_nFrames {71};
        QMutex m_mutex;
        QSize m_frameSize;
        QVector<QImage> m_frames;
        QVector<int> m_delayMap;
};

DelayGrabElement::DelayGrabElement(): AkElement()
{
    this->d = new DelayGrabElementPrivate;

    QObject::connect(this,
                     &DelayGrabElement::modeChanged,
                     this,
                     &DelayGrabElement::updateDelaymap);
    QObject::connect(this,
                     &DelayGrabElement::blockSizeChanged,
                     this,
                     &DelayGrabElement::updateDelaymap);
    QObject::connect(this,
                     &DelayGrabElement::nFramesChanged,
                     this,
                     &DelayGrabElement::updateDelaymap);
    QObject::connect(this,
                     &DelayGrabElement::frameSizeChanged,
                     this,
                     &DelayGrabElement::updateDelaymap);
}

DelayGrabElement::~DelayGrabElement()
{
    delete this->d;
}

QString DelayGrabElement::mode() const
{
    return modeToStr->value(this->d->m_mode);
}

int DelayGrabElement::blockSize() const
{
    return this->d->m_blockSize;
}

int DelayGrabElement::nFrames() const
{
    return this->d->m_nFrames;
}

QString DelayGrabElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/DelayGrab/share/qml/main.qml");
}

void DelayGrabElement::controlInterfaceConfigure(QQmlContext *context,
                                                 const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("DelayGrab", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void DelayGrabElement::setMode(const QString &mode)
{
    DelayGrabMode modeEnum = modeToStr->key(mode, DelayGrabModeRingsIncrease);

    if (this->d->m_mode == modeEnum)
        return;

    QMutexLocker locker(&this->d->m_mutex);
    this->d->m_mode = modeEnum;
    emit this->modeChanged(mode);
}

void DelayGrabElement::setBlockSize(int blockSize)
{
    if (this->d->m_blockSize == blockSize)
        return;

    QMutexLocker locker(&this->d->m_mutex);
    this->d->m_blockSize = blockSize;
    emit this->blockSizeChanged(blockSize);
}

void DelayGrabElement::setNFrames(int nFrames)
{
    if (this->d->m_nFrames == nFrames)
        return;

    QMutexLocker locker(&this->d->m_mutex);
    this->d->m_nFrames = nFrames;
    emit this->nFramesChanged(nFrames);
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

AkPacket DelayGrabElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    QImage oFrame = QImage(src.size(), src.format());
    QRgb *destBits = reinterpret_cast<QRgb *>(oFrame.bits());

    if (src.size() != this->d->m_frameSize) {
        this->updateDelaymap();
        this->d->m_frames.clear();
        this->d->m_frameSize = src.size();
        emit this->frameSizeChanged(this->d->m_frameSize);
    }

    int nFrames = this->d->m_nFrames > 0? this->d->m_nFrames: 1;
    this->d->m_frames << src.copy();
    int diff = this->d->m_frames.size() - nFrames;

    for (int i = 0; i < diff; i++)
        this->d->m_frames.removeFirst();

    if (this->d->m_frames.isEmpty())
        akSend(packet)

    this->d->m_mutex.lock();
    int blockSize = this->d->m_blockSize > 0? this->d->m_blockSize: 1;
    int delayMapWidth = src.width() / blockSize;
    int delayMapHeight = src.height() / blockSize;
    QVector<int> delayMap = this->d->m_delayMap;
    this->d->m_mutex.unlock();

    if (delayMap.isEmpty())
        akSend(packet)

    // Copy image blockwise to screenbuffer
    for (int i = 0, y = 0; y < delayMapHeight; y++) {
        for (int x = 0; x < delayMapWidth ; i++, x++) {
            int curFrame = qAbs(this->d->m_frames.size() - 1 - delayMap[i])
                           % this->d->m_frames.size();
            int curFrameWidth = this->d->m_frames[curFrame].width();
            int xyoff = blockSize * (x + y * curFrameWidth);

            // source
            auto source = reinterpret_cast<const QRgb *>(this->d->m_frames[curFrame].constBits());
            source += xyoff;

            // target
            QRgb *dest = destBits;
            dest += xyoff;

            // copy block
            for (int j = 0; j < blockSize; j++) {
                memcpy(dest, source, size_t(4 * blockSize));
                source += curFrameWidth;
                dest += curFrameWidth;
            }
        }
    }

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

void DelayGrabElement::updateDelaymap()
{
    QMutexLocker locker(&this->d->m_mutex);

    if (this->d->m_frameSize.isEmpty())
        return;

    int nFrames = this->d->m_nFrames > 0? this->d->m_nFrames: 1;
    int blockSize = this->d->m_blockSize > 0? this->d->m_blockSize: 1;
    int delayMapWidth = this->d->m_frameSize.width() / blockSize;
    int delayMapHeight = this->d->m_frameSize.height() / blockSize;

    this->d->m_delayMap.resize(delayMapHeight * delayMapWidth);

    int minX = -(delayMapWidth >> 1);
    int maxX = (delayMapWidth >> 1);
    int minY = -(delayMapHeight >> 1);
    int maxY = (delayMapHeight >> 1);

    for (int y = minY, i = 0; y < maxY; y++) {
        for (int x = minX; x < maxX; x++, i++) {
            qreal value;

            if (this->d->m_mode == DelayGrabModeRandomSquare) {
                // Random delay with square distribution
                qreal d = qreal(qrand()) / RAND_MAX;
                value = 16.0 * d * d;
            } else if (this->d->m_mode == DelayGrabModeVerticalIncrease) {
                value = qAbs(x) / 2.0;
            } else if (this->d->m_mode == DelayGrabModeHorizontalIncrease) {
                value = qAbs(y) / 2.0;
            } else {
                // Rings of increasing delay outward from center
                value = sqrt(x * x + y * y) / 2;
            }

            // Clip values
            this->d->m_delayMap[i] = int(value) % nFrames;
        }
    }
}

#include "moc_delaygrabelement.cpp"

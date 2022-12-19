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

#include <QDataStream>
#include <QDateTime>
#include <QMap>
#include <QMutex>
#include <QQmlContext>
#include <QRandomGenerator>
#include <QSize>
#include <QVector>
#include <QtMath>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "delaygrabelement.h"

class DelayGrabElementPrivate
{
    public:
        DelayGrabElement::DelayGrabMode m_mode {DelayGrabElement::DelayGrabModeRingsIncrease};
        int m_blockSize {2};
        int m_nFrames {71};
        QMutex m_mutex;
        QSize m_frameSize;
        QVector<AkVideoPacket> m_frames;
        AkVideoPacket m_delayMap;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        void updateDelaymap();
};

DelayGrabElement::DelayGrabElement(): AkElement()
{
    this->d = new DelayGrabElementPrivate;
}

DelayGrabElement::~DelayGrabElement()
{
    delete this->d;
}

DelayGrabElement::DelayGrabMode DelayGrabElement::mode() const
{
    return this->d->m_mode;
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

AkPacket DelayGrabElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_frameSize) {
        this->d->m_frames.clear();
        this->d->m_frameSize = frameSize;
        emit this->frameSizeChanged(this->d->m_frameSize);
        this->d->updateDelaymap();
    }

    int nFrames = this->d->m_nFrames > 0? this->d->m_nFrames: 1;
    this->d->m_frames << src;
    int diff = this->d->m_frames.size() - nFrames;

    for (int i = 0; i < diff; i++)
        this->d->m_frames.removeFirst();

    if (this->d->m_frames.isEmpty()) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    this->d->m_mutex.lock();

    if (!this->d->m_delayMap) {
        this->d->m_mutex.unlock();

        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    int blockSize = this->d->m_blockSize > 0? this->d->m_blockSize: 1;
    int delayMapWidth = src.caps().width() / blockSize;
    int delayMapHeight = src.caps().height() / blockSize;

    AkVideoPacket dst(src.caps(), true);
    dst.copyMetadata(src);
    size_t oLineSize = dst.lineSize(0);

    // Copy image blockwise to frame buffer
    for (int y = 0; y < delayMapHeight; y++) {
        auto yb = blockSize * y;
        auto dstLine = dst.line(0, yb);
        auto delayLine =
                reinterpret_cast<quint16 *>(this->d->m_delayMap.line(0, y));

        for (int x = 0; x < delayMapWidth; x++) {
            int curFrame = qAbs(this->d->m_frames.size() - delayLine[x] - 1)
                           % this->d->m_frames.size();
            auto &frame = this->d->m_frames[curFrame];
            size_t iLineSize = frame.lineSize(0);
            size_t xoffset = blockSize * x * sizeof(QRgb);
            auto srcLineX = frame.constLine(0, yb) + xoffset;
            auto dstLineX = dstLine + xoffset;

            // copy block
            for (int j = 0; j < blockSize; j++) {
                memcpy(dstLineX, srcLineX, blockSize * sizeof(QRgb));
                srcLineX += iLineSize;
                dstLineX += oLineSize;
            }
        }
    }

    this->d->m_mutex.unlock();

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void DelayGrabElement::setMode(DelayGrabMode mode)
{
    if (this->d->m_mode == mode)
        return;

    this->d->m_mutex.lock();
    this->d->m_mode = mode;
    this->d->m_mutex.unlock();
    emit this->modeChanged(mode);
    this->d->updateDelaymap();
}

void DelayGrabElement::setBlockSize(int blockSize)
{
    if (this->d->m_blockSize == blockSize)
        return;

    this->d->m_mutex.lock();
    this->d->m_blockSize = blockSize;
    this->d->m_mutex.unlock();
    emit this->blockSizeChanged(blockSize);
    this->d->updateDelaymap();
}

void DelayGrabElement::setNFrames(int nFrames)
{
    if (this->d->m_nFrames == nFrames)
        return;

    this->d->m_mutex.lock();
    this->d->m_nFrames = nFrames;
    this->d->m_mutex.unlock();
    emit this->nFramesChanged(nFrames);
    this->d->updateDelaymap();
}

void DelayGrabElement::resetMode()
{
    this->setMode(DelayGrabModeRingsIncrease);
}

void DelayGrabElement::resetBlockSize()
{
    this->setBlockSize(2);
}

void DelayGrabElement::resetNFrames()
{
    this->setNFrames(71);
}

QDataStream &operator >>(QDataStream &istream, DelayGrabElement::DelayGrabMode &mode)
{
    int modeInt;
    istream >> modeInt;
    mode = static_cast<DelayGrabElement::DelayGrabMode>(modeInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, DelayGrabElement::DelayGrabMode mode)
{
    ostream << static_cast<int>(mode);

    return ostream;
}

void DelayGrabElementPrivate::updateDelaymap()
{
    QMutexLocker locker(&this->m_mutex);

    if (this->m_frameSize.isEmpty())
        return;

    int nFrames = this->m_nFrames > 0? this->m_nFrames: 1;
    int blockSize = this->m_blockSize > 0? this->m_blockSize: 1;
    int delayMapWidth = this->m_frameSize.width() / blockSize;
    int delayMapHeight = this->m_frameSize.height() / blockSize;

    this->m_delayMap = AkVideoPacket({AkVideoCaps::Format_gray16,
                                      delayMapWidth,
                                      delayMapHeight,
                                      {}});

    int minX = -(delayMapWidth >> 1);
    int maxX = delayMapWidth >> 1;
    int minY = -(delayMapHeight >> 1);
    int maxY = delayMapHeight >> 1;

    for (int y = minY; y < maxY; y++) {
        auto delayLine =
                reinterpret_cast<quint16 *>(this->m_delayMap.line(0, y - minY));

        for (int x = minX; x < maxX; x++) {
            int value = 0;

            switch (this->m_mode) {
            case DelayGrabElement::DelayGrabModeRandomSquare: {
                // Random delay with square distribution
                auto d = QRandomGenerator::global()->bounded(1.0);
                value = qRound(16.0 * d * d);

                break;
            }
            case DelayGrabElement::DelayGrabModeVerticalIncrease: {
                value = qAbs(x) >> 1;

                break;
            }
            case DelayGrabElement::DelayGrabModeHorizontalIncrease: {
                value = qAbs(y) >> 1;

                break;
            }
            default: {
                // Rings of increasing delay outward from center
                value = qRound(sqrt(x * x + y * y) / 2.0);

                break;
            }
            }

            // Clip values
            delayLine[x - minX] = value % nFrames;
        }
    }
}

#include "moc_delaygrabelement.cpp"

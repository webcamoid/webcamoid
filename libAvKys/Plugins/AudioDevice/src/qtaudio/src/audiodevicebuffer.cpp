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

#include "audiodevicebuffer.h"

#define TIME_OUT 500
#define BLOCK_SIZE (2 * 2 * 1024)

AudioDeviceBuffer::AudioDeviceBuffer(QObject *parent):
    QIODevice(parent)
{
    this->m_blockSize = BLOCK_SIZE;
    this->m_maxBufferSize = 4 * BLOCK_SIZE;
    this->m_isOpen = false;
}

AudioDeviceBuffer::~AudioDeviceBuffer()
{
    this->close();
}

qint64 AudioDeviceBuffer::blockSize() const
{
    return this->m_blockSize;
}

qint64 AudioDeviceBuffer::maxBufferSize() const
{
    return this->m_maxBufferSize;
}

bool AudioDeviceBuffer::atEnd() const
{
    return !this->isOpen();
}

qint64 AudioDeviceBuffer::bytesAvailable() const
{
    return this->m_blockSize;
}

qint64 AudioDeviceBuffer::bytesToWrite() const
{
    return 0;
}

bool AudioDeviceBuffer::canReadLine() const
{
    return false;
}

void AudioDeviceBuffer::close()
{
    this->m_isOpen = false;

    this->m_mutex.lock();
    this->m_buffer.clear();
    this->m_mutex.unlock();
    QIODevice::close();
}

bool AudioDeviceBuffer::isSequential() const
{
    return true;
}

bool AudioDeviceBuffer::open(QIODevice::OpenMode mode)
{
    this->m_mutex.lock();
    this->m_buffer.clear();
    this->m_isOpen = QIODevice::open(mode);
    this->m_mutex.unlock();

    return this->m_isOpen;
}

qint64 AudioDeviceBuffer::pos() const
{
    return 0;
}

bool AudioDeviceBuffer::reset()
{
    return false;
}

bool AudioDeviceBuffer::seek(qint64 pos)
{
    Q_UNUSED(pos)

    return false;
}

qint64 AudioDeviceBuffer::size() const
{
    return this->bytesAvailable();
}

bool AudioDeviceBuffer::waitForBytesWritten(int msecs)
{
    Q_UNUSED(msecs);

    return true;
}

bool AudioDeviceBuffer::waitForReadyRead(int msecs)
{
    Q_UNUSED(msecs);

    return true;
}

qint64 AudioDeviceBuffer::readData(char *data, qint64 maxSize)
{
    if (!this->m_isOpen)
        return 0;

    memset(data, 0, size_t(maxSize));

    this->m_mutex.lock();
    auto copyBytes = qMin<qint64>(this->m_buffer.size(), maxSize);
    memcpy(data, this->m_buffer.constData(), size_t(copyBytes));
    this->m_buffer.remove(0, int(copyBytes));

    if (this->m_buffer.size() < this->m_maxBufferSize)
        this->m_bufferNotFull.wakeAll();

    this->m_mutex.unlock();

    return maxSize;
}

qint64 AudioDeviceBuffer::writeData(const char *data, qint64 maxSize)
{
    qint64 writenSize = 0;

    this->m_mutex.lock();

    while (this->m_isOpen) {
        if (this->m_buffer.size() >= this->m_maxBufferSize)
            if (!this->m_bufferNotFull.wait(&this->m_mutex, TIME_OUT))
                continue;

        this->m_buffer.append(QByteArray::fromRawData(data, int(maxSize)));
        writenSize = maxSize;

        break;
    }

    this->m_mutex.unlock();

    return writenSize;
}

void AudioDeviceBuffer::setBlockSize(qint64 blockSize)
{
    if (this->m_blockSize == blockSize)
        return;

    this->m_blockSize = blockSize;
    emit this->blockSizeChanged(blockSize);
}

void AudioDeviceBuffer::setMaxBufferSize(qint64 maxBufferSize)
{
    if (this->m_maxBufferSize == maxBufferSize)
        return;

    this->m_maxBufferSize = maxBufferSize;
    emit this->maxBufferSizeChanged(maxBufferSize);
}

void AudioDeviceBuffer::resetBlockSize()
{
    this->setBlockSize(BLOCK_SIZE);
}

void AudioDeviceBuffer::resetMaxBufferSize()
{
    this->setMaxBufferSize(4 * BLOCK_SIZE);
}

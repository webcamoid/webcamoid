/* Webcamoid, camera capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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

class AudioDeviceBufferPrvate
{
    public:
        QByteArray m_buffer;
        qint64 m_blockSize {0};
        qint64 m_maxBufferSize {0};
        QMutex m_mutex;
        QWaitCondition m_bufferNotEmpty;
        QWaitCondition m_bufferNotFull;
        bool m_isOpen {false};
};

AudioDeviceBuffer::AudioDeviceBuffer(QObject *parent):
    QIODevice(parent)
{
    this->d = new AudioDeviceBufferPrvate;
    this->d->m_blockSize = BLOCK_SIZE;
    this->d->m_maxBufferSize = 4 * BLOCK_SIZE;
    this->d->m_isOpen = false;
}

AudioDeviceBuffer::~AudioDeviceBuffer()
{
    this->close();
    delete this->d;
}

qint64 AudioDeviceBuffer::blockSize() const
{
    return this->d->m_blockSize;
}

qint64 AudioDeviceBuffer::maxBufferSize() const
{
    return this->d->m_maxBufferSize;
}

bool AudioDeviceBuffer::atEnd() const
{
    return !this->isOpen();
}

qint64 AudioDeviceBuffer::bytesAvailable() const
{
    return this->d->m_blockSize;
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
    this->d->m_isOpen = false;

    this->d->m_mutex.lock();
    this->d->m_buffer.clear();
    this->d->m_mutex.unlock();
    QIODevice::close();
}

bool AudioDeviceBuffer::isSequential() const
{
    return true;
}

bool AudioDeviceBuffer::open(QIODevice::OpenMode mode)
{
    this->d->m_mutex.lock();
    this->d->m_buffer.clear();
    this->d->m_isOpen = QIODevice::open(mode);
    this->d->m_mutex.unlock();

    return this->d->m_isOpen;
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
    this->d->m_mutex.lock();

    if (this->d->m_buffer.size() >= this->d->m_maxBufferSize)
        if (!this->d->m_bufferNotFull.wait(&this->d->m_mutex, msecs)) {
            this->d->m_mutex.unlock();

            return false;
        }

    this->d->m_mutex.unlock();

    return true;
}

bool AudioDeviceBuffer::waitForReadyRead(int msecs)
{
    this->d->m_mutex.lock();

    if (this->d->m_buffer.size() < 1)
        if (!this->d->m_bufferNotEmpty.wait(&this->d->m_mutex, msecs)) {
            this->d->m_mutex.unlock();

            return false;
        }

    this->d->m_mutex.unlock();

    return true;
}

qint64 AudioDeviceBuffer::readData(char *data, qint64 maxSize)
{
    if (!this->d->m_isOpen)
        return 0;

    memset(data, 0, size_t(maxSize));

    this->d->m_mutex.lock();

    if (this->d->m_buffer.size() < 1)
        if (!this->d->m_bufferNotEmpty.wait(&this->d->m_mutex, TIME_OUT)) {
            this->d->m_mutex.unlock();

            return 0;
        }

    auto copyBytes = qMin<qint64>(this->d->m_buffer.size(), maxSize);
    memcpy(data, this->d->m_buffer.constData(), size_t(copyBytes));
    this->d->m_buffer.remove(0, int(copyBytes));

    if (this->d->m_buffer.size() < this->d->m_maxBufferSize)
        this->d->m_bufferNotFull.wakeAll();

    this->d->m_mutex.unlock();

    return maxSize;
}

qint64 AudioDeviceBuffer::writeData(const char *data, qint64 maxSize)
{
    qint64 writenSize = 0;

    this->d->m_mutex.lock();

    for (int i = 0; i < 3 && this->d->m_isOpen; i++) {
        if (this->d->m_buffer.size() >= this->d->m_maxBufferSize)
            if (!this->d->m_bufferNotFull.wait(&this->d->m_mutex, TIME_OUT))
                continue;

        this->d->m_buffer.append(QByteArray::fromRawData(data, int(maxSize)));
        writenSize = maxSize;

        break;
    }

    this->d->m_bufferNotEmpty.wakeAll();

    this->d->m_mutex.unlock();

    return writenSize;
}

void AudioDeviceBuffer::setBlockSize(qint64 blockSize)
{
    if (this->d->m_blockSize == blockSize)
        return;

    this->d->m_blockSize = blockSize;
    emit this->blockSizeChanged(blockSize);
}

void AudioDeviceBuffer::setMaxBufferSize(qint64 maxBufferSize)
{
    if (this->d->m_maxBufferSize == maxBufferSize)
        return;

    this->d->m_maxBufferSize = maxBufferSize;
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

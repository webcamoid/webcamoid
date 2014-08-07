/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "audiobuffer.h"

AudioBuffer::AudioBuffer(QObject *parent):
    QIODevice(parent)
{
    this->resetMaxSize();
}

qint64 AudioBuffer::maxSize() const
{
    return this->m_maxSize;
}

bool AudioBuffer::atEnd() const
{
    return false;
}

qint64 AudioBuffer::bytesAvailable() const
{
    return this->m_audioBuffer.size();
}

qint64 AudioBuffer::bytesToWrite() const
{
    return 0;
}

bool AudioBuffer::canReadLine() const
{
    return false;
}

void AudioBuffer::close()
{
    this->m_mutex.lock();
    this->m_audioBuffer.clear();
    this->m_bufferNotFull.wakeAll();
    this->m_mutex.unlock();

    QIODevice::close();
}

bool AudioBuffer::isSequential() const
{
    return true;
}

bool AudioBuffer::open(QIODevice::OpenMode mode)
{
    this->m_mutex.lock();
    this->m_audioBuffer.clear();
    this->m_bufferNotFull.wakeAll();
    this->m_mutex.unlock();

    return QIODevice::open(mode);
}

qint64 AudioBuffer::pos() const
{
    return 0;
}

bool AudioBuffer::seek(qint64 pos)
{
    Q_UNUSED(pos)

    return false;
}

qint64 AudioBuffer::size() const
{
    return this->bytesAvailable();
}

qint64 AudioBuffer::writePacket(const QbPacket &packet)
{
    qint64 bufferSize = 0;

    this->m_mutex.lock();

    if (this->isOpen()) {
        if (this->m_audioBuffer.size() >= this->m_maxSize)
            this->m_bufferNotFull.wait(&this->m_mutex);

        bufferSize = packet.bufferSize();

        if (bufferSize > 0 && packet.buffer()) {
            this->m_audioBuffer.append(const_cast<char *>(packet.buffer().data()), bufferSize);
            emit this->bytesWritten(bufferSize);
            emit this->readyRead();
        }
    }

    this->m_mutex.unlock();

    return bufferSize;
}

qint64 AudioBuffer::readData(char *data, qint64 maxSize)
{
    if (this->isOpen()) {
        this->m_mutex.lock();
        int bufferSize = this->m_audioBuffer.size();
        this->m_mutex.unlock();

        if (data) {
            int size = qMin((qint64) bufferSize, maxSize);

            if (size) {
                this->m_mutex.lock();
                memcpy(data, this->m_audioBuffer.constData(), size);
                this->m_audioBuffer.remove(0, size);
                this->m_mutex.unlock();

                bufferSize -= size;

                emit this->bytesConsumed();
            }

            maxSize = size;
        }
        else
            maxSize = 0;

        if (bufferSize < this->m_maxSize) {
            this->m_mutex.lock();
            this->m_bufferNotFull.wakeAll();
            this->m_mutex.unlock();

            if (bufferSize < 1)
                emit this->cleared();
        }
    }
    else
        maxSize = 0;

    return maxSize;
}

qint64 AudioBuffer::writeData(const char *data, qint64 maxSize)
{
    Q_UNUSED(data)
    Q_UNUSED(maxSize)

    return 0;
}

void AudioBuffer::setMaxSize(qint64 maxSize)
{
    this->m_maxSize = maxSize;
}

void AudioBuffer::resetMaxSize()
{
    this->setMaxSize(1024);
}

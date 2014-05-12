/* Webcamod, webcam capture plasmoid.
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

qint64 AudioBuffer::readData(char *data, qint64 maxSize)
{
    this->m_mutex.lock();

    if (this->isOpen()) {
        if (data) {
            memset(data, 0, maxSize);
            qint64 bytes = qMin((qint64) this->m_audioBuffer.size(), maxSize);
            memcpy(data, this->m_audioBuffer.constData(), bytes);
            this->m_audioBuffer.remove(0, bytes);
        }

        if (this->m_audioBuffer.size() < this->m_maxSize) {
            this->m_bufferNotFull.wakeAll();

            if (this->m_audioBuffer.size() < 1)
                emit this->cleared();
        }
    }
    else
        maxSize = -1;

    this->m_mutex.unlock();

    return maxSize;
}

qint64 AudioBuffer::writeData(const char *data, qint64 maxSize)
{
    this->m_mutex.lock();

    if (this->isOpen()) {
        if (this->m_audioBuffer.size() >= this->m_maxSize) {
            this->m_bufferNotFull.wait(&this->m_mutex);
        }

        if (maxSize > 0 && data) {
            this->m_audioBuffer.append(data, maxSize);
            emit this->bytesWritten(maxSize);
            emit this->readyRead();
        }
    }
    else
        maxSize = -1;

    this->m_mutex.unlock();

    return maxSize;
}

void AudioBuffer::setMaxSize(qint64 maxSize)
{
    this->m_maxSize = maxSize;
}

void AudioBuffer::resetMaxSize()
{
    this->setMaxSize(1024);
}

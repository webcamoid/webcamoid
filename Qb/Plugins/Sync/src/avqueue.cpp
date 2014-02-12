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

#include "avqueue.h"

AVQueue::AVQueue(QObject *parent): QObject(parent)
{
    this->resetMaxSize();
}

AVQueue::~AVQueue()
{
}

qint64 AVQueue::size(const QString &mimeType, bool lock)
{
    if (lock)
        this->m_mutex.lock();

    qint64 size = 0;

    if (mimeType.isEmpty())
    {
        foreach (int s, this->m_queueSize.values())
            size += s;
    }
    else if (this->m_queueSize.contains(mimeType))
        size = this->m_queueSize[mimeType];

    if (lock)
        this->m_mutex.unlock();

    return size;
}

qint64 AVQueue::maxSize() const
{
    return this->m_maxSize;
}

QbPacket AVQueue::read(QString mimeType)
{
    QbPacket packet;

    this->m_mutex.lock();

    foreach (QbPacket qPacket, this->m_avQueue)
        if (qPacket.caps().mimeType() == mimeType)
        {
            packet = qPacket;

            break;
        }

    this->m_mutex.unlock();

    return packet;
}

QbPacket AVQueue::dequeue(QString mimeType)
{
    QbPacket packet;
    int i = 0;

    this->m_mutex.lock();

    foreach (QbPacket qPacket, this->m_avQueue)
    {
        if (qPacket.caps().mimeType() == mimeType)
        {
            packet = qPacket;

            break;
        }

        i++;
    }

    if (packet)
    {
        this->m_avQueue.removeAt(i);
        qint64 dataSize = packet.bufferSize();
        this->m_queueSize[mimeType] -= dataSize;
        this->m_queueNotFull.wakeAll();
    }

    this->m_mutex.unlock();

    return packet;
}

void AVQueue::clear()
{
    this->m_mutex.lock();
    this->m_avQueue.clear();
    this->m_queueSize.clear();
    this->m_queueNotFull.wakeAll();
    this->m_mutex.unlock();
}

void AVQueue::enqueue(const QbPacket &packet)
{
    this->m_mutex.lock();

    int bufferSize = this->size("", false);

    if (bufferSize >= this->m_maxSize)
        this->m_queueNotFull.wait(&this->m_mutex);

    this->m_avQueue.enqueue(packet);
    QString mimeType = packet.caps().mimeType();
    qint64 dataSize = packet.bufferSize();

    if (this->m_queueSize.contains(mimeType))
        this->m_queueSize[mimeType] += dataSize;
    else
        this->m_queueSize[mimeType] = dataSize;

    this->m_mutex.unlock();
}

void AVQueue::setMaxSize(qint64 size)
{
    this->m_maxSize = size;
}

void AVQueue::resetMaxSize()
{
    this->setMaxSize(15 * 1024 * 1024);
}

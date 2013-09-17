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
    this->resetSize();
}

int AVQueue::size() const
{
    return this->m_size;
}

bool AVQueue::isEmpty(QString mimeType)
{
    this->m_queueMutex.lock();

    bool isEmpty;

    if (mimeType == "audio/x-raw")
        isEmpty = this->m_audioQueue.isEmpty();
    else
        isEmpty = this->m_videoQueue.isEmpty();

    this->m_queueMutex.unlock();

    return isEmpty;
}

QbPacket AVQueue::dequeue(QString mimeType)
{
    QbPacket packet;

    qDebug() << "aq:" << this->m_audioQueue.size() << "vq:" << this->m_videoQueue.size();

    this->m_queueMutex.lock();

    if (mimeType == "audio/x-raw")
        packet = this->m_audioQueue.dequeue();
    else
        packet = this->m_videoQueue.dequeue();

    if (this->m_audioQueue.size() < this->size() ||
        this->m_videoQueue.size() < this->size())
        this->m_enqueueMutex.unlock();

    this->m_queueMutex.unlock();

    return packet;
}

QbPacket AVQueue::check(QString mimeType)
{
    QbPacket packet;

    this->m_queueMutex.lock();

    if (mimeType == "audio/x-raw")
        packet = this->m_audioQueue.head();
    else
        packet = this->m_videoQueue.head();

    this->m_queueMutex.unlock();

    return packet;
}

void AVQueue::enqueue(const QbPacket &packet)
{
    this->m_enqueueMutex.lock();
    this->m_enqueueMutex.unlock();

    this->m_queueMutex.lock();

    if (packet.caps().mimeType() == "audio/x-raw")
        this->m_audioQueue.enqueue(packet);
    else
        this->m_videoQueue.enqueue(packet);

    if (this->m_audioQueue.size() >= this->size() &&
        this->m_videoQueue.size() >= this->size())
        this->m_enqueueMutex.lock();

    this->m_queueMutex.unlock();
}

void AVQueue::setSize(int size)
{
    this->m_size = size;
}

void AVQueue::resetSize()
{
    this->setSize(5);
}

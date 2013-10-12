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
    this->m_fill = false;

    this->m_log = true;
}

AVQueue::~AVQueue()
{
}

int AVQueue::size(const QString &mimeType)
{
    this->m_queueMutex.lock();

    int size;

    if (mimeType == "audio/x-raw")
        size = this->m_audioQueue.size();
    else if (mimeType == "video/x-raw")
        size = this->m_videoQueue.size();
    else
        size = this->m_audioQueue.size() + this->m_videoQueue.size();

    this->m_queueMutex.unlock();

    return size;
}

int AVQueue::maxSize() const
{
    return this->m_maxSize;
}

QbPacket AVQueue::dequeue(QString mimeType)
{
    if (mimeType == "audio/x-raw")
        return this->dequeueAudio();
    else if (mimeType == "video/x-raw")
        return this->dequeueVideo();

    return QbPacket();
}

QbPacket AVQueue::dequeueAudio()
{
    this->m_aoMutex.lock();

    if (this->size("audio/x-raw") < 1)
        this->m_audioQueueNotEmpty.wait(&this->m_aoMutex);

    this->m_queueMutex.lock();

    QbPacket packet;

    if (this->m_audioQueue.size() > 0)
        packet = this->m_audioQueue.dequeue();

    this->m_queueMutex.unlock();

    this->m_iMutex.lock();
    this->m_queueNotFull.wakeAll();
    this->m_iMutex.unlock();

    this->m_aoMutex.unlock();

    return packet;
}

QbPacket AVQueue::dequeueVideo()
{
    this->m_voMutex.lock();

    if (this->size("video/x-raw") < 1)
        this->m_videoQueueNotEmpty.wait(&this->m_voMutex);

    this->m_queueMutex.lock();
    QbPacket packet = this->m_videoQueue.dequeue();
    this->m_queueMutex.unlock();

    this->m_iMutex.lock();
    this->m_queueNotFull.wakeAll();
    this->m_iMutex.unlock();

    this->m_voMutex.unlock();

    return packet;
}

void AVQueue::enqueue(const QbPacket &packet)
{
    this->m_iMutex.lock();

    int bufferSize = this->size();

    if (bufferSize < 1)
        this->m_fill = true;

    if (bufferSize >= this->m_maxSize)
    {
        if (this->m_fill)
        {
            if (this->size("audio/x-raw") > 0)
            {
                this->m_aoMutex.lock();
                this->m_audioQueueNotEmpty.wakeAll();
                this->m_aoMutex.unlock();
            }

            if (this->size("video/x-raw") > 0)
            {
                this->m_voMutex.lock();
                this->m_videoQueueNotEmpty.wakeAll();
                this->m_voMutex.unlock();
            }

            this->m_fill = false;
        }

        this->m_queueNotFull.wait(&this->m_iMutex);
    }

    this->m_iMutex.unlock();

    this->m_queueMutex.lock();

    if (packet.caps().mimeType() == "audio/x-raw")
        this->m_audioQueue.enqueue(packet);
    else if (packet.caps().mimeType() == "video/x-raw")
        this->m_videoQueue.enqueue(packet);

    this->m_queueMutex.unlock();

    if (this->m_fill && this->m_log)
        qDebug() << QString("filling buffer %1").arg(100.0
                                                     * this->size()
                                                     / this->m_maxSize, 3, 'f', 1).toStdString().c_str();

    if (!this->m_fill)
    {
        if (this->size("audio/x-raw") > 0)
        {
            this->m_aoMutex.lock();
            this->m_audioQueueNotEmpty.wakeAll();
            this->m_aoMutex.unlock();
        }

        if (this->size("video/x-raw") > 0)
        {
            this->m_voMutex.lock();
            this->m_videoQueueNotEmpty.wakeAll();
            this->m_voMutex.unlock();
        }
    }
}

void AVQueue::setMaxSize(int size)
{
    this->m_maxSize = size;
}

void AVQueue::resetMaxSize()
{
    this->setMaxSize(128);
}

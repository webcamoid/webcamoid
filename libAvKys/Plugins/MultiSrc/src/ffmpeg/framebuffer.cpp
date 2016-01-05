/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "framebuffer.h"

FrameBuffer::FrameBuffer(QObject *parent):
    QObject(parent)
{
    this->m_maxSize = 16;
}

int FrameBuffer::maxSize() const
{
    return this->m_maxSize;
}

int &FrameBuffer::maxSize()
{
    return this->m_maxSize;
}

int FrameBuffer::size()
{
    this->m_mutex.lock();
    int size = this->m_buffer.size();
    this->m_mutex.unlock();

    return size;
}

AVFramePtr FrameBuffer::dequeue()
{
    this->m_mutex.lock();

    if (this->m_buffer.isEmpty())
        if (!this->m_bufferNotEmpty.wait(&this->m_mutex, THREAD_WAIT_LIMIT)) {
            this->m_mutex.unlock();

            return AVFramePtr();
        }

    AVFramePtr frame;

    if (!this->m_buffer.isEmpty()) {
        frame = this->m_buffer.dequeue();
        emit this->sizeChanged(this->m_buffer.size());

        if (this->m_buffer.size() < this->m_maxSize)
            this->m_bufferNotFull.wakeAll();
    }

    this->m_mutex.unlock();

    return frame;
}

void FrameBuffer::deleteFrame(AVFrame *frame)
{
    av_frame_unref(frame);
    av_frame_free(&frame);
}

void FrameBuffer::setMaxSize(int maxSize)
{
    if (this->m_maxSize == maxSize)
        return;

    this->m_maxSize = maxSize;
    emit this->maxSizeChanged(maxSize);
}

void FrameBuffer::resetMaxSize()
{
    this->setMaxSize(16);
}

void FrameBuffer::enqueue(AVFrame *frame)
{
    this->m_mutex.lock();

    if (this->m_buffer.size() >= this->m_maxSize)
        this->m_bufferNotFull.wait(&this->m_mutex);

    this->m_buffer.enqueue(AVFramePtr(frame, this->deleteFrame));
    this->m_bufferNotEmpty.wakeAll();
    emit this->sizeChanged(this->m_buffer.size());

    this->m_mutex.unlock();
}

void FrameBuffer::clear()
{
    this->m_mutex.lock();
    this->m_bufferNotEmpty.wakeAll();
    this->m_bufferNotFull.wakeAll();
    this->m_buffer.clear();
    this->m_mutex.unlock();
}

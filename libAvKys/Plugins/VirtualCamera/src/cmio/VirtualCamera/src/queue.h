/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#ifndef QUEUE_H
#define QUEUE_H

#include <memory>
#include <CoreMedia/CMSimpleQueue.h>

namespace AkVCam
{
    template <typename T>
    class Queue;
    template <typename T>
    using QueuePtr = std::shared_ptr<Queue<T>>;

    template <typename T>
    class Queue
    {
        public:
            Queue(int32_t capacity):
                m_queue(nullptr)
            {
                CMSimpleQueueCreate(kCFAllocatorDefault,
                                    capacity,
                                    &this->m_queue);
            }

            ~Queue()
            {
                CFRelease(this->m_queue);
            }

            void enqueue(const T &t)
            {
                CMSimpleQueueEnqueue(this->m_queue,
                                     t);
            }

            T dequeue()
            {
                return CMSimpleQueueDequeue(this->m_queue);
            }

            void clear()
            {
                CMSimpleQueueReset(this->m_queue);
            }

            int32_t capacity()
            {
                return CMSimpleQueueGetCapacity(this->m_queue);
            }

            int32_t count()
            {
                return CMSimpleQueueGetCount(this->m_queue);
            }

            int32_t size()
            {
                return this->count();
            }

            Float32 fullness()
            {
                return CMSimpleQueueGetFullness(this->m_queue);
            }

            CFTypeID typeID()
            {
                return CMSimpleQueueGetTypeID();
            }

            CMSimpleQueueRef ref()
            {
                return m_queue;
            }

        private:
            CMSimpleQueueRef m_queue;
    };
}

#endif // QUEUE_H

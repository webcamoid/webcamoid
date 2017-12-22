/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef STREAM_H
#define STREAM_H

#include <thread>

#include "object.h"
#include "clock.h"
#include "queue.h"

namespace AkVCam
{
    class Stream;
    typedef std::shared_ptr<Stream> StreamPtr;
    typedef Queue<CMSampleBufferRef> SampleBufferQueue;
    typedef QueuePtr<CMSampleBufferRef> SampleBufferQueuePtr;

    class Stream: public Object
    {
        public:
            Stream(bool registerObject=false, Object *m_parent=nullptr);
            ~Stream();

            OSStatus createObject();
            OSStatus registerObject(bool regist=true);
            void setFormats(const std::vector<VideoFormat> &formats);
            void setFormat(const VideoFormat &format);
            void setFrameRate(Float64 frameRate);
            bool start();
            void stop();
            bool running();

            // Stream Interface
            OSStatus copyBufferQueue(CMIODeviceStreamQueueAlteredProc queueAlteredProc,
                                     void *queueAlteredRefCon,
                                     CMSimpleQueueRef *queue);
            OSStatus deckPlay();
            OSStatus deckStop();
            OSStatus deckJog(SInt32 speed);
            OSStatus deckCueTo(Float64 frameNumber, Boolean playOnCue);

        private:
            ClockPtr m_clock;
            SampleBufferQueuePtr m_queue;
            CMIODeviceStreamQueueAlteredProc m_queueAltered;
            VideoFormat m_format;
            void *m_queueAlteredRefCon;
            CFRunLoopTimerRef m_timer;
            bool m_running;

            static void streamLoop(CFRunLoopTimerRef timer, void *info);
    };
}

#endif // STREAM_H

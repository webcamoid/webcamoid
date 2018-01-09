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
#include "../VCamUtils/src/image/videoframe.h"

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

            void frameReady(const VideoFrame &frame);
            void setBroadcasting(bool broadcasting);
            void setMirror(bool horizontalMirror, bool verticalMirror);
            void setScaling(VideoFrame::Scaling scaling);
            void setAspectRatio(VideoFrame::AspectRatio aspectRatio);

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
            UInt64 m_sequence;
            CMTime m_pts;
            SampleBufferQueuePtr m_queue;
            CMIODeviceStreamQueueAlteredProc m_queueAltered;
            VideoFormat m_format;
            double m_fps;
            VideoFrame m_currentFrame;
            VideoFrame m_testFrame;
            VideoFrame m_testFrameAdapted;
            void *m_queueAlteredRefCon;
            CFRunLoopTimerRef m_timer;
            bool m_running;
            bool m_broadcasting;
            bool m_horizontalMirror;
            bool m_verticalMirror;
            VideoFrame::Scaling m_scaling;
            VideoFrame::AspectRatio m_aspectRatio;
            std::mutex m_mutex;

            bool startTimer();
            void stopTimer();
            static void streamLoop(CFRunLoopTimerRef timer, void *info);
            void sendFrame(const VideoFrame &frame);
            void updateTestFrame();
    };
}

#endif // STREAM_H

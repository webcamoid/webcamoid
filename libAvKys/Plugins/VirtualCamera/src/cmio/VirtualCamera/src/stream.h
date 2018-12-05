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

#ifndef STREAM_H
#define STREAM_H

#include "ipcbridge.h"
#include "VCamUtils/src/image/videoframetypes.h"
#include "VCamUtils/src/fraction.h"
#include "object.h"
#include "queue.h"

namespace AkVCam
{
    class StreamPrivate;
    class Stream;
    class VideoFrame;
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
            void setFrameRate(const Fraction &frameRate);
            bool start();
            void stop();
            bool running();

            void serverStateChanged(IpcBridge::ServerState state);
            void frameReady(const VideoFrame &frame);
            void setBroadcasting(const std::string &broadcaster);
            void setMirror(bool horizontalMirror, bool verticalMirror);
            void setScaling(Scaling scaling);
            void setAspectRatio(AspectRatio aspectRatio);
            void setSwapRgb(bool swap);

            // Stream Interface
            OSStatus copyBufferQueue(CMIODeviceStreamQueueAlteredProc queueAlteredProc,
                                     void *queueAlteredRefCon,
                                     CMSimpleQueueRef *queue);
            OSStatus deckPlay();
            OSStatus deckStop();
            OSStatus deckJog(SInt32 speed);
            OSStatus deckCueTo(Float64 frameNumber, Boolean playOnCue);

        private:
            StreamPrivate *d;

            friend class StreamPrivate;
    };
}

#endif // STREAM_H

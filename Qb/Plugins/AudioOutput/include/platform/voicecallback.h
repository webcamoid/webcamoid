/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#ifndef VOICECALLBACK_H
#define VOICECALLBACK_H

#include <QMutex>
#include <QWaitCondition>
#include <xaudio2.h>

struct Lock
{
    QMutex m_mutex;
    QWaitCondition m_waitForBufferEnd;
};

class VoiceCallback: public IXAudio2VoiceCallback
{
    public:
        VoiceCallback();
        ~VoiceCallback();

        //Unused methods are stubs
        void STDMETHODCALLTYPE OnVoiceProcessingPassEnd();
        void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 SamplesRequired);
        void STDMETHODCALLTYPE OnStreamEnd();
        void STDMETHODCALLTYPE OnBufferEnd(void *pBufferContext);
        void STDMETHODCALLTYPE OnBufferStart(void *pBufferContext);
        void STDMETHODCALLTYPE OnLoopEnd(void *pBufferContext);
        void STDMETHODCALLTYPE OnVoiceError(void *pBufferContext, HRESULT Error);

    private:
        HANDLE m_hBufferEndEvent;
};

#endif // VOICECALLBACK_H

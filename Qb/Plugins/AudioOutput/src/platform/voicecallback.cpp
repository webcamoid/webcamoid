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

#include "platform/voicecallback.h"

VoiceCallback::VoiceCallback():
    m_hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
{

}

VoiceCallback::~VoiceCallback()
{
    CloseHandle(this->m_hBufferEndEvent);
}

void VoiceCallback::OnVoiceProcessingPassEnd()
{
}

void VoiceCallback::OnVoiceProcessingPassStart(UINT32 SamplesRequired)
{
    Q_UNUSED(SamplesRequired)
}

//Called when the voice has just finished playing a contiguous audio stream.
void VoiceCallback::OnStreamEnd()
{
    SetEvent(this->m_hBufferEndEvent);
}

void VoiceCallback::OnBufferEnd(void *pBufferContext)
{
    Lock *lock = (Lock *) pBufferContext;

    lock->m_mutex.lock();
    lock->m_waitForBufferEnd.wakeAll();
    lock->m_mutex.unlock();
}

void VoiceCallback::OnBufferStart(void *pBufferContext)
{
    Q_UNUSED(pBufferContext)
}

void VoiceCallback::OnLoopEnd(void *pBufferContext)
{
    Q_UNUSED(pBufferContext)
}

void VoiceCallback::OnVoiceError(void *pBufferContext, HRESULT Error)
{
    Q_UNUSED(pBufferContext)
    Q_UNUSED(Error)
}

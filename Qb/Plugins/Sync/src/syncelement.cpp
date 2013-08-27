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

#include "sleep.h"
#include "syncelement.h"

SyncElement::SyncElement(): QbElement()
{
    this->m_fst = true;
    this->m_ready = false;
}

void SyncElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        this->state() != ElementStatePlaying)
        return;

    if (!this->m_ready)
    {
        this->m_ready = true;
        emit this->ready(packet.index());
    }

    this->m_globlLock.wait();

    if (this->m_fst)
    {
        this->m_audioClock.init(true);
        this->m_videoClock.init(true);
        this->m_extrnClock.init();

        this->m_globlLock.init(2);

        this->m_fst = false;
    }

    QString streamType = packet.caps().mimeType();

    if (streamType == "audio/x-raw")
        QtConcurrent::run(this, &SyncElement::processAudioFrame, packet);
    else
        QtConcurrent::run(this, &SyncElement::processVideoFrame, packet);
}

void SyncElement::setState(ElementState state)
{
    QbElement::setState(state);

    if (this->state() == QbElement::ElementStateReady ||
        this->state() == QbElement::ElementStateNull)
    {
        this->m_ready = false;
        this->m_fst = true;
    }
}

void SyncElement::processAudioFrame(const QbPacket &packet)
{
    emit this->oStream(packet);
}

void SyncElement::processVideoFrame(const QbPacket &packet)
{
    this->m_videoLock.lock();
    this->m_globlLock.lock();

    double clock = this->m_extrnClock.clock();
    double pts = this->m_videoClock.clock(packet.pts() * packet.timeBase().value());
    double diff = clock - pts;
    bool show = false;

    if (fabs(diff) < AV_SYNC_THRESHOLD_MIN)
    {
        show = true;
        emit this->oStream(packet);
    }
    else if (fabs(diff) < AV_SYNC_THRESHOLD_MAX)
    {
        if (diff < 0)
        {
            // Add a delay
            show = true;
            Sleep::usleep(1e6 * abs(diff));

            emit this->oStream(packet);
        }
        else
        {
            // Discard frame
        }
    }
    else
    {
        // Resync to the master clock
        show = true;
        this->m_videoClock.syncTo(clock);

        emit this->oStream(packet);
    }
/*
    print(packet['mimeType'][0],
          '{0:.2f}'.format(clock),
          '{0:.2f}'.format(pts),
          '{0:.2f}'.format(diff),
          show)
*/
    this->m_globlLock.unlock();
    this->m_videoLock.unlock();
}

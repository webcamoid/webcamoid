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

    if (packet.caps().property("sync").toBool())
        this->sendFrame(packet);
    else
        emit this->oStream(packet);
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

void SyncElement::sendFrame(const QbPacket &packet)
{
    if (this->m_fst)
    {
        this->m_iPts = packet.pts() * packet.timeBase().value();
        this->m_duration = packet.duration() * packet.timeBase().value();
        this->m_elapsedTimer.start();
        emit this->oStream(packet);
        Sleep::usleep(1e6 * this->m_duration);
        this->m_fst = false;
    }
    else
    {
        double pts = packet.pts() * packet.timeBase().value();

        int clockN = this->m_elapsedTimer.nsecsElapsed() / 1.0e9 / this->m_duration;
        int packetN = (pts - this->m_iPts) / this->m_duration;

        if (packetN < clockN)
        {
            int diff = clockN - packetN;

            if (diff > 5)
            {
                this->m_iPts = packet.pts() * packet.timeBase().value();
                this->m_duration = packet.duration() * packet.timeBase().value();
                this->m_elapsedTimer.restart();
                emit this->oStream(packet);
                Sleep::usleep(1e6 * this->m_duration);
            }
        }
        else if (packetN > clockN)
        {
            double duration = 1.0e6 * (pts - this->m_iPts) - this->m_elapsedTimer.nsecsElapsed() / 1.0e3;

            if (duration < 0)
                duration = 0;

            Sleep::usleep(duration);
            emit this->oStream(packet);
        }
        else
        {
            double duration = pts - this->m_iPts + this->m_duration - this->m_elapsedTimer.nsecsElapsed() / 1.0e9;

            if (duration < 0)
                duration = 0;

            emit this->oStream(packet);
            Sleep::usleep(1e3 * duration);
        }
    }
}

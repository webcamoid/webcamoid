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
 * Web-Site: http://webcamoid.github.io/
 */

#include "rtptselement.h"

RtPtsElement::RtPtsElement(): AkElement()
{
    this->m_prevPts = -1;
    this->m_fps = AkFrac(30000, 1001);
    this->m_timeBase = this->m_fps.invert();
    this->m_timer.setInterval(1e3 * this->m_fps.invert().value());

    QObject::connect(&this->m_timer,
                     &QTimer::timeout,
                     this,
                     &RtPtsElement::readPacket);
}

RtPtsElement::~RtPtsElement()
{
    this->uninit();
}

AkFrac RtPtsElement::fps() const
{
    return this->m_fps;
}

void RtPtsElement::sendPacket(RtPtsElement *element,
                              const AkVideoPacket &packet)
{
    emit element->oStream(packet.toPacket());
}

void RtPtsElement::stateChange(AkElement::ElementState from, AkElement::ElementState to)
{
    if (from == AkElement::ElementStateNull
        && to == AkElement::ElementStatePaused)
        this->init();
    else if (from == AkElement::ElementStatePaused
             && to == AkElement::ElementStateNull)
        this->uninit();
}

void RtPtsElement::setFps(const AkFrac &fps)
{
    if (this->m_fps == fps)
        return;

    this->m_fps = fps.num() && fps.den()? fps: AkFrac(30000, 1001);
    this->m_timeBase = this->m_fps.invert();
    this->m_timer.setInterval(1e3 * this->m_fps.invert().value());
    emit this->fpsChanged(fps);
}

void RtPtsElement::resetFps()
{
    this->setFps(AkFrac(30000, 1001));
}

AkPacket RtPtsElement::iStream(const AkVideoPacket &packet)
{
    this->m_mutex.lock();
    this->m_inPacket = packet;
    this->m_mutex.unlock();

    return packet.toPacket();
}

bool RtPtsElement::init()
{
    this->m_prevPts = -1;
    this->m_elapsedTimer.start();
    this->m_timer.start();

    return true;
}

void RtPtsElement::uninit()
{
    this->m_timer.stop();
    this->m_threadStatus.waitForFinished();
}

void RtPtsElement::readPacket()
{
    if (!this->m_threadStatus.isRunning()) {
        this->m_mutex.lock();
        this->m_curPacket = this->m_inPacket;
        this->m_mutex.unlock();

        if (!this->m_curPacket)
            return;

        qint64 pts = 1.0e-3 * this->m_elapsedTimer.elapsed() * this->m_fps.value();

        if (pts == this->m_prevPts)
            return;

        this->m_prevPts = pts;
        this->m_curPacket.caps().fps() = this->m_fps;
        this->m_curPacket.pts() = pts;
        this->m_curPacket.timeBase() = this->m_timeBase;

        this->m_threadStatus = QtConcurrent::run(&this->m_threadPool,
                                                 this->sendPacket,
                                                 this,
                                                 this->m_curPacket);
    }
}

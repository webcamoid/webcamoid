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

#include "rtptselement.h"

RtPtsElement::RtPtsElement(): QbElement()
{
    this->m_thread = new QThread();
    this->m_thread->start();
    this->m_timer.moveToThread(this->m_thread);

    QObject::connect(&this->m_timer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(readPacket()),
                     Qt::DirectConnection);


    this->m_pts = 0;
    this->resetFps();
}

RtPtsElement::~RtPtsElement()
{
    QMetaObject::invokeMethod(&this->m_timer, "stop");

    if (this->m_thread) {
        this->m_thread->quit();
        this->m_thread->wait();
        delete this->m_thread;
        this->m_thread = NULL;
    }
}

QbFrac RtPtsElement::fps() const
{
    return this->m_fps;
}

void RtPtsElement::setFps(const QbFrac &fps)
{
    this->m_fps = fps;

    int interval = fps.num()?
                       fps.invert().value():
                       INT_MAX;

    this->m_ptsInc = fps.den();
    this->m_timeBase = QbFrac(1, fps.num());
    this->m_timer.setInterval(interval);
}

void RtPtsElement::resetFps()
{
    this->setFps(QbFrac(30, 1));
}

void RtPtsElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);

    if (this->state() == ElementStatePlaying)
       QMetaObject::invokeMethod(&this->m_timer, "start");
    else {
        QMetaObject::invokeMethod(&this->m_timer, "stop");

        if (this->state() == ElementStateNull)
            this->m_pts = 0;
    }
}

void RtPtsElement::iStream(const QbPacket &packet)
{
    this->m_mutex.lock();
    this->m_curPacket = packet;
    this->m_mutex.unlock();
}

void RtPtsElement::readPacket()
{
    this->m_mutex.lock();

    if (!this->m_curPacket) {
        this->m_mutex.unlock();

        return;
    }

    QbPacket packet = this->m_curPacket;
    this->m_mutex.unlock();

    packet.setPts(this->m_pts);
    packet.setTimeBase(this->m_timeBase);
    this->m_pts += this->m_ptsInc;

    emit this->oStream(packet);
}

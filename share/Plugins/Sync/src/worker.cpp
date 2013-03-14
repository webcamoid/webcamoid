/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

#include "worker.h"
#include "sleep.h"

Worker::Worker(QObject *parent): QObject(parent)
{
    this->m_state = QbElement::ElementStateNull;
}

Worker::~Worker()
{
    this->setState(QbElement::ElementStateNull);
}

QbElement::ElementState Worker::state() const
{
    return this->m_state;
}

void Worker::doWork()
{
    while (this->state() == QbElement::ElementStatePlaying)
        while (!this->m_queue.isEmpty())
        {
            PacketInfo *packetInfo = this->m_queue.dequeue();
            QbPacket oPacket = packetInfo->packet();
            this->m_data = packetInfo->data();
            delete packetInfo;

            oPacket.setData(this->m_data.constData());
            emit this->oStream(oPacket);
            ulong wait = 1e6 * oPacket.duration() * oPacket.timeBase().value();

            Sleep::usleep(wait);
        }
}

void Worker::appendPacketInfo(const PacketInfo &packetInfo)
{
    this->m_queue.enqueue(new PacketInfo(packetInfo));
}

void Worker::setState(QbElement::ElementState state)
{
    QbElement::ElementState preState = this->m_state;
    this->m_state = state;

    if (preState != QbElement::ElementStatePlaying &&
        state == QbElement::ElementStatePlaying)
        this->doWork();
}

void Worker::resetState()
{
    this->setState(QbElement::ElementStateNull);
}

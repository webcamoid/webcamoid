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

#include "syncelement.h"

SyncElement::SyncElement(): QbElement()
{
    this->m_ready = false;

    this->m_worker = new Worker;
    this->m_thread = new QThread(this);

    QObject::connect(this->m_worker,
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SIGNAL(oStream(const QbPacket &)));

    QObject::connect(this->m_thread,
                     SIGNAL(finished()),
                     this->m_worker,
                     SLOT(deleteLater()));

    this->m_worker->moveToThread(this->m_thread);
    this->m_thread->start();
    this->resetWaitUnlock();
}

SyncElement::~SyncElement()
{
    this->m_worker->setState(ElementStateNull);
    this->m_thread->quit();
    this->m_thread->wait();
}

bool SyncElement::waitUnlock() const
{
    return this->m_worker->waitUnlock();
}

void SyncElement::setWaitUnlock(bool waitUnlock)
{
    this->m_worker->setWaitUnlock(waitUnlock);
}

void SyncElement::resetWaitUnlock()
{
    this->m_worker->resetWaitUnlock();
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

    this->m_worker->appendPacketInfo(PacketInfo(packet,
                                                QByteArray((const char *) packet.data(),
                                                           packet.dataSize())));
}

void SyncElement::setState(ElementState state)
{
    QbElement::setState(state);

    if (this->state() == ElementStateNull ||
        this->state() == ElementStateReady)
        this->m_ready = false;

    QMetaObject::invokeMethod(this->m_worker,
                              "setState",
                              Qt::QueuedConnection,
                              Q_ARG(QbElement::ElementState, state));
}

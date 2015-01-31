/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "outputthread.h"

OutputThread::OutputThread(QObject *parent):
    QThread(parent)
{

}

OutputThread::~OutputThread()
{

}

void OutputThread::run()
{
    while (!this->isInterruptionRequested()) {
        this->m_mutex.lock();

        if (!this->m_packet)
            this->m_packetReady.wait(&this->m_mutex, 1000);

        if (this->m_packet) {
            emit this->oStream(this->m_packet);
            this->m_packet = QbPacket();
        }

        this->m_mutex.unlock();
    }
}

void OutputThread::setPacket(const QbPacket &packet)
{
    this->m_mutex.lock();
    this->m_packet = packet;
    this->m_packetReady.wakeAll();
    this->m_mutex.unlock();
}

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

#include "probeelement.h"

ProbeElement::ProbeElement(): QbElement()
{
    this->resetLog();
}

ProbeElement::~ProbeElement()
{
}

bool ProbeElement::log() const
{
    return this->m_log;
}

void ProbeElement::setLog(bool on)
{
    this->m_log = on;
}

void ProbeElement::resetLog()
{
    this->setLog(true);
}

void ProbeElement::iStream(const QbPacket &packet)
{
    if (this->state() != ElementStatePlaying)
        return;

    if (this->log())
    {
        QString packetInfo = QString("%1: %2\n"
                                     "\tData Size: %3\n"
                                     "\tDts      : %4\n"
                                     "\tPts      : %5\n"
                                     "\tDuration : %6\n"
                                     "\tTime Base: %7\n"
                                     "\tIndex    : %8\n").arg(this->objectName())
                                                         .arg(packet.caps().toString())
                                                         .arg(packet.dataSize())
                                                         .arg(packet.dts())
                                                         .arg(packet.pts())
                                                         .arg(packet.duration())
                                                         .arg(packet.timeBase().toString())
                                                         .arg(packet.index());

        qDebug() << packetInfo.toUtf8().constData();
    }

    emit this->oStream(packet);
}

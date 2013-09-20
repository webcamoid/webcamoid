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

#include "probeelement.h"

ProbeElement::ProbeElement(): QbElement()
{
    this->resetLog();
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
    if (this->log())
    {
        qDebug().nospace() << "\"" << this->objectName().toStdString().c_str() << "\"";

        foreach (QString line, packet.toString().split('\n'))
            qDebug().nospace() << "\t"
                               << line.toStdString().c_str();
    }

    emit this->oStream(packet);
}

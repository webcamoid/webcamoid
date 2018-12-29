/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <QDebug>
#include <akpacket.h>

#include "probeelement.h"

class ProbeElementPrivate
{
    public:
        bool m_log {false};
};

ProbeElement::ProbeElement(): AkElement()
{
    this->d = new ProbeElementPrivate;
}

ProbeElement::~ProbeElement()
{
    delete this->d;
}

bool ProbeElement::log() const
{
    return this->d->m_log;
}

void ProbeElement::setLog(bool log)
{
    if (this->d->m_log == log)
        return;

    this->d->m_log = log;
    emit this->logChanged(log);
}

void ProbeElement::resetLog()
{
    this->setLog(false);
}

AkPacket ProbeElement::iStream(const AkPacket &packet)
{
    if (this->d->m_log) {
        qDebug().nospace() << "\"" << this->objectName().toStdString().c_str() << "\"";

        for (const QString &line: packet.toString().split('\n'))
            qDebug().nospace() << "\t"
                               << line.toStdString().c_str();
    }

    akSend(packet);
}

#include "moc_probeelement.cpp"

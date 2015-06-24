/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "multiplexelement.h"

MultiplexElement::MultiplexElement(): QbElement()
{
    this->resetInputIndex();
    this->resetOutputIndex();
    this->resetCaps();
}

int MultiplexElement::inputIndex() const
{
    return this->m_inputIndex;
}

int MultiplexElement::outputIndex() const
{
    return this->m_outputIndex;
}

QString MultiplexElement::caps() const
{
    return this->m_caps;
}

void MultiplexElement::setInputIndex(int method)
{
    this->m_inputIndex = method;
}

void MultiplexElement::setOutputIndex(int params)
{
    this->m_outputIndex = params;
}

void MultiplexElement::setCaps(QString caps)
{
    this->m_caps = caps;
}

void MultiplexElement::resetInputIndex()
{
    this->setInputIndex(-1);
}

void MultiplexElement::resetOutputIndex()
{
    this->setOutputIndex(-1);
}

void MultiplexElement::resetCaps()
{
    this->setCaps("");
}

QbPacket MultiplexElement::iStream(const QbPacket &packet)
{
    if (packet.caps().isValid()
        && (this->inputIndex() < 0
            || (this->inputIndex() >= 0
                && packet.index() == this->inputIndex()))
        && (this->caps().isEmpty()
            || packet.caps().isCompatible(this->caps())))
    {
        QbPacket oPacket(packet);

        if (this->outputIndex() >= 0)
            oPacket.setIndex(this->outputIndex());

        qbSend(oPacket)
    }

    qbSend(packet)
}

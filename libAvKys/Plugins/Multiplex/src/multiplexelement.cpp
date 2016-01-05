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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "multiplexelement.h"

MultiplexElement::MultiplexElement(): AkElement()
{
    this->m_inputIndex = -1;
    this->m_outputIndex = -1;
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

AkPacket MultiplexElement::iStream(const AkPacket &packet)
{
    if (this->m_inputIndex >= 0
        && packet.index() != this->m_inputIndex)
        return AkPacket();

    if (!this->m_caps.isEmpty()
        && !packet.caps().isCompatible(this->m_caps))
        return AkPacket();

    AkPacket oPacket(packet);

    if (this->m_outputIndex >= 0)
        oPacket.setIndex(this->m_outputIndex);

    akSend(oPacket)
}

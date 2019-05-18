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

#include <akcaps.h>
#include <akpacket.h>

#include "multiplexelement.h"

class MultiplexElementPrivate
{
    public:
        AkCaps m_caps;
        int m_inputIndex {-1};
        int m_outputIndex {-1};
};

MultiplexElement::MultiplexElement(): AkElement()
{
    this->d = new MultiplexElementPrivate;
}

MultiplexElement::~MultiplexElement()
{
    delete this->d;
}

AkCaps MultiplexElement::caps() const
{
    return this->d->m_caps;
}

int MultiplexElement::inputIndex() const
{
    return this->d->m_inputIndex;
}

int MultiplexElement::outputIndex() const
{
    return this->d->m_outputIndex;
}

void MultiplexElement::setCaps(const AkCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void MultiplexElement::setInputIndex(int inputIndex)
{
    if (this->d->m_inputIndex == inputIndex)
        return;

    this->d->m_inputIndex = inputIndex;
    emit this->inputIndexChanged(inputIndex);
}

void MultiplexElement::setOutputIndex(int outputIndex)
{
    if (this->d->m_outputIndex == outputIndex)
        return;

    this->d->m_outputIndex = outputIndex;
    emit this->outputIndexChanged(outputIndex);
}

void MultiplexElement::resetCaps()
{
    this->setCaps({});
}

void MultiplexElement::resetInputIndex()
{
    this->setInputIndex(-1);
}

void MultiplexElement::resetOutputIndex()
{
    this->setOutputIndex(-1);
}

AkPacket MultiplexElement::iStream(const AkPacket &packet)
{
    if (this->d->m_inputIndex >= 0
        && packet.index() != this->d->m_inputIndex)
        return {};

    if (this->d->m_caps
        && !packet.caps().isCompatible(this->d->m_caps))
        return {};

    AkPacket oPacket(packet);

    if (this->d->m_outputIndex >= 0)
        oPacket.setIndex(this->d->m_outputIndex);

    akSend(oPacket)
}

#include "moc_multiplexelement.cpp"

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

#include <akpacket.h>
#include <akcaps.h>

#include "multiplexelement.h"

class MultiplexElementPrivate
{
    public:
        int m_inputIndex {-1};
        int m_outputIndex {-1};
        QString m_caps;
};

MultiplexElement::MultiplexElement(): AkElement()
{
    this->d = new MultiplexElementPrivate;
}

MultiplexElement::~MultiplexElement()
{
    delete this->d;
}

int MultiplexElement::inputIndex() const
{
    return this->d->m_inputIndex;
}

int MultiplexElement::outputIndex() const
{
    return this->d->m_outputIndex;
}

QString MultiplexElement::caps() const
{
    return this->d->m_caps;
}

void MultiplexElement::setInputIndex(int method)
{
    this->d->m_inputIndex = method;
}

void MultiplexElement::setOutputIndex(int params)
{
    this->d->m_outputIndex = params;
}

void MultiplexElement::setCaps(const QString &caps)
{
    this->d->m_caps = caps;
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
    if (this->d->m_inputIndex >= 0
        && packet.index() != this->d->m_inputIndex)
        return AkPacket();

    if (!this->d->m_caps.isEmpty()
        && !packet.caps().isCompatible(this->d->m_caps))
        return AkPacket();

    AkPacket oPacket(packet);

    if (this->d->m_outputIndex >= 0)
        oPacket.setIndex(this->d->m_outputIndex);

    akSend(oPacket)
}

#include "moc_multiplexelement.cpp"

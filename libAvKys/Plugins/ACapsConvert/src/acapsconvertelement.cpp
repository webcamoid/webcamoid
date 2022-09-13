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

#include <QMutex>
#include <QSharedPointer>
#include <akaudioconverter.h>
#include <akaudiopacket.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>

#include "acapsconvertelement.h"

class ACapsConvertElementPrivate
{
    public:
        AkAudioConverter m_audioConvert;
};

ACapsConvertElement::ACapsConvertElement():
    AkElement()
{
    this->d = new ACapsConvertElementPrivate;
    QObject::connect(&this->d->m_audioConvert,
                     &AkAudioConverter::outputCapsChanged,
                     this,
                     &ACapsConvertElement::capsChanged);
}

ACapsConvertElement::~ACapsConvertElement()
{
    delete this->d;
}

AkAudioCaps ACapsConvertElement::caps() const
{
    return this->d->m_audioConvert.outputCaps();
}

AkPacket ACapsConvertElement::iAudioStream(const AkAudioPacket &packet)
{
    auto oPacket = this->d->m_audioConvert.convert(packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

void ACapsConvertElement::setCaps(const AkAudioCaps &caps)
{
    this->d->m_audioConvert.setOutputCaps(caps);
}

void ACapsConvertElement::resetCaps()
{
    this->d->m_audioConvert.resetOutputCaps();
}

bool ACapsConvertElement::setState(AkElement::ElementState state)
{
    auto curState = this->state();

    if (state == AkElement::ElementStatePlaying
        && curState == AkElement::ElementStatePlaying)
        this->d->m_audioConvert.reset();

    return AkElement::setState(state);
}

#include "moc_acapsconvertelement.cpp"

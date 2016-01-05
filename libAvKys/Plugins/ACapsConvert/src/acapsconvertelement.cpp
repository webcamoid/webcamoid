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

#include "acapsconvertelement.h"

ACapsConvertElement::ACapsConvertElement(): AkElement()
{
}

QString ACapsConvertElement::caps() const
{
    return this->m_caps.toString();
}

void ACapsConvertElement::setCaps(const QString &caps)
{
    if (this->m_caps == caps)
        return;

    this->m_caps = caps;
    emit this->capsChanged(caps);
}

void ACapsConvertElement::resetCaps()
{
    this->setCaps("");
}

AkPacket ACapsConvertElement::iStream(const AkAudioPacket &packet)
{
    AkPacket oPacket = this->m_convertAudio.convert(packet, this->m_caps);

    akSend(oPacket)
}

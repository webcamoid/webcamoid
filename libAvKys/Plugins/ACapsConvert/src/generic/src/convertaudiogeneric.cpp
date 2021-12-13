/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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
#include <QMap>
#include <QMutex>
#include <akfrac.h>
#include <akpacket.h>
#include <akaudiocaps.h>
#include <akaudioconverter.h>
#include <akaudiopacket.h>

#include "convertaudiogeneric.h"

class ConvertAudioGenericPrivate
{
    public:
        AkAudioConverter m_converter;
};

ConvertAudioGeneric::ConvertAudioGeneric(QObject *parent):
    ConvertAudio(parent)
{
    this->d = new ConvertAudioGenericPrivate;
}

ConvertAudioGeneric::~ConvertAudioGeneric()
{
    this->uninit();
    delete this->d;
}

bool ConvertAudioGeneric::init(const AkAudioCaps &caps)
{
    this->d->m_converter.reset();
    this->d->m_converter.setOutputCaps(caps);

    return true;
}

AkPacket ConvertAudioGeneric::convert(const AkAudioPacket &packet)
{
    if (!this->d->m_converter.outputCaps() || packet.buffer().size() < 1)
        return {};

    return this->d->m_converter.convert(packet);
}

void ConvertAudioGeneric::uninit()
{
}

#include "moc_convertaudiogeneric.cpp"

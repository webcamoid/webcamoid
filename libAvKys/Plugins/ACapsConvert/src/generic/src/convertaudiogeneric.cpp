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

#include <QMap>
#include <QMutex>
#include <akfrac.h>
#include <akpacket.h>
#include <akaudiocaps.h>
#include <akaudiopacket.h>

#include "convertaudiogeneric.h"

class ConvertAudioGenericPrivate
{
    public:
        AkAudioCaps m_caps;
        AkAudioCaps m_previousCaps;
        QMutex m_mutex;
        qreal m_sampleCorrection {0};
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
    QMutexLocker mutexLocker(&this->d->m_mutex);
    this->d->m_caps = caps;
    this->d->m_previousCaps = AkAudioCaps();
    this->d->m_sampleCorrection = 0;

    return true;
}

AkPacket ConvertAudioGeneric::convert(const AkAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_caps || packet.buffer().size() < 1)
        return {};

    if (packet.caps() != this->d->m_previousCaps) {
        this->d->m_previousCaps = packet.caps();
        this->d->m_sampleCorrection = 0;
    }

    return packet.convertFormat(this->d->m_caps.format())
                 .convertLayout(this->d->m_caps.layout())
                 .convertPlanar(this->d->m_caps.planar())
                 .convertSampleRate(this->d->m_caps.rate(),
                                    this->d->m_sampleCorrection);
}

void ConvertAudioGeneric::uninit()
{
    QMutexLocker mutexLocker(&this->d->m_mutex);
    this->d->m_caps = AkAudioCaps();
}

#include "moc_convertaudiogeneric.cpp"

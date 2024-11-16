/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#include "akaudioencoder.h"
#include "../akaudiocaps.h"

class AkAudioEncoderPrivate
{
    public:
        AkAudioCaps m_inputCaps;
        int m_bitrate {128000};
};

AkAudioEncoder::AkAudioEncoder(QObject *parent):
    AkElement{parent}
{
    this->d = new AkAudioEncoderPrivate();
}

AkAudioEncoder::~AkAudioEncoder()
{
    delete this->d;
}

AkAudioCaps AkAudioEncoder::inputCaps() const
{
    return this->d->m_inputCaps;
}

int AkAudioEncoder::bitrate() const
{
    return this->d->m_bitrate;
}

AkCompressedPackets AkAudioEncoder::headers() const
{
    return {};
}

void AkAudioEncoder::setInputCaps(const AkAudioCaps &inputCaps)
{
    if (this->d->m_inputCaps == inputCaps)
        return;

    this->d->m_inputCaps = inputCaps;
    emit this->inputCapsChanged(inputCaps);
}

void AkAudioEncoder::setBitrate(int bitrate)
{
    if (this->d->m_bitrate == bitrate)
        return;

    this->d->m_bitrate = bitrate;
    emit this->bitrateChanged(bitrate);
}

void AkAudioEncoder::resetInputCaps()
{
    this->setInputCaps({});
}

void AkAudioEncoder::resetBitrate()
{
    this->setBitrate(128000);
}

void AkAudioEncoder::resetOptions()
{
    this->resetBitrate();
}

#include "moc_akaudioencoder.cpp"

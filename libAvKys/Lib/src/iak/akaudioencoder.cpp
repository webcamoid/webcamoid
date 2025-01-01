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
        QString m_codec;
        AkAudioCaps m_inputCaps;
        int m_bitrate {128000};
        bool m_fillGaps {false};
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

QString AkAudioEncoder::codec() const
{
    return this->d->m_codec;
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

bool AkAudioEncoder::fillGaps() const
{
    return this->d->m_fillGaps;
}

void AkAudioEncoder::setCodec(const QString &codec)
{
    if (this->d->m_codec == codec)
        return;

    this->d->m_codec = codec;
    emit this->codecChanged(codec);
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

void AkAudioEncoder::setFillGaps(bool fillGaps)
{
    if (this->d->m_fillGaps == fillGaps)
        return;

    this->d->m_fillGaps = fillGaps;
    emit this->fillGapsChanged(fillGaps);
}

void AkAudioEncoder::resetCodec()
{
    this->setCodec({});
}

void AkAudioEncoder::resetInputCaps()
{
    this->setInputCaps({});
}

void AkAudioEncoder::resetBitrate()
{
    this->setBitrate(128000);
}

void AkAudioEncoder::resetFillGaps()
{
    this->setFillGaps(false);
}

void AkAudioEncoder::resetOptions()
{
    this->resetBitrate();
}

#include "moc_akaudioencoder.cpp"

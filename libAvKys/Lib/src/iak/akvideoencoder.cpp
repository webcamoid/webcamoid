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

#include "akvideoencoder.h"
#include "../akvideocaps.h"

class AkVideoEncoderPrivate
{
    public:
        AkVideoCaps m_inputCaps;
        int m_bitrate {1500000};
        int m_gop {1000};
};

AkVideoEncoder::AkVideoEncoder(QObject *parent):
    AkElement{parent}
{
    this->d = new AkVideoEncoderPrivate();
}

AkVideoEncoder::~AkVideoEncoder()
{
    delete this->d;
}

AkVideoCaps AkVideoEncoder::inputCaps() const
{
    return this->d->m_inputCaps;
}

int AkVideoEncoder::bitrate() const
{
    return this->d->m_bitrate;
}

int AkVideoEncoder::gop() const
{
    return this->d->m_gop;
}

AkCompressedVideoPackets AkVideoEncoder::headers() const
{
    return {};
}

void AkVideoEncoder::setInputCaps(const AkVideoCaps &inputCaps)
{
    if (this->d->m_inputCaps == inputCaps)
        return;

    this->d->m_inputCaps = inputCaps;
    emit this->inputCapsChanged(inputCaps);
}

void AkVideoEncoder::setBitrate(int bitrate)
{
    if (this->d->m_bitrate == bitrate)
        return;

    this->d->m_bitrate = bitrate;
    emit this->bitrateChanged(bitrate);
}

void AkVideoEncoder::setGop(int gop)
{
    if (this->d->m_gop == gop)
        return;

    this->d->m_gop = gop;
    emit this->gopChanged(gop);
}

void AkVideoEncoder::resetInputCaps()
{
    this->setInputCaps({});
}

void AkVideoEncoder::resetBitrate()
{
    this->setBitrate(1500000);
}

void AkVideoEncoder::resetGop()
{
    this->setGop(1000);
}

void AkVideoEncoder::resetOptions()
{
    this->resetBitrate();
    this->resetGop();
}

#include "moc_akvideoencoder.cpp"

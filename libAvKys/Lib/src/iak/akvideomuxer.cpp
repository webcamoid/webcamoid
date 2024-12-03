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

#include "akvideomuxer.h"

struct StreamConfig
{
    AkCompressedCaps caps;
    AkCompressedPackets headers;
    int bitrate;
};

class AkVideoMuxerPrivate
{
    public:
        QString m_location;
        StreamConfig m_audioConfigs;
        StreamConfig m_videoConfigs;
};

AkVideoMuxer::AkVideoMuxer(QObject *parent):
    AkElement{parent}
{
    this->d = new AkVideoMuxerPrivate();
}

AkVideoMuxer::~AkVideoMuxer()
{
    delete this->d;
}

QString AkVideoMuxer::location() const
{
    return this->d->m_location;
}

bool AkVideoMuxer::gapsAllowed() const
{
    return true;
}

AkCompressedCaps AkVideoMuxer::streamCaps(AkCodecType type) const
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return this->d->m_audioConfigs.caps;
    case AkCompressedCaps::CapsType_Video:
        return this->d->m_videoConfigs.caps;
    default:
        break;
    }

    return {};
}

int AkVideoMuxer::streamBitrate(AkCodecType type) const
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return this->d->m_audioConfigs.bitrate;
    case AkCompressedCaps::CapsType_Video:
        return this->d->m_videoConfigs.bitrate;
    default:
        break;
    }

    return 0;
}

AkCompressedPackets AkVideoMuxer::streamHeaders(AkCodecType type) const
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return this->d->m_audioConfigs.headers;
    case AkCompressedCaps::CapsType_Video:
        return this->d->m_videoConfigs.headers;
    default:
        break;
    }

    return {};
}

void AkVideoMuxer::setStreamCaps(const AkCompressedCaps &caps)
{
    switch (caps.type()) {
    case AkCompressedCaps::CapsType_Audio: {
        if (this->d->m_audioConfigs.caps == caps)
            return;

        this->d->m_audioConfigs.caps = caps;
        emit this->streamCapsUpdated(caps.type(), caps);

        break;
    }
    case AkCompressedCaps::CapsType_Video: {
        if (this->d->m_videoConfigs.caps == caps)
            return;

        this->d->m_videoConfigs.caps = caps;
        emit this->streamCapsUpdated(caps.type(), caps);

        break;
    }
    default:
        break;
    }
}

void AkVideoMuxer::setStreamBitrate(AkCodecType type, int bitrate)
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio: {
        if (this->d->m_audioConfigs.bitrate == bitrate)
            return;

        this->d->m_audioConfigs.bitrate = bitrate;
        emit this->streamBitrateUpdated(type, bitrate);

        break;
    }
    case AkCompressedCaps::CapsType_Video: {
        if (this->d->m_videoConfigs.bitrate == bitrate)
            return;

        this->d->m_videoConfigs.bitrate = bitrate;
        emit this->streamBitrateUpdated(type, bitrate);

        break;
    }
    default:
        break;
    }
}

void AkVideoMuxer::setStreamHeaders(AkCodecType type,
                                    const AkCompressedPackets &headers)
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio: {
        if (this->d->m_audioConfigs.headers == headers)
            return;

        this->d->m_audioConfigs.headers = headers;
        emit this->streamHeadersUpdated(type, headers);

        break;
    }
    case AkCompressedCaps::CapsType_Video: {
        if (this->d->m_videoConfigs.headers == headers)
            return;

        this->d->m_videoConfigs.headers = headers;
        emit this->streamHeadersUpdated(type, headers);

        break;
    }
    default:
        break;
    }
}

void AkVideoMuxer::setLocation(const QString &location)
{
    if (this->d->m_location == location)
        return;

    this->d->m_location = location;
    emit this->locationChanged(location);
}

void AkVideoMuxer::resetLocation()
{
    this->setLocation({});
}

void AkVideoMuxer::resetOptions()
{
}

#include "moc_akvideomuxer.cpp"

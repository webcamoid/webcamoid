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

#include <QMap>

#include "akvideomuxer.h"

class AkVideoMuxerPrivate
{
    public:
        QString m_location;
        QMap<AkCompressedCaps::CapsType, AkCompressedCaps> m_streamCaps;
        QMap<AkCompressedCaps::CapsType, AkCompressedPackets> m_streamHeaders;
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

AkCompressedCaps AkVideoMuxer::streamCaps(AkCompressedCaps::CapsType type) const
{
    return this->d->m_streamCaps.value(type);
}

AkCompressedPackets AkVideoMuxer::streamHeaders(AkCompressedCaps::CapsType type) const
{
    return this->d->m_streamHeaders.value(type);
}

void AkVideoMuxer::setStreamCaps(const AkCompressedCaps &caps)
{
    if (this->d->m_streamCaps.value(caps.type()) == caps)
        return;

    this->d->m_streamCaps[caps.type()] = caps;
    emit this->streamCapsUpdated();
}

void AkVideoMuxer::setStreamHeaders(AkCompressedCaps::CapsType type,
                                    const AkCompressedPackets &headers)
{
    if (this->d->m_streamHeaders.value(type) == headers)
        return;

    this->d->m_streamHeaders[type] = headers;
    emit this->streamHeadersUpdated();
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

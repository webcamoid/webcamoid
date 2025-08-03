/* Webcamoid, camera capture application.
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

#include <QVariant>

#include "akvideomuxer.h"

struct StreamConfig
{
    AkCompressedCaps caps;
    QByteArray headers;
    int bitrate;
    qint64 duration;
};

class AkVideoMuxerPrivate
{
    public:
        QString m_muxer;
        QString m_location;
        StreamConfig m_audioConfigs;
        StreamConfig m_videoConfigs;
        QVariantMap m_optionValues;
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

QString AkVideoMuxer::muxer() const
{
    return this->d->m_muxer;
}

QString AkVideoMuxer::location() const
{
    return this->d->m_location;
}

bool AkVideoMuxer::gapsAllowed(AkCodecType type) const
{
    Q_UNUSED(type)

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

QByteArray AkVideoMuxer::streamHeaders(AkCodecType type) const
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

qint64 AkVideoMuxer::streamDuration(AkCodecType type) const
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return this->d->m_audioConfigs.duration;
    case AkCompressedCaps::CapsType_Video:
        return this->d->m_videoConfigs.duration;
    default:
        break;
    }

    return {};
}

AkPropertyOptions AkVideoMuxer::options() const
{
    return {};
}

QVariant AkVideoMuxer::optionValue(const QString &option) const
{
    auto options = this->options();

    if (options.isEmpty())
        return {};

    auto it = std::find_if(options.constBegin(),
                           options.constEnd(),
                           [option] (const AkPropertyOption &propertyOption) {
        return propertyOption.name() == option;
    });

    if (it == options.constEnd())
        return {};

    return this->d->m_optionValues.value(option, it->defaultValue());
}

bool AkVideoMuxer::isOptionSet(const QString &option) const
{
    auto options = this->options();

    if (options.isEmpty())
        return {};

    auto it = std::find_if(options.constBegin(),
                           options.constEnd(),
                           [option] (const AkPropertyOption &propertyOption) {
        return propertyOption.name() == option;
    });

    return it != options.constEnd();
}

void AkVideoMuxer::setMuxer(const QString &muxer)
{
    if (this->d->m_muxer == muxer)
        return;

    this->d->m_muxer = muxer;
    emit this->muxerChanged(muxer);
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
                                    const QByteArray &headers)
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

void AkVideoMuxer::setStreamDuration(AkCodecType type, qint64 duration)
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio: {
        if (this->d->m_audioConfigs.duration == duration)
            return;

        this->d->m_audioConfigs.duration = duration;
        emit this->streamDurationUpdated(type, duration);

        break;
    }
    case AkCompressedCaps::CapsType_Video: {
        if (this->d->m_videoConfigs.duration == duration)
            return;

        this->d->m_videoConfigs.duration = duration;
        emit this->streamDurationUpdated(type, duration);

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

void AkVideoMuxer::setOptionValue(const QString &option, const QVariant &value)
{
    auto curValue = this->optionValue(option);

    if (curValue == value)
        return;

    auto options = this->options();

    if (options.isEmpty())
        return;

    auto it = std::find_if(options.constBegin(),
                           options.constEnd(),
                           [option] (const AkPropertyOption &propertyOption) {
        return propertyOption.name() == option;
    });

    QVariant defaultValue;

    if (it != options.constEnd())
        defaultValue = it->defaultValue();

    if (value == defaultValue)
        this->d->m_optionValues.remove(option);
    else
        this->d->m_optionValues[option] = value;

    emit this->optionValueChanged(option, value);
}

void AkVideoMuxer::resetMuxer()
{
    this->setMuxer({});
}

void AkVideoMuxer::resetLocation()
{
    this->setLocation({});
}

void AkVideoMuxer::resetOptionValue(const QString &option)
{
    auto options = this->options();

    if (options.isEmpty())
        return;

    auto it = std::find_if(options.constBegin(),
                           options.constEnd(),
                           [option] (const AkPropertyOption &propertyOption) {
        return propertyOption.name() == option;
    });

    QVariant defaultValue;

    if (it != options.constEnd())
        defaultValue = it->defaultValue();

    this->setOptionValue(option, defaultValue);
}

void AkVideoMuxer::resetOptions()
{
    for (auto &option: this->options())
        this->resetOptionValue(option.name());
}

#include "moc_akvideomuxer.cpp"

/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <QElapsedTimer>
#include <QFileInfo>
#include <QMetaEnum>
#include <QTimer>
#include <QUrl>
#include <QVariant>

#include "akvideostreamer.h"
#include "../akpacket.h"
#include "../akstreamingstats.h"

struct StreamConfig
{
    AkCompressedCaps caps;
    QByteArray headers;
    int bitrate {0};
    qint64 duration {0};
};

class AkVideoStreamerPrivate
{
    public:
        AkVideoStreamer *self;
        QStringList m_destinations;
        AkVideoStreamer::StreamingState m_streamState {AkVideoStreamer::StreamingState_Idle};
        AkStreamingStats m_stats;
        StreamConfig m_audioConfigs;
        StreamConfig m_videoConfigs;
        QVariantMap m_optionValues;
        QTimer m_statsTimer;
        QElapsedTimer m_statsElapsed;
        qint64 m_lastBytesSent {0};
        qint64 m_lastPacketsSent {0};
        qint64 m_lastVideoPacketsSent {0};
        qint64 m_lastAudioPacketsSent {0};

        explicit AkVideoStreamerPrivate(AkVideoStreamer *self);
        ~AkVideoStreamerPrivate();
        void updateStatsTimer();
        void calculateBitrates();
};

AkVideoStreamer::AkVideoStreamer(QObject *parent):
    AkElement(parent)
{
    this->d = new AkVideoStreamerPrivate(this);
}

AkVideoStreamer::~AkVideoStreamer()
{
    delete this->d;
}

QStringList AkVideoStreamer::destinations() const
{
    return this->d->m_destinations;
}

AkVideoStreamer::StreamingState AkVideoStreamer::streamState() const
{
    return this->d->m_streamState;
}

AkStreamingStats AkVideoStreamer::stats() const
{
    return this->d->m_stats;
}

bool AkVideoStreamer::supportsUrl(const QString &url) const
{
    return !this->protocolForUrl(url).isEmpty();
}

QString AkVideoStreamer::defaultFormat(const QString &protocol) const
{
    return this->supportedFormats(protocol).value(0);
}

QString AkVideoStreamer::formatForUrl(const QString &url) const
{
    auto protocol = this->protocolForUrl(url);

    if (protocol.isEmpty())
        return {};

    auto ext = QFileInfo(QUrl(url).path()).suffix().toLower();
    auto formats = this->supportedFormats(protocol);

    if (!ext.isEmpty() && formats.contains(ext))
        return ext;

    return this->defaultFormat(protocol);
}

AkCompressedCaps AkVideoStreamer::streamCaps(AkCodecType type) const
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

int AkVideoStreamer::streamBitrate(AkCodecType type) const
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

QByteArray AkVideoStreamer::streamHeaders(AkCodecType type) const
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

qint64 AkVideoStreamer::streamDuration(AkCodecType type) const
{
    switch (type) {
    case AkCompressedCaps::CapsType_Audio:
        return this->d->m_audioConfigs.duration;
    case AkCompressedCaps::CapsType_Video:
        return this->d->m_videoConfigs.duration;
    default:
        break;
    }

    return 0;
}

AkPropertyOptions AkVideoStreamer::options() const
{
    return {};
}

QVariant AkVideoStreamer::optionValue(const QString &option) const
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

bool AkVideoStreamer::isOptionSet(const QString &option) const
{
    auto options = this->options();

    if (options.isEmpty())
        return false;

    auto it = std::find_if(options.constBegin(),
                           options.constEnd(),
                           [option] (const AkPropertyOption &propertyOption) {
        return propertyOption.name() == option;
    });

    return it != options.constEnd();
}

void AkVideoStreamer::setDestinations(const QStringList &destinations)
{
    if (this->d->m_destinations == destinations)
        return;

    this->d->m_destinations = destinations;
    emit this->destinationsChanged(destinations);
}

void AkVideoStreamer::resetDestinations()
{
    this->setDestinations({});
}

void AkVideoStreamer::setStreamCaps(const AkCompressedCaps &caps)
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

void AkVideoStreamer::setStreamBitrate(AkCodecType type, int bitrate)
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

void AkVideoStreamer::setStreamHeaders(AkCodecType type,
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

void AkVideoStreamer::setStreamDuration(AkCodecType type, qint64 duration)
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

void AkVideoStreamer::setOptionValue(const QString &option,
                                     const QVariant &value)
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

void AkVideoStreamer::resetOptionValue(const QString &option)
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

void AkVideoStreamer::resetOptions()
{
    for (auto &option: this->options())
        this->resetOptionValue(option.name());
}

void AkVideoStreamer::updateStats(const AkPacket &packet)
{
    bool isVideo = packet.type() == AkPacket::PacketVideo
                   || packet.type() == AkPacket::PacketVideoCompressed;
    bool isAudio = packet.type() == AkPacket::PacketAudio
                   || packet.type() == AkPacket::PacketAudioCompressed;

    this->d->m_stats.setBytesSent(this->d->m_stats.bytesSent() + packet.size());
    this->d->m_stats.setPacketsSent(this->d->m_stats.packetsSent() + 1);

    if (isVideo)
        this->d->m_stats.setVideoPacketsSent(this->d->m_stats.videoPacketsSent() + 1);

    if (isAudio)
        this->d->m_stats.setAudioPacketsSent(this->d->m_stats.audioPacketsSent() + 1);
}

void AkVideoStreamer::setStreamingState(StreamingState state)
{
    if (this->d->m_streamState == state)
        return;

    this->d->m_streamState = state;
    this->d->updateStatsTimer();
    emit this->streamStateChanged(state);
}

void AkVideoStreamer::setConnectionState(const QString &url, StreamingState state)
{
    emit this->connectionStateChanged(url, state);
}

AkVideoStreamerPrivate::AkVideoStreamerPrivate(AkVideoStreamer *self):
    self(self)
{
    this->m_statsTimer.setInterval(1000);

    QObject::connect(&this->m_statsTimer,
                     &QTimer::timeout,
                     [this]() {
        this->calculateBitrates();
    });
}

AkVideoStreamerPrivate::~AkVideoStreamerPrivate()
{
    this->m_statsTimer.stop();
}

void AkVideoStreamerPrivate::updateStatsTimer()
{
    if (this->m_streamState == AkVideoStreamer::StreamingState_Streaming) {
        this->m_statsElapsed.start();
        this->m_statsTimer.start();
    } else {
        this->m_statsTimer.stop();
    }
}

void AkVideoStreamerPrivate::calculateBitrates()
{
    qint64 elapsed = this->m_statsElapsed.elapsed();

    if (elapsed == 0)
        return;

    qint64 bytesDiff = this->m_stats.bytesSent() - this->m_lastBytesSent;
    qint64 videoPacketsDiff = this->m_stats.videoPacketsSent() - this->m_lastVideoPacketsSent;
    qint64 audioPacketsDiff = this->m_stats.audioPacketsSent() - this->m_lastAudioPacketsSent;

    // Total bitrate in kbps
    qreal totalBitrate = (bytesDiff * 8.0) / elapsed;
    this->m_stats.setVideoBitrate(totalBitrate);
    this->m_stats.setAudioBitrate(0.0);

    // Framerate
    this->m_stats.setFramerate((videoPacketsDiff * 1000.0) / elapsed);

    this->m_lastBytesSent = this->m_stats.bytesSent();
    this->m_lastVideoPacketsSent = this->m_stats.videoPacketsSent();
    this->m_lastAudioPacketsSent = this->m_stats.audioPacketsSent();

    emit self->statsChanged(this->m_stats);
}

#include "moc_akvideostreamer.cpp"

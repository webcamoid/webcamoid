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

#include <QDebug>
#include <QQmlEngine>

#include "akstreamingstats.h"

class AkStreamingStatsPrivate
{
    public:
        qint64 m_bytesSent {0};
        qint64 m_packetsSent {0};
        qint64 m_videoPacketsSent {0};
        qint64 m_audioPacketsSent {0};
        qreal m_videoBitrate {0.0};
        qreal m_audioBitrate {0.0};
        qreal m_framerate {0.0};
        qreal m_latency {0.0};
        int m_activeDestinations {0};
        int m_failedDestinations {0};
        QString m_lastError;
};

AkStreamingStats::AkStreamingStats(QObject *parent):
    QObject(parent)
{
    this->d = new AkStreamingStatsPrivate();
}

AkStreamingStats::AkStreamingStats(const AkStreamingStats &other):
    QObject()
{
    this->d = new AkStreamingStatsPrivate();
    this->d->m_bytesSent = other.d->m_bytesSent;
    this->d->m_packetsSent = other.d->m_packetsSent;
    this->d->m_videoPacketsSent = other.d->m_videoPacketsSent;
    this->d->m_audioPacketsSent = other.d->m_audioPacketsSent;
    this->d->m_videoBitrate = other.d->m_videoBitrate;
    this->d->m_audioBitrate = other.d->m_audioBitrate;
    this->d->m_framerate = other.d->m_framerate;
    this->d->m_latency = other.d->m_latency;
    this->d->m_activeDestinations = other.d->m_activeDestinations;
    this->d->m_failedDestinations = other.d->m_failedDestinations;
    this->d->m_lastError = other.d->m_lastError;
}

AkStreamingStats::~AkStreamingStats()
{
    delete this->d;
}

AkStreamingStats &AkStreamingStats::operator=(const AkStreamingStats &other)
{
    if (this != &other) {
        this->d->m_bytesSent = other.d->m_bytesSent;
        this->d->m_packetsSent = other.d->m_packetsSent;
        this->d->m_videoPacketsSent = other.d->m_videoPacketsSent;
        this->d->m_audioPacketsSent = other.d->m_audioPacketsSent;
        this->d->m_videoBitrate = other.d->m_videoBitrate;
        this->d->m_audioBitrate = other.d->m_audioBitrate;
        this->d->m_framerate = other.d->m_framerate;
        this->d->m_latency = other.d->m_latency;
        this->d->m_activeDestinations = other.d->m_activeDestinations;
        this->d->m_failedDestinations = other.d->m_failedDestinations;
        this->d->m_lastError = other.d->m_lastError;
    }

    return *this;
}

qint64 AkStreamingStats::bytesSent() const
{
    return this->d->m_bytesSent;
}

qint64 AkStreamingStats::packetsSent() const
{
    return this->d->m_packetsSent;
}

qint64 AkStreamingStats::videoPacketsSent() const
{
    return this->d->m_videoPacketsSent;
}

qint64 AkStreamingStats::audioPacketsSent() const
{
    return this->d->m_audioPacketsSent;
}

qreal AkStreamingStats::videoBitrate() const
{
    return this->d->m_videoBitrate;
}

qreal AkStreamingStats::audioBitrate() const
{
    return this->d->m_audioBitrate;
}

qreal AkStreamingStats::framerate() const
{
    return this->d->m_framerate;
}

qreal AkStreamingStats::latency() const
{
    return this->d->m_latency;
}

int AkStreamingStats::activeDestinations() const
{
    return this->d->m_activeDestinations;
}

int AkStreamingStats::failedDestinations() const
{
    return this->d->m_failedDestinations;
}

QString AkStreamingStats::lastError() const
{
    return this->d->m_lastError;
}

void AkStreamingStats::setBytesSent(qint64 bytesSent)
{
    if (this->d->m_bytesSent == bytesSent)
        return;

    this->d->m_bytesSent = bytesSent;
    emit this->bytesSentChanged(bytesSent);
}

void AkStreamingStats::setPacketsSent(qint64 packetsSent)
{
    if (this->d->m_packetsSent == packetsSent)
        return;

    this->d->m_packetsSent = packetsSent;
    emit this->packetsSentChanged(packetsSent);
}

void AkStreamingStats::setVideoPacketsSent(qint64 videoPacketsSent)
{
    if (this->d->m_videoPacketsSent == videoPacketsSent)
        return;

    this->d->m_videoPacketsSent = videoPacketsSent;
    emit this->videoPacketsSentChanged(videoPacketsSent);
}

void AkStreamingStats::setAudioPacketsSent(qint64 audioPacketsSent)
{
    if (this->d->m_audioPacketsSent == audioPacketsSent)
        return;

    this->d->m_audioPacketsSent = audioPacketsSent;
    emit this->audioPacketsSentChanged(audioPacketsSent);
}

void AkStreamingStats::setVideoBitrate(qreal videoBitrate)
{
    if (qFuzzyCompare(this->d->m_videoBitrate, videoBitrate))
        return;

    this->d->m_videoBitrate = videoBitrate;
    emit this->videoBitrateChanged(videoBitrate);
}

void AkStreamingStats::setAudioBitrate(qreal audioBitrate)
{
    if (qFuzzyCompare(this->d->m_audioBitrate, audioBitrate))
        return;

    this->d->m_audioBitrate = audioBitrate;
    emit this->audioBitrateChanged(audioBitrate);
}

void AkStreamingStats::setFramerate(qreal framerate)
{
    if (qFuzzyCompare(this->d->m_framerate, framerate))
        return;

    this->d->m_framerate = framerate;
    emit this->framerateChanged(framerate);
}

void AkStreamingStats::setLatency(qreal latency)
{
    if (qFuzzyCompare(this->d->m_latency, latency))
        return;

    this->d->m_latency = latency;
    emit this->latencyChanged(latency);
}

void AkStreamingStats::setActiveDestinations(int activeDestinations)
{
    if (this->d->m_activeDestinations == activeDestinations)
        return;

    this->d->m_activeDestinations = activeDestinations;
    emit this->activeDestinationsChanged(activeDestinations);
}

void AkStreamingStats::setFailedDestinations(int failedDestinations)
{
    if (this->d->m_failedDestinations == failedDestinations)
        return;

    this->d->m_failedDestinations = failedDestinations;
    emit this->failedDestinationsChanged(failedDestinations);
}

void AkStreamingStats::setLastError(QString lastError)
{
    if (this->d->m_lastError == lastError)
        return;

    this->d->m_lastError = lastError;
    emit this->lastErrorChanged(lastError);
}

void AkStreamingStats::resetBytesSent()
{
    this->setBytesSent(0);
}

void AkStreamingStats::resetPacketsSent()
{
    this->setPacketsSent(0);
}

void AkStreamingStats::resetVideoPacketsSent()
{
    this->setVideoPacketsSent(0);
}

void AkStreamingStats::resetAudioPacketsSent()
{
    this->setAudioPacketsSent(0);
}

void AkStreamingStats::resetVideoBitrate()
{
    this->setVideoBitrate(0.0);
}

void AkStreamingStats::resetAudioBitrate()
{
    this->setAudioBitrate(0.0);
}

void AkStreamingStats::resetFramerate()
{
    this->setFramerate(0.0);
}

void AkStreamingStats::resetLatency()
{
    this->setLatency(0.0);
}

void AkStreamingStats::resetActiveDestinations()
{
    this->setActiveDestinations(0);
}

void AkStreamingStats::resetFailedDestinations()
{
    this->setFailedDestinations(0);
}

void AkStreamingStats::resetLastError()
{
    this->setLastError({});
}

void AkStreamingStats::registerTypes()
{
    qRegisterMetaType<AkStreamingStats>("AkStreamingStats");
    qmlRegisterSingletonType<AkStreamingStats>("Ak", 1, 0, "AkStreamingStats",
                                     [] (QQmlEngine *qmlEngine,
                                         QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkStreamingStats();
    });
}

QDebug operator <<(QDebug debug, const AkStreamingStats &stats)
{
    debug.nospace() << "AkStreamingStats("
                    << "bytesSent=" << stats.bytesSent()
                    << ", packetsSent=" << stats.packetsSent()
                    << ", videoPacketsSent=" << stats.videoPacketsSent()
                    << ", audioPacketsSent=" << stats.audioPacketsSent()
                    << ", videoBitrate=" << stats.videoBitrate()
                    << ", audioBitrate=" << stats.audioBitrate()
                    << ", framerate=" << stats.framerate()
                    << ", latency=" << stats.latency()
                    << ", activeDestinations=" << stats.activeDestinations()
                    << ", failedDestinations=" << stats.failedDestinations()
                    << ", lastError=" << stats.lastError()
                    << ")";

    return debug;
}

#include "moc_akstreamingstats.cpp"

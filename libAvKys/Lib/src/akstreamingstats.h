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

#ifndef AKSTREAMINGSTATS_H
#define AKSTREAMINGSTATS_H

#include <QObject>

#include "akcommons.h"

class AkStreamingStatsPrivate;
class QDataStream;

class AKCOMMONS_EXPORT AkStreamingStats: public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 bytesSent
               READ bytesSent
               WRITE setBytesSent
               RESET resetBytesSent
               NOTIFY bytesSentChanged)
    Q_PROPERTY(qint64 packetsSent
               READ packetsSent
               WRITE setPacketsSent
               RESET resetPacketsSent
               NOTIFY packetsSentChanged)
    Q_PROPERTY(qint64 videoPacketsSent
               READ videoPacketsSent
               WRITE setVideoPacketsSent
               RESET resetVideoPacketsSent
               NOTIFY videoPacketsSentChanged)
    Q_PROPERTY(qint64 audioPacketsSent
               READ audioPacketsSent
               WRITE setAudioPacketsSent
               RESET resetAudioPacketsSent
               NOTIFY audioPacketsSentChanged)
    Q_PROPERTY(qreal videoBitrate
               READ videoBitrate
               WRITE setVideoBitrate
               RESET resetVideoBitrate
               NOTIFY videoBitrateChanged)
    Q_PROPERTY(qreal audioBitrate
               READ audioBitrate
               WRITE setAudioBitrate
               RESET resetAudioBitrate
               NOTIFY audioBitrateChanged)
    Q_PROPERTY(qreal framerate
               READ framerate
               WRITE setFramerate
               RESET resetFramerate
               NOTIFY framerateChanged)
    Q_PROPERTY(qreal latency
               READ latency
               WRITE setLatency
               RESET resetLatency
               NOTIFY latencyChanged)
    Q_PROPERTY(int activeDestinations
               READ activeDestinations
               WRITE setActiveDestinations
               RESET resetActiveDestinations
               NOTIFY activeDestinationsChanged)
    Q_PROPERTY(int failedDestinations
               READ failedDestinations
               WRITE setFailedDestinations
               RESET resetFailedDestinations
               NOTIFY failedDestinationsChanged)
    Q_PROPERTY(QString lastError
               READ lastError
               WRITE setLastError
               RESET resetLastError
               NOTIFY lastErrorChanged)

    public:
        AkStreamingStats(QObject *parent=nullptr);
        AkStreamingStats(const AkStreamingStats &other);
        virtual ~AkStreamingStats();
        AkStreamingStats &operator=(const AkStreamingStats &other);

        qint64 bytesSent() const;
        qint64 packetsSent() const;
        qint64 videoPacketsSent() const;
        qint64 audioPacketsSent() const;
        qreal videoBitrate() const;
        qreal audioBitrate() const;
        qreal framerate() const;
        qreal latency() const;
        int activeDestinations() const;
        int failedDestinations() const;
        QString lastError() const;

    private:
        AkStreamingStatsPrivate *d;

    Q_SIGNALS:
        void bytesSentChanged(qint64 bytesSent);
        void packetsSentChanged(qint64 packetsSent);
        void videoPacketsSentChanged(qint64 videoPacketsSent);
        void audioPacketsSentChanged(qint64 audioPacketsSent);
        void videoBitrateChanged(qreal videoBitrate);
        void audioBitrateChanged(qreal audioBitrate);
        void framerateChanged(qreal framerate);
        void latencyChanged(qreal latency);
        void activeDestinationsChanged(int activeDestinations);
        void failedDestinationsChanged(int failedDestinations);
        void lastErrorChanged(QString lastError);

    public Q_SLOTS:
        void setBytesSent(qint64 bytesSent);
        void setPacketsSent(qint64 packetsSent);
        void setVideoPacketsSent(qint64 videoPacketsSent);
        void setAudioPacketsSent(qint64 audioPacketsSent);
        void setVideoBitrate(qreal videoBitrate);
        void setAudioBitrate(qreal audioBitrate);
        void setFramerate(qreal framerate);
        void setLatency(qreal latency);
        void setActiveDestinations(int activeDestinations);
        void setFailedDestinations(int failedDestinations);
        void setLastError(QString lastError);
        void resetBytesSent();
        void resetPacketsSent();
        void resetVideoPacketsSent();
        void resetAudioPacketsSent();
        void resetVideoBitrate();
        void resetAudioBitrate();
        void resetFramerate();
        void resetLatency();
        void resetActiveDestinations();
        void resetFailedDestinations();
        void resetLastError();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug,
                                    const AkStreamingStats &streamingStats);

Q_DECLARE_METATYPE(AkStreamingStats)

#endif // AKSTREAMINGSTATS_H

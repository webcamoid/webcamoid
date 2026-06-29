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

#ifndef AKVIDEOSTREAMER_H
#define AKVIDEOSTREAMER_H

#include "akelement.h"
#include "../akcompressedcaps.h"
#include "../akcompressedpacket.h"
#include "../akpropertyoption.h"

class AkVideoStreamer;
class AkVideoStreamerPrivate;
class AkStreamingStats;
class QVariant;

using AkVideoStreamerPtr = QSharedPointer<AkVideoStreamer>;
using AkCodecType = AkCompressedCaps::CapsType;

class AKCOMMONS_EXPORT AkVideoStreamer: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QStringList destinations
               READ destinations
               WRITE setDestinations
               RESET resetDestinations
               NOTIFY destinationsChanged)
    Q_PROPERTY(StreamingState streamState
               READ streamState
               NOTIFY streamStateChanged)
    Q_PROPERTY(AkPropertyOptions options
               READ options
               NOTIFY optionsChanged)
    Q_PROPERTY(AkStreamingStats stats
               READ stats
               NOTIFY statsChanged)

    public:
        enum ProtocolID
        {
            ProtocolID_unknown = AK_MAKE_FOURCC(0, 0, 0, 0),
            ProtocolID_http    = AK_MAKE_FOURCC('H', 'T', 'T', 'P'), // HTTP
            ProtocolID_rtmp    = AK_MAKE_FOURCC('R', 'T', 'M', 'P'), // Adobe RTMP
            ProtocolID_hls     = AK_MAKE_FOURCC('H', 'L', 'S', 0),   // HTTP Live Streaming
            ProtocolID_dash    = AK_MAKE_FOURCC('D', 'A', 'S', 'H'), // DASH container
        };
        Q_ENUM(ProtocolID)

        enum StreamingState
        {
            StreamingState_Idle,
            StreamingState_Connecting,
            StreamingState_Streaming,
            StreamingState_Reconnecting,
            StreamingState_Error,
            StreamingState_Stopped
        };
        Q_ENUM(StreamingState)

        explicit AkVideoStreamer(QObject *parent=nullptr);
        virtual ~AkVideoStreamer();

        // Query properties
        Q_INVOKABLE QStringList destinations() const;
        Q_INVOKABLE StreamingState streamState() const;
        Q_INVOKABLE AkStreamingStats stats() const;

        // Protocols supported by the plugin
        Q_INVOKABLE virtual QStringList protocols() const = 0;
        Q_INVOKABLE virtual ProtocolID protocolID(const QString &protocol) const = 0;
        Q_INVOKABLE virtual QString description(const QString &protocol) const = 0;

        // Capabilities per URL
        Q_INVOKABLE bool supportsUrl(const QString &url) const;
        Q_INVOKABLE virtual QString protocolForUrl(const QString &url) const = 0;
        Q_INVOKABLE virtual QStringList supportedFormats(const QString &protocol) const = 0;
        Q_INVOKABLE virtual QString defaultFormat(const QString &protocol) const;
        Q_INVOKABLE virtual QString formatForUrl(const QString &url) const;
        Q_INVOKABLE virtual QList<AkCodecID> supportedCodecs(const QString &format,
                                                             AkCodecType type) const = 0;
        Q_INVOKABLE virtual AkCodecID defaultCodec(const QString &format,
                                                   AkCodecType type) const = 0;

        // Stream configuration
        Q_INVOKABLE AkCompressedCaps streamCaps(AkCodecType type) const;
        Q_INVOKABLE int streamBitrate(AkCodecType type) const;
        Q_INVOKABLE QByteArray streamHeaders(AkCodecType type) const;
        Q_INVOKABLE qint64 streamDuration(AkCodecType type) const;

        // Protocol specific options
        Q_INVOKABLE virtual AkPropertyOptions options() const;
        Q_INVOKABLE QVariant optionValue(const QString &option) const;
        Q_INVOKABLE bool isOptionSet(const QString &option) const;

    private:
        AkVideoStreamerPrivate *d;

    Q_SIGNALS:
        void destinationsChanged(const QStringList &destinations);
        void streamStateChanged(StreamingState state);
        void statsChanged(const AkStreamingStats &stats);
        void streamCapsUpdated(AkCodecType type, const AkCompressedCaps &caps);
        void streamBitrateUpdated(AkCodecType type, int bitrate);
        void streamHeadersUpdated(AkCodecType type, const QByteArray &headers);
        void streamDurationUpdated(AkCodecType type, qint64 duration);
        void optionsChanged(const AkPropertyOptions &options);
        void optionValueChanged(const QString &option, const QVariant &value);

        // Network events
        void connectionStateChanged(const QString &url, StreamingState state);
        void streamingError(const QString &error);
        void streamingWarning(const QString &warning);

    public Q_SLOTS:
        void setDestinations(const QStringList &destinations);
        void resetDestinations();
        void setStreamCaps(const AkCompressedCaps &caps);
        void setStreamBitrate(AkCodecType type, int bitrate);
        void setStreamHeaders(AkCodecType type, const QByteArray &headers);
        void setStreamDuration(AkCodecType type, qint64 duration);
        void setOptionValue(const QString &option, const QVariant &value);
        void resetOptionValue(const QString &option);
        virtual void resetOptions();

    protected:
        void updateStats(const AkPacket &packet);

        // Notify status changes
        void setStreamingState(StreamingState state);
        void setConnectionState(const QString &url, StreamingState state);
        void setActiveDestinations(int count);
        void setFailedDestinations(int count);
        void setLastError(const QString &error);
};

Q_DECLARE_METATYPE(AkVideoStreamer::ProtocolID)
Q_DECLARE_METATYPE(AkVideoStreamer::StreamingState)

#endif // AKVIDEOSTREAMER_H

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

#ifndef STREAMING_H
#define STREAMING_H

#include <akcaps.h>
#include <akpropertyoption.h>
#include <iak/akelement.h>

class StreamingPrivate;
class Streaming;
class AkAudioCaps;
class AkVideoCaps;
class QQmlApplicationEngine;

using StreamingPtr = QSharedPointer<Streaming>;

class Streaming: public QObject
{
    Q_OBJECT

    Q_PROPERTY(AkAudioCaps audioCaps
               READ audioCaps
               WRITE setAudioCaps
               RESET resetAudioCaps
               NOTIFY audioCapsChanged)
    Q_PROPERTY(AkVideoCaps videoCaps
               READ videoCaps
               WRITE setVideoCaps
               RESET resetVideoCaps
               NOTIFY videoCapsChanged)
    Q_PROPERTY(int videoGOP
               READ videoGOP
               WRITE setVideoGOP
               RESET resetVideoGOP
               NOTIFY videoGOPChanged)
    Q_PROPERTY(int defaultVideoGOP
               READ defaultVideoGOP
               CONSTANT)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)
    Q_PROPERTY(QStringList platforms
               READ platforms
               WRITE setPlatforms
               RESET resetPlatforms
               NOTIFY platformsChanged)
    Q_PROPERTY(QStringList supportedPlatforms
               READ supportedPlatforms
               NOTIFY supportedPlatformsChanged)
    Q_PROPERTY(QStringList unconfiguredPlatforms
               READ unconfiguredPlatforms
               NOTIFY unconfiguredPlatformsChanged)
    Q_PROPERTY(bool isStreamingSupported
               READ isStreamingSupported
               CONSTANT)

    public:
        Streaming(QQmlApplicationEngine *engine=nullptr,
                  QObject *parent=nullptr);
        ~Streaming();

        Q_INVOKABLE AkAudioCaps audioCaps() const;
        Q_INVOKABLE AkVideoCaps videoCaps() const;
        Q_INVOKABLE int videoGOP() const;
        Q_INVOKABLE int defaultVideoGOP() const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE QStringList platforms() const;
        Q_INVOKABLE QStringList supportedPlatforms() const;
        Q_INVOKABLE QStringList unconfiguredPlatforms() const;
        Q_INVOKABLE bool isStreamingSupported() const;
        Q_INVOKABLE QString platformWebsite(const QString &platform) const;
        Q_INVOKABLE QString platformStreamingUrl(const QString &platform) const;
        Q_INVOKABLE QString platformStreamingKey(const QString &platform) const;
        Q_INVOKABLE QString platformKeyConfigsUrl(const QString &platform) const;
        Q_INVOKABLE QString platformDocsUrl(const QString &platform) const;
        Q_INVOKABLE bool platformNeedsKey(const QString &platform) const;
        Q_INVOKABLE QString codec(AkCaps::CapsType type) const;
        Q_INVOKABLE QString defaultCodec(const QString &platform,
                                         AkCaps::CapsType type) const;
        Q_INVOKABLE QStringList supportedCodecs(const QString &platform,
                                                AkCaps::CapsType type) const;
        Q_INVOKABLE QString codecDescription(const QString &codec) const;
        Q_INVOKABLE AkPropertyOptions codecOptions(AkCaps::CapsType type) const;
        Q_INVOKABLE QVariant codecOptionValue(AkCaps::CapsType type,
                                              const QString &option) const;
        Q_INVOKABLE int bitrate(AkCaps::CapsType type) const;
        Q_INVOKABLE int defaultBitrate(AkCaps::CapsType type) const;

    private:
        StreamingPrivate *d;

    signals:
        void audioCapsChanged(const AkAudioCaps &audioCaps);
        void videoCapsChanged(const AkVideoCaps &videoCaps);
        void videoGOPChanged(int videoGOP);
        void stateChanged(AkElement::ElementState state);
        void platformsChanged(const QStringList &platforms);
        void supportedPlatformsChanged(const QStringList &supportedPlatforms);
        void unconfiguredPlatformsChanged(const QStringList &unconfiguredPlatforms);
        void codecChanged(AkCaps::CapsType type, const QString &codec);
        void codecOptionsChanged(AkCaps::CapsType type,
                                 const AkPropertyOptions &options);
        void codecOptionValueChanged(AkCaps::CapsType type,
                                     const QString &option,
                                     const QVariant &value);
        void bitrateChanged(AkCaps::CapsType type, int bitrate);
        void streamingError(const QString &error);
        void streamingWarning(const QString &warning);

    public slots:
        void setAudioCaps(const AkAudioCaps &audioCaps);
        void setVideoCaps(const AkVideoCaps &videoCaps);
        void setVideoGOP(int gop);
        bool setState(AkElement::ElementState state);
        void setPlatforms(const QStringList &platforms);
        void addSupportedPlatform(const QString &platform);
        void removeSupportedPlatform(const QString &platform);
        void setPlatformWebsite(const QString &platform,
                                const QString &website);
        void setPlatformStreamingUrl(const QString &platform,
                                     const QString &StreamingUrl);
        void setPlatformStreamingKey(const QString &platform,
                                     const QString &key);
        void setPlatformKeyConfigsUrl(const QString &platform,
                                      const QString &keyConfigsUrl);
        void setPlatformDocsUrl(const QString &platform,
                                const QString &docsUrl);
        void setPlatformNeedsKey(const QString &platform,
                                 bool needsKey);
        void setCodec(AkCaps::CapsType type, const QString &codec);
        void setCodecOptionValue(AkCaps::CapsType type,
                                 const QString &option,
                                 const QVariant &value);
        void setBitrate(AkCaps::CapsType type, int bitrate);
        void resetAudioCaps();
        void resetVideoCaps();
        void resetVideoGOP();
        void resetState();
        void resetPlatforms();
        void resetCodec(AkCaps::CapsType type);
        void resetCodecOptionValue(AkCaps::CapsType type,
                                   const QString &option);
        void resetCodecOptions(AkCaps::CapsType type);
        void resetBitrate(AkCaps::CapsType type);
        AkPacket iStream(const AkPacket &packet);
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);
};

#endif // STREAMING_H

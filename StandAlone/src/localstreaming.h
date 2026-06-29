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

#ifndef LOCALSTREAMING_H
#define LOCALSTREAMING_H

#include <akcaps.h>
#include <akpropertyoption.h>
#include <iak/akelement.h>

class LocalStreamingPrivate;
class LocalStreaming;
class AkAudioCaps;
class AkVideoCaps;
class QQmlApplicationEngine;

using LocalStreamingPtr = QSharedPointer<LocalStreaming>;

class LocalStreaming: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString location
               READ location
               WRITE setLocation
               RESET resetLocation
               NOTIFY locationChanged)
    Q_PROPERTY(QString defaultURL
               READ defaultURL
               CONSTANT)
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
    Q_PROPERTY(bool isLocalStreamingSupported
               READ isLocalStreamingSupported
               CONSTANT)
    Q_PROPERTY(QStringList videoFormats
               READ videoFormats
               CONSTANT)
    Q_PROPERTY(QString defaultVideoFormat
               READ defaultVideoFormat
               CONSTANT)

    public:
        LocalStreaming(QQmlApplicationEngine *engine=nullptr,
                       QObject *parent=nullptr);
        ~LocalStreaming();

        Q_INVOKABLE QString location() const;
        Q_INVOKABLE QString defaultURL() const;
        Q_INVOKABLE AkAudioCaps audioCaps() const;
        Q_INVOKABLE AkVideoCaps videoCaps() const;
        Q_INVOKABLE int videoGOP() const;
        Q_INVOKABLE int defaultVideoGOP() const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE bool isLocalStreamingSupported() const;
        Q_INVOKABLE QString locationFormat(const QString &location) const;
        Q_INVOKABLE QStringList videoFormats() const;
        Q_INVOKABLE QString defaultVideoFormat() const;
        Q_INVOKABLE QString formatDescription(const QString &format) const;
        Q_INVOKABLE QString codec(AkCaps::CapsType type) const;
        Q_INVOKABLE QString defaultCodec(const QString &format,
                                         AkCaps::CapsType type) const;
        Q_INVOKABLE QStringList supportedCodecs(const QString &format,
                                                AkCaps::CapsType type) const;
        Q_INVOKABLE QString codecDescription(const QString &codec) const;
        Q_INVOKABLE AkPropertyOptions codecOptions(AkCaps::CapsType type) const;
        Q_INVOKABLE QVariant codecOptionValue(AkCaps::CapsType type,
                                              const QString &option) const;
        Q_INVOKABLE int bitrate(AkCaps::CapsType type) const;
        Q_INVOKABLE int defaultBitrate(AkCaps::CapsType type) const;

    private:
        LocalStreamingPrivate *d;

    signals:
        void locationChanged(const QString &location);
        void audioCapsChanged(const AkAudioCaps &audioCaps);
        void videoCapsChanged(const AkVideoCaps &videoCaps);
        void videoGOPChanged(int videoGOP);
        void stateChanged(AkElement::ElementState state);
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
        void setLocation(const QString &location);
        void setAudioCaps(const AkAudioCaps &audioCaps);
        void setVideoCaps(const AkVideoCaps &videoCaps);
        void setVideoGOP(int gop);
        bool setState(AkElement::ElementState state);
        void setCodec(AkCaps::CapsType type, const QString &codec);
        void setCodecOptionValue(AkCaps::CapsType type,
                                 const QString &option,
                                 const QVariant &value);
        void setBitrate(AkCaps::CapsType type, int bitrate);
        void resetLocation();
        void resetAudioCaps();
        void resetVideoCaps();
        void resetVideoGOP();
        void resetState();
        void resetCodec(AkCaps::CapsType type);
        void resetCodecOptionValue(AkCaps::CapsType type,
                                   const QString &option);
        void resetCodecOptions(AkCaps::CapsType type);
        void resetBitrate(AkCaps::CapsType type);
        AkPacket iStream(const AkPacket &packet);
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);
};

#endif // LOCALSTREAMING_H

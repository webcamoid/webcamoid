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

#include "encoding.h"

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
    Q_PROPERTY(int httpPort
               READ httpPort
               WRITE setHttpPort
               RESET resetHttpPort
               NOTIFY httpPortChanged)
    Q_PROPERTY(int rtspPort
               READ rtspPort
               WRITE setRtspPort
               RESET resetRtspPort
               NOTIFY rtspPortChanged)
    Q_PROPERTY(QString resource
               READ resource
               WRITE setResource
               RESET resetResource
               NOTIFY resourceChanged)
    Q_PROPERTY(bool authEnabled
               READ authEnabled
               WRITE setAuthEnabled
               RESET resetAuthEnabled
               NOTIFY authEnabledChanged)
    Q_PROPERTY(QString user
               READ user
               WRITE setUser
               RESET resetUser
               NOTIFY userChanged)
    Q_PROPERTY(QString password
               READ password
               WRITE setPassword
               RESET resetPassword
               NOTIFY passwordChanged)
    Q_PROPERTY(QString placeholderImage
               READ placeholderImage
               WRITE setPlaceholderImage
               RESET resetPlaceholderImage
               NOTIFY placeholderImageChanged)
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
    Q_PROPERTY(bool hasActiveClients
               READ hasActiveClients
               NOTIFY hasActiveClientsChanged)

    public:
        LocalStreaming(const EncodingPtr &encoding,
                       QQmlApplicationEngine *engine=nullptr,
                       QObject *parent=nullptr);
        ~LocalStreaming();

        Q_INVOKABLE QString location() const;
        Q_INVOKABLE QString defaultURL() const;
        Q_INVOKABLE int httpPort() const;
        Q_INVOKABLE int rtspPort() const;
        Q_INVOKABLE QString resource() const;
        Q_INVOKABLE bool authEnabled() const;
        Q_INVOKABLE QString user() const;
        Q_INVOKABLE QString password() const;
        Q_INVOKABLE QString placeholderImage() const;
        Q_INVOKABLE AkAudioCaps audioCaps() const;
        Q_INVOKABLE AkVideoCaps videoCaps() const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE bool isLocalStreamingSupported() const;
        Q_INVOKABLE QString locationFormat(const QString &location) const;
        Q_INVOKABLE QStringList videoFormats() const;
        Q_INVOKABLE QString defaultVideoFormat() const;
        Q_INVOKABLE bool hasActiveClients() const;
        Q_INVOKABLE QString formatDescription(const QString &format) const;
        Q_INVOKABLE bool isVideoOnlyFormat(const QString &format) const;
        Q_INVOKABLE QString codec(AkCaps::CapsType type) const;
        Q_INVOKABLE QString defaultCodec(const QString &format,
                                         AkCaps::CapsType type) const;
        Q_INVOKABLE QStringList supportedCodecs(const QString &format,
                                                AkCaps::CapsType type) const;
        Q_INVOKABLE QString codecDescription(const QString &codec) const;

    private:
        LocalStreamingPrivate *d;

    signals:
        void locationChanged(const QString &location);
        void httpPortChanged(int httpPort);
        void rtspPortChanged(int rtspPort);
        void resourceChanged(const QString &resource);
        void authEnabledChanged(bool authEnabled);
        void userChanged(const QString &user);
        void passwordChanged(const QString &password);
        void placeholderImageChanged(const QString &placeholderImage);
        void hasActiveClientsChanged(bool hasActiveClients);
        void audioCapsChanged(const AkAudioCaps &audioCaps);
        void videoCapsChanged(const AkVideoCaps &videoCaps);
        void stateChanged(AkElement::ElementState state);
        void streamingError(const QString &error);
        void streamingWarning(const QString &warning);

    public slots:
        void setLocation(const QString &location);
        void setHttpPort(int httpPort);
        void setRtspPort(int rtspPort);
        void setResource(const QString &resource);
        void setAuthEnabled(bool authEnabled);
        void setUser(const QString &user);
        void setPassword(const QString &password);
        void setPlaceholderImage(const QString &placeholderImage);
        void setAudioCaps(const AkAudioCaps &audioCaps);
        void setVideoCaps(const AkVideoCaps &videoCaps);
        bool setState(AkElement::ElementState state);
        void resetLocation();
        void resetHttpPort();
        void resetRtspPort();
        void resetResource();
        void resetAuthEnabled();
        void resetUser();
        void resetPassword();
        void resetPlaceholderImage();
        void resetAudioCaps();
        void resetVideoCaps();
        void resetState();
        AkPacket iStream(const AkPacket &packet);
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);
};

#endif // LOCALSTREAMING_H

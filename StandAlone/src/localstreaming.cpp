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

#include <QFileInfo>
#include <QHostAddress>
#include <QMutex>
#include <QNetworkInterface>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <ak.h>
#include <akaudiocaps.h>
#include <akcaps.h>
#include <akcompressedcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>
#include <iak/akelement.h>
#include <iak/akaudioencoder.h>
#include <iak/akvideoencoder.h>
#include <iak/akvideostreamer.h>

#include "localstreaming.h"
#include "encoding.h"

#define DEFAULT_HTTP_PORT 8080
#define DEFAULT_RTSP_PORT 8554
#define DEFAULT_RESOURCE "stream"
#define DEFAULT_USER "webcamoid"
#define PASSWORD_LENGTH 8

struct LocalStreamingFormatInfo
{
    QString pluginID;
    QString name;
    QString description;
    QStringList audioCodecs; // Generic codecs.
    QStringList videoCodecs;
    QString defaultAudioCodec;
    QString defaultVideoCodec;
};

struct LocalStreamingPluginPriority
{
    QString pluginID;
    int priority;
    bool hasHwCodec {false};
};

using ObjectPtr = QSharedPointer<QObject>;

class LocalStreamingPrivate
{
    public:
        LocalStreaming *self;
        QQmlApplicationEngine *m_engine {nullptr};
        quint32 m_localPort {DEFAULT_HTTP_PORT};
        QString m_localResource {DEFAULT_RESOURCE};
        QString m_location;
        bool m_enabled {false};

        // Caps / state
        AkAudioCaps m_audioCaps;
        AkVideoCaps m_videoCaps;
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        bool m_isStreaming {false};
        bool m_pause {false};

        // Shared encoding settings
        EncodingPtr m_encoding;

        QVector<LocalStreamingFormatInfo> m_supportedFormats;
        QString m_defaultFormat;

        // Active encoder/streamer instances
        AkVideoStreamerPtr m_streamer;

        QMetaObject::Connection m_audioHeadersChangedConnection;
        QMetaObject::Connection m_videoHeadersChangedConnection;
        std::atomic<bool> m_hasActiveClients {false};
        QMutex m_mutex;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        QTimer m_ipCheckTimer;

        // Other settings of the local server.
        quint16 m_rtspPort {DEFAULT_RTSP_PORT};
        bool m_authEnabled {false};
        QString m_user;
        QString m_password;
        QString m_placeholderImage;

        explicit LocalStreamingPrivate(LocalStreaming *self);
        void loadConfigs();
        void initSupportedFormats();
        QString getLocalIPAddress() const;
        QString buildLocation() const;
        void updateLocation();
        static QString randomPassword();
        void ensureCredentials();
        void saveValue(const QString &key, const QVariant &value);
        QString extensionForFormat(const QString &formatId) const;
        static AkVideoStreamerPtr streamerPluginForUrl(const QString &url);
        QString formatFromLocation(const QString &location) const;
        QString defaultCodec(const QString &format,
                             AkCaps::CapsType type) const;
        static QString normalizePluginID(const QString &pluginID);
        void printStreamingParameters();
        bool init();
        void uninit();
        bool startServer();
        void stopServer();
        void applyConfig();
        void saveLocation(const QString &location);
        void saveAudioCaps(const AkAudioCaps &audioCaps);
        void saveVideoCaps(const AkVideoCaps &videoCaps);
};

LocalStreaming::LocalStreaming(const EncodingPtr &encoding,
                               QQmlApplicationEngine *engine,
                               QObject *parent):
    QObject(parent)
{
    this->d = new LocalStreamingPrivate(this);
    this->d->m_encoding = encoding;
    this->setQmlEngine(engine);
    this->d->initSupportedFormats();
    this->d->loadConfigs();

    // The server restarts itself, and its clients are disconnected, to use
    // the new encoding settings: wait until the user finishes editing them
    // instead of restarting it with every change.
    QObject::connect(this->d->m_encoding.data(),
                     &Encoding::changesCommitted,
                     this,
                     [this] () {
        // The codecs available for each format depend on the encoders.
        this->d->initSupportedFormats();
        this->d->applyConfig();
    });

    this->d->m_ipCheckTimer.setInterval(5000);
    QObject::connect(&this->d->m_ipCheckTimer,
                     &QTimer::timeout,
                    this,
                     [this] () {
        this->d->updateLocation();
    });
    this->d->m_ipCheckTimer.start();

    // The server runs while the location is set, whatever the state of the
    // pipeline is.
    if (this->d->m_enabled)
        this->d->startServer();
}

LocalStreaming::~LocalStreaming()
{
    this->setState(AkElement::ElementStateNull);
    this->d->stopServer();
    delete this->d;
}

QString LocalStreaming::location() const
{
    return this->d->m_location;
}

QString LocalStreaming::defaultURL() const
{
    return QString("http://%1:%2/")
           .arg(this->d->getLocalIPAddress())
           .arg(this->d->m_localPort);
}

int LocalStreaming::httpPort() const
{
    return int(this->d->m_localPort);
}

int LocalStreaming::rtspPort() const
{
    return this->d->m_rtspPort;
}

QString LocalStreaming::resource() const
{
    return this->d->m_localResource;
}

bool LocalStreaming::authEnabled() const
{
    return this->d->m_authEnabled;
}

QString LocalStreaming::user() const
{
    return this->d->m_user;
}

QString LocalStreaming::password() const
{
    return this->d->m_password;
}

QString LocalStreaming::placeholderImage() const
{
    return this->d->m_placeholderImage;
}

AkAudioCaps LocalStreaming::audioCaps() const
{
    return this->d->m_audioCaps;
}

AkVideoCaps LocalStreaming::videoCaps() const
{
    return this->d->m_videoCaps;
}

AkElement::ElementState LocalStreaming::state() const
{
    return this->d->m_state;
}

bool LocalStreaming::isLocalStreamingSupported() const
{
    auto plugins =
            akPluginManager->listPlugins("^LocalStreaming([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);

    return !plugins.isEmpty();
}

QString LocalStreaming::locationFormat(const QString &location) const
{
    return this->d->formatFromLocation(location);
}

QStringList LocalStreaming::videoFormats() const
{
    QStringList formats;

    for (auto &format: this->d->m_supportedFormats) {
        QString id = format.pluginID + ':' + format.name;

        if (!formats.contains(id))
            formats << id;
    }

    return formats;
}

QString LocalStreaming::defaultVideoFormat() const
{
    return this->d->m_defaultFormat;
}

bool LocalStreaming::hasActiveClients() const
{
    return this->d->m_hasActiveClients;
}

QString LocalStreaming::formatDescription(const QString &format) const
{
    auto formatParts = format.split(':');

    if (formatParts.size() < 2)
        return {};

    auto pluginID = formatParts[0];
    auto muxerID = formatParts[1];

    auto it = std::find_if(this->d->m_supportedFormats.begin(),
                           this->d->m_supportedFormats.end(),
                           [&pluginID, &muxerID] (const LocalStreamingFormatInfo &formatInfo) -> bool {
        return formatInfo.pluginID == pluginID && formatInfo.name == muxerID;
    });

    if (it == this->d->m_supportedFormats.end())
        return {};

    return it->description;
}

bool LocalStreaming::isVideoOnlyFormat(const QString &format) const
{
    auto formatParts = format.split(':');

    if (formatParts.size() < 2)
        return false;

    auto pluginID = formatParts[0];
    auto muxerID = formatParts[1];

    auto muxerPlugin = akPluginManager->create<AkVideoStreamer>(pluginID);

    if (!muxerPlugin)
        return false;

    return muxerPlugin->isVideoOnlyFormat(muxerID);
}

QString LocalStreaming::codec(AkCaps::CapsType type) const
{
    // The codecs depend on the format of the location.
    return this->d->defaultCodec(this->d->formatFromLocation(this->d->m_location),
                                 type);
}

QString LocalStreaming::defaultCodec(const QString &format,
                                     AkCaps::CapsType type) const
{
    return this->d->defaultCodec(format, type);
}

QStringList LocalStreaming::supportedCodecs(const QString &format,
                                            AkCaps::CapsType type) const
{
    auto formatParts = format.split(':');

    if (formatParts.size() < 2)
        return {};

    auto pluginID = formatParts[0];
    auto muxerID = formatParts[1];

    auto it = std::find_if(this->d->m_supportedFormats.begin(),
                           this->d->m_supportedFormats.end(),
                           [&pluginID, &muxerID] (const LocalStreamingFormatInfo &formatInfo) -> bool {
        return formatInfo.pluginID == pluginID && formatInfo.name == muxerID;
    });

    if (it == this->d->m_supportedFormats.end())
        return {};

    QStringList codecs;

    if (type == AkCaps::CapsAudio || type == AkCaps::CapsAny)
        codecs << it->audioCodecs;

    if (type == AkCaps::CapsVideo || type == AkCaps::CapsAny)
        codecs << it->videoCodecs;

    return codecs;
}

QString LocalStreaming::codecDescription(const QString &codec) const
{
    return this->d->m_encoding->codecDescription(codec);
}

// The location is the address of the web page of the server:
// http://<local IP address>:<HTTP port>/. The IP address is always the one of
// this computer, so only the port of the given location is used.
void LocalStreaming::setLocation(const QString &location)
{
    auto enabled = !location.isEmpty();

    if (enabled) {
        auto port = QUrl(location).port();

        if (port > 0
            && port <= 65535
            && quint32(port) != this->d->m_localPort) {
            this->d->m_localPort = quint32(port);
            emit this->httpPortChanged(port);
            this->d->saveValue("localPort", port);
        }
    }

    auto newLocation = enabled? this->d->buildLocation(): QString();

    if (this->d->m_enabled == enabled && this->d->m_location == newLocation)
        return;

    this->d->m_enabled = enabled;
    this->d->m_location = newLocation;
    emit this->locationChanged(newLocation);
    this->d->saveLocation(newLocation);

    if (!this->d->m_enabled)
        this->d->stopServer();
    else if (!this->d->m_streamer)
        this->d->startServer();
    else
        this->d->applyConfig();
}

void LocalStreaming::setHttpPort(int httpPort)
{
    httpPort = qBound(1, httpPort, 65535);

    if (int(this->d->m_localPort) == httpPort)
        return;

    this->d->m_localPort = quint32(httpPort);
    emit this->httpPortChanged(httpPort);
    this->d->saveValue("localPort", httpPort);

    // The address of the server includes the port.
    this->d->updateLocation();
}

void LocalStreaming::setRtspPort(int rtspPort)
{
    rtspPort = qBound(1, rtspPort, 65535);

    if (this->d->m_rtspPort == rtspPort)
        return;

    this->d->m_rtspPort = quint16(rtspPort);
    emit this->rtspPortChanged(rtspPort);
    this->d->saveValue("rtspPort", rtspPort);
    this->d->applyConfig();
}

// It is the base name of the paths of the streams, so it can only have the
// characters that the server accepts.
void LocalStreaming::setResource(const QString &resource)
{
    static const QRegularExpression resourceValidator("^[A-Za-z0-9_.-]+$");
    auto value = resource.trimmed();

    if (value.isEmpty())
        value = DEFAULT_RESOURCE;

    if (!resourceValidator.match(value).hasMatch()) {
        qWarning() << "Invalid resource name:" << resource;

        return;
    }

    if (this->d->m_localResource == value)
        return;

    this->d->m_localResource = value;
    emit this->resourceChanged(value);
    this->d->saveValue("localResource", value);
    this->d->applyConfig();
}

// With the authentication enabled, empty fields are filled: a default user
// and a random password.
void LocalStreaming::setAuthEnabled(bool authEnabled)
{
    if (this->d->m_authEnabled == authEnabled)
        return;

    this->d->m_authEnabled = authEnabled;

    if (authEnabled)
        this->d->ensureCredentials();

    emit this->authEnabledChanged(authEnabled);
    this->d->saveValue("authEnabled", authEnabled);
    this->d->applyConfig();
}

void LocalStreaming::setUser(const QString &user)
{
    auto value = user.trimmed();

    if (this->d->m_authEnabled && value.isEmpty())
        value = DEFAULT_USER;

    if (this->d->m_user == value)
        return;

    this->d->m_user = value;
    emit this->userChanged(value);
    this->d->saveValue("user", value);
    this->d->applyConfig();
}

void LocalStreaming::setPassword(const QString &password)
{
    auto value = password;

    if (this->d->m_authEnabled && value.isEmpty())
        value = LocalStreamingPrivate::randomPassword();

    if (this->d->m_password == value)
        return;

    this->d->m_password = value;
    emit this->passwordChanged(value);
    this->d->saveValue("password", value);
    this->d->applyConfig();
}

// The path of an image file.
void LocalStreaming::setPlaceholderImage(const QString &placeholderImage)
{
    auto value = placeholderImage.trimmed();

    if (value.startsWith("file:"))
        value = QUrl(value).toLocalFile();

    if (this->d->m_placeholderImage == value)
        return;

    this->d->m_placeholderImage = value;
    emit this->placeholderImageChanged(value);
    this->d->saveValue("placeholderImage", value);
    this->d->applyConfig();
}

void LocalStreaming::setAudioCaps(const AkAudioCaps &audioCaps)
{
    if (this->d->m_audioCaps == audioCaps)
        return;

    this->d->m_audioCaps = audioCaps;
    emit this->audioCapsChanged(audioCaps);
    this->d->saveAudioCaps(audioCaps);
    this->d->applyConfig();
}

void LocalStreaming::setVideoCaps(const AkVideoCaps &videoCaps)
{
    if (this->d->m_videoCaps == videoCaps)
        return;

    this->d->m_videoCaps = videoCaps;
    emit this->videoCapsChanged(videoCaps);
    this->d->saveVideoCaps(videoCaps);
    this->d->applyConfig();
}

bool LocalStreaming::setState(AkElement::ElementState state)
{
    switch (this->d->m_state) {
    case AkElement::ElementStateNull:
        switch (state) {
        case AkElement::ElementStatePaused:
            this->d->m_pause = true;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        case AkElement::ElementStatePlaying:
            if (!this->d->init())
                return false;

            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        default:
            break;
        }

        break;
    case AkElement::ElementStatePaused:
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->uninit();
            this->d->m_pause = false;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        case AkElement::ElementStatePlaying:
            this->d->m_pause = false;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        default:
            break;
        }

        break;
    case AkElement::ElementStatePlaying:
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->uninit();
            this->d->m_pause = false;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        case AkElement::ElementStatePaused:
            this->d->m_pause = true;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        default:
            break;
        }

        break;
    }

    return false;
}

void LocalStreaming::resetLocation()
{
    this->setLocation({});
}

void LocalStreaming::resetHttpPort()
{
    this->setHttpPort(DEFAULT_HTTP_PORT);
}

void LocalStreaming::resetRtspPort()
{
    this->setRtspPort(DEFAULT_RTSP_PORT);
}

void LocalStreaming::resetResource()
{
    this->setResource(DEFAULT_RESOURCE);
}

void LocalStreaming::resetAuthEnabled()
{
    this->setAuthEnabled(false);
}

void LocalStreaming::resetUser()
{
    this->setUser({});
}

void LocalStreaming::resetPassword()
{
    this->setPassword({});
}

void LocalStreaming::resetPlaceholderImage()
{
    this->setPlaceholderImage({});
}

void LocalStreaming::resetAudioCaps()
{
    this->setAudioCaps({});
}

void LocalStreaming::resetVideoCaps()
{
    this->setVideoCaps({});
}

void LocalStreaming::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

// The raw audio and video go straight to the server, it encodes them as the
// clients ask for.
AkPacket LocalStreaming::iStream(const AkPacket &packet)
{
    if (!this->d->m_hasActiveClients.load(std::memory_order_relaxed))
        return {};

    if (packet.type() != AkPacket::PacketAudio
        && packet.type() != AkPacket::PacketVideo)
        return {};

    // The server can be replaced from another thread.
    QMutexLocker locker(&this->d->m_mutex);

    if (this->d->m_streamer)
        this->d->m_streamer->iStream(packet);

    return {};
}

void LocalStreaming::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("localStreaming", this);
}

LocalStreamingPrivate::LocalStreamingPrivate(LocalStreaming *self):
    self(self)
{
}

void LocalStreamingPrivate::initSupportedFormats()
{
    this->m_supportedFormats.clear();

    auto muxerPlugins =
            akPluginManager->listPlugins("^LocalStreaming([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);
    QVector<LocalStreamingPluginPriority> formatsPriority;

    // The codec of each format is a generic one, a codec is available when
    // the muxer supports the codec ID used by the encoder currently associated
    // to it.
    auto codecsFor = [this] (AkCaps::CapsType type,
                             const QList<AkCodecID> &supported,
                             AkCodecID defaultID,
                             QString &defaultCodec) -> QStringList {
        QStringList codecs;

        for (auto &codec: this->m_encoding->codecs(type)) {
            auto id = this->m_encoding->encoderCodecID(type, codec);

            if (!supported.contains(id))
                continue;

            codecs << codec;

            if (id == defaultID)
                defaultCodec = codec;
        }

        if (defaultCodec.isEmpty())
            defaultCodec = codecs.value(0);

        return codecs;
    };

    for (auto &muxerPluginId: muxerPlugins) {
        auto muxerInfo = akPluginManager->pluginInfo(muxerPluginId);
        auto muxerPlugin = akPluginManager->create<AkVideoStreamer>(muxerPluginId);

        if (!muxerPlugin)
            continue;

        for (auto &protocol: muxerPlugin->protocols()) {
            for (auto &muxer: muxerPlugin->supportedFormats(protocol)) {
                QString defaultAudioCodec;
                auto audioCodecs =
                        codecsFor(AkCaps::CapsAudio,
                                  muxerPlugin->supportedCodecs(muxer,
                                                               AkCompressedCaps::CapsType_Audio),
                                  muxerPlugin->defaultCodec(muxer,
                                                            AkCompressedCaps::CapsType_Audio),
                                  defaultAudioCodec);

                if (audioCodecs.isEmpty()
                    && !muxerPlugin->isVideoOnlyFormat(muxer))
                    continue;

                QString defaultVideoCodec;
                auto videoCodecs =
                        codecsFor(AkCaps::CapsVideo,
                                  muxerPlugin->supportedCodecs(muxer,
                                                               AkCompressedCaps::CapsType_Video),
                                  muxerPlugin->defaultCodec(muxer,
                                                            AkCompressedCaps::CapsType_Video),
                                  defaultVideoCodec);

                if (videoCodecs.isEmpty())
                    continue;

                this->m_supportedFormats << LocalStreamingFormatInfo {
                    muxerPluginId,
                    muxer,
                    muxerPlugin->description(muxer),
                    audioCodecs,
                    videoCodecs,
                    defaultAudioCodec,
                    defaultVideoCodec
                };

                bool formatHasHwCodec = false;

                for (auto &codec: videoCodecs)
                    if (this->m_encoding->encoderIsHardware(this->m_encoding->encoder(AkCaps::CapsVideo, codec))) {
                        formatHasHwCodec = true;

                        break;
                    }

                formatsPriority << LocalStreamingPluginPriority {muxerPluginId + ':' + muxer,
                                                   muxerInfo.priority(),
                                                   formatHasHwCodec};
            }
        }
    }

    std::sort(this->m_supportedFormats.begin(),
              this->m_supportedFormats.end(),
              [] (const LocalStreamingFormatInfo &fi1, const LocalStreamingFormatInfo &fi2) {
        return fi1.description < fi2.description;
    });

    if (formatsPriority.isEmpty()) {
        this->m_defaultFormat = {};

        return;
    }

    std::sort(formatsPriority.begin(),
              formatsPriority.end(),
              [] (const LocalStreamingPluginPriority &plugin1,
                  const LocalStreamingPluginPriority &plugin2) -> bool {
#if 0
        if (plugin1.hasHwCodec != plugin2.hasHwCodec)
            return plugin1.hasHwCodec > plugin2.hasHwCodec;
#endif
        return plugin1.priority > plugin2.priority;
    });
    this->m_defaultFormat = formatsPriority.first().pluginID;
}

QString LocalStreamingPrivate::getLocalIPAddress() const
{
    auto interfaces = QNetworkInterface::allInterfaces();

    for (auto &iface: interfaces) {
        if (!iface.isValid() || !(iface.flags() & QNetworkInterface::IsRunning))
            continue;

        if (iface.flags() & QNetworkInterface::IsLoopBack)
            continue;

        for (const auto &entry: iface.addressEntries()) {
            auto ip = entry.ip();

            if (ip.protocol() == QAbstractSocket::IPv4Protocol)
                return ip.toString();
        }
    }

    return QHostAddress(QHostAddress::LocalHost).toString();
}

QString LocalStreamingPrivate::buildLocation() const
{
    return QString("http://%1:%2/")
           .arg(this->getLocalIPAddress())
           .arg(this->m_localPort);
}

QString LocalStreamingPrivate::randomPassword()
{
    static const char passwordChars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    QString password;

    for (int i = 0; i < PASSWORD_LENGTH; i++)
        password += QChar(passwordChars[QRandomGenerator::system()->bounded(int(sizeof(passwordChars)) - 1)]);

    return password;
}

// Fills the empty credentials. Used when the authentication is enabled.
void LocalStreamingPrivate::ensureCredentials()
{
    if (this->m_user.isEmpty()) {
        this->m_user = DEFAULT_USER;
        emit self->userChanged(this->m_user);
        this->saveValue("user", this->m_user);
    }

    if (this->m_password.isEmpty()) {
        this->m_password = LocalStreamingPrivate::randomPassword();
        emit self->passwordChanged(this->m_password);
        this->saveValue("password", this->m_password);
    }
}

void LocalStreamingPrivate::saveValue(const QString &key, const QVariant &value)
{
    QSettings config;
    config.beginGroup("LocalStreamingConfigs");
    config.setValue(key, value);
    config.endGroup();
}

void LocalStreamingPrivate::updateLocation()
{
    if (!this->m_enabled) {
        if (this->m_location.isEmpty())
            return;

        this->m_location.clear();
        emit self->locationChanged(this->m_location);

        return;
    }

    auto newLocation = this->buildLocation();

    if (newLocation == this->m_location)
        return;

    this->m_location = newLocation;
    emit self->locationChanged(this->m_location);

    // The server announces the address, it restarts to use the new one.
    if (this->m_streamer)
        this->applyConfig();
}

QString LocalStreamingPrivate::extensionForFormat(const QString &formatId) const
{
    if (formatId.isEmpty())
        return {};

    auto parts = formatId.split(':');

    return parts.size() > 1? parts[1]: formatId;
}

// Returns the streamer plugin ID that best handles a given URL.
AkVideoStreamerPtr LocalStreamingPrivate::streamerPluginForUrl(const QString &url)
{
    auto plugins =
            akPluginManager->listPlugins("^LocalStreaming([/]([0-9a-zA-Z_])+)+$",
                                          {},
                                          AkPluginManager::FilterEnabled
                                          | AkPluginManager::FilterRegexp);

    for (auto &id: plugins) {
        auto plugin = akPluginManager->create<AkVideoStreamer>(id);

        if (plugin && plugin->supportsUrl(url))
            return plugin;
    }

    return {};
}

// Infers format from location URL extension.
// Returns pluginID:formatName or empty if cannot determine.
QString LocalStreamingPrivate::formatFromLocation(const QString &location) const
{
    if (location.isEmpty())
        return {};

    QFileInfo fileInfo(location);
    QString ext = fileInfo.suffix().toLower();

    if (ext.isEmpty())
        return {};

    // Find a format whose name matches the extension
    for (auto &format: this->m_supportedFormats)
        if (format.name == ext)
            return format.pluginID + ':' + format.name;

    return {};
}

QString LocalStreamingPrivate::defaultCodec(const QString &format,
                                            AkCaps::CapsType type) const
{
    auto formatParts = format.split(':');

    if (formatParts.size() < 2)
        return {};

    auto pluginID = formatParts[0];
    auto muxerID = formatParts[1];

    auto it = std::find_if(this->m_supportedFormats.begin(),
                           this->m_supportedFormats.end(),
                           [&pluginID, &muxerID] (const LocalStreamingFormatInfo &formatInfo) -> bool {
        return formatInfo.pluginID == pluginID && formatInfo.name == muxerID;
    });

    if (it == this->m_supportedFormats.end())
        return {};

    switch (type) {
    case AkCaps::CapsAudio:
        return it->defaultAudioCodec;

    case AkCaps::CapsVideo:
        return it->defaultVideoCodec;

    default:
        break;
    }

    return {};
}

void LocalStreamingPrivate::loadConfigs()
{
    QSettings config;
    config.beginGroup("LocalStreamingConfigs");

    // Location
    this->m_enabled = config.value("enabled", false).toBool();
    this->m_localPort = config.value("localPort", DEFAULT_HTTP_PORT).toUInt();
    this->m_localResource = config.value("localResource", DEFAULT_RESOURCE).toString();
    this->m_rtspPort = quint16(config.value("rtspPort", DEFAULT_RTSP_PORT).toUInt());
    this->m_user = config.value("user").toString();
    this->m_password = config.value("password").toString();
    this->m_authEnabled = config.value("authEnabled", !this->m_user.isEmpty()).toBool();
    this->m_placeholderImage = config.value("placeholderImage").toString();

    if (this->m_authEnabled)
        this->ensureCredentials();

    this->updateLocation();

    // Video / audio caps
    auto outputWidth      = qMax(config.value("outputWidth",     1280 ).toInt(), 160);
    auto outputHeight     = qMax(config.value("outputHeight",    720  ).toInt(), 90);
    auto outputFPS        = qMax(config.value("outputFPS",       30   ).toInt(), 1);

    this->m_videoCaps = {AkVideoCaps::Format_yuv420p,
                         outputWidth, outputHeight, {outputFPS, 1}};
    this->m_audioCaps = {AkAudioCaps::SampleFormat_s16,
                         AkAudioCaps::Layout_stereo,
                         false,
                         this->m_encoding->audioSampleRate()};

    config.endGroup();
}

QString LocalStreamingPrivate::normalizePluginID(const QString &pluginID)
{
    static const char *valid =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    QString result;

    for (auto &c: pluginID)
        result += (std::strchr(valid, c.toLatin1())? c: QChar('_'));

    return result;
}

void LocalStreamingPrivate::printStreamingParameters()
{
    qInfo() << "Local streaming parameters:";
    qInfo() << "    Location:" << this->m_location;
    qInfo() << "    RTSP port:" << this->m_rtspPort;
    qInfo() << "    Resource:" << this->m_localResource;
    qInfo() << "    Authentication:" << this->m_authEnabled;

    {
        qInfo() << "    Audio:";
        qInfo() << "        sample format:" << this->m_audioCaps.format();
        qInfo() << "        channels:" << this->m_audioCaps.channels();
        qInfo() << "        layout:" << this->m_audioCaps.layout();
        qInfo() << "        sample rate:" << this->m_audioCaps.rate();
        qInfo() << "        codec:" << self->codec(AkCaps::CapsAudio);
        qInfo() << "        bitrate:" << this->m_encoding->audioBitrate();
    }

    qInfo() << "    Video:";
    qInfo() << "        pixel format:" << this->m_videoCaps.format();
    qInfo() << "        width:" << this->m_videoCaps.width();
    qInfo() << "        height:" << this->m_videoCaps.height();
    qInfo() << "        frame rate:" << this->m_videoCaps.fps().toString();
    qInfo() << "        codec:" << self->codec(AkCaps::CapsVideo);
    qInfo() << "        bitrate:" << this->m_encoding->videoBitrate();
}

bool LocalStreamingPrivate::init()
{
    // The server is on while the location is set, no matter the state of the
    // pipeline: here it only has to be running.
    return this->startServer();
}

void LocalStreamingPrivate::uninit()
{
    // Nothing to do: the server keeps running while the location is set. It
    // sends a placeholder while the pipeline is not delivering packets.
}

bool LocalStreamingPrivate::startServer()
{
    if (this->m_streamer)
        return true;

    qInfo() << "Starting local streaming";
    this->printStreamingParameters();

    if (!this->m_enabled || this->m_location.isEmpty()) {
        qCritical() << "Location (URL) not set";

        return false;
    }

    // Select the streamer plugin based on the URL
    auto streamer = streamerPluginForUrl(this->m_location);

    if (!streamer) {
        qCritical() << "No streamer plugin found for" << this->m_location;

        return false;
    }

    QObject::connect(streamer.data(),
                     &AkVideoStreamer::hasActiveClientsChanged,
                     [this] (bool hasActiveClients) {
                         this->m_hasActiveClients.store(hasActiveClients,
                                                        std::memory_order_relaxed);
                         emit self->hasActiveClientsChanged(hasActiveClients);
                     });
    QObject::connect(streamer.data(),
                     &AkVideoStreamer::streamingError,
                     self,
                     &LocalStreaming::streamingError);

    {
        QMutexLocker locker(&this->m_mutex);
        this->m_streamer = streamer;
    }

    this->applyConfig();
    streamer->setState(AkElement::ElementStatePlaying);
    qInfo() << "Local streaming started:" << this->m_location;

    return true;
}

void LocalStreamingPrivate::stopServer()
{
    AkVideoStreamerPtr streamer;

    {
        QMutexLocker locker(&this->m_mutex);
        streamer = this->m_streamer;
        this->m_streamer = {};
    }

    if (!streamer)
        return;

    qInfo() << "Stopping local streaming";

    QObject::disconnect(streamer.data(),
                        &AkVideoStreamer::hasActiveClientsChanged,
                        nullptr, nullptr);
    QObject::disconnect(streamer.data(),
                        &AkVideoStreamer::streamingError,
                        self,
                        &LocalStreaming::streamingError);

    if (this->m_hasActiveClients.exchange(false, std::memory_order_relaxed))
        emit self->hasActiveClientsChanged(false);

    streamer->setState(AkElement::ElementStateNull);

    // Releasing the last reference destroys the server.
    streamer.clear();
    qInfo() << "Local streaming stopped";
}

// Pass the configuration to the server. Whatever changed, the server restarts
// itself once, to use it.
void LocalStreamingPrivate::applyConfig()
{
    AkVideoStreamerPtr streamer;

    {
        QMutexLocker locker(&this->m_mutex);
        streamer = this->m_streamer;
    }

    if (!streamer || this->m_location.isEmpty())
        return;

    streamer->setProperty("videoCaps", QVariant::fromValue(this->m_videoCaps));
    auto audioCaps = this->m_audioCaps;
    audioCaps.setRate(this->m_encoding->audioSampleRate());
    streamer->setProperty("audioCaps", QVariant::fromValue(audioCaps));
    streamer->setProperty("gop", this->m_encoding->videoGOP());
    streamer->setProperty("encoderPreferences",
                          this->m_encoding->encoderPreferences());
    streamer->setProperty("rtspPort", this->m_rtspPort);
    streamer->setProperty("resource", this->m_localResource);
    // The server asks for the credentials when it has a user.
    streamer->setProperty("user", this->m_authEnabled? this->m_user: QString());
    streamer->setProperty("password", this->m_authEnabled? this->m_password: QString());
    streamer->setProperty("placeholderImage", this->m_placeholderImage);
    streamer->setStreamBitrate(AkCompressedCaps::CapsType_Video,
                               this->m_encoding->videoBitrate());
    streamer->setStreamBitrate(AkCompressedCaps::CapsType_Audio,
                               this->m_encoding->audioBitrate());
    streamer->setDestinations({this->m_location});
}

void LocalStreamingPrivate::saveLocation(const QString &location)
{
    // The port, the resource name and the rest of the settings are saved when
    // they change.
    this->saveValue("enabled", !location.isEmpty());
}

void LocalStreamingPrivate::saveAudioCaps(const AkAudioCaps &audioCaps)
{
    QSettings config;
    config.beginGroup("LocalStreamingConfigs");
    config.setValue("audioSampleRate", audioCaps.rate());
    config.endGroup();
}

void LocalStreamingPrivate::saveVideoCaps(const AkVideoCaps &videoCaps)
{
    QSettings config;
    config.beginGroup("LocalStreamingConfigs");
    config.setValue("outputWidth",  videoCaps.width());
    config.setValue("outputHeight", videoCaps.height());
    config.setValue("outputFPS",    qRound(videoCaps.fps().value()));
    config.endGroup();
}

#include "moc_localstreaming.cpp"

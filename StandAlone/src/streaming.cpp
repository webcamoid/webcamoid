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

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QThreadPool>
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
#include <iak/akvideomuxer.h>
#include <iak/akvideostreamer.h>

#include "streaming.h"

#define DEFAULT_AUDIO_BITRATE 128000
#define DEFAULT_VIDEO_BITRATE 1500000
#define DEFAULT_VIDEO_GOP 2000

struct StreamingSiteInfo
{
    QString name;
    QString website;
    QString protocol;
    QString streamingUrl;
    QString streamingKey;
    QString keyConfigsUrl;
    QString docsUrl;
    bool needsKey {true};
    bool builtIn {true};
};

struct CodecInfo
{
    QString pluginID;
    AkCaps::CapsType type;
    AkCodecID codecID;
    QString name;
    QString description;
    int priority;
};

struct PluginPriority
{
    QString pluginID;
    int priority;
};

using ObjectPtr = QSharedPointer<QObject>;

class StreamingPrivate
{
    public:
        Streaming *self;
        QQmlApplicationEngine *m_engine {nullptr};

        // Caps / state
        AkAudioCaps m_audioCaps;
        AkVideoCaps m_videoCaps;
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        bool m_isStreaming {false};
        bool m_pause {false};

        // Bitrates
        int m_audioBitrate {DEFAULT_AUDIO_BITRATE};
        int m_videoBitrate {DEFAULT_VIDEO_BITRATE};
        int m_videoGOP {DEFAULT_VIDEO_GOP};

        // Platforms
        QVector<StreamingSiteInfo> m_sites;   // built-in + custom
        QStringList                m_platforms;

        // Codecs
        QVector<CodecInfo> m_supportedCodecs;

        // Active encoder/streamer instances
        AkVideoStreamerPtr m_streamer;
        QString            m_streamerPluginID;
        AkAudioEncoderPtr  m_audioEncoder;
        QString            m_audioPluginID;
        AkVideoEncoderPtr  m_videoEncoder;
        QString            m_videoPluginID;

        QMetaObject::Connection m_audioHeadersChangedConnection;
        QMetaObject::Connection m_videoHeadersChangedConnection;
        QMutex m_mutex;
        QThreadPool m_threadPool;
        AkVideoPacket m_curPacket;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit StreamingPrivate(Streaming *self);
        void loadBuiltInSites();
        void loadBuiltInOverrides();
        void loadConfigs();
        void loadCodecOptions(AkCaps::CapsType type);
        void initSupportedCodecs();
        static QString streamerPluginForUrl(const QString &url);
        QString streamerPluginForPlatform(const QString &platform) const;
        StreamingSiteInfo *findSite(const QString &name);
        const StreamingSiteInfo *findSite(const QString &name) const;
        QString defaultCodecForPlatform(const QString &streamerPluginID,
                                        const QString &platform,
                                        AkCaps::CapsType type) const;
        QStringList supportedCodecsForPlatform(const QString &streamerPluginID,
                                               const QString &platform,
                                               AkCaps::CapsType type) const;
        static QString normalizePluginID(const QString &pluginID);
        void printStreamingParameters();
        bool init();
        void uninit();
        void saveAudioCaps(const AkAudioCaps &audioCaps);
        void saveVideoCaps(const AkVideoCaps &videoCaps);
        void saveCodec(AkCaps::CapsType type, const QString &codec);
        void saveCodecOptionValue(AkCaps::CapsType type,
                                  const QString &option,
                                  const QVariant &value);
        void saveBitrate(AkCaps::CapsType type, int bitrate);
        void saveVideoGOP(int gop);
        void savePlatforms(const QStringList &platforms);
        void saveCustomSites();
        void saveBuiltInSiteOverrides();
};

Streaming::Streaming(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new StreamingPrivate(this);
    this->setQmlEngine(engine);
    this->d->loadConfigs();
}

Streaming::~Streaming()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

AkAudioCaps Streaming::audioCaps() const
{
    return this->d->m_audioCaps;
}

AkVideoCaps Streaming::videoCaps() const
{
    return this->d->m_videoCaps;
}

int Streaming::videoGOP() const
{
    return this->d->m_videoGOP;
}

int Streaming::defaultVideoGOP() const
{
    return DEFAULT_VIDEO_GOP;
}

AkElement::ElementState Streaming::state() const
{
    return this->d->m_state;
}

QStringList Streaming::platforms() const
{
    return this->d->m_platforms;
}

QStringList Streaming::supportedPlatforms() const
{
    QStringList names;

    for (auto &site: this->d->m_sites)
        names << site.name;

    return names;
}

bool Streaming::isStreamingSupported() const
{
    auto plugins =
            akPluginManager->listPlugins("^VideoStreaming([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);

    return !plugins.isEmpty();
}

QString Streaming::platformWebsite(const QString &platform) const
{
    auto site = this->d->findSite(platform);

    return site? site->website: QString();
}

QString Streaming::platformStreamingUrl(const QString &platform) const
{
    auto site = this->d->findSite(platform);

    return site? site->streamingUrl: QString();
}

QString Streaming::platformStreamingKey(const QString &platform) const
{
    auto site = this->d->findSite(platform);

    return site? site->streamingKey: QString();
}

QString Streaming::platformKeyConfigsUrl(const QString &platform) const
{
    auto site = this->d->findSite(platform);

    return site? site->keyConfigsUrl: QString();
}

QString Streaming::platformDocsUrl(const QString &platform) const
{
    auto site = this->d->findSite(platform);

    return site? site->docsUrl: QString();
}

bool Streaming::platformNeedsKey(const QString &platform) const
{
    auto site = this->d->findSite(platform);

    return site? site->needsKey: false;
}

QString Streaming::codec(AkCaps::CapsType type) const
{
    switch (type) {
    case AkCaps::CapsAudio:
        if (this->d->m_audioEncoder)
            return this->d->m_audioPluginID + ':' + this->d->m_audioEncoder->codec();

        break;

    case AkCaps::CapsVideo:
        if (this->d->m_videoEncoder)
            return this->d->m_videoPluginID + ':' + this->d->m_videoEncoder->codec();

        break;
    default:
        break;
    }

    return {};
}

QString Streaming::defaultCodec(const QString &platform,
                                AkCaps::CapsType type) const
{
    auto site = this->d->findSite(platform);

    if (!site)
        return {};

    auto streamerID = this->d->streamerPluginForPlatform(platform);

    return this->d->defaultCodecForPlatform(streamerID, platform, type);
}

QStringList Streaming::supportedCodecs(const QString &platform,
                                       AkCaps::CapsType type) const
{
    auto site = this->d->findSite(platform);

    if (!site)
        return {};

    auto streamerID = this->d->streamerPluginForPlatform(platform);

    return this->d->supportedCodecsForPlatform(streamerID, platform, type);
}

QString Streaming::codecDescription(const QString &codec) const
{
    auto parts = codec.split(':');

    if (parts.size() < 2)
        return {};

    auto pluginID  = parts[0];
    auto codecName = parts[1];

    for (auto &c: this->d->m_supportedCodecs)
        if (c.pluginID == pluginID && c.name == codecName)
            return c.description;

    return {};
}

AkPropertyOptions Streaming::codecOptions(AkCaps::CapsType type) const
{
    switch (type) {
    case AkCaps::CapsAudio:
        return this->d->m_audioEncoder? this->d->m_audioEncoder->options(): AkPropertyOptions();
    case AkCaps::CapsVideo:
        return this->d->m_videoEncoder? this->d->m_videoEncoder->options(): AkPropertyOptions();
    default:
        break;
    }

    return {};
}

QVariant Streaming::codecOptionValue(AkCaps::CapsType type,
                                     const QString &option) const
{
    switch (type) {
    case AkCaps::CapsAudio:
        return this->d->m_audioEncoder? this->d->m_audioEncoder->optionValue(option): QVariant {};
    case AkCaps::CapsVideo:
        return this->d->m_videoEncoder? this->d->m_videoEncoder->optionValue(option): QVariant {};
    default:
        break;
    }

    return {};
}

int Streaming::bitrate(AkCaps::CapsType type) const
{
    switch (type) {
    case AkCaps::CapsAudio:
        return this->d->m_audioBitrate;

    case AkCaps::CapsVideo:
        return this->d->m_videoBitrate;

    default:
        break;
    }

    return 0;
}

int Streaming::defaultBitrate(AkCaps::CapsType type) const
{
    switch (type) {
    case AkCaps::CapsAudio:
        return DEFAULT_AUDIO_BITRATE;

    case AkCaps::CapsVideo:
        return DEFAULT_VIDEO_BITRATE;

    default:
        break;
    }

    return 0;
}

void Streaming::setAudioCaps(const AkAudioCaps &audioCaps)
{
    if (this->d->m_audioCaps == audioCaps)
        return;

    this->d->m_audioCaps = audioCaps;
    emit this->audioCapsChanged(audioCaps);
    this->d->saveAudioCaps(audioCaps);
}

void Streaming::setVideoCaps(const AkVideoCaps &videoCaps)
{
    if (this->d->m_videoCaps == videoCaps)
        return;

    this->d->m_videoCaps = videoCaps;
    emit this->videoCapsChanged(videoCaps);
    this->d->saveVideoCaps(videoCaps);
}

void Streaming::setVideoGOP(int gop)
{
    if (this->d->m_videoGOP == gop)
        return;

    this->d->m_videoGOP = gop;
    emit this->videoGOPChanged(gop);
    this->d->saveVideoGOP(gop);
}

bool Streaming::setState(AkElement::ElementState state)
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

void Streaming::setPlatforms(const QStringList &platforms)
{
    if (this->d->m_platforms == platforms)
        return;

    this->d->m_platforms = platforms;
    emit this->platformsChanged(platforms);
    this->d->savePlatforms(platforms);
}

void Streaming::addSupportedPlatform(const QString &platform)
{
    if (platform.isEmpty() || this->d->findSite(platform))
        return;

    StreamingSiteInfo site;
    site.name    = platform;
    site.builtIn = false;
    this->d->m_sites << site;
    emit this->supportedPlatformsChanged(this->supportedPlatforms());
    this->d->saveCustomSites();
}

void Streaming::removeSupportedPlatform(const QString &platform)
{
    auto it = std::find_if(this->d->m_sites.begin(),
                           this->d->m_sites.end(),
                           [&platform](const StreamingSiteInfo &s) {
                               return s.name == platform;
                           });

    if (it == this->d->m_sites.end())
        return;

    this->d->m_sites.erase(it);
    emit this->supportedPlatformsChanged(this->supportedPlatforms());

    if (this->d->m_platforms.removeAll(platform) > 0)
        emit this->platformsChanged(this->d->m_platforms);

    this->d->saveCustomSites();
}

void Streaming::setPlatformWebsite(const QString &platform,
                                   const QString &website)
{
    auto site = this->d->findSite(platform);

    if (!site || site->website == website)
        return;

    site->website = website;
    this->d->saveCustomSites();
}

void Streaming::setPlatformStreamingUrl(const QString &platform,
                                        const QString &streamingUrl)
{
    auto site = this->d->findSite(platform);

    if (!site || site->streamingUrl == streamingUrl)
        return;

    site->streamingUrl = streamingUrl;

    if (site->builtIn)
        this->d->saveBuiltInSiteOverrides();
    else
        this->d->saveCustomSites();
}

void Streaming::setPlatformStreamingKey(const QString &platform,
                                        const QString &key)
{
    auto site = this->d->findSite(platform);

    if (!site || site->streamingKey == key)
        return;

    site->streamingKey = key;

    if (site->builtIn)
        this->d->saveBuiltInSiteOverrides();
    else
        this->d->saveCustomSites();
}

void Streaming::setPlatformKeyConfigsUrl(const QString &platform,
                                         const QString &keyConfigsUrl)
{
    auto site = this->d->findSite(platform);

    if (!site || site->keyConfigsUrl == keyConfigsUrl)
        return;

    site->keyConfigsUrl = keyConfigsUrl;
    this->d->saveCustomSites();
}

void Streaming::setPlatformDocsUrl(const QString &platform,
                                   const QString &docsUrl)
{
    auto site = this->d->findSite(platform);

    if (!site || site->docsUrl == docsUrl)
        return;

    site->docsUrl = docsUrl;
    this->d->saveCustomSites();
}

void Streaming::setPlatformNeedsKey(const QString &platform,
                                    bool needsKey)
{
    auto site = this->d->findSite(platform);

    if (!site || site->needsKey == needsKey)
        return;

    site->needsKey = needsKey;
    this->d->saveCustomSites();
}

void Streaming::setCodec(AkCaps::CapsType type, const QString &codec)
{
    auto parts      = codec.split(':');
    auto pluginID   = parts.value(0);
    auto codecName  = parts.value(1);

    switch (type) {
    case AkCaps::CapsAudio: {
        auto cur = this->d->m_audioEncoder?
                       this->d->m_audioPluginID + ':' + this->d->m_audioEncoder->codec():
                       QString();

        if (codec == cur)
            return;

        auto enc = akPluginManager->create<AkAudioEncoder>(pluginID);

        if (enc)
            enc->setCodec(codecName);
        else
            qWarning() << "Failed to create audio encoder:" << pluginID;

        this->d->m_audioEncoder  = enc;
        this->d->m_audioPluginID = pluginID;
        emit this->codecChanged(type, codec);
        this->d->saveCodec(type, codec);
        this->d->loadCodecOptions(AkCaps::CapsAudio);

        break;
    }

    case AkCaps::CapsVideo: {
        auto cur = this->d->m_videoEncoder?
                       this->d->m_videoPluginID + ':' + this->d->m_videoEncoder->codec():
                       QString();

        if (codec == cur)
            return;

        auto enc = akPluginManager->create<AkVideoEncoder>(pluginID);

        if (enc)
            enc->setCodec(codecName);
        else
            qWarning() << "Failed to create video encoder:" << pluginID;

        this->d->m_videoEncoder  = enc;
        this->d->m_videoPluginID = pluginID;
        emit this->codecChanged(type, codec);
        this->d->saveCodec(type, codec);
        this->d->loadCodecOptions(AkCaps::CapsVideo);

        break;
    }

    default:
        break;
    }
}

void Streaming::setCodecOptionValue(AkCaps::CapsType type,
                                    const QString &option,
                                    const QVariant &value)
{
    switch (type) {
    case AkCaps::CapsAudio:
        if (!this->d->m_audioEncoder)
            return;

        if (this->d->m_audioEncoder->optionValue(option) == value)
            return;

        this->d->m_audioEncoder->setOptionValue(option, value);
        emit this->codecOptionValueChanged(type, option, value);
        this->d->saveCodecOptionValue(type, option, value);

        break;

    case AkCaps::CapsVideo:
        if (!this->d->m_videoEncoder)
            return;

        if (this->d->m_videoEncoder->optionValue(option) == value)
            return;

        this->d->m_videoEncoder->setOptionValue(option, value);
        emit this->codecOptionValueChanged(type, option, value);
        this->d->saveCodecOptionValue(type, option, value);

        break;

    default:
        break;
    }
}

void Streaming::setBitrate(AkCaps::CapsType type, int bitrate)
{
    switch (type) {
    case AkCaps::CapsAudio:
        if (this->d->m_audioBitrate == bitrate)
            return;

        this->d->m_audioBitrate = bitrate;
        emit this->bitrateChanged(type, bitrate);
        this->d->saveBitrate(type, bitrate);

        break;

    case AkCaps::CapsVideo:
        if (this->d->m_videoBitrate == bitrate)
            return;

        this->d->m_videoBitrate = bitrate;
        emit this->bitrateChanged(type, bitrate);
        this->d->saveBitrate(type, bitrate);

        break;

    default:
        break;
    }
}

void Streaming::resetAudioCaps()
{
    this->setAudioCaps({});
}

void Streaming::resetVideoCaps()
{
    this->setVideoCaps({});
}

void Streaming::resetVideoGOP()
{
    this->setVideoGOP(DEFAULT_VIDEO_GOP);
}

void Streaming::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void Streaming::resetPlatforms()
{
    this->setPlatforms({});
}

void Streaming::resetCodec(AkCaps::CapsType type)
{
    QString platform;

    if (!this->d->m_platforms.isEmpty())
        platform = this->d->m_platforms.first();
    else if (!this->d->m_sites.isEmpty())
        platform = this->d->m_sites.first().name;

    if (platform.isEmpty()) {
        this->setCodec(type, "");

        return;
    }

    auto streamerID   = this->d->streamerPluginForPlatform(platform);
    auto defaultCodec = this->d->defaultCodecForPlatform(streamerID,
                                                         platform,
                                                         type);

    if (!defaultCodec.isEmpty())
        this->setCodec(type, defaultCodec);
}

void Streaming::resetCodecOptionValue(AkCaps::CapsType type,
                                      const QString &option)
{
    this->setCodecOptionValue(type, option,
                              this->codecOptionValue(type, option));
}

void Streaming::resetCodecOptions(AkCaps::CapsType type)
{
    for (auto &option: this->codecOptions(type))
        this->resetCodecOptionValue(type, option.name());
}

void Streaming::resetBitrate(AkCaps::CapsType type)
{
    this->setBitrate(type,
                     type == AkCaps::CapsVideo?
                         DEFAULT_VIDEO_BITRATE:
                         DEFAULT_AUDIO_BITRATE);
}

AkPacket Streaming::iStream(const AkPacket &packet)
{
    if (packet.type() == AkPacket::PacketVideo) {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_curPacket = packet;
    }

    if (this->d->m_isStreaming) {
        switch (packet.type()) {
        case AkPacket::PacketAudio:
            if (this->d->m_audioEncoder)
                this->d->m_audioEncoder->iStream(packet);

            break;

        case AkPacket::PacketVideo:
            if (this->d->m_videoEncoder)
                this->d->m_videoEncoder->iStream(packet);

            break;

        default:
            break;
        }
    }

    return {};
}

void Streaming::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("streaming", this);
}

StreamingPrivate::StreamingPrivate(Streaming *self):
    self(self)
{
    this->initSupportedCodecs();
    this->loadBuiltInSites();
    this->loadBuiltInOverrides();
}

StreamingSiteInfo *StreamingPrivate::findSite(const QString &name)
{
    for (auto &site: this->m_sites)
        if (site.name == name)
            return &site;

    return nullptr;
}

const StreamingSiteInfo *StreamingPrivate::findSite(const QString &name) const
{
    for (auto &site: this->m_sites)
        if (site.name == name)
            return &site;

    return nullptr;
}

void StreamingPrivate::loadBuiltInSites()
{
    QFile f(":/Webcamoid/share/StreamingSites.json");

    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open StreamingSites.json";

        return;
    }

    auto doc = QJsonDocument::fromJson(f.readAll());

    for (auto entry: doc.array()) {
        auto obj = entry.toObject();
        StreamingSiteInfo site;
        site.name          = obj["name"].toString();
        site.website       = obj["website"].toString();
        site.protocol      = obj["protocol"].toString();
        site.streamingUrl  = obj["streamingUrl"].toString();
        site.keyConfigsUrl = obj["keyConfigsUrl"].toString();
        site.docsUrl       = obj["docsUrl"].toString();
        site.needsKey      = obj["needsKey"].toBool(true);
        site.builtIn       = true;
        this->m_sites << site;
    }
}

void StreamingPrivate::loadBuiltInOverrides()
{
    QSettings config;
    config.beginGroup("StreamingConfigs");

    int nOverrides = config.beginReadArray("builtInOverrides");

    for (int i = 0; i < nOverrides; ++i) {
        config.setArrayIndex(i);
        auto name = config.value("name").toString();
        auto site = this->findSite(name);

        if (site && site->builtIn) {
            site->streamingUrl = config.value("streamingUrl", site->streamingUrl).toString();
            site->streamingKey = config.value("streamingKey").toString();
        }
    }

    config.endArray();
    config.endGroup();
}

void StreamingPrivate::initSupportedCodecs()
{
    this->m_supportedCodecs.clear();

    auto audioEncoders =
            akPluginManager->listPlugins("^AudioEncoder([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);

    for (auto &encoder: audioEncoders) {
        auto plugin = akPluginManager->create<AkAudioEncoder>(encoder);

        if (!plugin)
            continue;

        auto info = akPluginManager->pluginInfo(encoder);

        for (auto &codec: plugin->codecs())
            this->m_supportedCodecs << CodecInfo {encoder,
                                                  AkCaps::CapsAudio,
                                                  plugin->codecID(codec),
                                                  codec,
                                                  plugin->codecDescription(codec),
                                                  info.priority()};
    }

    auto videoEncoders =
            akPluginManager->listPlugins("^VideoEncoder([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);

    for (auto &encoder: videoEncoders) {
        auto plugin = akPluginManager->create<AkVideoEncoder>(encoder);

        if (!plugin)
            continue;

        auto info = akPluginManager->pluginInfo(encoder);

        for (auto &codec: plugin->codecs())
            this->m_supportedCodecs << CodecInfo {encoder,
                                                  AkCaps::CapsVideo,
                                                  plugin->codecID(codec),
                                                  codec,
                                                  plugin->codecDescription(codec),
                                                  info.priority()};
    }

    std::sort(this->m_supportedCodecs.begin(),
              this->m_supportedCodecs.end(),
              [] (const CodecInfo &a, const CodecInfo &b) {
                  return a.description < b.description;
              });
}

// Returns the streamer plugin ID that best handles a given URL/protocol.
// Simple heuristic: use the first enabled VideoStreaming plugin that declares
// support for the URL via supportsUrl().
QString StreamingPrivate::streamerPluginForUrl(const QString &url)
{
    auto plugins =
            akPluginManager->listPlugins("^VideoStreaming([/]([0-9a-zA-Z_])+)+$",
                                          {},
                                          AkPluginManager::FilterEnabled
                                          | AkPluginManager::FilterRegexp);

    for (auto &id: plugins) {
        auto plugin = akPluginManager->create<AkVideoStreamer>(id);

        if (plugin && plugin->supportsUrl(url))
            return id;
    }

    return {};
}

QString StreamingPrivate::streamerPluginForPlatform(const QString &platform) const
{
    auto site = this->findSite(platform);

    if (!site)
        return {};

    auto plugins =
            akPluginManager->listPlugins("^VideoStreaming([/]([0-9a-zA-Z_])+)+$",
                                          {},
                                          AkPluginManager::FilterEnabled
                                          | AkPluginManager::FilterRegexp);

    for (auto &id: plugins) {
        auto plugin = akPluginManager->create<AkVideoStreamer>(id);

        if (plugin && plugin->protocols().contains(site->protocol))
            return id;
    }

    return {};
}

QString StreamingPrivate::defaultCodecForPlatform(const QString &streamerPluginID,
                                                  const QString &platform,
                                                  AkCaps::CapsType type) const
{
    auto site = this->findSite(platform);

    if (!site)
        return {};

    auto plugin = akPluginManager->create<AkVideoStreamer>(streamerPluginID);

    if (!plugin)
        return {};

    auto codecType = type == AkCaps::CapsAudio
                         ? AkCompressedCaps::CapsType_Audio
                         : AkCompressedCaps::CapsType_Video;
    auto defaultID = plugin->defaultCodec(site->protocol, codecType);

    for (auto &codec: this->m_supportedCodecs)
        if (codec.codecID == defaultID && codec.type == type)
            return codec.pluginID + ':' + codec.name;

    return {};
}

QStringList StreamingPrivate::supportedCodecsForPlatform(const QString &streamerPluginID,
                                                         const QString &platform,
                                                         AkCaps::CapsType type) const
{
    auto site = this->findSite(platform);

    if (!site)
        return {};

    auto plugin = akPluginManager->create<AkVideoStreamer>(streamerPluginID);

    if (!plugin)
        return {};

    auto codecType = type == AkCaps::CapsAudio?
                         AkCompressedCaps::CapsType_Audio:
                         AkCompressedCaps::CapsType_Video;
    auto supported = plugin->supportedCodecs(site->protocol, codecType);

    QStringList result;

    for (auto &codec: this->m_supportedCodecs)
        if (supported.contains(codec.codecID) && codec.type == type)
            result << codec.pluginID + ':' + codec.name;

    return result;
}

void StreamingPrivate::loadCodecOptions(AkCaps::CapsType type)
{
    switch (type) {
    case AkCaps::CapsAudio: {
        if (!this->m_audioEncoder)
            return;

        emit self->codecOptionsChanged(type, this->m_audioEncoder->options());

        QSettings config;
        auto id = this->normalizePluginID(this->m_audioPluginID
                                          + ':' + this->m_audioEncoder->codec());
        config.beginGroup("StreamingConfigs_AudioCodecOptions_" + id);

        for (auto &opt: this->m_audioEncoder->options())
            if (config.contains(opt.name()))
                this->m_audioEncoder->setOptionValue(opt.name(),
                                                     config.value(opt.name()));

        config.endGroup();

        break;
    }
    case AkCaps::CapsVideo: {
        if (!this->m_videoEncoder)
            return;

        emit self->codecOptionsChanged(type, this->m_videoEncoder->options());

        QSettings config;
        auto id = this->normalizePluginID(this->m_videoPluginID
                                          + ':' + this->m_videoEncoder->codec());
        config.beginGroup("StreamingConfigs_VideoCodecOptions_" + id);

        for (auto &opt: this->m_videoEncoder->options())
            if (config.contains(opt.name()))
                this->m_videoEncoder->setOptionValue(opt.name(),
                                                     config.value(opt.name()));

        config.endGroup();

        break;
    }
    default:
        break;
    }
}

void StreamingPrivate::loadConfigs()
{
    QSettings config;
    config.beginGroup("StreamingConfigs");

    // Video / audio caps
    auto outputWidth      = qMax(config.value("outputWidth",      1280 ).toInt(), 160);
    auto outputHeight     = qMax(config.value("outputHeight",     720  ).toInt(), 90);
    auto outputFPS        = qMax(config.value("outputFPS",        30   ).toInt(), 1);
    auto audioSampleRate  = qMax(config.value("audioSampleRate",  48000).toInt(), 8000);

    this->m_videoCaps = {AkVideoCaps::Format_yuv420p,
                         outputWidth, outputHeight, {outputFPS, 1}};
    this->m_audioCaps = {AkAudioCaps::SampleFormat_s16,
                         AkAudioCaps::Layout_stereo,
                         false,
                         audioSampleRate};

    this->m_audioBitrate = qMax(config.value("audioBitrate", DEFAULT_AUDIO_BITRATE).toInt(), 1000);
    this->m_videoBitrate = qMax(config.value("videoBitrate", DEFAULT_VIDEO_BITRATE).toInt(), 100000);
    this->m_videoGOP     = qMax(config.value("videoGOP",     DEFAULT_VIDEO_GOP    ).toInt(), 1);

    // Custom platforms added by the user
    int n = config.beginReadArray("customPlatforms");

    for (int i = 0; i < n; ++i) {
        config.setArrayIndex(i);
        StreamingSiteInfo site;
        site.name          = config.value("name").toString();
        site.website       = config.value("website").toString();
        site.protocol      = config.value("protocol").toString();
        site.streamingUrl  = config.value("streamingUrl").toString();
        site.streamingKey  = config.value("streamingKey").toString();
        site.keyConfigsUrl = config.value("keyConfigsUrl").toString();
        site.docsUrl       = config.value("docsUrl").toString();
        site.needsKey      = config.value("needsKey", true).toBool();
        site.builtIn       = false;

        if (!site.name.isEmpty())
            this->m_sites << site;
    }

    config.endArray();

    n = config.beginReadArray("platforms");

    for (int i = 0; i < n; ++i) {
        config.setArrayIndex(i);
        auto platform = config.value("platform").toString();

        if (platform.isEmpty())
            continue;

        auto it = std::find_if(this->m_sites.constBegin(),
                               this->m_sites.constEnd(),
                               [&platform](const StreamingSiteInfo &site) {
                                   return site.name == platform;
                               });

        if (it == this->m_sites.constEnd())
            continue;

        this->m_platforms << platform;
    }

    config.endArray();

    this->m_platforms.erase(std::remove_if(this->m_platforms.begin(),
                                           this->m_platforms.end(),
                                           [this](const QString &name) {
                                               return !this->findSite(name);
                                           }),
                                           this->m_platforms.end());

    // Active codecs - restore last used, or fall back to defaults
    auto savedAudio = config.value("audioCodec").toString();
    auto savedVideo = config.value("videoCodec").toString();

    config.endGroup();

    // If nothing is saved, use the default codec of the first platform
    if (savedAudio.isEmpty() || savedVideo.isEmpty()) {
        QString firstPlatform = this->m_sites.isEmpty()?
                                    QString():
                                    this->m_sites.first().name;

        if (!firstPlatform.isEmpty()) {
            auto streamerID = streamerPluginForPlatform(firstPlatform);

            if (savedAudio.isEmpty())
                savedAudio = this->defaultCodecForPlatform(streamerID,
                                                           firstPlatform,
                                                           AkCaps::CapsAudio);
            if (savedVideo.isEmpty())
                savedVideo = this->defaultCodecForPlatform(streamerID,
                                                           firstPlatform,
                                                           AkCaps::CapsVideo);
        }
    }

    auto tryLoadEncoder = [this](const QString &id, AkCaps::CapsType type) {
        auto parts = id.split(':');

        if (parts.size() < 2) {
            qWarning() << "Invalid codec id:" << id;
            return;
        }

        auto pluginID   = parts[0];
        auto codecName  = parts[1];

        if (type == AkCaps::CapsAudio) {
            auto enc = akPluginManager->create<AkAudioEncoder>(pluginID);

            if (enc && enc->codecs().contains(codecName)) {
                enc->setCodec(codecName);
                this->m_audioEncoder  = enc;
                this->m_audioPluginID = pluginID;
                this->loadCodecOptions(AkCaps::CapsAudio);
            } else {
                qWarning() << "Could not load audio encoder:" << id;
            }
        } else {
            auto enc = akPluginManager->create<AkVideoEncoder>(pluginID);

            if (enc && enc->codecs().contains(codecName)) {
                enc->setCodec(codecName);
                this->m_videoEncoder  = enc;
                this->m_videoPluginID = pluginID;
                this->loadCodecOptions(AkCaps::CapsVideo);
            } else {
                qWarning() << "Could not load video encoder:" << id;
            }
        }
    };

    tryLoadEncoder(savedAudio, AkCaps::CapsAudio);
    tryLoadEncoder(savedVideo, AkCaps::CapsVideo);
}

QString StreamingPrivate::normalizePluginID(const QString &pluginID)
{
    static const char *valid =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    QString result;

    for (auto &c: pluginID)
        result += (std::strchr(valid, c.toLatin1())? c: QChar('_'));

    return result;
}

void StreamingPrivate::printStreamingParameters()
{
    qInfo() << "Streaming parameters:";
    qInfo() << "    Platforms:" << self->platforms();

    qInfo() << "    Audio:";
    qInfo() << "        sample format:" << this->m_audioCaps.format();
    qInfo() << "        channels:" << this->m_audioCaps.channels();
    qInfo() << "        layout:" << this->m_audioCaps.layout();
    qInfo() << "        sample rate:" << this->m_audioCaps.rate();
    qInfo() << "        codec:" << self->codec(AkCaps::CapsAudio);
    qInfo() << "        bitrate:" << this->m_audioBitrate;

    qInfo() << "    Video:";
    qInfo() << "        pixel format:" << this->m_videoCaps.format();
    qInfo() << "        width:" << this->m_videoCaps.width();
    qInfo() << "        height:" << this->m_videoCaps.height();
    qInfo() << "        frame rate:" << this->m_videoCaps.fps().toString();
    qInfo() << "        codec:" << self->codec(AkCaps::CapsVideo);
    qInfo() << "        bitrate:" << this->m_videoBitrate;
}

bool StreamingPrivate::init()
{
    qInfo() << "Starting streaming";
    this->printStreamingParameters();

    if (!this->m_videoEncoder) {
        qCritical() << "Video codec not set";

        return false;
    }

    if (this->m_platforms.isEmpty()) {
        qCritical() << "No active platforms selected";

        return false;
    }

    // Build the list of destination URLs
    QStringList destinations;

    for (auto &name: this->m_platforms) {
        auto site = this->findSite(name);

        if (!site || site->streamingUrl.isEmpty())
            continue;

        auto fullUrl = site->streamingUrl;

        if (site->needsKey && !site->streamingKey.isEmpty())
            fullUrl.replace("${KEY}", site->streamingKey);

        destinations << fullUrl;
    }

    if (destinations.isEmpty()) {
        qCritical() << "No valid streaming URLs in active platforms";

        return false;
    }

    // Select the streamer plugin based on the first URL
    // (Multiple streamers may be needed in a multi-protocol scenario)
    auto streamerID = streamerPluginForPlatform(this->m_platforms.first());
    auto streamer   = akPluginManager->create<AkVideoStreamer>(streamerID);

    if (!streamer) {
        qCritical() << "No streamer plugin found for" << destinations.first();

        return false;
    }

    streamer->setDestinations(destinations);
    this->m_streamer         = streamer;
    this->m_streamerPluginID = streamerID;
    this->m_videoEncoder->setInputCaps(this->m_videoCaps);
    this->m_videoEncoder->setBitrate(this->m_videoBitrate);
    this->m_videoEncoder->setGop(this->m_videoGOP);
    this->m_videoEncoder->setFillGaps(true);
    this->m_videoEncoder->setBitrateMode(AkVideoEncoder::BitrateMode_CBR);
    this->m_streamer->setStreamCaps(this->m_videoEncoder->outputCaps());
    this->m_streamer->setStreamBitrate(AkCompressedCaps::CapsType_Video,
                                       this->m_videoEncoder->bitrate());
    this->m_videoEncoder->link(this->m_streamer, Qt::DirectConnection);
    this->m_videoHeadersChangedConnection =
            QObject::connect(this->m_videoEncoder.data(),
                             &AkVideoEncoder::headersChanged,
                             [this] (const QByteArray &headers) {
                                 this->m_streamer->setStreamHeaders(
                                     AkCompressedCaps::CapsType_Video, headers);
                             });

    QObject::connect(this->m_streamer.data(),
                     &AkVideoStreamer::streamingError,
                     self,
                     &Streaming::streamingError);

    if (this->m_audioEncoder) {
        this->m_audioEncoder->setInputCaps(this->m_audioCaps);
        this->m_audioEncoder->setBitrate(this->m_audioBitrate);
        this->m_audioEncoder->setFillGaps(true);
        this->m_streamer->setStreamCaps(this->m_audioEncoder->outputCaps());
        this->m_streamer->setStreamBitrate(AkCompressedCaps::CapsType_Audio,
                                           this->m_audioEncoder->bitrate());
        this->m_audioEncoder->link(this->m_streamer, Qt::DirectConnection);
        this->m_audioHeadersChangedConnection =
                QObject::connect(this->m_audioEncoder.data(),
                                 &AkAudioEncoder::headersChanged,
                                 [this] (const QByteArray &headers) {
                                     this->m_streamer->setStreamHeaders(
                                         AkCompressedCaps::CapsType_Audio, headers);
                                 });

        this->m_audioEncoder->setState(AkElement::ElementStatePaused);
        this->m_streamer->setStreamHeaders(AkCompressedCaps::CapsType_Audio,
                                           this->m_audioEncoder->headers());
    }

    this->m_videoEncoder->setState(AkElement::ElementStatePaused);
    this->m_streamer->setStreamHeaders(AkCompressedCaps::CapsType_Video,
                                       this->m_videoEncoder->headers());
    this->m_streamer->setState(AkElement::ElementStatePlaying);

    if (this->m_audioEncoder)
        this->m_audioEncoder->setState(AkElement::ElementStatePlaying);

    this->m_videoEncoder->setState(AkElement::ElementStatePlaying);
    qInfo() << "Streaming started";
    this->m_isStreaming = true;

    return true;
}

void StreamingPrivate::uninit()
{
    if (!this->m_isStreaming)
        return;

    qInfo() << "Stopping streaming";
    this->m_isStreaming = false;

    if (this->m_videoEncoder) {
        this->m_videoEncoder->setState(AkElement::ElementStateNull);
        QObject::disconnect(this->m_videoHeadersChangedConnection);
    }

    if (this->m_audioEncoder) {
        this->m_audioEncoder->setState(AkElement::ElementStateNull);
        QObject::disconnect(this->m_audioHeadersChangedConnection);
    }

    if (this->m_streamer)
        this->m_streamer->setState(AkElement::ElementStateNull);

    QObject::disconnect(this->m_streamer.data(),
                        &AkVideoStreamer::streamingError,
                        self,
                        &Streaming::streamingError);

    qInfo() << "Streaming stopped";
}

void StreamingPrivate::saveAudioCaps(const AkAudioCaps &audioCaps)
{
    QSettings config;
    config.beginGroup("StreamingConfigs");
    config.setValue("audioSampleRate", audioCaps.rate());
    config.endGroup();
}

void StreamingPrivate::saveVideoCaps(const AkVideoCaps &videoCaps)
{
    QSettings config;
    config.beginGroup("StreamingConfigs");
    config.setValue("outputWidth",  videoCaps.width());
    config.setValue("outputHeight", videoCaps.height());
    config.setValue("outputFPS",    qRound(videoCaps.fps().value()));
    config.endGroup();
}

void StreamingPrivate::saveCodec(AkCaps::CapsType type, const QString &codec)
{
    QSettings config;

    config.beginGroup("StreamingConfigs");

    if (type == AkCaps::CapsAudio)
        config.setValue("audioCodec", codec);
    else if (type == AkCaps::CapsVideo)
        config.setValue("videoCodec", codec);

    config.endGroup();
}

void StreamingPrivate::saveCodecOptionValue(AkCaps::CapsType type,
                                             const QString &option,
                                             const QVariant &value)
{
    QSettings config;
    auto id = this->normalizePluginID(self->codec(type));

    if (type == AkCaps::CapsAudio) {
        config.beginGroup("StreamingConfigs_AudioCodecOptions_" + id);
        config.setValue(option, value);
        config.endGroup();
    } else if (type == AkCaps::CapsVideo) {
        config.beginGroup("StreamingConfigs_VideoCodecOptions_" + id);
        config.setValue(option, value);
        config.endGroup();
    }
}

void StreamingPrivate::saveBitrate(AkCaps::CapsType type, int bitrate)
{
    QSettings config;

    config.beginGroup("StreamingConfigs");

    if (type == AkCaps::CapsAudio)
        config.setValue("audioBitrate", bitrate);
    else if (type == AkCaps::CapsVideo)
        config.setValue("videoBitrate", bitrate);

    config.endGroup();
}

void StreamingPrivate::saveVideoGOP(int gop)
{
    QSettings config;
    config.beginGroup("StreamingConfigs");
    config.setValue("videoGOP", gop);
    config.endGroup();
}

void StreamingPrivate::savePlatforms(const QStringList &platforms)
{
    QSettings config;

    config.beginGroup("StreamingConfigs");
    config.beginWriteArray("platforms");
    int i = 0;

    for (auto &platform: platforms) {
        config.setArrayIndex(i);
        config.setValue("platform", platform);
        i++;
    }

    config.endArray();
    config.endGroup();
}

void StreamingPrivate::saveCustomSites()
{
    QSettings config;

    config.beginGroup("StreamingConfigs");
    config.beginWriteArray("customPlatforms");
    int i = 0;

    for (auto &site: this->m_sites) {
        if (site.builtIn)
            continue;

        config.setArrayIndex(i);
        config.setValue("name",          site.name);
        config.setValue("website",       site.website);
        config.setValue("protocol",      site.protocol);
        config.setValue("streamingUrl",  site.streamingUrl);
        config.setValue("streamingKey",  site.streamingKey);
        config.setValue("keyConfigsUrl", site.keyConfigsUrl);
        config.setValue("docsUrl",       site.docsUrl);
        config.setValue("needsKey",      site.needsKey);
        i++;
    }

    config.endArray();
    config.endGroup();
}

void StreamingPrivate::saveBuiltInSiteOverrides()
{
    QSettings config;
    config.beginGroup("StreamingConfigs");
    config.beginWriteArray("builtInOverrides");
    int i = 0;

    for (auto &site: this->m_sites) {
        if (!site.builtIn)
            continue;

        config.setArrayIndex(i);
        config.setValue("name",         site.name);
        config.setValue("streamingUrl", site.streamingUrl);
        config.setValue("streamingKey", site.streamingKey);
        i++;
    }

    config.endArray();
    config.endGroup();
}

#include "moc_streaming.cpp"

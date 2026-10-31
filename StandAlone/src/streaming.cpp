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
#include <atomic>
#include <ak.h>
#include <akaudiocaps.h>
#include <akcaps.h>
#include <akclock.h>
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
#include "encoding.h"

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
        std::atomic<bool> m_originSet {false};
        QMutex m_originMutex;
        bool m_pause {false};

        // Shared encoding settings
        EncodingPtr m_encoding;

        // Platforms
        QVector<StreamingSiteInfo> m_sites; // built-in + custom
        QStringList m_platforms;
        QStringList m_unconfiguredPlatforms;

        // Generic codecs, the encoders are configured in the encoding options
        QString m_audioCodec;
        QString m_videoCodec;

        AkVideoStreamerPtr m_streamerPlugin;
        QString m_streamerFormat;
        AkCodecID m_defaultVideoCodecID {0};
        AkCodecID m_defaultAudioCodecID {0};
        QList<AkCodecID> m_streamerSupportedVideoCodecIDs;
        QList<AkCodecID> m_streamerSupportedAudioCodecIDs;

        // Active encoder/streamer instances
        AkVideoStreamerPtr m_streamer;
        AkAudioEncoderPtr  m_audioEncoder;
        AkVideoEncoderPtr  m_videoEncoder;

        QMetaObject::Connection m_audioHeadersChangedConnection;
        QMetaObject::Connection m_videoHeadersChangedConnection;
        QMutex m_mutex;
        QThreadPool m_threadPool;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit StreamingPrivate(Streaming *self);
        void loadBuiltInSites();
        void loadBuiltInOverrides();
        void loadConfigs();
        QStringList supportedCodecsFor(AkCaps::CapsType type) const;
        QString defaultCodecFor(AkCaps::CapsType type) const;
        void refreshCodecs();
        void resolveStreamerCapabilities();
        void updateUnconfiguredPlatforms();
        AkVideoStreamerPtr streamerPluginForPlatform(const QString &platform) const;
        StreamingSiteInfo *findSite(const QString &name);
        const StreamingSiteInfo *findSite(const QString &name) const;
        static QString normalizePluginID(const QString &pluginID);
        void printStreamingParameters();
        bool init();
        void syncOrigin(const AkPacket &packet);
        void uninit();
        void saveAudioCaps(const AkAudioCaps &audioCaps);
        void saveVideoCaps(const AkVideoCaps &videoCaps);
        void saveCodec(AkCaps::CapsType type, const QString &codec);
        void savePlatforms(const QStringList &platforms);
        void saveCustomSites();
        void saveBuiltInSiteOverrides();
};

Streaming::Streaming(const EncodingPtr &encoding,
                     QQmlApplicationEngine *engine,
                     QObject *parent):
    QObject(parent)
{
    this->d = new StreamingPrivate(this);
    this->d->m_encoding = encoding;
    this->setQmlEngine(engine);
    this->d->loadConfigs();

    QObject::connect(this->d->m_encoding.data(),
                     &Encoding::encoderChanged,
                     this,
                     [this] () {
        this->d->refreshCodecs();
    });
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

QStringList Streaming::unconfiguredPlatforms() const
{
    return this->d->m_unconfiguredPlatforms;
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
        return this->d->m_audioCodec;

    case AkCaps::CapsVideo:
        return this->d->m_videoCodec;

    default:
        break;
    }

    return {};
}
QString Streaming::defaultCodec(const QString &platform,
                                AkCaps::CapsType type) const
{
    if (!this->d->findSite(platform))
        return {};

    return this->d->defaultCodecFor(type);
}

QStringList Streaming::supportedCodecs(const QString &platform,
                                       AkCaps::CapsType type) const
{
    if (!this->d->findSite(platform))
        return {};

    return this->d->supportedCodecsFor(type);
}

QString Streaming::codecDescription(const QString &codec) const
{
    return this->d->m_encoding->codecDescription(codec);
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
    this->d->updateUnconfiguredPlatforms();
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
    auto unconfiguredPlatforms = this->d->m_unconfiguredPlatforms;
    this->d->updateUnconfiguredPlatforms();

    if (unconfiguredPlatforms == this->d->m_unconfiguredPlatforms)
        emit this->unconfiguredPlatformsChanged(this->d->m_unconfiguredPlatforms);

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
    auto unconfiguredPlatforms = this->d->m_unconfiguredPlatforms;
    this->d->updateUnconfiguredPlatforms();

    if (unconfiguredPlatforms == this->d->m_unconfiguredPlatforms)
        emit this->unconfiguredPlatformsChanged(this->d->m_unconfiguredPlatforms);

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
    if (this->d->m_isStreaming
        || !this->d->supportedCodecsFor(type).contains(codec))
        return;

    switch (type) {
    case AkCaps::CapsAudio:
        if (this->d->m_audioCodec == codec)
            return;

        this->d->m_audioCodec = codec;

        break;

    case AkCaps::CapsVideo:
        if (this->d->m_videoCodec == codec)
            return;

        this->d->m_videoCodec = codec;

        break;

    default:
        return;
    }

    emit this->codecChanged(type, codec);
    this->d->saveCodec(type, codec);
}

void Streaming::resetAudioCaps()
{
    this->setAudioCaps({});
}

void Streaming::resetVideoCaps()
{
    this->setVideoCaps({});
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
    this->setCodec(type, this->d->defaultCodecFor(type));
}

AkPacket Streaming::iStream(const AkPacket &packet)
{
    if (this->d->m_isStreaming) {
        this->d->syncOrigin(packet);

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
    this->resolveStreamerCapabilities();
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

QStringList StreamingPrivate::supportedCodecsFor(AkCaps::CapsType type) const
{
    auto &supportedIDs = type == AkCaps::CapsAudio?
                             this->m_streamerSupportedAudioCodecIDs:
                             this->m_streamerSupportedVideoCodecIDs;
    QStringList codecs;

    // A codec is available when the streamer supports the codec ID used by
    // the encoder currently associated to it.
    for (auto &codec: this->m_encoding->codecs(type))
        if (supportedIDs.contains(this->m_encoding->encoderCodecID(type, codec)))
            codecs << codec;

    return codecs;
}

QString StreamingPrivate::defaultCodecFor(AkCaps::CapsType type) const
{
    auto defaultID = type == AkCaps::CapsAudio?
                         this->m_defaultAudioCodecID:
                         this->m_defaultVideoCodecID;
    auto codecs = this->supportedCodecsFor(type);

    for (auto &codec: codecs)
        if (this->m_encoding->encoderCodecID(type, codec) == defaultID)
            return codec;

    return codecs.value(0);
}

void StreamingPrivate::refreshCodecs()
{
    if (!this->m_isStreaming)
        for (auto type: {AkCaps::CapsAudio, AkCaps::CapsVideo}) {
            auto &current = type == AkCaps::CapsAudio?
                                this->m_audioCodec:
                                this->m_videoCodec;

            if (!this->supportedCodecsFor(type).contains(current)) {
                current = this->defaultCodecFor(type);
                emit self->codecChanged(type, current);
                this->saveCodec(type, current);
            }
        }

    emit self->supportedCodecsChanged();
}

void StreamingPrivate::resolveStreamerCapabilities()
{
    auto plugins =
                akPluginManager->listPlugins("^VideoStreaming([/]([0-9a-zA-Z_])+)+$",
                                             {},
                                             AkPluginManager::FilterEnabled
                                             | AkPluginManager::FilterRegexp);

    for (auto &id: plugins) {
        auto plugin = akPluginManager->create<AkVideoStreamer>(id);

        if (plugin && plugin->protocols().contains("rtmp")) {
            this->m_streamerPlugin = plugin;

            break;
        }
    }

    if (!this->m_streamerPlugin)
        return;

    this->m_streamerFormat = this->m_streamerPlugin->defaultFormat("rtmp");

    if (this->m_streamerFormat.isEmpty())
        return;

    this->m_defaultVideoCodecID =
            this->m_streamerPlugin->defaultCodec(this->m_streamerFormat,
                                                 AkCompressedCaps::CapsType_Video);
    this->m_defaultAudioCodecID =
            this->m_streamerPlugin->defaultCodec(this->m_streamerFormat,
                                                 AkCompressedCaps::CapsType_Audio);
    this->m_streamerSupportedVideoCodecIDs =
            this->m_streamerPlugin->supportedCodecs(this->m_streamerFormat,
                                                    AkCompressedCaps::CapsType_Video);
    this->m_streamerSupportedAudioCodecIDs =
            this->m_streamerPlugin->supportedCodecs(this->m_streamerFormat,
                                                    AkCompressedCaps::CapsType_Audio);
}

void StreamingPrivate::updateUnconfiguredPlatforms()
{
    QStringList unconfiguredPlatforms;

    for (auto &platform: this->m_platforms) {
        auto it = std::find_if(this->m_sites.constBegin(),
                               this->m_sites.constEnd(),
                               [&platform] (const StreamingSiteInfo &info) {
            return info.name == platform;
        });

        if (it == this->m_sites.constEnd())
            continue;

        if (it->streamingUrl.isEmpty()
            || (it->needsKey && it->streamingKey.isEmpty())) {
            unconfiguredPlatforms << platform;
        }
    }

    if (unconfiguredPlatforms != this->m_unconfiguredPlatforms) {
        this->m_unconfiguredPlatforms = unconfiguredPlatforms;
        emit self->unconfiguredPlatformsChanged(unconfiguredPlatforms);
    }
}

AkVideoStreamerPtr StreamingPrivate::streamerPluginForPlatform(const QString &platform) const
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
            return plugin;
    }

    return {};
}

void StreamingPrivate::loadConfigs()
{
    QSettings config;
    config.beginGroup("StreamingConfigs");

    // Video / audio caps
    auto outputWidth      = qMax(config.value("outputWidth",      1280 ).toInt(), 160);
    auto outputHeight     = qMax(config.value("outputHeight",     720  ).toInt(), 90);
    auto outputFPS        = qMax(config.value("outputFPS",        30   ).toInt(), 1);

    this->m_videoCaps = {AkVideoCaps::Format_yuv420p,
                         outputWidth, outputHeight, {outputFPS, 1}};
    this->m_audioCaps = {AkAudioCaps::SampleFormat_s16,
                         AkAudioCaps::Layout_stereo,
                         false,
                         this->m_encoding->audioSampleRate()};

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

    // Previous versions stored the encoder implementation, not the codec.
    if (savedAudio.contains(':'))
        savedAudio = this->m_encoding->encoderCodec(savedAudio);

    if (savedVideo.contains(':'))
        savedVideo = this->m_encoding->encoderCodec(savedVideo);

    this->m_audioCodec =
            this->supportedCodecsFor(AkCaps::CapsAudio).contains(savedAudio)?
                savedAudio:
                this->defaultCodecFor(AkCaps::CapsAudio);
    this->m_videoCodec =
            this->supportedCodecsFor(AkCaps::CapsVideo).contains(savedVideo)?
                savedVideo:
                this->defaultCodecFor(AkCaps::CapsVideo);

    this->updateUnconfiguredPlatforms();
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
    qInfo() << "        bitrate:" << this->m_encoding->audioBitrate();

    qInfo() << "    Video:";
    qInfo() << "        pixel format:" << this->m_videoCaps.format();
    qInfo() << "        width:" << this->m_videoCaps.width();
    qInfo() << "        height:" << this->m_videoCaps.height();
    qInfo() << "        frame rate:" << this->m_videoCaps.fps().toString();
    qInfo() << "        codec:" << self->codec(AkCaps::CapsVideo);
    qInfo() << "        bitrate:" << this->m_encoding->videoBitrate();
}

bool StreamingPrivate::init()
{
    qInfo() << "Starting streaming";
    this->printStreamingParameters();

    // The settings of the encoders are shared with the other outputs and can
    // change at any time, so create the encoders for each streaming.
    // The encoders that can't be used for streaming are skipped.
    this->m_videoEncoder =
            this->m_encoding->createVideoEncoder(this->m_videoCodec,
                                                 {"h264_mediacodec"});

    if (!this->m_videoEncoder) {
        qCritical() << "Video codec not set";

        return false;
    }

    this->m_audioEncoder =
            this->m_audioCodec.isEmpty()?
                AkAudioEncoderPtr():
                this->m_encoding->createAudioEncoder(this->m_audioCodec);
    this->m_audioCaps.setRate(this->m_encoding->audioSampleRate());

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
    auto streamer = streamerPluginForPlatform(this->m_platforms.first());

    if (!streamer) {
        qCritical() << "No streamer plugin found for" << destinations.first();

        return false;
    }

    streamer->setDestinations(destinations);
    this->m_streamer         = streamer;
    this->m_videoEncoder->setInputCaps(this->m_videoCaps);
    this->m_videoEncoder->setBitrate(this->m_encoding->videoBitrate());
    this->m_videoEncoder->setGop(this->m_encoding->videoGOP());
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
        this->m_audioEncoder->setBitrate(this->m_encoding->audioBitrate());
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

    /* The common origin of the audio and video encoders is set with the
     * first packet received, see syncOrigin().
     */
    this->m_originSet = false;
    qInfo() << "Streaming started";
    this->m_isStreaming = true;

    return true;
}

/* The audio and the video encoders measure their timestamps from the same
 * origin, so the real offset between the first video frame and the first audio
 * block is preserved: the first packet that arrives, audio or video, is the
 * time zero, and the other stream starts at its real offset from it. The
 * packets are timestamped with the shared clock (AkClock) by the compositor and
 * the audio mixer, so they are comparable.
 *
 * The audio and the video arrive from different threads: the one that comes
 * second waits until the origin is set, so no packet reaches an encoder before
 * it.
 */
void StreamingPrivate::syncOrigin(const AkPacket &packet)
{
    if (this->m_originSet.load(std::memory_order_acquire))
        return;

    if (packet.type() != AkPacket::PacketAudio
        && packet.type() != AkPacket::PacketVideo)
        return;

    QMutexLocker locker(&this->m_originMutex);

    if (this->m_originSet.load(std::memory_order_relaxed))
        return;

    auto startTime = AkClock::ptsToNsecs(packet.pts(), packet.timeBase());

    if (this->m_audioEncoder)
        this->m_audioEncoder->setStartTime(startTime);

    if (this->m_videoEncoder)
        this->m_videoEncoder->setStartTime(startTime);

    this->m_originSet.store(true, std::memory_order_release);
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

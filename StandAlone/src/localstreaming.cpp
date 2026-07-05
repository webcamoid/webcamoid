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
#include <QSettings>
#include <QThreadPool>
#include <QTimer>
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

#define DEFAULT_AUDIO_BITRATE 128000
#define DEFAULT_VIDEO_BITRATE 1500000
#define DEFAULT_VIDEO_GOP 2000

struct LocalStreamingFormatInfo
{
    QString pluginID;
    QString name;
    QString description;
    QStringList audioPluginsID;
    QStringList videoPluginsID;
    QString defaultAudioPluginID;
    QString defaultVideoPluginID;
};

struct LocalStreamingCodecInfo
{
    QString pluginID;
    AkCaps::CapsType type;
    AkCodecID codecID;
    QString name;
    QString description;
    int priority;
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
        quint32 m_localPort {8080};
        QString m_localResource {"stream"};
        QString m_localFormat {"webm"};
        QString m_location;
        bool m_enabled {false};

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

        QVector<LocalStreamingFormatInfo> m_supportedFormats;
        QVector<LocalStreamingCodecInfo> m_supportedCodecs;
        QString m_defaultFormat;

        // Active encoder/streamer instances
        AkVideoStreamerPtr m_streamer;
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
        QTimer m_ipCheckTimer;

        explicit LocalStreamingPrivate(LocalStreaming *self);
        void loadConfigs();
        void loadCodecOptions(AkCaps::CapsType type);
        void initSupportedCodecs();
        void initSupportedFormats();
        QString getLocalIPAddress() const;
        void updateLocation();
        QString extensionForFormat(const QString &formatId) const;
        static AkVideoStreamerPtr streamerPluginForUrl(const QString &url);
        QString formatFromLocation(const QString &location) const;
        QString defaultCodec(const QString &format,
                             AkCaps::CapsType type) const;
        static QString normalizePluginID(const QString &pluginID);
        void printStreamingParameters();
        bool init();
        void uninit();
        void saveLocation(const QString &location);
        void saveAudioCaps(const AkAudioCaps &audioCaps);
        void saveVideoCaps(const AkVideoCaps &videoCaps);
        void saveCodec(AkCaps::CapsType type, const QString &codec);
        void saveCodecOptionValue(AkCaps::CapsType type,
                                  const QString &option,
                                  const QVariant &value);
        void saveBitrate(AkCaps::CapsType type, int bitrate);
        void saveVideoGOP(int gop);
};

LocalStreaming::LocalStreaming(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new LocalStreamingPrivate(this);
    this->setQmlEngine(engine);
    this->d->loadConfigs();

    this->d->m_ipCheckTimer.setInterval(5000);
    QObject::connect(&this->d->m_ipCheckTimer,
                     &QTimer::timeout,
                    this,
                     [this] () {
        this->d->updateLocation();
    });
    this->d->m_ipCheckTimer.start();
}

LocalStreaming::~LocalStreaming()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QString LocalStreaming::location() const
{
    return this->d->m_location;
}

QString LocalStreaming::defaultURL() const
{
    auto host = this->d->getLocalIPAddress();
    QString ext = "webm";
    auto formatParts = this->d->m_defaultFormat.split(':');

    if (formatParts.size() >= 2)
        ext = formatParts[1];

    return QString("http://%1:8080/stream.%2").arg(host, ext);
}

AkAudioCaps LocalStreaming::audioCaps() const
{
    return this->d->m_audioCaps;
}

AkVideoCaps LocalStreaming::videoCaps() const
{
    return this->d->m_videoCaps;
}

int LocalStreaming::videoGOP() const
{
    return this->d->m_videoGOP;
}

int LocalStreaming::defaultVideoGOP() const
{
    return DEFAULT_VIDEO_GOP;
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

QString LocalStreaming::codec(AkCaps::CapsType type) const
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
        for (auto &codec: it->audioPluginsID)
            codecs << codec;

    if (type == AkCaps::CapsVideo || type == AkCaps::CapsAny)
        for (auto &codec: it->videoPluginsID)
            codecs << codec;

    return codecs;
}

QString LocalStreaming::codecDescription(const QString &codec) const
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

AkPropertyOptions LocalStreaming::codecOptions(AkCaps::CapsType type) const
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

QVariant LocalStreaming::codecOptionValue(AkCaps::CapsType type,
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

int LocalStreaming::bitrate(AkCaps::CapsType type) const
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

int LocalStreaming::defaultBitrate(AkCaps::CapsType type) const
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

void LocalStreaming::setLocation(const QString &location)
{
    if (this->d->m_location == location)
        return;

    this->d->m_enabled = !location.isEmpty();
    this->d->m_location = location;
    emit this->locationChanged(location);
    this->d->saveLocation(location);

    // Reload codecs based on the new location format
    auto format = this->d->formatFromLocation(location);

    if (!format.isEmpty()) {
        auto defaultAudio = this->d->defaultCodec(format, AkCaps::CapsAudio);
        auto defaultVideo = this->d->defaultCodec(format, AkCaps::CapsVideo);

        if (!defaultAudio.isEmpty())
            this->setCodec(AkCaps::CapsAudio, defaultAudio);

        if (!defaultVideo.isEmpty())
            this->setCodec(AkCaps::CapsVideo, defaultVideo);
    }
}

void LocalStreaming::setAudioCaps(const AkAudioCaps &audioCaps)
{
    if (this->d->m_audioCaps == audioCaps)
        return;

    this->d->m_audioCaps = audioCaps;
    emit this->audioCapsChanged(audioCaps);
    this->d->saveAudioCaps(audioCaps);
}

void LocalStreaming::setVideoCaps(const AkVideoCaps &videoCaps)
{
    if (this->d->m_videoCaps == videoCaps)
        return;

    this->d->m_videoCaps = videoCaps;
    emit this->videoCapsChanged(videoCaps);
    this->d->saveVideoCaps(videoCaps);
}

void LocalStreaming::setVideoGOP(int gop)
{
    if (this->d->m_videoGOP == gop)
        return;

    this->d->m_videoGOP = gop;
    emit this->videoGOPChanged(gop);
    this->d->saveVideoGOP(gop);
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

void LocalStreaming::setCodec(AkCaps::CapsType type, const QString &codec)
{
    auto parts     = codec.split(':');
    auto pluginID  = parts.value(0);
    auto codecName = parts.value(1);

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

void LocalStreaming::setCodecOptionValue(AkCaps::CapsType type,
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

void LocalStreaming::setBitrate(AkCaps::CapsType type, int bitrate)
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

void LocalStreaming::resetLocation()
{
    this->setLocation({});
}

void LocalStreaming::resetAudioCaps()
{
    this->setAudioCaps({});
}

void LocalStreaming::resetVideoCaps()
{
    this->setVideoCaps({});
}

void LocalStreaming::resetVideoGOP()
{
    this->setVideoGOP(DEFAULT_VIDEO_GOP);
}

void LocalStreaming::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void LocalStreaming::resetCodec(AkCaps::CapsType type)
{
    auto format = this->d->formatFromLocation(this->d->m_location);
    this->setCodec(type, this->d->defaultCodec(format, type));
}

void LocalStreaming::resetCodecOptionValue(AkCaps::CapsType type,
                                      const QString &option)
{
    this->setCodecOptionValue(type, option,
                              this->codecOptionValue(type, option));
}

void LocalStreaming::resetCodecOptions(AkCaps::CapsType type)
{
    for (auto &option: this->codecOptions(type))
        this->resetCodecOptionValue(type, option.name());
}

void LocalStreaming::resetBitrate(AkCaps::CapsType type)
{
    this->setBitrate(type,
                     type == AkCaps::CapsVideo?
                         DEFAULT_VIDEO_BITRATE:
                         DEFAULT_AUDIO_BITRATE);
}

AkPacket LocalStreaming::iStream(const AkPacket &packet)
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
    this->initSupportedCodecs();
    this->initSupportedFormats();
}

void LocalStreamingPrivate::initSupportedCodecs()
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
            this->m_supportedCodecs << LocalStreamingCodecInfo {encoder,
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

    QString excludedCodecs = {
        "h264_mediacodec"
    };

    for (auto &encoder: videoEncoders) {
        auto plugin = akPluginManager->create<AkVideoEncoder>(encoder);

        if (!plugin)
            continue;

        auto info = akPluginManager->pluginInfo(encoder);

        for (auto &codec: plugin->codecs())
            if (!excludedCodecs.contains(codec)) {
                this->m_supportedCodecs << LocalStreamingCodecInfo {encoder,
                                           AkCaps::CapsVideo,
                                           plugin->codecID(codec),
                                           codec,
                                           plugin->codecDescription(codec),
                                           info.priority()};
            }
    }

    std::sort(this->m_supportedCodecs.begin(),
              this->m_supportedCodecs.end(),
              [] (const LocalStreamingCodecInfo &a, const LocalStreamingCodecInfo &b) {
                  return a.description < b.description;
              });
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

    for (auto &muxerPluginId: muxerPlugins) {
        auto muxerInfo = akPluginManager->pluginInfo(muxerPluginId);
        auto muxerPlugin = akPluginManager->create<AkVideoStreamer>(muxerPluginId);

        if (!muxerPlugin)
            continue;

        for (auto &protocol: muxerPlugin->protocols()) {
            for (auto &muxer: muxerPlugin->supportedFormats(protocol)) {
                QVector<LocalStreamingPluginPriority> codecsPriority;
                QVector<QString> audioPluginsID;
                auto supportedAudioCodecs =
                        muxerPlugin->supportedCodecs(muxer,
                                                     AkCompressedCaps::CapsType_Audio);
                auto defaultAudioCodec =
                        muxerPlugin->defaultCodec(muxer,
                                                  AkCompressedCaps::CapsType_Audio);

                for (auto &codec: this->m_supportedCodecs)
                    if (supportedAudioCodecs.contains(codec.codecID)
                        && codec.type == AkCaps::CapsAudio) {
                        auto id = codec.pluginID + ':' + codec.name;
                        audioPluginsID << id;

                        if (codec.codecID == defaultAudioCodec)
                            codecsPriority << LocalStreamingPluginPriority {id, codec.priority};
                    }

                if (audioPluginsID.isEmpty())
                    continue;

                std::sort(codecsPriority.begin(),
                          codecsPriority.end(),
                          [] (const LocalStreamingPluginPriority &plugin1,
                              const LocalStreamingPluginPriority &pluhgin2) -> bool {
                    return plugin1.priority > pluhgin2.priority;
                });
                QString defaultAudioPluginID;

                if (!codecsPriority.isEmpty())
                    defaultAudioPluginID = codecsPriority[0].pluginID;

                codecsPriority.clear();
                QVector<QString> videoPluginsID;
                auto supportedVideoCodecs =
                        muxerPlugin->supportedCodecs(muxer,
                                                     AkCompressedCaps::CapsType_Video);
                auto defaultVideoCodec =
                        muxerPlugin->defaultCodec(muxer,
                                                  AkCompressedCaps::CapsType_Video);

                for (auto &codec: this->m_supportedCodecs)
                    if (supportedVideoCodecs.contains(codec.codecID)
                        && codec.type == AkCaps::CapsVideo) {
                        auto id = codec.pluginID + ':' + codec.name;
                        videoPluginsID << id;

                        if (codec.codecID == defaultVideoCodec)
                            codecsPriority << LocalStreamingPluginPriority {id, codec.priority};
                    }

                if (videoPluginsID.isEmpty())
                    continue;

                std::sort(codecsPriority.begin(),
                          codecsPriority.end(),
                          [] (const LocalStreamingPluginPriority &plugin1,
                              const LocalStreamingPluginPriority &plugin2) -> bool {
                    return plugin1.priority > plugin2.priority;
                });
                auto defaultVideoPluginID = codecsPriority.first().pluginID;

                this->m_supportedFormats << LocalStreamingFormatInfo {
                    muxerPluginId,
                    muxer,
                    muxerPlugin->description(muxer),
                    audioPluginsID,
                    videoPluginsID,
                    defaultAudioPluginID,
                    defaultVideoPluginID
                };

                bool formatHasHwCodec = false;

                for (auto &videoPluginID: videoPluginsID) {
                    auto parts = videoPluginID.split(':');

                    if (parts.size() < 2)
                        continue;

                    auto encoderPlugin = akPluginManager->create<AkVideoEncoder>(parts[0]);

                    if (encoderPlugin && encoderPlugin->hasHardwareSupport(parts[1])) {
                        formatHasHwCodec = true;

                        break;
                    }
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

void LocalStreamingPrivate::updateLocation()
{
    if (!this->m_enabled) {
        if (this->m_location.isEmpty())
            return;

        this->m_location.clear();
        emit self->locationChanged(this->m_location);

        return;
    }

    auto newLocation = QString("http://%1:%2/%3.%4")
                           .arg(this->getLocalIPAddress())
                           .arg(this->m_localPort)
                           .arg(this->m_localResource)
                           .arg(this->m_localFormat);

    if (newLocation == this->m_location)
        return;

    this->m_location = newLocation;
    emit self->locationChanged(this->m_location);
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
        return it->defaultAudioPluginID;

    case AkCaps::CapsVideo:
        return it->defaultVideoPluginID;

    default:
        break;
    }

    return {};
}

void LocalStreamingPrivate::loadCodecOptions(AkCaps::CapsType type)
{
    switch (type) {
    case AkCaps::CapsAudio: {
        if (!this->m_audioEncoder)
            return;

        emit self->codecOptionsChanged(type, this->m_audioEncoder->options());

        QSettings config;
        auto id = this->normalizePluginID(this->m_audioPluginID
                                          + ':' + this->m_audioEncoder->codec());
        config.beginGroup("LocalStreamingConfigs_AudioCodecOptions_" + id);

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
        config.beginGroup("LocalStreamingConfigs_VideoCodecOptions_" + id);

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

void LocalStreamingPrivate::loadConfigs()
{
    QSettings config;
    config.beginGroup("LocalStreamingConfigs");

    // Location
    this->m_enabled = config.value("enabled", false).toBool();
    this->m_localPort = config.value("localPort", 8080).toUInt();
    this->m_localResource = config.value("localResource", "stream").toString();
    auto ext = this->extensionForFormat(this->m_defaultFormat);
    this->m_localFormat = config.value("localFormat", ext).toString();
    this->updateLocation();

    // Video / audio caps
    auto outputWidth      = qMax(config.value("outputWidth",     1280 ).toInt(), 160);
    auto outputHeight     = qMax(config.value("outputHeight",    720  ).toInt(), 90);
    auto outputFPS        = qMax(config.value("outputFPS",       30   ).toInt(), 1);
    auto audioSampleRate  = qMax(config.value("audioSampleRate", 48000).toInt(), 8000);

    this->m_videoCaps = {AkVideoCaps::Format_yuv420p,
                         outputWidth, outputHeight, {outputFPS, 1}};
    this->m_audioCaps = {AkAudioCaps::SampleFormat_s16,
                         AkAudioCaps::Layout_stereo,
                         false,
                         audioSampleRate};

    this->m_audioBitrate = qMax(config.value("audioBitrate", DEFAULT_AUDIO_BITRATE).toInt(), 1000);
    this->m_videoBitrate = qMax(config.value("videoBitrate", DEFAULT_VIDEO_BITRATE).toInt(), 100000);
    this->m_videoGOP     = qMax(config.value("videoGOP",     DEFAULT_VIDEO_GOP    ).toInt(), 1);

    // Active codecs - restore last used, or fall back to defaults inferred from location
    auto savedAudio = config.value("audioCodec").toString();
    auto savedVideo = config.value("videoCodec").toString();

    config.endGroup();

    // If no saved codecs, use defaults from location format
    auto format = this->formatFromLocation(this->m_location);

    if (savedAudio.isEmpty() && !format.isEmpty())
        savedAudio = this->defaultCodec(format, AkCaps::CapsAudio);

    if (savedVideo.isEmpty() && !format.isEmpty())
        savedVideo = this->defaultCodec(format, AkCaps::CapsVideo);

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

bool LocalStreamingPrivate::init()
{
    qInfo() << "Starting local streaming";
    this->printStreamingParameters();

    if (!this->m_enabled || this->m_location.isEmpty()) {
        qCritical() << "Location (URL) not set";

        return false;
    }

    if (!this->m_videoEncoder) {
        qCritical() << "Video codec not set";

        return false;
    }

    // Select the streamer plugin based on the URL
    auto streamer = streamerPluginForUrl(this->m_location);

    if (!streamer) {
        qCritical() << "No streamer plugin found for" << this->m_location;

        return false;
    }

    // Set single destination using location
    QStringList destinations {this->m_location};
    streamer->setDestinations(destinations);

    this->m_streamer = streamer;

    // Configure video encoder
    this->m_videoEncoder->setInputCaps(this->m_videoCaps);
    this->m_videoEncoder->setBitrate(this->m_videoBitrate);
    this->m_videoEncoder->setGop(this->m_videoGOP);
    this->m_videoEncoder->setFillGaps(true);
    this->m_videoEncoder->setBitrateMode(AkVideoEncoder::BitrateMode_CBR);

    // Configure streamer with video caps
    this->m_streamer->setStreamCaps(this->m_videoEncoder->outputCaps());
    this->m_streamer->setStreamBitrate(AkCompressedCaps::CapsType_Video,
                                       this->m_videoEncoder->bitrate());
    this->m_videoEncoder->link(this->m_streamer, Qt::DirectConnection);

    // Connect headers changed signal
    this->m_videoHeadersChangedConnection =
            QObject::connect(this->m_videoEncoder.data(),
                             &AkVideoEncoder::headersChanged,
                             [this] (const QByteArray &headers) {
                                 this->m_streamer->setStreamHeaders(
                                     AkCompressedCaps::CapsType_Video, headers);
                             });

    // Connect error signal
    QObject::connect(this->m_streamer.data(),
                     &AkVideoStreamer::streamingError,
                     self,
                     &LocalStreaming::streamingError);

    // Configure audio if available
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

    // Start streaming pipeline
    this->m_videoEncoder->setState(AkElement::ElementStatePaused);
    this->m_streamer->setStreamHeaders(AkCompressedCaps::CapsType_Video,
                                       this->m_videoEncoder->headers());
    this->m_streamer->setState(AkElement::ElementStatePlaying);

    if (this->m_audioEncoder)
        this->m_audioEncoder->setState(AkElement::ElementStatePlaying);

    this->m_videoEncoder->setState(AkElement::ElementStatePlaying);

    qInfo() << "Local streaming started to:" << this->m_location;
    this->m_isStreaming = true;

    return true;
}

void LocalStreamingPrivate::uninit()
{
    if (!this->m_isStreaming)
        return;

    qInfo() << "Stopping local streaming";
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
                        &LocalStreaming::streamingError);

    qInfo() << "Local streaming stopped";
}

void LocalStreamingPrivate::saveLocation(const QString &location)
{
    QSettings config;
    config.beginGroup("LocalStreamingConfigs");

    config.setValue("enabled", !location.isEmpty());

    if (location.isEmpty()) {
        config.endGroup();

        return;
    }

    QUrl url(location);
    auto port = quint16(url.port(8080));
    auto path = url.path();

    if (path.startsWith('/'))
        path = path.mid(1);

    auto resource = path;
    auto format = this->m_defaultFormat;

    int dotIndex = path.lastIndexOf('.');

    if (dotIndex >= 0) {
        resource = path.left(dotIndex);
        format = path.mid(dotIndex + 1);
    }

    if (resource.isEmpty())
        resource = "stream";

    config.setValue("localPort", port);
    config.setValue("localResource", resource);
    config.setValue("localFormat", format);

    config.endGroup();
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

void LocalStreamingPrivate::saveCodec(AkCaps::CapsType type, const QString &codec)
{
    QSettings config;

    config.beginGroup("LocalStreamingConfigs");

    if (type == AkCaps::CapsAudio)
        config.setValue("audioCodec", codec);
    else if (type == AkCaps::CapsVideo)
        config.setValue("videoCodec", codec);

    config.endGroup();
}

void LocalStreamingPrivate::saveCodecOptionValue(AkCaps::CapsType type,
                                             const QString &option,
                                             const QVariant &value)
{
    QSettings config;
    auto id = this->normalizePluginID(self->codec(type));

    if (type == AkCaps::CapsAudio) {
        config.beginGroup("LocalStreamingConfigs_AudioCodecOptions_" + id);
        config.setValue(option, value);
        config.endGroup();
    } else if (type == AkCaps::CapsVideo) {
        config.beginGroup("LocalStreamingConfigs_VideoCodecOptions_" + id);
        config.setValue(option, value);
        config.endGroup();
    }
}

void LocalStreamingPrivate::saveBitrate(AkCaps::CapsType type, int bitrate)
{
    QSettings config;

    config.beginGroup("LocalStreamingConfigs");

    if (type == AkCaps::CapsAudio)
        config.setValue("audioBitrate", bitrate);
    else if (type == AkCaps::CapsVideo)
        config.setValue("videoBitrate", bitrate);

    config.endGroup();
}

void LocalStreamingPrivate::saveVideoGOP(int gop)
{
    QSettings config;
    config.beginGroup("LocalStreamingConfigs");
    config.setValue("videoGOP", gop);
    config.endGroup();
}

#include "moc_localstreaming.cpp"

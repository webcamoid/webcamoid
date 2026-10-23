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

#include <QMap>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <algorithm>
#include <ak.h>
#include <akcompressedaudiocaps.h>
#include <akcompressedvideocaps.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>

#include "encoding.h"

#define DEFAULT_AUDIO_BITRATE 128000
#define DEFAULT_VIDEO_BITRATE 1500000
#define DEFAULT_VIDEO_GOP 1000
#define DEFAULT_AUDIO_SAMPLE_RATE 48000

// Each plugin family tags the codec IDs it uses with a prefix, so the muxers
// only accept streams from the encoders of its own family. The generic codecs
// shown to the user are resolved from the IDs of all the known families.
#define FF_ID(a, b, c) AkCodecID(AK_MAKE_FOURCC(0xFF, a, b, c))

struct CodecKind
{
    const char *key;
    const char *description;
    AkCaps::CapsType type;
    AkCodecID genericID;
    AkCodecID ffmpegID;

    static inline const CodecKind *table()
    {
        static const CodecKind kinds[] = {
            // Video
            {"h264" , "H.264", AkCaps::CapsVideo, AkCodecID(AkCompressedVideoCaps::VideoCodecID_h264) , FF_ID('A', 'V', 'C')},
            {"h265" , "H.265", AkCaps::CapsVideo, AkCodecID(AkCompressedVideoCaps::VideoCodecID_hevc) , FF_ID('H', 'E', 'V')},
            {"vp8"  , "VP8"  , AkCaps::CapsVideo, AkCodecID(AkCompressedVideoCaps::VideoCodecID_vp8)  , FF_ID('V', 'P', '8')},
            {"vp9"  , "VP9"  , AkCaps::CapsVideo, AkCodecID(AkCompressedVideoCaps::VideoCodecID_vp9)  , FF_ID('V', 'P', '9')},
            {"av1"  , "AV1"  , AkCaps::CapsVideo, AkCodecID(AkCompressedVideoCaps::VideoCodecID_av1)  , FF_ID('A', 'V', '1')},
            {"mjpeg", "MJPEG", AkCaps::CapsVideo, AkCodecID(AkCompressedVideoCaps::VideoCodecID_mjpeg), FF_ID('M', 'J', 'P')},

            // Audio
            {"aac"   , "AAC"   , AkCaps::CapsAudio, AkCodecID(AkCompressedAudioCaps::AudioCodecID_aac)   , FF_ID('A', 'A', 'C')},
            {"mp3"   , "MP3"   , AkCaps::CapsAudio, AkCodecID(AkCompressedAudioCaps::AudioCodecID_mp3)   , FF_ID('M', 'P', '3')},
            {"opus"  , "Opus"  , AkCaps::CapsAudio, AkCodecID(AkCompressedAudioCaps::AudioCodecID_opus)  , FF_ID('O', 'P', 'U')},
            {"vorbis", "Vorbis", AkCaps::CapsAudio, AkCodecID(AkCompressedAudioCaps::AudioCodecID_vorbis), FF_ID('V', 'O', 'R')},

            {nullptr, nullptr, AkCaps::CapsUnknown, 0, 0},
        };

        return kinds;
    }

    static inline const CodecKind *byKey(const QString &key)
    {
        for (auto kind = table(); kind->key; ++kind)
            if (key == kind->key)
                return kind;

        return nullptr;
    }

    static inline const CodecKind *byID(AkCaps::CapsType type, AkCodecID id)
    {
        for (auto kind = table(); kind->key; ++kind)
            if (kind->type == type
                && (kind->genericID == id || kind->ffmpegID == id))
                return kind;

        return nullptr;
    }
};

struct EncoderInfo
{
    QString id; // pluginID:codecName
    QString pluginID;
    QString name;
    AkCaps::CapsType type;
    QString codec; // Generic codec key.
    AkCodecID codecID;
    QString description;
    int priority;
    bool hardware;
};

class EncodingPrivate
{
    public:
        Encoding *self;
        QQmlApplicationEngine *m_engine {nullptr};
        int m_videoBitrate {DEFAULT_VIDEO_BITRATE};
        int m_videoGOP {DEFAULT_VIDEO_GOP};
        int m_audioBitrate {DEFAULT_AUDIO_BITRATE};
        int m_audioSampleRate {DEFAULT_AUDIO_SAMPLE_RATE};
        QVector<EncoderInfo> m_encoders;
        bool m_dirty {false};
        QMap<QString, QString> m_associations;
        mutable QMap<QString, AkVideoEncoderPtr> m_videoProfiles;
        mutable QMap<QString, AkAudioEncoderPtr> m_audioProfiles;

        explicit EncodingPrivate(Encoding *self);
        void initEncoders();
        void loadConfigs();
        static QString normatizeID(const QString &id);
        static QString associationKey(AkCaps::CapsType type,
                                      const QString &codec);
        const EncoderInfo *info(const QString &encoder) const;
        const EncoderInfo *usableInfo(AkCaps::CapsType type,
                                      const QString &codec,
                                      const QStringList &excludedEncoders) const;
        QStringList encodersFor(AkCaps::CapsType type,
                                const QString &codec) const;
        QString optionsGroup(const EncoderInfo &info) const;
        AkVideoEncoderPtr createVideo(const EncoderInfo &info) const;
        AkAudioEncoderPtr createAudio(const EncoderInfo &info) const;
        AkVideoEncoderPtr videoProfile(const QString &encoder) const;
        AkAudioEncoderPtr audioProfile(const QString &encoder) const;
        template<typename EncoderPtr>
        void applyStoredOptions(const EncoderPtr &encoder,
                                const EncoderInfo &info) const;
        void saveValue(const QString &key, const QVariant &value) const;
};

Encoding::Encoding(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new EncodingPrivate(this);
    this->setQmlEngine(engine);
    this->d->initEncoders();
    this->d->loadConfigs();
}

Encoding::~Encoding()
{
    delete this->d;
}

int Encoding::videoBitrate() const
{
    return this->d->m_videoBitrate;
}

int Encoding::videoGOP() const
{
    return this->d->m_videoGOP;
}

int Encoding::audioBitrate() const
{
    return this->d->m_audioBitrate;
}

int Encoding::audioSampleRate() const
{
    return this->d->m_audioSampleRate;
}

int Encoding::defaultVideoBitrate() const
{
    return DEFAULT_VIDEO_BITRATE;
}

int Encoding::defaultVideoGOP() const
{
    return DEFAULT_VIDEO_GOP;
}

int Encoding::defaultAudioBitrate() const
{
    return DEFAULT_AUDIO_BITRATE;
}

int Encoding::defaultAudioSampleRate() const
{
    return DEFAULT_AUDIO_SAMPLE_RATE;
}

QStringList Encoding::codecs(AkCaps::CapsType type) const
{
    QStringList codecs;

    for (auto kind = CodecKind::table(); kind->key; ++kind) {
        if (type != AkCaps::CapsAny && kind->type != type)
            continue;

        if (!this->d->encodersFor(kind->type, kind->key).isEmpty())
            codecs << kind->key;
    }

    return codecs;
}

QString Encoding::codecDescription(const QString &codec) const
{
    auto kind = CodecKind::byKey(codec);

    return kind? QString(kind->description): QString();
}

AkCaps::CapsType Encoding::codecType(const QString &codec) const
{
    auto kind = CodecKind::byKey(codec);

    return kind? kind->type: AkCaps::CapsUnknown;
}

QString Encoding::encoderCodec(const QString &encoder) const
{
    auto info = this->d->info(encoder);

    return info? info->codec: QString();
}

QStringList Encoding::encoders(AkCaps::CapsType type,
                               const QString &codec) const
{
    return this->d->encodersFor(type, codec);
}

QString Encoding::encoder(AkCaps::CapsType type, const QString &codec) const
{
    return this->d->m_associations.value(this->d->associationKey(type, codec));
}

QString Encoding::defaultEncoder(AkCaps::CapsType type,
                                 const QString &codec) const
{
    return this->d->encodersFor(type, codec).value(0);
}

QString Encoding::encoderDescription(const QString &encoder) const
{
    auto info = this->d->info(encoder);

    return info? info->description: QString();
}

bool Encoding::encoderIsHardware(const QString &encoder) const
{
    auto info = this->d->info(encoder);

    return info && info->hardware;
}

quint32 Encoding::encoderCodecID(AkCaps::CapsType type,
                                 const QString &codec) const
{
    auto info = this->d->info(this->encoder(type, codec));

    return info? info->codecID: 0;
}

AkPropertyOptions Encoding::encoderOptions(const QString &encoder) const
{
    auto info = this->d->info(encoder);

    if (!info)
        return {};

    if (info->type == AkCaps::CapsVideo) {
        auto profile = this->d->videoProfile(encoder);

        return profile? profile->options(): AkPropertyOptions();
    }

    auto profile = this->d->audioProfile(encoder);

    return profile? profile->options(): AkPropertyOptions();
}

QVariant Encoding::encoderOptionValue(const QString &encoder,
                                      const QString &option) const
{
    auto info = this->d->info(encoder);

    if (!info)
        return {};

    if (info->type == AkCaps::CapsVideo) {
        auto profile = this->d->videoProfile(encoder);

        return profile? profile->optionValue(option): QVariant();
    }

    auto profile = this->d->audioProfile(encoder);

    return profile? profile->optionValue(option): QVariant();
}

QVariantList Encoding::encoderPreferences() const
{
    QVariantList preferences;

    for (auto kind = CodecKind::table(); kind->key; ++kind) {
        auto info = this->d->info(this->encoder(kind->type, kind->key));

        if (!info)
            continue;

        // Only the options changed by the user are stored.
        QVariantMap options;
        QSettings config;
        config.beginGroup(this->d->optionsGroup(*info));

        for (auto &key: config.childKeys())
            options[key] = config.value(key);

        config.endGroup();

        preferences << QVariantMap {
            {"type"   , kind->type == AkCaps::CapsVideo? "video": "audio"},
            {"codecID", quint32(kind->genericID)},
            {"plugin" , info->pluginID},
            {"codec"  , info->name},
            {"options", options},
        };
    }

    return preferences;
}

AkVideoEncoderPtr Encoding::createVideoEncoder(const QString &codec,
                                               const QStringList &excludedEncoders) const
{
    auto info = this->d->usableInfo(AkCaps::CapsVideo, codec, excludedEncoders);

    if (!info)
        return {};

    return this->d->createVideo(*info);
}

AkAudioEncoderPtr Encoding::createAudioEncoder(const QString &codec,
                                               const QStringList &excludedEncoders) const
{
    auto info = this->d->usableInfo(AkCaps::CapsAudio, codec, excludedEncoders);

    if (!info)
        return {};

    return this->d->createAudio(*info);
}

void Encoding::setVideoBitrate(int videoBitrate)
{
    videoBitrate = qMax(videoBitrate, 100000);

    if (this->d->m_videoBitrate == videoBitrate)
        return;

    this->d->m_videoBitrate = videoBitrate;
    emit this->videoBitrateChanged(videoBitrate);
    this->d->m_dirty = true;
    this->d->saveValue("videoBitrate", videoBitrate);
}

void Encoding::setVideoGOP(int videoGOP)
{
    videoGOP = qMax(videoGOP, 1);

    if (this->d->m_videoGOP == videoGOP)
        return;

    this->d->m_videoGOP = videoGOP;
    emit this->videoGOPChanged(videoGOP);
    this->d->m_dirty = true;
    this->d->saveValue("videoGOP", videoGOP);
}

void Encoding::setAudioBitrate(int audioBitrate)
{
    audioBitrate = qMax(audioBitrate, 1000);

    if (this->d->m_audioBitrate == audioBitrate)
        return;

    this->d->m_audioBitrate = audioBitrate;
    emit this->audioBitrateChanged(audioBitrate);
    this->d->m_dirty = true;
    this->d->saveValue("audioBitrate", audioBitrate);
}

void Encoding::setAudioSampleRate(int audioSampleRate)
{
    audioSampleRate = qMax(audioSampleRate, 8000);

    if (this->d->m_audioSampleRate == audioSampleRate)
        return;

    this->d->m_audioSampleRate = audioSampleRate;
    emit this->audioSampleRateChanged(audioSampleRate);
    this->d->m_dirty = true;
    this->d->saveValue("audioSampleRate", audioSampleRate);
}

void Encoding::setEncoder(AkCaps::CapsType type,
                          const QString &codec,
                          const QString &encoder)
{
    if (!this->d->encodersFor(type, codec).contains(encoder))
        return;

    auto key = this->d->associationKey(type, codec);

    if (this->d->m_associations.value(key) == encoder)
        return;

    this->d->m_associations[key] = encoder;
    emit this->encoderChanged(type, codec, encoder);
    this->d->m_dirty = true;
    this->d->saveValue(key, encoder);
}

void Encoding::setEncoderOptionValue(const QString &encoder,
                                     const QString &option,
                                     const QVariant &value)
{
    auto info = this->d->info(encoder);

    if (!info)
        return;

    if (info->type == AkCaps::CapsVideo) {
        auto profile = this->d->videoProfile(encoder);

        if (!profile || profile->optionValue(option) == value)
            return;

        profile->setOptionValue(option, value);
    } else {
        auto profile = this->d->audioProfile(encoder);

        if (!profile || profile->optionValue(option) == value)
            return;

        profile->setOptionValue(option, value);
    }

    emit this->encoderOptionValueChanged(encoder, option, value);
    this->d->m_dirty = true;

    QSettings config;
    config.beginGroup(this->d->optionsGroup(*info));
    config.setValue(option, value);
    config.endGroup();
}

void Encoding::resetVideoBitrate()
{
    this->setVideoBitrate(DEFAULT_VIDEO_BITRATE);
}

void Encoding::resetVideoGOP()
{
    this->setVideoGOP(DEFAULT_VIDEO_GOP);
}

void Encoding::resetAudioBitrate()
{
    this->setAudioBitrate(DEFAULT_AUDIO_BITRATE);
}

void Encoding::resetAudioSampleRate()
{
    this->setAudioSampleRate(DEFAULT_AUDIO_SAMPLE_RATE);
}

void Encoding::resetEncoder(AkCaps::CapsType type, const QString &codec)
{
    this->setEncoder(type, codec, this->defaultEncoder(type, codec));
}

void Encoding::resetEncoderOptionValue(const QString &encoder,
                                       const QString &option)
{
    auto info = this->d->info(encoder);

    if (!info)
        return;

    for (auto &opt: this->encoderOptions(encoder))
        if (opt.name() == option) {
            this->setEncoderOptionValue(encoder, option, opt.defaultValue());

            break;
        }

    // Don't keep a stored value for the default, so the defaults can evolve.
    this->d->m_dirty = true;
    QSettings config;
    config.beginGroup(this->d->optionsGroup(*info));
    config.remove(option);
    config.endGroup();
}

void Encoding::resetEncoderOptions(const QString &encoder)
{
    for (auto &option: this->encoderOptions(encoder))
        this->resetEncoderOptionValue(encoder, option.name());
}

void Encoding::finishEditing()
{
    if (!this->d->m_dirty)
        return;

    this->d->m_dirty = false;
    emit this->changesCommitted();
}

void Encoding::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("encoding", this);
}

EncodingPrivate::EncodingPrivate(Encoding *self):
    self(self)
{
}

void EncodingPrivate::initEncoders()
{
    this->m_encoders.clear();

    auto audioEncoders =
            akPluginManager->listPlugins("^AudioEncoder([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);

    for (auto &pluginID: audioEncoders) {
        auto plugin = akPluginManager->create<AkAudioEncoder>(pluginID);

        if (!plugin)
            continue;

        auto pluginInfo = akPluginManager->pluginInfo(pluginID);

        for (auto &name: plugin->codecs()) {
            auto id = plugin->codecID(name);
            auto kind = CodecKind::byID(AkCaps::CapsAudio, id);

            // Only the codecs that can be referred generically are shown.
            if (!kind)
                continue;

            this->m_encoders << EncoderInfo {pluginID + ':' + name,
                                             pluginID,
                                             name,
                                             AkCaps::CapsAudio,
                                             kind->key,
                                             id,
                                             plugin->codecDescription(name),
                                             pluginInfo.priority(),
                                             false};
        }
    }

    auto videoEncoders =
            akPluginManager->listPlugins("^VideoEncoder([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);

    for (auto &pluginID: videoEncoders) {
        auto plugin = akPluginManager->create<AkVideoEncoder>(pluginID);

        if (!plugin)
            continue;

        auto pluginInfo = akPluginManager->pluginInfo(pluginID);

        for (auto &name: plugin->codecs()) {
            auto id = plugin->codecID(name);
            auto kind = CodecKind::byID(AkCaps::CapsVideo, id);

            if (!kind)
                continue;

            this->m_encoders << EncoderInfo {pluginID + ':' + name,
                                             pluginID,
                                             name,
                                             AkCaps::CapsVideo,
                                             kind->key,
                                             id,
                                             plugin->codecDescription(name),
                                             pluginInfo.priority(),
                                             plugin->hasHardwareSupport(name)};
        }
    }

    // The first encoder of each codec is its default one: highest priority
    // plugin first, then software encoders before the hardware ones, because
    // the hardware encoders are not guaranteed to be usable in the current
    // system.
    std::sort(this->m_encoders.begin(),
              this->m_encoders.end(),
              [] (const EncoderInfo &a, const EncoderInfo &b) {
        if (a.priority != b.priority)
            return a.priority > b.priority;

        if (a.hardware != b.hardware)
            return !a.hardware;

        if (a.description != b.description)
            return a.description < b.description;

        return a.id < b.id;
    });
}

void EncodingPrivate::loadConfigs()
{
    QSettings config;

    // The values used to be stored by the recording module.
    auto legacy = [&config] (const QString &key, int defaultValue) -> int {
        config.beginGroup("RecordConfigs");
        auto value = config.value(key, defaultValue).toInt();
        config.endGroup();

        return value;
    };

    auto videoBitrate = legacy("videoBitrate", DEFAULT_VIDEO_BITRATE);
    auto videoGOP = legacy("videoGOP", DEFAULT_VIDEO_GOP);
    auto audioBitrate = legacy("audioBitrate", DEFAULT_AUDIO_BITRATE);
    auto audioSampleRate = legacy("audioSampleRate", DEFAULT_AUDIO_SAMPLE_RATE);

    config.beginGroup("Encoding");
    this->m_videoBitrate =
            qMax(config.value("videoBitrate", videoBitrate).toInt(), 100000);
    this->m_videoGOP =
            qMax(config.value("videoGOP", videoGOP).toInt(), 1);
    this->m_audioBitrate =
            qMax(config.value("audioBitrate", audioBitrate).toInt(), 1000);
    this->m_audioSampleRate =
            qMax(config.value("audioSampleRate", audioSampleRate).toInt(), 8000);

    for (auto kind = CodecKind::table(); kind->key; ++kind) {
        auto encoders = this->encodersFor(kind->type, kind->key);

        if (encoders.isEmpty())
            continue;

        auto key = associationKey(kind->type, kind->key);
        auto encoder = config.value(key).toString();

        if (!encoders.contains(encoder))
            encoder = encoders.first();

        this->m_associations[key] = encoder;
    }

    config.endGroup();
}

QString EncodingPrivate::normatizeID(const QString &id)
{
    QString normalized;

    for (auto &c: id)
        normalized += c.isLetterOrNumber() && c.unicode() < 128? c: QChar('_');

    return normalized;
}

QString EncodingPrivate::associationKey(AkCaps::CapsType type,
                                        const QString &codec)
{
    return QString(type == AkCaps::CapsVideo? "videoEncoder_": "audioEncoder_")
           + codec;
}

const EncoderInfo *EncodingPrivate::info(const QString &encoder) const
{
    for (auto &info: this->m_encoders)
        if (info.id == encoder)
            return &info;

    return nullptr;
}

const EncoderInfo *EncodingPrivate::usableInfo(AkCaps::CapsType type,
                                               const QString &codec,
                                               const QStringList &excludedEncoders) const
{
    // The associated encoder is preferred.
    auto selected = this->info(this->m_associations.value(associationKey(type, codec)));

    if (selected && !excludedEncoders.contains(selected->name))
        return selected;

    for (auto &info: this->m_encoders)
        if (info.type == type
            && info.codec == codec
            && !excludedEncoders.contains(info.name))
            return &info;

    return nullptr;
}

QStringList EncodingPrivate::encodersFor(AkCaps::CapsType type,
                                         const QString &codec) const
{
    QStringList encoders;

    for (auto &info: this->m_encoders)
        if (info.type == type && info.codec == codec)
            encoders << info.id;

    return encoders;
}

QString EncodingPrivate::optionsGroup(const EncoderInfo &info) const
{
    // Same groups used before the encoding options were unified, so the
    // options already configured are preserved.
    return QString(info.type == AkCaps::CapsVideo?
                       "RecordConfigs_VideoCodecOptions_":
                       "RecordConfigs_AudioCodecOptions_")
           + normatizeID(info.id);
}

template<typename EncoderPtr>
void EncodingPrivate::applyStoredOptions(const EncoderPtr &encoder,
                                         const EncoderInfo &info) const
{
    QSettings config;
    config.beginGroup(this->optionsGroup(info));

    for (auto &option: encoder->options())
        if (config.contains(option.name()))
            encoder->setOptionValue(option.name(),
                                    config.value(option.name()));

    config.endGroup();
}

AkVideoEncoderPtr EncodingPrivate::createVideo(const EncoderInfo &info) const
{
    auto encoder = akPluginManager->create<AkVideoEncoder>(info.pluginID);

    if (!encoder) {
        qWarning() << "Failed to create the video encoder:" << info.pluginID;

        return {};
    }

    encoder->setCodec(info.name);
    this->applyStoredOptions(encoder, info);

    return encoder;
}

AkAudioEncoderPtr EncodingPrivate::createAudio(const EncoderInfo &info) const
{
    auto encoder = akPluginManager->create<AkAudioEncoder>(info.pluginID);

    if (!encoder) {
        qWarning() << "Failed to create the audio encoder:" << info.pluginID;

        return {};
    }

    encoder->setCodec(info.name);
    this->applyStoredOptions(encoder, info);

    return encoder;
}

AkVideoEncoderPtr EncodingPrivate::videoProfile(const QString &encoder) const
{
    auto profile = this->m_videoProfiles.value(encoder);

    if (profile)
        return profile;

    auto info = this->info(encoder);

    if (!info || info->type != AkCaps::CapsVideo)
        return {};

    profile = this->createVideo(*info);

    if (profile)
        this->m_videoProfiles[encoder] = profile;

    return profile;
}

AkAudioEncoderPtr EncodingPrivate::audioProfile(const QString &encoder) const
{
    auto profile = this->m_audioProfiles.value(encoder);

    if (profile)
        return profile;

    auto info = this->info(encoder);

    if (!info || info->type != AkCaps::CapsAudio)
        return {};

    profile = this->createAudio(*info);

    if (profile)
        this->m_audioProfiles[encoder] = profile;

    return profile;
}

void EncodingPrivate::saveValue(const QString &key, const QVariant &value) const
{
    QSettings config;
    config.beginGroup("Encoding");
    config.setValue(key, value);
    config.endGroup();
}

#include "moc_encoding.cpp"

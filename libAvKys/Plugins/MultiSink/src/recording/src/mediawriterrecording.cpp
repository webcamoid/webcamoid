/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#include <QDebug>
#include <QFileInfo>
#include <QMutex>
#include <QSharedPointer>
#include <QVector>
#include <QVariant>
#include <akaudiocaps.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akpluginmanager.h>
#include <akplugininfo.h>
#include <iak/akelement.h>
#include <iak/akaudioencoder.h>
#include <iak/akvideoencoder.h>
#include <iak/akvideomuxer.h>

#include "mediawriterrecording.h"

struct CodecInfo
{
    QString pluginID;
    AkCodecType type;
    AkCodecID codecID;
    QString name;
    QString description;
};

struct FormatInfo
{
    QString pluginID;
    AkVideoMuxer::FormatID formatID;
    QString name;
    QString description;
    QString extension;
    QVector<QString> audioPluginsID;
    QVector<QString> videoPluginsID;
    QString defaultAudioPluginID;
    QString defaultVideoPluginID;
};

struct PluginPriority
{
    QString pluginID;
    int priority;
};

class MediaWriterRecordingPrivate
{
    public:
        MediaWriterRecording *self;
        QString m_outputFormat;
        QVector<CodecInfo> m_supportedCodecs;
        QVector<FormatInfo> m_supportedFormats;
        QString m_defaultFormat;
        QMap<QString, QVariantMap> m_formatOptions;
        QMap<QString, QVariantMap> m_codecOptions;
        QList<QVariantMap> m_streamConfigs;
        AkVideoMuxerPtr m_muxer;
        AkAudioEncoderPtr m_audioEncoder;
        AkVideoEncoderPtr m_videoEncoder;
        int m_audioIndex {-1};
        int m_videoIndex {-1};
        qint64 m_maxPacketQueueSize {15 * 1024 * 1024};
        QMutex m_packetMutex;
        bool m_isRecording {false};

        explicit MediaWriterRecordingPrivate(MediaWriterRecording *self);
        inline void initSupportedCodecs();
        inline void initSupportedFormats();
        QVector<QString> pluginsForID(AkCodecID codecID, AkCodecType type) const;
        AkCodecID codecID(const QString &pluginID);
        QString guessFormat() const;
};

MediaWriterRecording::MediaWriterRecording(QObject *parent):
    MediaWriter(parent)
{
    this->d = new MediaWriterRecordingPrivate(this);
}

MediaWriterRecording::~MediaWriterRecording()
{
    this->uninit();
    delete this->d;
}

QString MediaWriterRecording::defaultFormat()
{
    return this->d->m_defaultFormat;
}

QString MediaWriterRecording::outputFormat() const
{
    return this->d->m_outputFormat;
}

QVariantList MediaWriterRecording::streams() const
{
    QVariantList streams;

    for (auto &stream: this->d->m_streamConfigs)
        streams << stream;

    return streams;
}

qint64 MediaWriterRecording::maxPacketQueueSize() const
{
    return this->d->m_maxPacketQueueSize;
}

QStringList MediaWriterRecording::supportedFormats()
{
    QStringList formats;

    for (auto &format: this->d->m_supportedFormats)
        if (!this->m_formatsBlackList.contains(format.pluginID))
            formats << format.pluginID;

    std::sort(formats.begin(), formats.end());

    return formats;
}

QStringList MediaWriterRecording::fileExtensions(const QString &format)
{
    auto it = std::find_if(this->d->m_supportedFormats.begin(),
                           this->d->m_supportedFormats.end(),
                           [&format] (const FormatInfo &formatInfo) -> bool {
        return formatInfo.pluginID == format;
    });

    if (it == this->d->m_supportedFormats.end())
        return {};

    return {it->extension};
}

QString MediaWriterRecording::formatDescription(const QString &format)
{
    auto it = std::find_if(this->d->m_supportedFormats.begin(),
                           this->d->m_supportedFormats.end(),
                           [&format] (const FormatInfo &formatInfo) -> bool {
        return formatInfo.pluginID == format;
    });

    if (it == this->d->m_supportedFormats.end())
        return {};

    return it->name;
}

QVariantList MediaWriterRecording::formatOptions()
{
    return {};
}

QStringList MediaWriterRecording::supportedCodecs(const QString &format)
{
    return this->supportedCodecs(format, AkCaps::CapsUnknown);
}

QStringList MediaWriterRecording::supportedCodecs(const QString &format,
                                                  AkCaps::CapsType type)
{
    auto it = std::find_if(this->d->m_supportedFormats.begin(),
                           this->d->m_supportedFormats.end(),
                           [&format] (const FormatInfo &formatInfo) -> bool {
        return formatInfo.pluginID == format;
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

QString MediaWriterRecording::defaultCodec(const QString &format,
                                           AkCaps::CapsType type)
{
    auto it = std::find_if(this->d->m_supportedFormats.begin(),
                           this->d->m_supportedFormats.end(),
                           [&format] (const FormatInfo &formatInfo) -> bool {
        return formatInfo.pluginID == format;
    });

    if (it == this->d->m_supportedFormats.end())
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

QString MediaWriterRecording::codecDescription(const QString &codec)
{
    auto it = std::find_if(this->d->m_supportedCodecs.begin(),
                           this->d->m_supportedCodecs.end(),
                           [&codec] (const CodecInfo &codecInfo) -> bool {
        return codecInfo.pluginID == codec;
    });

    if (it == this->d->m_supportedCodecs.end())
        return {};

    return it->name;
}

AkCaps::CapsType MediaWriterRecording::codecType(const QString &codec)
{
    auto it = std::find_if(this->d->m_supportedCodecs.begin(),
                           this->d->m_supportedCodecs.end(),
                           [&codec] (const CodecInfo &codecInfo) -> bool {
        return codecInfo.pluginID == codec;
    });

    if (it == this->d->m_supportedCodecs.end())
        return AkCaps::CapsUnknown;

    switch (it->type) {
    case AkCompressedCaps::CapsType_Audio:
        return AkCaps::CapsAudio;

    case AkCompressedCaps::CapsType_Video:
        return AkCaps::CapsVideo;

    default:
        break;
    }

    return AkCaps::CapsUnknown;
}

QVariantMap MediaWriterRecording::defaultCodecParams(const QString &codec)
{
    QVariantMap codecParams;

    switch (this->codecType(codec)) {
    case AkCaps::CapsAudio: {
        QPair<AkAudioCaps::SampleFormat, bool> fmt(AkAudioCaps::SampleFormat_s16, false);
        auto defaultSampleFormat = QVariant::fromValue(fmt);

        codecParams["supportedSampleRates"] = QVariantList();
        codecParams["supportedSampleFormats"] = QVariantList();
        codecParams["supportedChannelLayouts"] = QVariantList();
        codecParams["defaultSampleFormat"] = defaultSampleFormat;
        codecParams["defaultBitRate"] = 128000;
        codecParams["defaultSampleRate"] = 44100;
        auto defaultChannelLayout = AkAudioCaps::Layout_stereo;
        codecParams["defaultChannelLayout"] = defaultChannelLayout;
        codecParams["defaultChannels"] =
                AkAudioCaps::channelCount(defaultChannelLayout);

        break;
    }

    case AkCaps::CapsVideo: {
        codecParams["supportedFrameRates"] = QVariantList();
        codecParams["supportedPixelFormats"] = QVariantList();
        codecParams["defaultGOP"] = 1000;
        codecParams["defaultBitRate"] = 1500000;
        codecParams["defaultPixelFormat"] = int(AkVideoCaps::Format_yuv420p);

        break;
    }

    default:
        break;
    }

    return codecParams;
}

QVariantMap MediaWriterRecording::addStream(int streamIndex,
                                            const AkCaps &streamCaps)
{
    return this->addStream(streamIndex, streamCaps, {});
}

QVariantMap MediaWriterRecording::addStream(int streamIndex,
                                            const AkCaps &streamCaps,
                                            const QVariantMap &codecParams)
{
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return {};

    QVariantMap outputParams;
    outputParams["index"] = streamIndex;
    auto codec = codecParams.value("codec").toString();
    auto supportedCodecs = this->supportedCodecs(outputFormat, streamCaps.type());

    if (codec.isEmpty() || !supportedCodecs.contains(codec))
        codec = this->defaultCodec(outputFormat, streamCaps.type());

    outputParams["codec"] = codec;
    outputParams["caps"] = QVariant::fromValue(streamCaps);

    auto defaultCodecParams = this->defaultCodecParams(codec);

    if (streamCaps.type() == AkCaps::CapsAudio
        || streamCaps.type() == AkCaps::CapsVideo) {
        int bitrate = codecParams.value("bitrate").toInt();

        if (bitrate < 1)
            bitrate = defaultCodecParams["defaultBitRate"].toInt();

        outputParams["bitrate"] = bitrate;
    }

    if (streamCaps.type() == AkCaps::CapsVideo) {
        int gop = codecParams.value("gop").toInt();

        if (gop < 1)
            gop = defaultCodecParams["defaultGOP"].toInt();

        outputParams["gop"] = gop;
    }

    this->d->m_streamConfigs << outputParams;
    emit this->streamsChanged(this->streams());

    return outputParams;
}

QVariantMap MediaWriterRecording::updateStream(int index)
{
    return this->updateStream(index, {});
}

QVariantMap MediaWriterRecording::updateStream(int index,
                                               const QVariantMap &codecParams)
{
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return {};

    bool streamChanged = false;
    auto streamCaps = this->d->m_streamConfigs[index]["caps"].value<AkCaps>();

    if (codecParams.contains("caps")
        && this->d->m_streamConfigs[index]["caps"] != codecParams.value("caps")) {
        this->d->m_streamConfigs[index]["caps"] = codecParams.value("caps");
        streamChanged = true;
    }

    QString codec;

    if (codecParams.contains("codec")) {
        if (this->supportedCodecs(outputFormat, streamCaps.type())
            .contains(codecParams["codec"].toString())) {
            codec = codecParams["codec"].toString();
        } else
            codec = this->defaultCodec(outputFormat, streamCaps.type());

        this->d->m_streamConfigs[index]["codec"] = codec;
        streamChanged = true;
    } else
        codec = this->d->m_streamConfigs[index]["codec"].toString();

    auto codecDefaults = this->defaultCodecParams(codec);

    if ((streamCaps.type() == AkCaps::CapsAudio
         || streamCaps.type() == AkCaps::CapsVideo)
        && codecParams.contains("bitrate")) {
        int bitRate = codecParams["bitrate"].toInt();
        this->d->m_streamConfigs[index]["bitrate"] =
                bitRate > 0? bitRate: codecDefaults["defaultBitRate"].toInt();
        streamChanged = true;
    }

    if (streamCaps.type() == AkCaps::CapsVideo
        && codecParams.contains("gop")) {
        int gop = codecParams["gop"].toInt();
        this->d->m_streamConfigs[index]["gop"] =
                gop > 0? gop: codecDefaults["defaultGOP"].toInt();
        streamChanged = true;
    }

    if (streamChanged)
        emit this->streamsChanged(this->streams());

    return this->d->m_streamConfigs[index];
}

QVariantList MediaWriterRecording::codecOptions(int index)
{
    Q_UNUSED(index)

    return {};
}

MediaWriterRecordingPrivate::MediaWriterRecordingPrivate(MediaWriterRecording *self):
    self(self)
{
    this->initSupportedCodecs();
    this->initSupportedFormats();
}

void MediaWriterRecordingPrivate::initSupportedCodecs()
{
    this->m_supportedCodecs.clear();

    auto audioEncoders =
            akPluginManager->listPlugins("^AudioEncoder([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);

    for (auto &encoder: audioEncoders) {
        auto codecInfo = akPluginManager->pluginInfo(encoder);
        auto codecPlugin = akPluginManager->create<AkAudioEncoder>(encoder);
        this->m_supportedCodecs << CodecInfo {encoder,
                                              AkCompressedCaps::CapsType_Audio,
                                              codecPlugin->codec(),
                                              codecInfo.name(),
                                              codecInfo.description()};
    }

    auto videoEncoders =
            akPluginManager->listPlugins("^VideoEncoder([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);

    for (auto &encoder: videoEncoders) {
        auto codecInfo = akPluginManager->pluginInfo(encoder);
        auto codecPlugin = akPluginManager->create<AkVideoEncoder>(encoder);
        this->m_supportedCodecs << CodecInfo {encoder,
                                              AkCompressedCaps::CapsType_Video,
                                              codecPlugin->codec(),
                                              codecInfo.name(),
                                              codecInfo.description()};
    }
}

void MediaWriterRecordingPrivate::initSupportedFormats()
{
    this->m_supportedFormats.clear();

    auto muxers = akPluginManager->listPlugins("^VideoMuxer([/]([0-9a-zA-Z_])+)+$",
                                               {},
                                               AkPluginManager::FilterEnabled
                                               | AkPluginManager::FilterRegexp);
    QVector<PluginPriority> formatsPriority;

    for (auto &muxer: muxers) {
        auto muxerInfo = akPluginManager->pluginInfo(muxer);
        auto muxerPlugin = akPluginManager->create<AkVideoMuxer>(muxer);

        QVector<PluginPriority> codecsPriority;
        QVector<QString> audioPluginsID;
        auto supportedAudioCodecs =
                muxerPlugin->supportedCodecs(AkCompressedCaps::CapsType_Audio);
        auto defaultAudioCodec =
                muxerPlugin->defaultCodec(AkCompressedCaps::CapsType_Audio);

        for (auto &codecID: supportedAudioCodecs) {
            auto codecs =
                    this->pluginsForID(codecID,
                                      AkCompressedCaps::CapsType_Audio);
            audioPluginsID << codecs;

            if (codecID == defaultAudioCodec)
                for (auto &codec: codecs) {
                    auto codecInfo = akPluginManager->pluginInfo(codec);
                    codecsPriority << PluginPriority {codec,
                                                      codecInfo.priority()};
                }
        }

        if (audioPluginsID.isEmpty())
            continue;

        std::sort(codecsPriority.begin(),
                  codecsPriority.end(),
                  [] (const PluginPriority &plugin1,
                      const PluginPriority &pluhgin2) -> bool {
            return plugin1.priority > pluhgin2.priority;
        });
        QString defaultAudioPluginID;

        if (!codecsPriority.isEmpty())
            defaultAudioPluginID = codecsPriority[0].pluginID;

        codecsPriority.clear();
        QVector<QString> videoPluginsID;
        auto supportedVideoCodecs =
                muxerPlugin->supportedCodecs(AkCompressedCaps::CapsType_Video);
        auto defaultVideoCodec =
                muxerPlugin->defaultCodec(AkCompressedCaps::CapsType_Video);

        for (auto &codecID: supportedVideoCodecs) {
            auto codecs =
                    this->pluginsForID(codecID,
                                      AkCompressedCaps::CapsType_Video);
            videoPluginsID << codecs;

            if (codecID == defaultVideoCodec)
                for (auto &codec: codecs) {
                    auto codecInfo = akPluginManager->pluginInfo(codec);
                    codecsPriority << PluginPriority {codec,
                                                      codecInfo.priority()};
                }
        }

        if (videoPluginsID.isEmpty())
            continue;

        std::sort(codecsPriority.begin(),
                  codecsPriority.end(),
                  [] (const PluginPriority &plugin1,
                      const PluginPriority &plugin2) -> bool {
            return plugin1.priority > plugin2.priority;
        });
        auto defaultVideoPluginID = codecsPriority.first().pluginID;

        this->m_supportedFormats << FormatInfo {
            muxer,
            muxerPlugin->formatID(),
            muxerInfo.name(),
            muxerInfo.description(),
            muxerPlugin->extension(),
            audioPluginsID,
            videoPluginsID,
            defaultAudioPluginID,
            defaultVideoPluginID
        };

        formatsPriority << PluginPriority {muxer, muxerInfo.priority()};
    }

    if (formatsPriority.isEmpty()) {
        this->m_defaultFormat = {};

        return;
    }

    std::sort(formatsPriority.begin(),
              formatsPriority.end(),
              [] (const PluginPriority &plugin1,
                  const PluginPriority &plugin2) -> bool {
        return plugin1.priority > plugin2.priority;
    });
    this->m_defaultFormat = formatsPriority.first().pluginID;
}

QVector<QString> MediaWriterRecordingPrivate::pluginsForID(AkCodecID codecID,
                                                          AkCodecType type) const
{
    QVector<QString> codecs;

    for (auto &codec: this->m_supportedCodecs)
        if (codec.codecID == codecID && codec.type == type)
            codecs << codec.pluginID;

    return codecs;
}

AkCodecID MediaWriterRecordingPrivate::codecID(const QString &pluginID)
{
    for (auto &codec: this->m_supportedCodecs)
        if (codec.pluginID == pluginID)
            return codec.codecID;

    return 0;
}

QString MediaWriterRecordingPrivate::guessFormat() const
{
    if (self->supportedFormats().contains(this->m_outputFormat))
        return this->m_outputFormat;

    auto suffix = QFileInfo(self->location()).completeSuffix();
    auto it = std::find_if(this->m_supportedFormats.begin(),
                           this->m_supportedFormats.end(),
                           [&suffix] (const FormatInfo &formatInfo) -> bool {
        return formatInfo.extension == suffix;
    });

    if (it == this->m_supportedFormats.end())
        return {};

    return it->pluginID;
}

void MediaWriterRecording::setOutputFormat(const QString &outputFormat)
{
    if (this->d->m_outputFormat == outputFormat)
        return;

    this->d->m_outputFormat = outputFormat;
    emit this->outputFormatChanged(outputFormat);
}

void MediaWriterRecording::setFormatOptions(const QVariantMap &formatOptions)
{
    auto outputFormat = this->d->guessFormat();
    bool modified = false;

    for (auto it = formatOptions.begin();
         it != formatOptions.end();
         it++)
        if (it.value() != this->d->m_formatOptions.value(outputFormat).value(it.key())) {
            this->d->m_formatOptions[outputFormat][it.key()] = it.value();
            modified = true;
        }

    if (modified)
        emit this->formatOptionsChanged(this->d->m_formatOptions.value(outputFormat));
}

void MediaWriterRecording::setCodecOptions(int index,
                                           const QVariantMap &codecOptions)
{
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return;

    auto codec = this->d->m_streamConfigs.value(index).value("codec").toString();

    if (codec.isEmpty())
        return;

    auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(index).arg(codec);
    bool modified = false;

    for (auto it = codecOptions.begin();
         it != codecOptions.end();
         it++)
        if (it.value() != this->d->m_codecOptions.value(optKey).value(it.key())) {
            this->d->m_codecOptions[optKey][it.key()] = it.value();
            modified = true;
        }

    if (modified)
        emit this->codecOptionsChanged(optKey, this->d->m_codecOptions.value(optKey));
}

void MediaWriterRecording::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->d->m_maxPacketQueueSize == maxPacketQueueSize)
        return;

    this->d->m_maxPacketQueueSize = maxPacketQueueSize;
    emit this->maxPacketQueueSizeChanged(maxPacketQueueSize);
}

void MediaWriterRecording::resetOutputFormat()
{
    this->setOutputFormat("");
}

void MediaWriterRecording::resetFormatOptions()
{
    auto outputFormat = this->d->guessFormat();

    if (this->d->m_formatOptions.value(outputFormat).isEmpty())
        return;

    this->d->m_formatOptions.remove(outputFormat);
    emit this->formatOptionsChanged(QVariantMap());
}

void MediaWriterRecording::resetCodecOptions(int index)
{
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return;

    auto codec = this->d->m_streamConfigs.value(index).value("codec").toString();

    if (codec.isEmpty())
        return;

    auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(index).arg(codec);

    if (this->d->m_codecOptions.value(optKey).isEmpty())
        return;

    this->d->m_codecOptions.remove(optKey);
    emit this->codecOptionsChanged(optKey, QVariantMap());
}

void MediaWriterRecording::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void MediaWriterRecording::enqueuePacket(const AkPacket &packet)
{
    if (this->d->m_isRecording) {
        switch (packet.type()) {
        case AkPacket::PacketAudio:
            if (this->d->m_audioIndex == packet.index())
                this->d->m_audioEncoder->iStream(packet);

            break;

        case AkPacket::PacketVideo:
            if (this->d->m_videoIndex == packet.index())
                this->d->m_videoEncoder->iStream(packet);

            break;

        default:
            break;
        }
    }
}

void MediaWriterRecording::clearStreams()
{
    this->d->m_streamConfigs.clear();
    emit this->streamsChanged(this->streams());
}

bool MediaWriterRecording::init()
{
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty()) {
        qDebug() << "Suitable output format not found";

        return false;
    }

    this->d->m_muxer = akPluginManager->create<AkVideoMuxer>(outputFormat);

    if (!this->d->m_muxer) {
        qDebug() << "Failed to create the muxer:" << outputFormat;

        return false;
    }

    this->d->m_muxer->setLocation(this->m_location);

    // Initialize and run streams loops.
    auto streamConfigs = this->d->m_streamConfigs.toVector();
    this->d->m_audioIndex = -1;
    this->d->m_videoIndex = -1;

    for (int i = 0; i < streamConfigs.count(); i++) {
        auto configs = streamConfigs[i];
        auto codec = configs.value("codec").toString();
        auto codecID = this->d->codecID(codec);

        if (!codecID)
            continue;

        AkCaps streamCaps = configs["caps"].value<AkCaps>();
        int inputId = configs["index"].toInt();
        int bitrate = configs["bitrate"].toInt();

        if (streamCaps.type() == AkCaps::CapsAudio) {
            if (this->d->m_audioIndex < 0) {
                this->d->m_audioEncoder =
                        akPluginManager->create<AkAudioEncoder>(codec);
                this->d->m_audioEncoder->setInputCaps(streamCaps);
                this->d->m_audioEncoder->setBitrate(bitrate);
                AkAudioCaps audioCaps = streamCaps;
                AkCompressedAudioCaps caps(AkCompressedAudioCaps::AudioCodecID(codecID),
                                           audioCaps.bps(),
                                           audioCaps.channels(),
                                           audioCaps.rate());
                this->d->m_muxer->setStreamCaps(caps);
                this->d->m_muxer->setStreamBitrate(AkCompressedCaps::CapsType_Audio,
                                                   bitrate);
                this->d->m_audioIndex = inputId;

                this->d->m_audioEncoder->link(this->d->m_muxer,
                                              Qt::DirectConnection);
            }
        } else if (streamCaps.type() == AkCaps::CapsVideo) {
            if (this->d->m_videoIndex < 0) {
                this->d->m_videoEncoder =
                        akPluginManager->create<AkVideoEncoder>(codec);
                this->d->m_videoEncoder->setInputCaps(streamCaps);
                this->d->m_videoEncoder->setBitrate(bitrate);
                this->d->m_videoEncoder->setGop(configs["gop"].toInt());
                this->d->m_videoEncoder->setFillGaps(!this->d->m_muxer->gapsAllowed());
                AkVideoCaps videoCaps = streamCaps;
                AkCompressedVideoCaps caps(AkCompressedVideoCaps::VideoCodecID(codecID),
                                           videoCaps.width(),
                                           videoCaps.height(),
                                           videoCaps.fps());
                this->d->m_muxer->setStreamCaps(caps);
                this->d->m_muxer->setStreamBitrate(AkCompressedCaps::CapsType_Video,
                                                   bitrate);
                this->d->m_videoIndex = inputId;

                this->d->m_videoEncoder->link(this->d->m_muxer,
                                              Qt::DirectConnection);
            }
        }
    }

    if (this->d->m_audioEncoder) {
        this->d->m_audioEncoder->setState(AkElement::ElementStatePlaying);
        this->d->m_muxer->setStreamHeaders(AkCompressedCaps::CapsType_Audio,
                                           this->d->m_audioEncoder->headers());
    }

    this->d->m_videoEncoder->setState(AkElement::ElementStatePlaying);
    this->d->m_muxer->setStreamHeaders(AkCompressedCaps::CapsType_Video,
                                       this->d->m_videoEncoder->headers());
    this->d->m_muxer->setState(AkElement::ElementStatePlaying);
    this->d->m_isRecording = true;

    return true;
}

void MediaWriterRecording::uninit()
{
    this->d->m_isRecording = false;

    if (this->d->m_videoEncoder) {
        this->d->m_videoEncoder->setState(AkElement::ElementStateNull);
        this->d->m_videoEncoder = {};
    }

    if (this->d->m_audioEncoder) {
        this->d->m_audioEncoder->setState(AkElement::ElementStateNull);
        this->d->m_audioEncoder = {};
    }

    if (this->d->m_muxer) {
        this->d->m_muxer->setState(AkElement::ElementStateNull);
        this->d->m_muxer = {};
    }
}

#include "moc_mediawriterrecording.cpp"

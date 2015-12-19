/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <limits>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>

#include "mediasink.h"
#include "formatinfo.h"
#include "codecinfo.h"

typedef QVector<FormatInfo> FormatInfoVector;
Q_GLOBAL_STATIC(FormatInfoVector, recordingFormats)
typedef QVector<CodecInfo> CodecInfoVector;
Q_GLOBAL_STATIC(CodecInfoVector, audioCodecs)
Q_GLOBAL_STATIC(CodecInfoVector, videoCodecs)

inline bool loadProfiles(const QString &profile)
{
    QFile xmlFile(profile);
    QStringList pathList;
    QString path;

    if (!xmlFile.open(QIODevice::ReadOnly))
        return false;

    QXmlStreamReader xmlReader(&xmlFile);

    FormatInfo recordingFormat;
    CodecInfo audioCodec;
    CodecInfo videoCodec;

    while (!xmlReader.atEnd()) {
        QXmlStreamReader::TokenType token = xmlReader.readNext();

        if (token == QXmlStreamReader::Invalid) {
            qDebug() << xmlReader.errorString();

            return false;
        } else if (token == QXmlStreamReader::StartElement) {
            pathList << xmlReader.name().toString();

            if (path == "profile/formats"
                && xmlReader.name() == "format")
                recordingFormat = FormatInfo();
            else if (path == "profile/codecs/audio"
                && xmlReader.name() == "codec")
                audioCodec = CodecInfo();
            else if (path == "profile/codecs/video"
                && xmlReader.name() == "codec")
                videoCodec = CodecInfo();
        } else if (token == QXmlStreamReader::EndElement) {
            if (path == "profile/formats/format")
                *recordingFormats << recordingFormat;
            else if (path == "profile/codecs/audio/codec")
                     *audioCodecs << audioCodec;
            else if (path == "profile/codecs/video/codec")
                     *videoCodecs << videoCodec;

            pathList.removeLast();
        } else if (token == QXmlStreamReader::Characters) {
            if (path == "profile/formats/format/name")
                recordingFormat.name() = xmlReader.text().toString();
            else if (path == "profile/formats/format/long_name")
                recordingFormat.longName() = xmlReader.text().toString();
            else if (path == "profile/formats/format/extensions") {
                QStringList extensions;

                foreach (QString ext, xmlReader.text().toString().split(","))
                    extensions << ext.trimmed();

                recordingFormat.extensions() = extensions;
            } else if (path == "profile/formats/format/audio_codec") {
                QStringList audioCodec;

                foreach (QString codec, xmlReader.text().toString().split(","))
                    if (codec.contains("@")) {
                        QString defaultCodec = codec.replace("@", "").trimmed();
                        audioCodec << defaultCodec;
                        recordingFormat.defaultAudioCodec() = defaultCodec;
                    }
                    else
                        audioCodec << codec.trimmed();

                recordingFormat.audioCodec() = audioCodec;

                if (recordingFormat.defaultAudioCodec().isEmpty())
                    recordingFormat.defaultAudioCodec() = recordingFormat.audioCodec().first();
            } else if (path == "profile/formats/format/video_codec") {
                QStringList videoCodec;

                foreach (QString codec, xmlReader.text().toString().split(","))
                    if (codec.contains("@")) {
                        QString defaultCodec = codec.replace("@", "").trimmed();
                        videoCodec << defaultCodec;
                        recordingFormat.defaultVideoCodec() = defaultCodec;
                    }
                    else
                        videoCodec << codec.trimmed();

                recordingFormat.videoCodec() = videoCodec;

                if (recordingFormat.defaultVideoCodec().isEmpty())
                    recordingFormat.defaultVideoCodec() = recordingFormat.videoCodec().first();
            } else if (path == "profile/codecs/audio/codec/name") {
                audioCodec.name() = xmlReader.text().toString();
            } else if (path == "profile/codecs/audio/codec/long_name") {
                audioCodec.longName() = xmlReader.text().toString();
            } else if (path == "profile/codecs/audio/codec/supported_samplerates") {
                IntList sampleRates;

                foreach (QString rate, xmlReader.text().toString().split(","))
                    sampleRates << rate.trimmed().toInt();

                audioCodec.supportedSamplerates() = sampleRates;
            } else if (path == "profile/codecs/audio/codec/sample_fmts") {
                QStringList sampleFormats;

                foreach (QString format, xmlReader.text().toString().split(","))
                    sampleFormats << format.trimmed();

                audioCodec.sampleFormats() = sampleFormats;
            } else if (path == "profile/codecs/audio/codec/channel_layouts") {
                QStringList channelLayouts;

                foreach (QString layout, xmlReader.text().toString().split(","))
                    channelLayouts << layout.trimmed();

                audioCodec.channelLayouts() = channelLayouts;
            } else if (path == "profile/codecs/video/codec/name") {
                videoCodec.name() = xmlReader.text().toString();
            } else if (path == "profile/codecs/video/codec/long_name") {
                videoCodec.longName() = xmlReader.text().toString();
            } else if (path == "profile/codecs/video/codec/supported_framerates") {
                FracList frameRates;

                foreach (QString rate, xmlReader.text().toString().split(","))
                    frameRates << QbFrac(rate.trimmed());

                videoCodec.supportedFramerates() = frameRates;
            } else if (path == "profile/codecs/video/codec/pix_fmts") {
                QStringList pixelFormats;

                foreach (QString format, xmlReader.text().toString().split(","))
                    pixelFormats << format.trimmed();

                videoCodec.pixelFormats() = pixelFormats;
            }
        }

        path = pathList.join("/");
    }

    return true;
}

Q_GLOBAL_STATIC_WITH_ARGS(bool, profilesLoaded, (loadProfiles(":/MultiSink/gstreamer/share/profiles.xml")))

typedef QMap<QString, int> StringIntMap;

inline StringIntMap initChannelCountMap()
{
    StringIntMap channelCountMap;
    channelCountMap["mono"] = 1;
    channelCountMap["stereo"] = 2;

    return channelCountMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(StringIntMap, channelCountMap, (initChannelCountMap()))

inline StringIntMap initByteCountMap()
{
    StringIntMap byteCountMap;
    byteCountMap["u8"] = 8;
    byteCountMap["s16"] = 16;
    byteCountMap["s32"] = 32;
    byteCountMap["flt"] = 32;
    byteCountMap["dbl"] = 64;
    byteCountMap["u8p"] = 8;
    byteCountMap["s16p"] = 16;
    byteCountMap["s32p"] = 32;
    byteCountMap["fltp"] = 32;
    byteCountMap["dblp"] = 64;

    return byteCountMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(StringIntMap, byteCountMap, (initByteCountMap()))

MediaSink::MediaSink(QObject *parent): QObject(parent)
{
    gst_init(NULL, NULL);

    QObject::connect(this,
                     &MediaSink::outputFormatChanged,
                     this,
                     &MediaSink::updateStreams);
}

MediaSink::~MediaSink()
{
    this->uninit();
}

QString MediaSink::location() const
{
    return this->m_location;
}

QString MediaSink::outputFormat() const
{
    return this->m_outputFormat;
}

QVariantMap MediaSink::formatOptions() const
{
    return this->m_formatOptions;
}

QVariantList MediaSink::streams() const
{
    QVariantList streams;

    foreach (QVariantMap stream, this->m_streamConfigs)
        streams << stream;

    return streams;
}

QStringList MediaSink::supportedFormats()
{
    if (!*profilesLoaded)
        return QStringList();

    QStringList supportedFormats;

    foreach (FormatInfo format, *recordingFormats) {
        GstElementFactory *factory = gst_element_factory_find(format.name().toStdString().c_str());

        if (factory) {
            supportedFormats << format.name();
            gst_object_unref(factory);
        }
    }

    return supportedFormats;
}

QStringList MediaSink::fileExtensions(const QString &format)
{
    if (!*profilesLoaded)
        return QStringList();

    QStringList extensions;

    foreach (FormatInfo fmt, *recordingFormats)
        if (fmt.name() == format) {
            GstElementFactory *factory = gst_element_factory_find(fmt.name().toStdString().c_str());

            if (factory) {
                extensions = fmt.extensions();
                gst_object_unref(factory);
            }

            break;
        }

    return extensions;
}

QString MediaSink::formatDescription(const QString &format)
{
    if (!*profilesLoaded)
        return QString();

    QString description;

    foreach (FormatInfo fmt, *recordingFormats)
        if (fmt.name() == format) {
            GstElementFactory *factory = gst_element_factory_find(fmt.name().toStdString().c_str());

            if (factory) {
                description = fmt.longName();
                gst_object_unref(factory);
            }

            break;
        }

    return description;
}

QStringList MediaSink::supportedCodecs(const QString &format,
                                       const QString &type)
{
    QStringList supportedCodecs;

    foreach (FormatInfo fmt, *recordingFormats)
        if (fmt.name() == format) {
            GstElementFactory *factory = gst_element_factory_find(fmt.name().toStdString().c_str());

            if (factory) {
                if (type.isEmpty())
                    supportedCodecs << fmt.audioCodec() << fmt.videoCodec();
                else if (type == "audio/x-raw")
                    supportedCodecs = fmt.audioCodec();
                else if (type == "video/x-raw")
                    supportedCodecs = fmt.videoCodec();

                gst_object_unref(factory);
            }

            break;
        }

    return supportedCodecs;
}

QString MediaSink::defaultCodec(const QString &format, const QString &type)
{
    QString defaultCodec;

    foreach (FormatInfo fmt, *recordingFormats)
        if (fmt.name() == format) {
            GstElementFactory *factory = gst_element_factory_find(fmt.name().toStdString().c_str());

            if (factory) {
                if (type == "audio/x-raw")
                    defaultCodec = fmt.defaultAudioCodec();
                else if (type == "video/x-raw")
                    defaultCodec = fmt.defaultVideoCodec();

                gst_object_unref(factory);
            }

            break;
        }

    return defaultCodec;
}

QString MediaSink::codecDescription(const QString &codec)
{
    foreach (CodecInfo cdc, *audioCodecs)
        if (cdc.name() == codec) {
            GstElementFactory *factory = gst_element_factory_find(cdc.name().toStdString().c_str());

            if (factory) {
                gst_object_unref(factory);

                return cdc.longName();
            }
        }

    foreach (CodecInfo cdc, *videoCodecs)
        if (cdc.name() == codec) {
            GstElementFactory *factory = gst_element_factory_find(cdc.name().toStdString().c_str());

            if (factory) {
                gst_object_unref(factory);

                return cdc.longName();
            }
        }

    return QString();
}

QString MediaSink::codecType(const QString &codec)
{
    foreach (CodecInfo cdc, *audioCodecs)
        if (cdc.name() == codec) {
            GstElementFactory *factory = gst_element_factory_find(cdc.name().toStdString().c_str());

            if (factory) {
                gst_object_unref(factory);

                return QString("audio/x-raw");
            }
        }

    foreach (CodecInfo cdc, *videoCodecs)
        if (cdc.name() == codec) {
            GstElementFactory *factory = gst_element_factory_find(cdc.name().toStdString().c_str());

            if (factory) {
                gst_object_unref(factory);

                return QString("video/x-raw");
            }
        }

    return QString();
}

QVariantMap MediaSink::defaultCodecParams(const QString &codec)
{
    QVariantMap codecParams;

    foreach (CodecInfo cdc, *audioCodecs)
        if (cdc.name() == codec) {
            GstElementFactory *factory = gst_element_factory_find(cdc.name().toStdString().c_str());

            if (factory) {
                gst_object_unref(factory);

                codecParams["defaultBitRate"] = 128000;

                QVariantList supportedSamplerates;

                foreach (int rate, cdc.supportedSamplerates())
                    supportedSamplerates << QVariant::fromValue(rate);

                codecParams["supportedSampleFormats"] = cdc.sampleFormats();
                codecParams["supportedChannelLayouts"] = cdc.channelLayouts();
                codecParams["supportedSampleRates"] = supportedSamplerates;
                codecParams["defaultSampleFormat"] = cdc.sampleFormats().at(0);

                QString channelLayout = cdc.channelLayouts().isEmpty()?
                                            QString("stereo"): cdc.channelLayouts().at(0);
                codecParams["defaultChannelLayout"] = channelLayout;
                codecParams["defaultChannels"] = channelCountMap->value(channelLayout, 0);
                codecParams["defaultSampleRate"] = cdc.supportedSamplerates().isEmpty()?
                                                     44100: cdc.supportedSamplerates().at(0);

                return codecParams;
            }
        }

    foreach (CodecInfo cdc, *videoCodecs)
        if (cdc.name() == codec) {
            GstElementFactory *factory = gst_element_factory_find(cdc.name().toStdString().c_str());

            if (factory) {
                gst_object_unref(factory);

                codecParams["defaultBitRate"] = 200000;
                codecParams["defaultGOP"] = 12;

                QVariantList supportedFramerates;

                foreach (QbFrac rate, cdc.supportedFramerates())
                    supportedFramerates << QVariant::fromValue(rate);

                codecParams["supportedFrameRates"] = supportedFramerates;
                codecParams["supportedPixelFormats"] = cdc.pixelFormats();
                codecParams["defaultPixelFormat"] = cdc.pixelFormats().at(0);

                return codecParams;
            }
        }

    return codecParams;
}

QVariantMap MediaSink::addStream(int streamIndex,
                                 const QbCaps &streamCaps,
                                 const QVariantMap &codecParams)
{
    QString outputFormat;

    if (this->supportedFormats().contains(this->m_outputFormat))
        outputFormat = this->m_outputFormat;
    else
        outputFormat = guessFormat(this->m_location);

    if (outputFormat.isEmpty())
        return QVariantMap();

    QVariantMap outputParams;

    if (codecParams.contains("label"))
        outputParams["label"] = codecParams["label"];

    outputParams["index"] = streamIndex;
    QString codec;

    if (codecParams.contains("codec")) {
        if (this->supportedCodecs(outputFormat, streamCaps.mimeType())
            .contains(codecParams["codec"].toString())) {
            codec = codecParams["codec"].toString();
        } else
            codec = this->defaultCodec(outputFormat, streamCaps.mimeType());
    } else
        codec = this->defaultCodec(outputFormat, streamCaps.mimeType());

    outputParams["codec"] = codec;

    QVariantMap codecDefaults = this->defaultCodecParams(codec);

    outputParams["codecOptions"] = codecParams.value("codecOptions", QVariantMap());

    if (streamCaps.mimeType() == "audio/x-raw") {
        int bitRate = codecParams.value("bitrate",
                                        codecDefaults["defaultBitRate"]).toInt();
        outputParams["bitrate"] = bitRate > 0?
                                      bitRate:
                                      codecDefaults["defaultBitRate"].toInt();

        QbAudioCaps audioCaps(streamCaps);
        QString sampleFormat = QbAudioCaps::sampleFormatToString(audioCaps.format());
        QStringList supportedSampleFormats = codecDefaults["supportedSampleFormats"].toStringList();

        if (!supportedSampleFormats.isEmpty() && !supportedSampleFormats.contains(sampleFormat)) {
            QString defaultSampleFormat = codecDefaults["defaultSampleFormat"].toString();
            audioCaps.format() = QbAudioCaps::sampleFormatFromString(defaultSampleFormat);
            audioCaps.bps() = byteCountMap->value(defaultSampleFormat, 0);
        }

        QVariantList supportedSampleRates = codecDefaults["supportedSampleRates"].toList();

        if (!supportedSampleRates.isEmpty()) {
            int sampleRate = 0;
            int maxDiff = std::numeric_limits<int>::max();

            foreach (QVariant rate, supportedSampleRates) {
                int diff = qAbs(audioCaps.rate() - rate.toInt());

                if (diff < maxDiff) {
                    sampleRate = rate.toInt();

                    if (!diff)
                        break;

                    maxDiff = diff;
                }
            }

            audioCaps.rate() = sampleRate;
        }

        QString channelLayout = QbAudioCaps::channelLayoutToString(audioCaps.layout());
        QStringList supportedChannelLayouts = codecDefaults["supportedChannelLayouts"].toStringList();

        if (!supportedChannelLayouts.isEmpty() && !supportedChannelLayouts.contains(channelLayout)) {
            QString defaultChannelLayout = codecDefaults["defaultChannelLayout"].toString();
            audioCaps.layout() = QbAudioCaps::channelLayoutFromString(defaultChannelLayout);
            audioCaps.channels() = channelCountMap->value(defaultChannelLayout, 0);
        };

        outputParams["caps"] = QVariant::fromValue(audioCaps.toCaps());
        outputParams["timeBase"] = QVariant::fromValue(QbFrac(1, audioCaps.rate()));
    } else if (streamCaps.mimeType() == "video/x-raw") {
        int bitRate = codecParams.value("bitrate",
                                        codecDefaults["defaultBitRate"]).toInt();
        outputParams["bitrate"] = bitRate > 0?
                                      bitRate:
                                      codecDefaults["defaultBitRate"].toInt();
        int gop = codecParams.value("gop",
                                    codecDefaults["defaultGOP"]).toInt();
        outputParams["gop"] = gop > 0?
                                  gop:
                                  codecDefaults["defaultGOP"].toInt();

        QbVideoCaps videoCaps(streamCaps);
        QString pixelFormat = QbVideoCaps::pixelFormatToString(videoCaps.format());
        QStringList supportedPixelFormats = codecDefaults["supportedPixelFormats"].toStringList();

        if (!supportedPixelFormats.isEmpty() && !supportedPixelFormats.contains(pixelFormat)) {
            QString defaultPixelFormat = codecDefaults["defaultPixelFormat"].toString();
            videoCaps.format() = QbVideoCaps::pixelFormatFromString(defaultPixelFormat);
        }

        QVariantList supportedFrameRates = codecDefaults["supportedFrameRates"].toList();

        if (!supportedFrameRates.isEmpty()) {
            QbFrac frameRate;
            qreal maxDiff = std::numeric_limits<qreal>::max();

            foreach (QVariant rate, supportedFrameRates) {
                qreal diff = qAbs(videoCaps.fps().value() - rate.value<QbFrac>().value());

                if (diff < maxDiff) {
                    frameRate = rate.value<QbFrac>();

                    if (!diff)
                        break;

                    maxDiff = diff;
                }
            }

            videoCaps.fps() = frameRate;
        }

        outputParams["caps"] = QVariant::fromValue(videoCaps.toCaps());
        outputParams["timeBase"] = QVariant::fromValue(videoCaps.fps().invert());
    } else if (streamCaps.mimeType() == "text/x-raw") {
        outputParams["caps"] = QVariant::fromValue(streamCaps);
    }

    this->m_streamConfigs << outputParams;
    this->streamsChanged(this->streams());

    return outputParams;
}

QVariantMap MediaSink::updateStream(int index, const QVariantMap &codecParams)
{
    QString outputFormat;

    if (this->supportedFormats().contains(this->m_outputFormat))
        outputFormat = this->m_outputFormat;
    else
        outputFormat = guessFormat(this->m_location);

    if (outputFormat.isEmpty())
        return QVariantMap();

    if (codecParams.contains("label"))
        this->m_streamConfigs[index]["label"] = codecParams["label"];

    QbCaps streamCaps = this->m_streamConfigs[index]["caps"].value<QbCaps>();
    QString codec;
    bool streamChanged = false;

    if (codecParams.contains("codec")) {
        if (this->supportedCodecs(outputFormat, streamCaps.mimeType())
            .contains(codecParams["codec"].toString())) {
            codec = codecParams["codec"].toString();
        } else
            codec = this->defaultCodec(outputFormat, streamCaps.mimeType());

        this->m_streamConfigs[index]["codec"] = codec;
        streamChanged |= true;

        // Update sample format.
        QVariantMap codecDefaults = this->defaultCodecParams(codec);

        if (streamCaps.mimeType() == "audio/x-raw") {
            QbAudioCaps audioCaps(streamCaps);
            QString sampleFormat = QbAudioCaps::sampleFormatToString(audioCaps.format());
            QStringList supportedSampleFormats = codecDefaults["supportedSampleFormats"].toStringList();

            if (!supportedSampleFormats.isEmpty()
                && !supportedSampleFormats.contains(sampleFormat)) {
                QString defaultSampleFormat = codecDefaults["defaultSampleFormat"].toString();
                audioCaps.format() = QbAudioCaps::sampleFormatFromString(defaultSampleFormat);
                audioCaps.bps() = byteCountMap->value(defaultSampleFormat, 0);
            }

            QVariantList supportedSampleRates = codecDefaults["supportedSampleRates"].toList();

            if (!supportedSampleRates.isEmpty()) {
                int sampleRate = 0;
                int maxDiff = std::numeric_limits<int>::max();

                foreach (QVariant rate, supportedSampleRates) {
                    int diff = qAbs(audioCaps.rate() - rate.toInt());

                    if (diff < maxDiff) {
                        sampleRate = rate.toInt();

                        if (!diff)
                            break;

                        maxDiff = diff;
                    }
                }

                audioCaps.rate() = sampleRate;
            }

            QString channelLayout = QbAudioCaps::channelLayoutToString(audioCaps.layout());
            QStringList supportedChannelLayouts = codecDefaults["supportedChannelLayouts"].toStringList();

            if (!supportedChannelLayouts.isEmpty() && !supportedChannelLayouts.contains(channelLayout)) {
                QString defaultChannelLayout = codecDefaults["defaultChannelLayout"].toString();
                audioCaps.layout() = QbAudioCaps::channelLayoutFromString(defaultChannelLayout);
                audioCaps.channels() = channelCountMap->value(defaultChannelLayout, 0);
            }

            streamCaps = audioCaps.toCaps();
            this->m_streamConfigs[index]["timeBase"] = QVariant::fromValue(QbFrac(1, audioCaps.rate()));
        } else if (streamCaps.mimeType() == "video/x-raw") {
            QbVideoCaps videoCaps(streamCaps);
            QString pixelFormat = QbVideoCaps::pixelFormatToString(videoCaps.format());
            QStringList supportedPixelFormats = codecDefaults["supportedPixelFormats"].toStringList();

            if (!supportedPixelFormats.isEmpty()
                && !supportedPixelFormats.contains(pixelFormat)) {
                QString defaultPixelFormat = codecDefaults["defaultPixelFormat"].toString();
                videoCaps.format() = QbVideoCaps::pixelFormatFromString(defaultPixelFormat);
            }

            QVariantList supportedFrameRates = codecDefaults["supportedFrameRates"].toList();

            if (!supportedFrameRates.isEmpty()) {
                QbFrac frameRate;
                qreal maxDiff = std::numeric_limits<qreal>::max();

                foreach (QVariant rate, supportedFrameRates) {
                    qreal diff = qAbs(videoCaps.fps().value() - rate.value<QbFrac>().value());

                    if (diff < maxDiff) {
                        frameRate = rate.value<QbFrac>();

                        if (!diff)
                            break;

                        maxDiff = diff;
                    }
                }

                videoCaps.fps() = frameRate;
            }

            streamCaps = videoCaps.toCaps();
            this->m_streamConfigs[index]["timeBase"] = QVariant::fromValue(videoCaps.fps().invert());
        }

        this->m_streamConfigs[index]["caps"] = QVariant::fromValue(streamCaps);
    } else
        codec = this->m_streamConfigs[index]["codec"].toString();

    QVariantMap codecDefaults = this->defaultCodecParams(codec);

    if ((streamCaps.mimeType() == "audio/x-raw"
         || streamCaps.mimeType() == "video/x-raw")
        && codecParams.contains("bitrate")) {
        int bitRate = codecParams["bitrate"].toInt();
        this->m_streamConfigs[index]["bitrate"] = bitRate > 0?
                                                      bitRate:
                                                      codecDefaults["defaultBitRate"].toInt();
        streamChanged |= true;
    }

    if (streamCaps.mimeType() == "video/x-raw"
        && codecParams.contains("gop")) {
        int gop = codecParams["gop"].toInt();
        this->m_streamConfigs[index]["gop"] = gop > 0?
                                                  gop:
                                                  codecDefaults["defaultGOP"].toInt();
        streamChanged |= true;
    }

    if (codecParams.contains("codecOptions")) {
        this->m_streamConfigs[index]["codecOptions"] = codecParams["codecOptions"];
        streamChanged |= true;
    }

    if (streamChanged)
        this->streamUpdated(index);

    return this->m_streamConfigs[index];
}

QString MediaSink::guessFormat(const QString &fileName)
{
    if (!*profilesLoaded)
        return QString();

    QString ext = QFileInfo(fileName).suffix();

    foreach (FormatInfo format, *recordingFormats) {
        GstElementFactory *factory = gst_element_factory_find(format.name().toStdString().c_str());

        if (factory) {
            gst_object_unref(factory);

            if (format.extensions().contains(ext))
                return format.name();
        }
    }

    return QString();
}

void MediaSink::setLocation(const QString &location)
{
    if (this->m_location == location)
        return;

    this->m_location = location;
    emit this->locationChanged(location);
}

void MediaSink::setOutputFormat(const QString &outputFormat)
{
    if (this->m_outputFormat == outputFormat)
        return;

    this->m_outputFormat = outputFormat;
    emit this->outputFormatChanged(outputFormat);
}

void MediaSink::setFormatOptions(const QVariantMap &formatOptions)
{
    if (this->m_formatOptions == formatOptions)
        return;

    this->m_formatOptions = formatOptions;
    emit this->formatOptionsChanged(formatOptions);
}

void MediaSink::resetLocation()
{
    this->setLocation("");
}

void MediaSink::resetOutputFormat()
{
    this->setOutputFormat("");
}

void MediaSink::resetFormatOptions()
{
    this->setFormatOptions(QVariantMap());
}

void MediaSink::writeAudioPacket(const QbAudioPacket &packet)
{
    Q_UNUSED(packet)
}

void MediaSink::writeVideoPacket(const QbVideoPacket &packet)
{
    Q_UNUSED(packet)
}

void MediaSink::writeSubtitlePacket(const QbPacket &packet)
{
    Q_UNUSED(packet)
}

void MediaSink::clearStreams()
{
    this->m_streamConfigs.clear();
    this->streamsChanged(this->streams());
}

bool MediaSink::init()
{
    return false;
}

void MediaSink::uninit()
{

}

void MediaSink::updateStreams()
{
    QList<QVariantMap> streamConfigs = this->m_streamConfigs;
    this->clearStreams();

    foreach (QVariantMap configs, streamConfigs) {
        QbCaps caps = configs["caps"].value<QbCaps>();
        int index = configs["index"].toInt();
        this->addStream(index, caps, configs);
    }
}

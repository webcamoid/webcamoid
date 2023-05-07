/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <limits>
#include <QDebug>
#include <QVariant>
#include <QMap>
#include <QVector>
#include <QFileInfo>
#include <QtConcurrent>
#include <QThreadPool>
#include <akaudiocaps.h>
#include <akaudiopacket.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <aksubtitlecaps.h>
#include <aksubtitlepacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/pbutils/encoding-profile.h>
#include <gst/video/video.h>

#include "mediawritergstreamer.h"
#include "outputparams.h"

#define MINIMUM_PLUGIN_RANK GST_RANK_SECONDARY

using VectorInt = QVector<int>;
using SizeList = QList<QSize>;
using GstToSampleFormatMap = QMap<AkAudioCaps::SampleFormat, QString>;
using GstToPixelFormatMap = QMap<AkVideoCaps::PixelFormat, QString>;
using VectorVideoCaps = QVector<AkVideoCaps>;
using StringVectorIntMap = QMap<QString, VectorInt>;
using OptionTypeStrMap = QMap<GType, QString>;

class MediaWriterGStreamerPrivate
{
    public:
        MediaWriterGStreamer *self;
        QString m_outputFormat;
        QMap<QString, QVariantMap> m_formatOptions;
        QMap<QString, QVariantMap> m_codecOptions;
        QList<QVariantMap> m_streamConfigs;
        QList<OutputParams> m_streamParams;
        QThreadPool m_threadPool;
        GstElement *m_pipeline {nullptr};
        GMainLoop *m_mainLoop {nullptr};
        QFuture<void> m_mainLoopResult;
        guint m_busWatchId {0};
        bool m_isRecording {false};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_rgb24, 0, 0, {}}};

        explicit MediaWriterGStreamerPrivate(MediaWriterGStreamer *self);
        QString guessFormat(const QString &fileName);
        QStringList readCaps(const QString &element);
        QVariantList parseOptions(const GstElement *element) const;
        void waitState(GstState state);
        static gboolean busCallback(GstBus *bus,
                                    GstMessage *message,
                                    gpointer userData);
        void setElementOptions(GstElement *element, const QVariantMap &options);
        AkVideoCaps nearestDVCaps(const AkVideoCaps &caps) const;
        AkAudioCaps nearestFLVAudioCaps(const AkAudioCaps &caps,
                                        const QString &codec) const;
        AkAudioCaps nearestSampleRate(const AkAudioCaps &caps,
                                      const QVariantList &sampleRates) const;
        AkAudioCaps nearestSampleRate(const AkAudioCaps &caps,
                                      const QList<int> &sampleRates) const;
        AkVideoCaps nearestFrameRate(const AkVideoCaps &caps,
                                     const QVariantList &frameRates) const;
        AkVideoCaps nearestFrameRate(const AkVideoCaps &caps,
                                     const QList<AkFrac> &frameRates) const;
        AkVideoCaps nearestFrameSize(const AkVideoCaps &caps,
                                     const QList<QSize> &frameSizes) const;
        bool setDefaultAudioCodecParams(const QString &codec,
                                        GstCaps *rawCaps,
                                        QVariantMap &codecParams);
        bool setDefaultVideoCodecParams(const QString &codec,
                                        GstCaps *rawCaps,
                                        QVariantMap &codecParams);
        void initAudio(int index,
                       const QString &optKey,
                       const AkCaps &streamCaps,
                       const QVariantMap &codecDefaults,
                       const QVariantMap &configs,
                       const QString &codec,
                       const QString &outputFormat,
                       GstElement *muxer);
        void initVideo(int index,
                       const QString &optKey,
                       const AkCaps &streamCaps,
                       const QVariantMap &codecDefaults,
                       const QVariantMap &configs,
                       const QString &codec,
                       const QString &outputFormat,
                       GstElement *muxer);
        void writeAudioPacket(const AkAudioPacket &packet);
        void writeVideoPacket(const AkVideoPacket &packet);
        void writeSubtitlePacket(const AkSubtitlePacket &packet);

        inline static const GstToSampleFormatMap &gstToSampleFormat()
        {
            static const GstToSampleFormatMap gstToFormat {
                {AkAudioCaps::SampleFormat_s8   , "S8"   },
                {AkAudioCaps::SampleFormat_u8   , "U8"   },
                {AkAudioCaps::SampleFormat_s16le, "S16LE"},
                {AkAudioCaps::SampleFormat_s16be, "S16BE"},
                {AkAudioCaps::SampleFormat_u16le, "U16LE"},
                {AkAudioCaps::SampleFormat_u16be, "U16BE"},
                {AkAudioCaps::SampleFormat_s32le, "S32LE"},
                {AkAudioCaps::SampleFormat_s32be, "S32BE"},
                {AkAudioCaps::SampleFormat_u32le, "U32LE"},
                {AkAudioCaps::SampleFormat_u32be, "U32BE"},
                {AkAudioCaps::SampleFormat_fltle, "F32LE"},
                {AkAudioCaps::SampleFormat_fltbe, "F32BE"},
                {AkAudioCaps::SampleFormat_dblle, "F64LE"},
                {AkAudioCaps::SampleFormat_dblbe, "F64BE"},
            };

            return gstToFormat;
        }

        inline static const GstToPixelFormatMap &gstToPixelFormat()
        {
            static const GstToPixelFormatMap gstToFormat {
                {AkVideoCaps::Format_yuv420p     , "I420"     },
                {AkVideoCaps::Format_yuyv422     , "YUY2"     },
                {AkVideoCaps::Format_uyvy422     , "UYVY"     },
                {AkVideoCaps::Format_rgb0        , "RGBx"     },
                {AkVideoCaps::Format_bgr0        , "BGRx"     },
                {AkVideoCaps::Format_0rgb        , "xRGB"     },
                {AkVideoCaps::Format_0bgr        , "xBGR"     },
                {AkVideoCaps::Format_rgba        , "RGBA"     },
                {AkVideoCaps::Format_bgra        , "BGRA"     },
                {AkVideoCaps::Format_argb        , "ARGB"     },
                {AkVideoCaps::Format_abgr        , "ABGR"     },
                {AkVideoCaps::Format_rgb24       , "RGB"      },
                {AkVideoCaps::Format_bgr24       , "BGR"      },
                {AkVideoCaps::Format_yuv411p     , "Y41B"     },
                {AkVideoCaps::Format_yuv422p     , "Y42B"     },
                {AkVideoCaps::Format_yuv444p     , "Y444"     },
                {AkVideoCaps::Format_nv12        , "NV12"     },
                {AkVideoCaps::Format_nv21        , "NV21"     },
                {AkVideoCaps::Format_gray8       , "GRAY8"    },
                {AkVideoCaps::Format_gray16be    , "GRAY16_BE"},
                {AkVideoCaps::Format_gray16le    , "GRAY16_LE"},
                {AkVideoCaps::Format_rgb565le    , "RGB16"    },
                {AkVideoCaps::Format_bgr565le    , "BGR16"    },
                {AkVideoCaps::Format_rgb555le    , "RGB15"    },
                {AkVideoCaps::Format_rgb555le    , "BGR15"    },
                {AkVideoCaps::Format_yuva420p    , "A420"     },
                {AkVideoCaps::Format_yuv410p     , "YUV9"     },
                {AkVideoCaps::Format_ayuv64le    , "AYUV64"   },
                {AkVideoCaps::Format_yuv420p10be , "I420_10BE"},
                {AkVideoCaps::Format_yuv420p10le , "I420_10LE"},
                {AkVideoCaps::Format_yuv422p10be , "I422_10BE"},
                {AkVideoCaps::Format_yuv422p10le , "I422_10LE"},
                {AkVideoCaps::Format_yuv444p10be , "Y444_10BE"},
                {AkVideoCaps::Format_yuv444p10le , "Y444_10LE"},
                {AkVideoCaps::Format_gbrp        , "GBR"      },
                {AkVideoCaps::Format_gbrp10be    , "GBR_10BE" },
                {AkVideoCaps::Format_gbrp10le    , "GBR_10LE" },
                {AkVideoCaps::Format_nv16        , "NV16"     },
                {AkVideoCaps::Format_yuva420p10be, "A420_10BE"},
                {AkVideoCaps::Format_yuva420p10le, "A420_10LE"},
                {AkVideoCaps::Format_yuva422p10be, "A422_10BE"},
                {AkVideoCaps::Format_yuva422p10le, "A422_10LE"},
                {AkVideoCaps::Format_yuva444p10be, "A444_10BE"},
                {AkVideoCaps::Format_yuva444p10le, "A444_10LE"},
            };

            return gstToFormat;
        }

        inline static const VectorVideoCaps &dvSupportedCaps()
        {
            static const VectorVideoCaps dvSupportedCaps {
                // Digital Video doesn't support height > 576 yet.
                /*
                {AkVideoCaps::Format_yuv422p, 1440, 1080, {25, 1}      },
                {AkVideoCaps::Format_yuv422p, 1280, 1080, {30000, 1001}},
                {AkVideoCaps::Format_yuv422p, 960 , 720 , {60000, 1001}},
                {AkVideoCaps::Format_yuv422p, 960 , 720 , {50, 1}      },*/
                {AkVideoCaps::Format_yuv422p, 720 , 576 , {25, 1}      },
                {AkVideoCaps::Format_yuv420p, 720 , 576 , {25, 1}      },
                {AkVideoCaps::Format_yuv411p, 720 , 576 , {25, 1}      },
                {AkVideoCaps::Format_yuv422p, 720 , 480 , {30000, 1001}},
                {AkVideoCaps::Format_yuv411p, 720 , 480 , {30000, 1001}},
            };

            return dvSupportedCaps;
        }

        inline static const StringVectorIntMap &flvSupportedSampleRates()
        {
            static const StringVectorIntMap flvSupportedSampleRates {
                {"avenc_adpcm_swf" , {5512, 11025, 22050, 44100}              },
                {"lamemp3enc"      , {5512, 8000 , 11025, 22050, 44100}       },
                {"faac"            , {}                                       },
                {"avenc_nellymoser", {5512, 8000 , 11025, 16000, 22050, 44100}},
                {"identity"        , {5512, 11025, 22050, 44100}              },
                {"alawenc"         , {5512, 11025, 22050, 44100}              },
                {"mulawenc"        , {5512, 11025, 22050, 44100}              },
                {"speexenc"        , {16000}                                  },
            };

            return flvSupportedSampleRates;
        }

        inline static const OptionTypeStrMap &codecGstOptionTypeToStr()
        {
            static const OptionTypeStrMap optionTypeStrMap {
                {G_TYPE_STRING          , "string" },
                {G_TYPE_BOOLEAN         , "boolean"},
                {G_TYPE_ULONG           , "number" },
                {G_TYPE_LONG            , "number" },
                {G_TYPE_UINT            , "number" },
                {G_TYPE_INT             , "number" },
                {G_TYPE_UINT64          , "number" },
                {G_TYPE_INT64           , "number" },
                {G_TYPE_FLOAT           , "number" },
                {G_TYPE_DOUBLE          , "number" },
                {G_TYPE_CHAR            , "number" },
                {G_TYPE_UCHAR           , "number" },
                {G_TYPE_PARAM_ENUM      , "menu"   },
                {G_TYPE_PARAM_FLAGS     , "flags"  },
                {GST_TYPE_CAPS          , "caps"   },
                {GST_TYPE_PARAM_FRACTION, "frac"   },
            };

            return optionTypeStrMap;
        }

        template<typename R, typename S>
        inline static R align(R value, S align)
        {
            return (value + (align >> 1)) & ~(align - 1);
        }
};

MediaWriterGStreamer::MediaWriterGStreamer(QObject *parent):
    MediaWriter(parent)
{
    this->d = new MediaWriterGStreamerPrivate(this);
    //qputenv("GST_DEBUG", "2");
    auto binDir = QDir(BINDIR).absolutePath();
    auto gstPluginsDir = QDir(GST_PLUGINS_PATH).absolutePath();
    auto relGstPluginsDir = QDir(binDir).relativeFilePath(gstPluginsDir);
    QDir appDir = QCoreApplication::applicationDirPath();

    if (appDir.cd(relGstPluginsDir)) {
        auto path = appDir.absolutePath();
        path.replace("/", QDir::separator());

        if (QFileInfo::exists(path)) {
            if (qEnvironmentVariableIsEmpty("GST_PLUGIN_PATH"))
                qputenv("GST_PLUGIN_PATH", path.toLocal8Bit());

            auto scanner = QFileInfo(GST_PLUGINS_SCANNER_PATH).baseName();

            if (!scanner.isEmpty()) {
                auto scannerPath = path + "/" + scanner;

                if (QFileInfo::exists(scannerPath)) {
                    if (qEnvironmentVariableIsEmpty("GST_PLUGIN_SCANNER"))
                        qputenv("GST_PLUGIN_SCANNER", scannerPath.toLocal8Bit());
                }
            }
        }
    }

    gst_init(nullptr, nullptr);

    this->m_formatsBlackList = QStringList {
        "3gppmux",
        "mp4mux",
        "qtmux"
    };
}

MediaWriterGStreamer::~MediaWriterGStreamer()
{
    this->uninit();
    delete this->d;
}

QString MediaWriterGStreamer::defaultFormat()
{
    auto formats = this->supportedFormats();

    if (formats.isEmpty())
        return {};

    if (formats.contains("webmmux"))
        return QStringLiteral("webmmux");

    return formats.first();
}

QString MediaWriterGStreamer::outputFormat() const
{
    return this->d->m_outputFormat;
}

QVariantList MediaWriterGStreamer::streams() const
{
    QVariantList streams;

    for (auto &stream: this->d->m_streamConfigs)
        streams << stream;

    return streams;
}

QStringList MediaWriterGStreamer::supportedFormats()
{
    QStringList supportedFormats;
    auto factoryList =
            gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_MUXER,
                                                  MINIMUM_PLUGIN_RANK);

    for (auto featureItem = factoryList;
         featureItem;
         featureItem = g_list_next(featureItem)) {
        if (G_UNLIKELY(featureItem->data == nullptr))
          continue;

        auto factory = GST_ELEMENT_FACTORY(featureItem->data);

        if (this->m_formatsBlackList.contains(GST_OBJECT_NAME(factory)))
            continue;

        if (!supportedFormats.contains(GST_OBJECT_NAME(factory)))
            supportedFormats << GST_OBJECT_NAME(factory);
    }

    gst_plugin_list_free(factoryList);
    std::sort(supportedFormats.begin(), supportedFormats.end());

    return supportedFormats;
}

QStringList MediaWriterGStreamer::fileExtensions(const QString &format)
{
    static const QMap<QString, QStringList> alternativeExtensions = {
        {"3gppmux"   , {"3gp"                 }},
        {"avmux_3gp" , {"3gp"                 }},
        {"avmux_3g2" , {"3g2"                 }},
        {"ismlmux"   , {"isml", "ismv", "isma"}},
        {"mp4mux"    , {"mp4"                 }},
        {"avmux_mp4" , {"mp4"                 }},
        {"avmux_psp" , {"psp" , "mp4"         }},
        {"avmux_ipod", {"m4v" , "m4a"         }},
    };

    if (alternativeExtensions.contains(format))
        return alternativeExtensions[format];

    auto supportedCaps = this->d->readCaps(format);
    QStringList extensions;

    for (auto &formatCaps: supportedCaps) {
        auto caps = gst_caps_from_string(formatCaps.toStdString().c_str());
        caps = gst_caps_fixate(caps);
        auto prof = gst_encoding_container_profile_new(nullptr,
                                                       nullptr,
                                                       caps,
                                                       nullptr);
        gst_caps_unref(caps);

        auto extension =
                gst_encoding_profile_get_file_extension(reinterpret_cast<GstEncodingProfile *>(prof));

        if (extension && !extensions.contains(extension))
            extensions << extension;

        gst_encoding_profile_unref(prof);
    }

    return extensions;
}

QString MediaWriterGStreamer::formatDescription(const QString &format)
{
    QString description;
    auto factory = gst_element_factory_find(format.toStdString().c_str());

    if (factory) {
        auto feature = gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory));

        if (feature) {
            auto longName =
                    gst_element_factory_get_metadata(GST_ELEMENT_FACTORY(feature),
                                                     GST_ELEMENT_METADATA_LONGNAME);
            description = QString(longName);
            gst_object_unref(feature);
        }

        gst_object_unref(factory);
    }

    return description;
}

QVariantList MediaWriterGStreamer::formatOptions()
{
    QString outputFormat = this->d->m_outputFormat.isEmpty()?
                               this->d->guessFormat(this->m_location):
                               this->d->m_outputFormat;

    if (outputFormat.isEmpty())
        return {};

    auto element = gst_element_factory_make(outputFormat.toStdString().c_str(),
                                            nullptr);

    if (!element)
        return {};

    auto options = this->d->parseOptions(element);
    gst_object_unref(element);
    auto globalFormatOptions =
            this->d->m_formatOptions.value(outputFormat);
    QVariantList formatOptions;

    for (auto &option: options) {
        auto optionList = option.toList();
        auto key = optionList[0].toString();

        if (globalFormatOptions.contains(key))
            optionList[7] = globalFormatOptions[key];

        formatOptions << QVariant(optionList);
    }

    return formatOptions;
}

QStringList MediaWriterGStreamer::supportedCodecs(const QString &format)
{
    return this->supportedCodecs(format, AkCaps::CapsUnknown);
}

QStringList MediaWriterGStreamer::supportedCodecs(const QString &format,
                                                  AkCaps::CapsType type)
{
    auto factory = gst_element_factory_find(format.toStdString().c_str());

    if (!factory)
        return {};

    auto feature = gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory));

    if (!feature) {
        gst_object_unref(factory);

        return {};
    }

    static GstStaticCaps staticRawCaps =
            GST_STATIC_CAPS("video/x-raw;"
                            "audio/x-raw;"
                            "text/x-raw;"
                            "subpicture/x-dvd;"
                            "subpicture/x-pgs");

    static const QMap<QString, AkCaps::CapsType> structureTypeToAk {
        {"audio/x-raw", AkCaps::CapsAudio   },
        {"video/x-raw", AkCaps::CapsVideo   },
        {"text/x-raw" , AkCaps::CapsSubtitle},
    };

    auto rawCaps = gst_static_caps_get(&staticRawCaps);
    auto encodersList =
            gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_ENCODER,
                                                  MINIMUM_PLUGIN_RANK);

    auto pads = gst_element_factory_get_static_pad_templates(GST_ELEMENT_FACTORY(feature));
    QStringList supportedCodecs;

    for (auto padItem = pads; padItem; padItem = g_list_next(padItem)) {
        auto padtemplate =
                reinterpret_cast<GstStaticPadTemplate *>(padItem->data);

        if (padtemplate->direction == GST_PAD_SINK) {
            auto caps = gst_caps_from_string(padtemplate->static_caps.string);

            for (guint i = 0; i < gst_caps_get_size(caps); i++) {
                auto capsStructure = gst_caps_get_structure(caps, i);
                auto structureName = gst_structure_get_name(capsStructure);
                QString structureType(structureName);
                auto structureStr = gst_structure_to_string(capsStructure);
                auto compCaps = gst_caps_from_string(structureStr);

                if (gst_caps_can_intersect(compCaps, rawCaps)) {
                    if (type != AkCaps::CapsUnknown
                        && structureTypeToAk.value(structureType,
                                                   AkCaps::CapsUnknown) != type) {
                        gst_caps_unref(compCaps);
                        g_free(structureStr);

                        continue;
                    }

                    auto codecType = structureType.mid(0, structureType.indexOf('/'));

                    if (gst_structure_has_field(capsStructure, "format")) {
                        GType fieldType = gst_structure_get_field_type(capsStructure, "format");

                        if (fieldType == G_TYPE_STRING) {
                            auto format = gst_structure_get_string(capsStructure, "format");
                            auto codecId = QString("identity/%1/%2")
                                           .arg(codecType, format);

                            if (!supportedCodecs.contains(codecId)
                                && !this->m_codecsBlackList.contains(codecId))
                                supportedCodecs << codecId;
                        } else if (fieldType == GST_TYPE_LIST) {
                            auto formats = gst_structure_get_value(capsStructure, "format");

                            for (guint i = 0; i < gst_value_list_get_size(formats); i++) {
                                auto format = gst_value_list_get_value(formats, i);
                                auto codecId =
                                        QString("identity/%1/%2")
                                            .arg(codecType,
                                                 g_value_get_string(format));

                                if (!supportedCodecs.contains(codecId)
                                    && !this->m_codecsBlackList.contains(codecId))
                                    supportedCodecs << codecId;
                            }
                        }
                    }
                } else {
                    auto encoders =
                            gst_element_factory_list_filter(encodersList,
                                                            caps,
                                                            GST_PAD_SRC,
                                                            FALSE);

                    for (auto encoderItem = encoders;
                         encoderItem;
                         encoderItem = g_list_next(encoderItem)) {
                        auto encoder =
                                reinterpret_cast<GstElementFactory *>(encoderItem->data);

                        if (this->m_codecsBlackList.contains(GST_OBJECT_NAME(encoder)))
                            continue;

                        auto klass =
                                gst_element_factory_get_metadata(encoder,
                                                                 GST_ELEMENT_METADATA_KLASS);
                        AkCaps::CapsType codecType =
                                !strcmp(klass, "Codec/Encoder/Audio")?
                                    AkCaps::CapsAudio:
                                (!strcmp(klass, "Codec/Encoder/Video")
                                 || !strcmp(klass, "Codec/Encoder/Image"))?
                                     AkCaps::CapsVideo: AkCaps::CapsUnknown;

                        if (type != AkCaps::CapsUnknown && type != codecType)
                            continue;

                        if (!supportedCodecs.contains(GST_OBJECT_NAME(encoder)))
                            supportedCodecs << GST_OBJECT_NAME(encoder);
                    }

                    gst_plugin_feature_list_free(encoders);
                }

                gst_caps_unref(compCaps);
                g_free(structureStr);
            }

            gst_caps_unref(caps);
        }
    }

    gst_plugin_feature_list_free(encodersList);
    gst_caps_unref(rawCaps);
    gst_object_unref(feature);
    gst_object_unref(factory);

    // Disable conflictive codecs
    static const QMap<QString, QStringList> unsupportedCodecs = {
        {"mp4mux"     , {"schroenc"             }},
        {"flvmux"     , {"lamemp3enc"           }},
        {"matroskamux", {"avenc_tta",
                         "identity/audio/F32LE",
                         "identity/audio/F64LE",
                         "identity/audio/S16BE",
                         "identity/audio/S16LE",
                         "identity/audio/S24BE",
                         "identity/audio/S24LE",
                         "identity/audio/S32BE",
                         "identity/audio/S32LE",
                         "identity/audio/U8"    }},
        {"mpegtsmux"  , {"lamemp3enc",
                         "twolamemp2enc"        }},
        {"*"          , {"av1enc",
                         "avenc_dca",
                         "avenc_dnxhd",
                         "avenc_jpeg2000",
                         "avenc_prores_ks",
                         "avenc_rv10",
                         "avenc_rv20",
                         "openjpegenc",
                         "pngenc",
                         "theoraenc"            }},
    };

    for (auto &codec: unsupportedCodecs.value(format) + unsupportedCodecs["*"])
        supportedCodecs.removeAll(codec);

    std::sort(supportedCodecs.begin(), supportedCodecs.end());

    return supportedCodecs;
}

QString MediaWriterGStreamer::defaultCodec(const QString &format,
                                           AkCaps::CapsType type)
{
    auto codecs = this->supportedCodecs(format, type);

    if (codecs.isEmpty())
        return {};

    return codecs.first();
}

QString MediaWriterGStreamer::codecDescription(const QString &codec)
{
    if (codec.startsWith("identity/")) {
        QStringList parts = codec.split("/");

        return QString("%1 (%2)").arg(parts[0], parts[2]);
    }

    QString description;
    auto factory = gst_element_factory_find(codec.toStdString().c_str());

    if (factory) {
        auto feature = gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory));

        if (feature) {
            auto longName =
                    gst_element_factory_get_metadata(GST_ELEMENT_FACTORY(feature),
                                                     GST_ELEMENT_METADATA_LONGNAME);
            description = QString(longName);
            gst_object_unref(feature);
        }

        gst_object_unref(factory);
    }

    return description;
}

AkCaps::CapsType MediaWriterGStreamer::codecType(const QString &codec)
{
    if (codec.startsWith("identity/audio"))
        return AkCaps::CapsAudio;

    if (codec.startsWith("identity/video"))
        return AkCaps::CapsVideo;

    if (codec.startsWith("identity/text"))
        return AkCaps::CapsSubtitle;

    AkCaps::CapsType codecType;
    auto factory = gst_element_factory_find(codec.toStdString().c_str());

    if (factory) {
        auto feature = gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory));

        if (feature) {
            auto klass = gst_element_factory_get_metadata(GST_ELEMENT_FACTORY(feature),
                                                          GST_ELEMENT_METADATA_KLASS);
            codecType =
                    !strcmp(klass, "Codec/Encoder/Audio")?
                        AkCaps::CapsAudio:
                    (!strcmp(klass, "Codec/Encoder/Video")
                     || !strcmp(klass, "Codec/Encoder/Image"))?
                         AkCaps::CapsVideo: AkCaps::CapsUnknown;
            gst_object_unref(feature);
        }

        gst_object_unref(factory);
    }

    return codecType;
}

QVariantMap MediaWriterGStreamer::defaultCodecParams(const QString &codec)
{
    QVariantMap codecParams;

    static GstStaticCaps staticRawCaps = GST_STATIC_CAPS("video/x-raw;"
                                                         "audio/x-raw;"
                                                         "text/x-raw;"
                                                         "subpicture/x-dvd;"
                                                         "subpicture/x-pgs");

    auto rawCaps = gst_static_caps_get(&staticRawCaps);
    bool result = true;

    switch (this->codecType(codec)) {
    case AkCaps::CapsAudio:
        result = this->d->setDefaultAudioCodecParams(codec, rawCaps, codecParams);

        break;
    case AkCaps::CapsVideo:
        result = this->d->setDefaultVideoCodecParams(codec, rawCaps, codecParams);

        break;
    default:
        break;
    }

    gst_caps_unref(rawCaps);

    if (!result)
        codecParams.clear();

    return codecParams;
}

QVariantMap MediaWriterGStreamer::addStream(int streamIndex,
                                            const AkCaps &streamCaps)
{
    return this->addStream(streamIndex, streamCaps, {});
}

QVariantMap MediaWriterGStreamer::addStream(int streamIndex,
                                            const AkCaps &streamCaps,
                                            const QVariantMap &codecParams)
{
    QString outputFormat =
            this->supportedFormats().contains(this->d->m_outputFormat)?
                this->d->m_outputFormat: this->d->guessFormat(this->m_location);

    if (outputFormat.isEmpty())
        return QVariantMap();

    QVariantMap outputParams;
    outputParams["index"] = streamIndex;
    auto codec = codecParams.value("codec").toString();
    auto supportedCodecs = this->supportedCodecs(outputFormat, streamCaps.type());

    if (codec.isEmpty() || !supportedCodecs.contains(codec))
        codec = this->defaultCodec(outputFormat, streamCaps.type());

    outputParams["codec"] = codec;
    auto codecDefaults = this->defaultCodecParams(codec);

    if (streamCaps.type() == AkCaps::CapsAudio) {
        int bitRate = codecParams.value("bitrate").toInt();
        outputParams["bitrate"] = bitRate > 0?
                                      bitRate:
                                      codecDefaults["defaultBitRate"].toInt();
        outputParams["caps"] = QVariant::fromValue(streamCaps);
        AkAudioCaps audioCaps(streamCaps);
        outputParams["timeBase"] = QVariant::fromValue(AkFrac(1, audioCaps.rate()));
    } else if (streamCaps.type() == AkCaps::CapsVideo) {
        int bitRate = codecParams.value("bitrate").toInt();
        outputParams["bitrate"] = bitRate > 0?
                                      bitRate:
                                      codecDefaults["defaultBitRate"].toInt();
        int gop = codecParams.value("gop",
                                    codecDefaults["defaultGOP"]).toInt();
        outputParams["gop"] = gop > 0?
                                  gop:
                                  codecDefaults["defaultGOP"].toInt();

        outputParams["caps"] = QVariant::fromValue(streamCaps);
        AkVideoCaps videoCaps(streamCaps);
        outputParams["timeBase"] = QVariant::fromValue(videoCaps.fps().invert());
    } else if (streamCaps.type() == AkCaps::CapsSubtitle) {
        outputParams["caps"] = QVariant::fromValue(streamCaps);
    }

    this->d->m_streamConfigs << outputParams;
    emit this->streamsChanged(this->streams());

    return outputParams;
}

QVariantMap MediaWriterGStreamer::updateStream(int index)
{
    return this->updateStream(index, {});
}

QVariantMap MediaWriterGStreamer::updateStream(int index,
                                               const QVariantMap &codecParams)
{
    QString outputFormat;

    if (this->supportedFormats().contains(this->d->m_outputFormat))
        outputFormat = this->d->m_outputFormat;
    else
        outputFormat = this->d->guessFormat(this->m_location);

    if (outputFormat.isEmpty())
        return {};

    auto streamCaps = this->d->m_streamConfigs[index]["caps"].value<AkCaps>();
    QString codec;
    bool streamChanged = false;

    if (codecParams.contains("codec")) {
        if (this->supportedCodecs(outputFormat, streamCaps.type())
            .contains(codecParams["codec"].toString())) {
            codec = codecParams["codec"].toString();
        } else
            codec = this->defaultCodec(outputFormat, streamCaps.type());

        this->d->m_streamConfigs[index]["codec"] = codec;
        streamChanged = true;

        // Update sample format.

        if (streamCaps.type() == AkCaps::CapsAudio) {
            AkAudioCaps audioCaps(streamCaps);
            this->d->m_streamConfigs[index]["timeBase"] =
                    QVariant::fromValue(AkFrac(1, audioCaps.rate()));
        } else if (streamCaps.type() == AkCaps::CapsVideo) {
            AkVideoCaps videoCaps(streamCaps);
            this->d->m_streamConfigs[index]["timeBase"] =
                    QVariant::fromValue(videoCaps.fps().invert());
        }

        this->d->m_streamConfigs[index]["caps"] =
                QVariant::fromValue(streamCaps);
    } else {
        codec = this->d->m_streamConfigs[index]["codec"].toString();
    }

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

QVariantList MediaWriterGStreamer::codecOptions(int index)
{
    QString outputFormat =
            this->supportedFormats().contains(this->d->m_outputFormat)?
                this->d->m_outputFormat: this->d->guessFormat(this->m_location);

    if (outputFormat.isEmpty())
        return {};

    auto codec = this->d->m_streamConfigs.value(index).value("codec").toString();

    if (codec.isEmpty())
        return {};

    auto element = gst_element_factory_make(codec.toStdString().c_str(),
                                            nullptr);

    if (!element)
        return {};

    auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(index).arg(codec);
    auto options = this->d->parseOptions(element);
    gst_object_unref(element);
    auto globalCodecOptions = this->d->m_codecOptions.value(optKey);
    QVariantList codecOptions;

    for (auto &option: options) {
        auto optionList = option.toList();
        auto key = optionList[0].toString();

        if ((codec == "vp8enc" || codec == "vp9enc")
            && key == "deadline")
            optionList[6] = optionList[7] = 1;
        else if ((codec == "x264enc" || codec == "x265enc")
                 && key == "speed-preset")
            optionList[6] = optionList[7] = "ultrafast";

        if (globalCodecOptions.contains(key))
            optionList[7] = globalCodecOptions[key];

        codecOptions << QVariant(optionList);
    }

    return codecOptions;
}

void MediaWriterGStreamer::setOutputFormat(const QString &outputFormat)
{
    if (this->d->m_outputFormat == outputFormat)
        return;

    this->d->m_outputFormat = outputFormat;
    emit this->outputFormatChanged(outputFormat);
}

void MediaWriterGStreamer::setFormatOptions(const QVariantMap &formatOptions)
{
    QString outputFormat = this->d->m_outputFormat.isEmpty()?
                               this->d->guessFormat(this->m_location):
                               this->d->m_outputFormat;
    bool modified = false;

    for (auto it = formatOptions.cbegin(); it != formatOptions.cend(); it++)
        if (it.value() != this->d->m_formatOptions.value(outputFormat).value(it.key())) {
            this->d->m_formatOptions[outputFormat][it.key()] = it.value();
            modified = true;
        }

    if (modified)
        emit this->formatOptionsChanged(this->d->m_formatOptions.value(outputFormat));
}

void MediaWriterGStreamer::setCodecOptions(int index,
                                           const QVariantMap &codecOptions)
{
    auto outputFormat = this->d->m_outputFormat.isEmpty()?
                            this->d->guessFormat(this->m_location):
                            this->d->m_outputFormat;

    if (outputFormat.isEmpty())
        return;

    auto codec = this->d->m_streamConfigs.value(index).value("codec").toString();

    if (codec.isEmpty())
        return;

    auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(index).arg(codec);
    bool modified = false;

    for (auto it = codecOptions.cbegin(); it != codecOptions.cend(); it++)
        if (it.value() != this->d->m_codecOptions.value(optKey).value(it.key())) {
            this->d->m_codecOptions[optKey][it.key()] = it.value();
            modified = true;
        }

    if (modified)
        emit this->codecOptionsChanged(optKey, this->d->m_codecOptions.value(optKey));
}

void MediaWriterGStreamer::resetOutputFormat()
{
    this->setOutputFormat("");
}

void MediaWriterGStreamer::resetFormatOptions()
{
    QString outputFormat = this->d->m_outputFormat.isEmpty()?
                               this->d->guessFormat(this->m_location):
                               this->d->m_outputFormat;

    if (this->d->m_formatOptions.value(outputFormat).isEmpty())
        return;

    this->d->m_formatOptions.remove(outputFormat);
    emit this->formatOptionsChanged(QVariantMap());
}

void MediaWriterGStreamer::resetCodecOptions(int index)
{
    auto outputFormat = this->d->m_outputFormat.isEmpty()?
                            this->d->guessFormat(this->m_location):
                            this->d->m_outputFormat;

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

void MediaWriterGStreamer::enqueuePacket(const AkPacket &packet)
{
    if (!this->d->m_isRecording)
        return;

    switch (packet.type()) {
    case AkPacket::PacketAudio:
        this->d->writeAudioPacket(packet);
        break;
    case AkPacket::PacketVideo:
        this->d->writeVideoPacket(packet);
        break;
    case AkPacket::PacketSubtitle:
        this->d->writeSubtitlePacket(packet);
        break;
    default:
        break;
    }
}

void MediaWriterGStreamer::clearStreams()
{
    this->d->m_streamConfigs.clear();
    emit this->streamsChanged(this->streams());
}

bool MediaWriterGStreamer::init()
{
    QString outputFormat = this->d->m_outputFormat.isEmpty()?
                               this->d->guessFormat(this->m_location):
                               this->d->m_outputFormat;

    this->d->m_pipeline = gst_pipeline_new(nullptr);
    auto muxer = gst_element_factory_make(outputFormat.toStdString().c_str(),
                                          nullptr);

    if (!muxer)
        return false;

    // Set format options.
    this->d->setElementOptions(muxer, this->d->m_formatOptions.value(outputFormat));

    auto filesink = gst_element_factory_make("filesink", nullptr);
    g_object_set(G_OBJECT(filesink),
                 "location",
                 this->m_location.toStdString().c_str(),
                 nullptr);
    gst_bin_add_many(GST_BIN(this->d->m_pipeline), muxer, filesink, nullptr);
    gst_element_link_many(muxer, filesink, nullptr);

    auto streamConfigs = this->d->m_streamConfigs.toVector();

    for (int i = 0; i < streamConfigs.count(); i++) {
        auto configs = streamConfigs[i];
        auto streamCaps = configs["caps"].value<AkCaps>();
        auto codec = configs["codec"].toString();

        if (codec.startsWith("identity/"))
            codec = "identity";

        auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(i).arg(codec);
        auto codecDefaults = this->defaultCodecParams(codec);

        switch (streamCaps.type()) {
        case AkCaps::CapsAudio:
            this->d->initAudio(i,
                               optKey,
                               streamCaps,
                               codecDefaults,
                               configs,
                               codec,
                               outputFormat,
                               muxer);
            break;
        case AkCaps::CapsVideo:
            this->d->initVideo(i,
                               optKey,
                               streamCaps,
                               codecDefaults,
                               configs,
                               codec,
                               outputFormat,
                               muxer);
            break;
        default:
            break;
        }

        this->d->m_streamParams << OutputParams(configs["index"].toInt());
    }

    // Configure the message bus.
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->d->m_pipeline));
    this->d->m_busWatchId = gst_bus_add_watch(bus, this->d->busCallback, this);
    gst_object_unref(bus);

    // Run the main GStreamer loop.
    this->d->m_mainLoop = g_main_loop_new(nullptr, FALSE);
    this->d->m_mainLoopResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              g_main_loop_run,
                              this->d->m_mainLoop);
    gst_element_set_state(this->d->m_pipeline, GST_STATE_PLAYING);
    this->d->m_isRecording = true;

    return true;
}

void MediaWriterGStreamer::uninit()
{
    this->d->m_isRecording = false;

    if (this->d->m_pipeline) {
        auto sources = gst_bin_iterate_sources(GST_BIN(this->d->m_pipeline));
        GValue sourceItm = G_VALUE_INIT;
        gboolean done = FALSE;

        while (!done) {
            switch (gst_iterator_next(sources, &sourceItm)) {
            case GST_ITERATOR_OK: {
                    auto source = GST_ELEMENT(g_value_get_object(&sourceItm));

                    if (gst_app_src_end_of_stream(GST_APP_SRC(source)) != GST_FLOW_OK)
                        qWarning() << "Error sending EOS to "
                                   << gst_element_get_name(source);

                    g_value_reset(&sourceItm);
                }
                break;
            case GST_ITERATOR_RESYNC:
                // Rollback changes to items.
                gst_iterator_resync(sources);
                break;
            case GST_ITERATOR_ERROR:
                // Wrong parameters were given.
                done = TRUE;
                break;
            case GST_ITERATOR_DONE:
                done = TRUE;
                break;
            default:
                break;
            }
        }

        g_value_unset(&sourceItm);
        gst_iterator_free(sources);

        gst_element_send_event(this->d->m_pipeline, gst_event_new_eos());

        gst_element_set_state(this->d->m_pipeline, GST_STATE_NULL);
        this->d->waitState(GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(this->d->m_pipeline));
        g_source_remove(this->d->m_busWatchId);
        this->d->m_pipeline = nullptr;
        this->d->m_busWatchId = 0;
    }

    if (this->d->m_mainLoop) {
        g_main_loop_quit(this->d->m_mainLoop);
        g_main_loop_unref(this->d->m_mainLoop);
        this->d->m_mainLoop = nullptr;
    }

    this->d->m_streamParams.clear();
}

MediaWriterGStreamerPrivate::MediaWriterGStreamerPrivate(MediaWriterGStreamer *self):
    self(self)
{
}

QString MediaWriterGStreamerPrivate::guessFormat(const QString &fileName)
{
    auto ext = QFileInfo(fileName).suffix();

    for (auto &format: self->supportedFormats())
        if (self->fileExtensions(format).contains(ext))
            return format;

    return {};
}

QStringList MediaWriterGStreamerPrivate::readCaps(const QString &element)
{
    auto factory = gst_element_factory_find(element.toStdString().c_str());

    if (!factory)
        return {};

    QStringList elementCaps;
    auto feature = gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory));

    if (!feature) {
        gst_object_unref(factory);

        return {};
    }

    auto pads = gst_element_factory_get_static_pad_templates(GST_ELEMENT_FACTORY(feature));

    for (auto padItem = pads; padItem; padItem = g_list_next(padItem)) {
        auto padtemplate =
                reinterpret_cast<GstStaticPadTemplate *>(padItem->data);

        if (padtemplate->direction == GST_PAD_SRC
            && padtemplate->presence == GST_PAD_ALWAYS) {
            auto caps = gst_caps_from_string(padtemplate->static_caps.string);

            for (guint i = 0; i < gst_caps_get_size(caps); i++) {
                auto capsStructure = gst_caps_get_structure(caps, i);
                auto structureCaps = gst_structure_to_string(capsStructure);

                elementCaps << structureCaps;

                g_free(structureCaps);
            }

            gst_caps_unref(caps);
        }
    }

    gst_object_unref(feature);
    gst_object_unref(factory);

    return elementCaps;
}

QVariantList MediaWriterGStreamerPrivate::parseOptions(const GstElement *element) const
{
    QVariantList options;
    guint nprops = 0;

    auto propSpecs =
            g_object_class_list_properties(G_OBJECT_GET_CLASS(element),
                                           &nprops);

    for (guint i = 0; i < nprops; i++) {
        auto param = propSpecs[i];

        if ((param->flags & G_PARAM_READWRITE) != G_PARAM_READWRITE)
            continue;

#if 0
        if (param->flags & G_PARAM_DEPRECATED)
            continue;
#endif

        auto name = g_param_spec_get_name(param);

        if (!strcmp(name, "name")
            || !strcmp(name, "bitrate")
            || !strcmp(name, "target-bitrate")
            || !strcmp(name, "keyframe-max-dist")
            || !strcmp(name, "gop-size"))
            continue;

        QVariant defaultValue;
        QVariant value;
        qreal min = 0;
        qreal max = 0;
        qreal step = 0;
        QVariantList menu;
        auto paramType = MediaWriterGStreamerPrivate::codecGstOptionTypeToStr().value(param->value_type);

        GValue gValue;
        memset(&gValue, 0, sizeof(GValue));
        g_value_init(&gValue, param->value_type);
        g_object_get_property(G_OBJECT (element), param->name, &gValue);

        switch (param->value_type) {
            case G_TYPE_STRING: {
                value = g_value_get_string(&gValue);
                auto spec = G_PARAM_SPEC_STRING(param);
                defaultValue = spec->default_value;
                break;
            }
            case G_TYPE_BOOLEAN: {
                value = g_value_get_boolean(&gValue);
                auto spec = G_PARAM_SPEC_BOOLEAN(param);
                defaultValue = spec->default_value;
                break;
            }
            case G_TYPE_ULONG: {
                value = quint64(g_value_get_ulong(&gValue));
                auto spec = G_PARAM_SPEC_ULONG(param);
                defaultValue = quint64(spec->default_value);
                min = spec->minimum;
                max = spec->maximum;
                step = 1;
                break;
            }
            case G_TYPE_LONG: {
                value = qint64(g_value_get_long(&gValue));
                auto spec = G_PARAM_SPEC_LONG(param);
                defaultValue = qint64(spec->default_value);
                min = spec->minimum;
                max = spec->maximum;
                step = 1;
                break;
            }
            case G_TYPE_UINT: {
                value = g_value_get_uint(&gValue);
                auto spec = G_PARAM_SPEC_UINT(param);
                defaultValue = spec->default_value;
                min = spec->minimum;
                max = spec->maximum;
                step = 1;
                break;
            }
            case G_TYPE_INT: {
                value = g_value_get_int(&gValue);
                auto spec = G_PARAM_SPEC_INT(param);
                defaultValue = spec->default_value;
                min = spec->minimum;
                max = spec->maximum;
                step = 1;
                break;
            }
            case G_TYPE_UINT64: {
                value = quint64(g_value_get_uint64(&gValue));
                auto spec = G_PARAM_SPEC_UINT64(param);
                defaultValue = quint64(spec->default_value);
                min = spec->minimum;
                max = spec->maximum;
                step = 1;
                break;
            }
            case G_TYPE_INT64: {
                value = qint64(g_value_get_int64(&gValue));
                auto spec = G_PARAM_SPEC_INT64(param);
                defaultValue = qint64(spec->default_value);
                min = spec->minimum;
                max = spec->maximum;
                step = 1;
                break;
            }
            case G_TYPE_FLOAT: {
                value = g_value_get_float(&gValue);
                auto spec = G_PARAM_SPEC_FLOAT(param);
                defaultValue = spec->default_value;
                min = qreal(spec->minimum);
                max = qreal(spec->maximum);
                step = 0.01;
                break;
            }
            case G_TYPE_DOUBLE: {
                value = g_value_get_double(&gValue);
                auto spec = G_PARAM_SPEC_DOUBLE(param);
                defaultValue = spec->default_value;
                min = qreal(spec->minimum);
                max = qreal(spec->maximum);
                step = 0.01;
                break;
            }
            case G_TYPE_CHAR: {
                value = g_value_get_schar(&gValue);
                auto spec = G_PARAM_SPEC_CHAR(param);
                defaultValue = spec->default_value;
                min = spec->minimum;
                max = spec->maximum;
                step = 1;
                break;
            }
            case G_TYPE_UCHAR: {
                value = g_value_get_uchar(&gValue);
                auto spec = G_PARAM_SPEC_UCHAR(param);
                defaultValue = spec->default_value;
                min = spec->minimum;
                max = spec->maximum;
                step = 1;
                break;
            }
            default:
                if (G_IS_PARAM_SPEC_ENUM(param)) {
                    auto curValue = g_value_get_enum(&gValue);
                    value = curValue;
                    auto spec = G_PARAM_SPEC_ENUM(param);
                    auto gValue = G_ENUM_CLASS(g_type_class_ref(param->value_type))->values;

                    if (gValue) {
                        for (; gValue->value_name; gValue++) {
                            if (spec->default_value == gValue->value)
                                defaultValue = gValue->value_nick;

                            if (curValue == gValue->value)
                                value = gValue->value_nick;

                            menu << QVariant(QVariantList {
                                gValue->value_nick,
                                gValue->value_name,
                                gValue->value
                            });
                        }

                        if (!defaultValue.isNull())
                            defaultValue = menu.first().toList().first();

                        if (!value.isNull())
                            value = defaultValue;
                    }

                    paramType = "menu";
                } else if (G_IS_PARAM_SPEC_FLAGS(param)) {
                    // flag1+flag2+flags3+...
                    auto flags = g_value_get_flags(&gValue);
                    auto spec = G_PARAM_SPEC_FLAGS(param);
                    auto gValue = spec->flags_class->values;
                    QStringList defaultFlagList;
                    QStringList flagList;

                    if (gValue)
                        for (; gValue->value_name; gValue++) {
                            if ((spec->default_value & gValue->value) == gValue->value)
                                defaultFlagList << gValue->value_nick;

                            if ((flags & gValue->value) == gValue->value)
                                flagList << gValue->value_nick;

                            menu << QVariant(QVariantList {
                                gValue->value_nick,
                                gValue->value_name,
                                gValue->value
                            });
                        }

                    defaultValue = defaultFlagList;
                    value = flagList;
                    paramType = "flags";
                } else if (GST_IS_PARAM_SPEC_FRACTION(param)) {
                    auto num = gst_value_get_fraction_numerator(&gValue);
                    auto den = gst_value_get_fraction_denominator(&gValue);
                    value = AkFrac(num, den).toString();
                    defaultValue = value;
                    paramType = "frac";
                } else if (param->value_type == GST_TYPE_CAPS) {
                    auto caps = gst_caps_to_string(gst_value_get_caps(&gValue));
                    value = QString(caps);
                    g_free(caps);
                    defaultValue = value;
                    paramType = "caps";
                } else
                    continue;

                break;
        }

        g_value_unset(&gValue);

        options << QVariant(QVariantList {
                                name,
                                g_param_spec_get_blurb(param),
                                paramType,
                                min,
                                max,
                                step,
                                defaultValue,
                                value,
                                menu
                            });
    }

    g_free(propSpecs);

    return options;
}

void MediaWriterGStreamerPrivate::waitState(GstState state)
{
    forever {
        GstState curState;
        auto ret = gst_element_get_state(this->m_pipeline,
                                         &curState,
                                         nullptr,
                                         GST_CLOCK_TIME_NONE);

        if (ret == GST_STATE_CHANGE_FAILURE)
            break;

        if (ret == GST_STATE_CHANGE_SUCCESS
            && curState == state)
            break;
    }
}

gboolean MediaWriterGStreamerPrivate::busCallback(GstBus *bus,
                                                  GstMessage *message,
                                                  gpointer userData)
{
    Q_UNUSED(bus)
    auto self = static_cast<MediaWriterGStreamer *>(userData);

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        GError *err = nullptr;
        gchar *debug = nullptr;
        gst_message_parse_error(message, &err, &debug);

        qDebug() << "ERROR: from element"
                 << GST_MESSAGE_SRC_NAME(message)
                 << ":"
                 << err->message;

        if (debug)
            qDebug() << "Additional debug info:\n"
                     << debug;

        auto element = GST_ELEMENT(GST_MESSAGE_SRC(message));

        for (auto padItem = GST_ELEMENT_PADS(element);
             padItem;
             padItem = g_list_next(padItem)) {
            auto pad = GST_PAD_CAST(padItem->data);
            auto curCaps = gst_pad_get_current_caps(pad);
            auto curCapsStr = gst_caps_to_string(curCaps);

            qDebug() << "    Current caps:" << curCapsStr;

            g_free(curCapsStr);
            gst_caps_unref(curCaps);

            auto allCaps = gst_pad_get_allowed_caps(pad);
            auto allCapsStr = gst_caps_to_string(allCaps);

            qDebug() << "    Allowed caps:" << allCapsStr;

            g_free(allCapsStr);
            gst_caps_unref(allCaps);
        }

        g_error_free(err);
        g_free(debug);
        g_main_loop_quit(self->d->m_mainLoop);

        break;
    }
    case GST_MESSAGE_EOS:
        qDebug() << "EOS received";
        g_main_loop_quit(self->d->m_mainLoop);

        break;
    case GST_MESSAGE_STATE_CHANGED: {
        GstState oldstate;
        GstState newstate;
        GstState pending;
        gst_message_parse_state_changed(message, &oldstate, &newstate, &pending);
        qDebug() << "State changed from"
                 << gst_element_state_get_name(oldstate)
                 << "to"
                 << gst_element_state_get_name(newstate);

        break;
    }
    case GST_MESSAGE_STREAM_STATUS: {
        GstStreamStatusType type;
        GstElement *owner = nullptr;
        gst_message_parse_stream_status(message, &type, &owner);
        qDebug() << "Stream Status:"
                 << GST_ELEMENT_NAME(owner)
                 << "is"
                 << type;

        break;
    }
    case GST_MESSAGE_LATENCY: {
        qDebug() << "Recalculating latency";
        gst_bin_recalculate_latency(GST_BIN(self->d->m_pipeline));
        break;
    }
    case GST_MESSAGE_STREAM_START: {
        qDebug() << "Stream started";
        break;
    }
    case GST_MESSAGE_ASYNC_DONE: {
        GstClockTime runningTime;
        gst_message_parse_async_done(message, &runningTime);
        qDebug() << "ASYNC done";
        break;
    }
    case GST_MESSAGE_NEW_CLOCK: {
        GstClock *clock = nullptr;
        gst_message_parse_new_clock(message, &clock);
        qDebug() << "New clock:" << (clock? GST_OBJECT_NAME(clock): "NULL");
        break;
    }
    case GST_MESSAGE_DURATION_CHANGED: {
        GstFormat format;
        gint64 duration;
        gst_message_parse_duration(message, &format, &duration);
        qDebug() << "Duration changed:"
                 << gst_format_get_name(format)
                 << ","
                 << qreal(duration);
        break;
    }
    case GST_MESSAGE_TAG: {
        GstTagList *tagList = nullptr;
        gst_message_parse_tag(message, &tagList);
        auto tags = gst_tag_list_to_string(tagList);
//        qDebug() << "Tags:" << tags;
        g_free(tags);
        gst_tag_list_unref(tagList);

        break;
    }
    case GST_MESSAGE_ELEMENT: {
        auto messageStructure = gst_message_get_structure(message);
        auto structure = gst_structure_to_string(messageStructure);
//        qDebug() << structure;
        g_free(structure);

        break;
    }
    case GST_MESSAGE_QOS: {
        qDebug() << QString("Received QOS from element %1:")
                        .arg(GST_MESSAGE_SRC_NAME(message)).toStdString().c_str();

        gint64 jitter;
        gdouble proportion;
        gint quality;
        gst_message_parse_qos_values(message, &jitter, &proportion, &quality);
        qDebug() << "    Jitter =" << jitter;
        qDebug() << "    Proportion =" << proportion;
        qDebug() << "    Quality =" << quality;

        gboolean live;
        guint64 runningTime;
        guint64 streamTime;
        guint64 timestamp;
        guint64 duration;
        gst_message_parse_qos(message,
                              &live,
                              &runningTime,
                              &streamTime,
                              &timestamp,
                              &duration);
        qDebug() << "    Is live stream =" << live;
        qDebug() << "    Runninng time =" << runningTime;
        qDebug() << "    Stream time =" << streamTime;
        qDebug() << "    Timestamp =" << timestamp;
        qDebug() << "    Duration =" << duration;

        break;
    }
    default:
        qDebug() << "Unhandled message:" << GST_MESSAGE_TYPE_NAME(message);
    break;
    }

    return TRUE;
}

void MediaWriterGStreamerPrivate::setElementOptions(GstElement *element,
                                                    const QVariantMap &options)
{
    for (auto it = options.cbegin(); it != options.cend(); it++) {
        auto paramSpec =
                g_object_class_find_property(G_OBJECT_GET_CLASS(element),
                                             it.key().toStdString().c_str());

        if (!paramSpec || !(paramSpec->flags & G_PARAM_WRITABLE))
            continue;

        GValue gValue;
        memset(&gValue, 0, sizeof(GValue));
        g_value_init(&gValue, paramSpec->value_type);
        QString value;

        if (G_IS_PARAM_SPEC_FLAGS(paramSpec)) {
            auto flags = it.value().toStringList();
            value = flags.join('+');
        } else {
            value = it.value().toString();
        }

        if (!gst_value_deserialize(&gValue, value.toStdString().c_str()))
            continue;

        g_object_set_property(G_OBJECT(element),
                              it.key().toStdString().c_str(),
                              &gValue);
    }
}

AkVideoCaps MediaWriterGStreamerPrivate::nearestDVCaps(const AkVideoCaps &caps) const
{
    AkVideoCaps nearestCaps;
    qreal q = std::numeric_limits<qreal>::max();

    for (auto &sCaps: MediaWriterGStreamerPrivate::dvSupportedCaps()) {
        qreal dw = sCaps.width() - caps.width();
        qreal dh = sCaps.height() - caps.height();
        qreal df = sCaps.fps().value() - caps.fps().value();
        qreal k = dw * dw + dh * dh + df * df;

        if (k < q) {
            nearestCaps = sCaps;
            q = k;
        } else if (qFuzzyCompare(k, q) && sCaps.format() == caps.format())
            nearestCaps = sCaps;
    }

    return nearestCaps;
}

AkAudioCaps MediaWriterGStreamerPrivate::nearestFLVAudioCaps(const AkAudioCaps &caps,
                                                             const QString &codec) const
{
    int nearestSampleRate = caps.rate();
    int q = std::numeric_limits<int>::max();
    auto &sampleRates = MediaWriterGStreamerPrivate::flvSupportedSampleRates();

    for (auto &sampleRate: sampleRates.value(codec)) {
        int k = qAbs(sampleRate - caps.rate());

        if (k < q) {
            nearestSampleRate = sampleRate;
            q = k;

            if (k == 0)
                break;
        }
    }

    AkAudioCaps nearestCaps(caps);
    nearestCaps.setRate(nearestSampleRate);

    return nearestCaps;
}

AkAudioCaps MediaWriterGStreamerPrivate::nearestSampleRate(const AkAudioCaps &caps,
                                                           const QVariantList &sampleRates) const
{
    QList<int> rates;

    for (auto &rate: sampleRates)
        rates << rate.toInt();

    return this->nearestSampleRate(caps, rates);
}

AkAudioCaps MediaWriterGStreamerPrivate::nearestSampleRate(const AkAudioCaps &caps,
                                                           const QList<int> &sampleRates) const
{
    if (sampleRates.isEmpty())
        return caps;

    auto audioCaps = caps;
    int sampleRate = 0;
    int maxDiff = std::numeric_limits<int>::max();

    for (auto &rate: sampleRates) {
        int diff = qAbs(audioCaps.rate() - rate);

        if (diff < maxDiff) {
            sampleRate = rate;

            if (!diff)
                break;

            maxDiff = diff;
        }
    }

    audioCaps.setRate(sampleRate);

    return audioCaps;
}

AkVideoCaps MediaWriterGStreamerPrivate::nearestFrameRate(const AkVideoCaps &caps,
                                                          const QVariantList &frameRates) const
{
    QList<AkFrac> rates;

    for (auto &rate: frameRates)
        rates << rate.value<AkFrac>();

    return this->nearestFrameRate(caps, rates);
}

AkVideoCaps MediaWriterGStreamerPrivate::nearestFrameRate(const AkVideoCaps &caps,
                                                          const QList<AkFrac> &frameRates) const
{
    if (frameRates.isEmpty())
        return caps;

    auto videoCaps = caps;
    AkFrac frameRate;
    qreal maxDiff = std::numeric_limits<qreal>::max();

    for (auto &rate: frameRates) {
        qreal diff = qAbs(videoCaps.fps().value() - rate.value());

        if (diff < maxDiff) {
            frameRate = rate;

            if (qIsNull(diff))
                break;

            maxDiff = diff;
        }
    }

    videoCaps.fps() = frameRate;

    return videoCaps;
}

AkVideoCaps MediaWriterGStreamerPrivate::nearestFrameSize(const AkVideoCaps &caps,
                                                          const QList<QSize> &frameSizes) const
{
    if (frameSizes.isEmpty())
        return caps;

    QSize nearestSize;
    qreal q = std::numeric_limits<qreal>::max();

    for (auto &size: frameSizes) {
        qreal dw = size.width() - caps.width();
        qreal dh = size.height() - caps.height();
        qreal k = dw * dw + dh * dh;

        if (k < q) {
            nearestSize = size;
            q = k;

            if (k == 0.)
                break;
        }
    }

    AkVideoCaps nearestCaps(caps);
    nearestCaps.setWidth(nearestSize.width());
    nearestCaps.setHeight(nearestSize.height());

    return nearestCaps;
}

bool MediaWriterGStreamerPrivate::setDefaultAudioCodecParams(const QString &codec,
                                                             GstCaps *rawCaps,
                                                             QVariantMap &codecParams)
{
    if (codec.startsWith("identity/audio")) {
        auto gstFormat = codec.split("/").at(2);
        auto sampleFormat =
                MediaWriterGStreamerPrivate::gstToSampleFormat().key(gstFormat,
                                                                     AkAudioCaps::SampleFormat_none);
        codecParams["defaultBitRate"] = 128000;
        codecParams["supportedSampleFormats"] = QVariantList {sampleFormat};
        codecParams["supportedChannelLayouts"] =
                QVariantList {AkAudioCaps::Layout_mono,
                              AkAudioCaps::Layout_stereo};
        codecParams["supportedSampleRates"] = QVariantList();
        codecParams["defaultSampleFormat"] = sampleFormat;
        codecParams["defaultChannelLayout"] = AkAudioCaps::Layout_stereo;
        codecParams["defaultChannels"] = 2;
        codecParams["defaultSampleRate"] = 44100;
    } else {
        auto factory =
                gst_element_factory_find(codec.toStdString().c_str());

        if (!factory)
            return false;

        auto feature = gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory));

        if (!feature) {
            gst_object_unref(factory);

            return false;
        }

        QVariantList supportedSampleFormats;
        QVariantList supportedSamplerates;
        QVariantList supportedChannelLayouts;

        auto pads = gst_element_factory_get_static_pad_templates(GST_ELEMENT_FACTORY(feature));

        for (auto padItem = pads;
             padItem;
             padItem = g_list_next(padItem)) {
            auto padtemplate =
                    reinterpret_cast<GstStaticPadTemplate *>(padItem->data);

            if (padtemplate->direction == GST_PAD_SINK
                && padtemplate->presence == GST_PAD_ALWAYS) {
                auto caps =
                        gst_caps_from_string(padtemplate->static_caps.string);

                for (guint i = 0; i < gst_caps_get_size(caps); i++) {
                    auto capsStructure = gst_caps_get_structure(caps, i);
                    auto structureStr = gst_structure_to_string(capsStructure);
                    auto compCaps = gst_caps_from_string(structureStr);

                    if (gst_caps_can_intersect(compCaps, rawCaps)) {
                        // Get supported formats
                        if (gst_structure_has_field(capsStructure, "format")) {
                            GType fieldType = gst_structure_get_field_type(capsStructure, "format");

                            if (fieldType == G_TYPE_STRING) {
                                auto format = gst_structure_get_string(capsStructure, "format");
                                auto formatFF =
                                        MediaWriterGStreamerPrivate::gstToSampleFormat().key(format,
                                                                                             AkAudioCaps::SampleFormat_none);

                                if (formatFF != AkAudioCaps::SampleFormat_none
                                    && !supportedSampleFormats.contains(formatFF))
                                    supportedSampleFormats << formatFF;
                            } else if (fieldType == GST_TYPE_LIST) {
                                const GValue *formats = gst_structure_get_value(capsStructure, "format");

                                for (guint i = 0; i < gst_value_list_get_size(formats); i++) {
                                    auto format = gst_value_list_get_value(formats, i);
                                    auto formatId = g_value_get_string(format);
                                    auto formatFF =
                                            MediaWriterGStreamerPrivate::gstToSampleFormat().key(formatId,
                                                                                                 AkAudioCaps::SampleFormat_none);

                                    if (formatFF != AkAudioCaps::SampleFormat_none
                                        && !supportedSampleFormats.contains(formatFF))
                                        supportedSampleFormats << formatFF;
                                }
                            }
                        }

                        // Get supported sample rates
                        if (gst_structure_has_field(capsStructure, "rate")) {
                            GType fieldType = gst_structure_get_field_type(capsStructure, "rate");

                            if (fieldType == G_TYPE_INT) {
                                gint rate;
                                gst_structure_get_int(capsStructure, "rate", &rate);

                                if (!supportedSamplerates.contains(rate))
                                    supportedSamplerates << rate;
                            } else if (fieldType == GST_TYPE_INT_RANGE) {
                            } else if (fieldType == GST_TYPE_LIST) {
                                auto rates = gst_structure_get_value(capsStructure, "rate");

                                for (guint i = 0;
                                     i < gst_value_list_get_size(rates);
                                     i++) {
                                    auto rate = gst_value_list_get_value(rates, i);
                                    gint rateId = g_value_get_int(rate);

                                    if (!supportedSamplerates.contains(rateId))
                                        supportedSamplerates << rateId;
                                }
                            }
                        }

                        // Get supported channel layouts
                        if (gst_structure_has_field(capsStructure, "channels")) {
                            GType fieldType = gst_structure_get_field_type(capsStructure, "channels");

                            if (fieldType == G_TYPE_INT) {
                                gint channels;
                                gst_structure_get_int(capsStructure, "channels", &channels);
                                auto layout = AkAudioCaps::defaultChannelLayout(channels);

                                if (!supportedChannelLayouts.contains(layout))
                                    supportedChannelLayouts << layout;
                            } else if (fieldType == GST_TYPE_INT_RANGE) {
                                auto channels = gst_structure_get_value(capsStructure, "channels");

                                int min = gst_value_get_int_range_min(channels);
                                int max = gst_value_get_int_range_max(channels) + 1;
                                int step = gst_value_get_int_range_step(channels);

                                for (int i = min; i < max; i += step) {
                                    auto layout = AkAudioCaps::defaultChannelLayout(i);

                                    if (!supportedChannelLayouts.contains(layout))
                                        supportedChannelLayouts << layout;
                                }
                            } else if (fieldType == GST_TYPE_LIST) {
                                auto channels = gst_structure_get_value(capsStructure, "channels");

                                for (guint i = 0; i < gst_value_list_get_size(channels); i++) {
                                    auto nchannels = gst_value_list_get_value(channels, i);
                                    gint nchannelsId = g_value_get_int(nchannels);
                                    auto layout = AkAudioCaps::defaultChannelLayout(nchannelsId);

                                    if (!supportedChannelLayouts.contains(layout))
                                        supportedChannelLayouts << layout;
                                }
                            }
                        }
                    }

                    gst_caps_unref(compCaps);
                    g_free(structureStr);
                }

                gst_caps_unref(caps);
            }
        }

        auto element =
                gst_element_factory_create(GST_ELEMENT_FACTORY(feature),
                                           nullptr);

        if (!element) {
            gst_object_unref(feature);
            gst_object_unref(factory);

            return false;
        }

        int bitrate = 0;

        if (g_object_class_find_property(G_OBJECT_GET_CLASS(element), "bitrate"))
            g_object_get(G_OBJECT(element), "bitrate", &bitrate, nullptr);

        if (codec == "lamemp3enc")
            bitrate *= 1000;

        if (bitrate < 1)
            bitrate = 128000;

        codecParams["defaultBitRate"] = bitrate;
        codecParams["supportedSampleFormats"] = supportedSampleFormats;
        codecParams["supportedChannelLayouts"] = supportedChannelLayouts;
        codecParams["supportedSampleRates"] = supportedSamplerates;
        codecParams["defaultSampleFormat"] =
                supportedSampleFormats.isEmpty()?
                    AkAudioCaps::SampleFormat_s16:
                    supportedSampleFormats.at(0);
        AkAudioCaps::ChannelLayout channelLayout =
                supportedChannelLayouts.isEmpty()?
                    AkAudioCaps::Layout_stereo:
                    AkAudioCaps::ChannelLayout(supportedChannelLayouts.at(0).toInt());
        codecParams["defaultChannelLayout"] = channelLayout;
        codecParams["defaultChannels"] = AkAudioCaps::channelCount(channelLayout);
        codecParams["defaultSampleRate"] = supportedSamplerates.isEmpty()?
                                             44100: supportedSamplerates.at(0);

        gst_object_unref(element);
        gst_object_unref(feature);
        gst_object_unref(factory);
    }

    return true;
}

bool MediaWriterGStreamerPrivate::setDefaultVideoCodecParams(const QString &codec,
                                                             GstCaps *rawCaps,
                                                             QVariantMap &codecParams)
{
    if (codec.startsWith("identity/video")) {
        auto gstFormat = codec.split("/").at(2);
        auto pixelFormat =
                MediaWriterGStreamerPrivate::gstToPixelFormat().key(gstFormat,
                                                                    AkVideoCaps::Format_none);
        codecParams["defaultBitRate"] = 1500000;
        codecParams["defaultGOP"] = 12;
        codecParams["supportedFrameRates"] = QVariantList();
        codecParams["supportedPixelFormats"] = QVariantList {int(pixelFormat)};
        codecParams["supportedFrameSizes"] = QVariantList();
        codecParams["defaultPixelFormat"] = int(pixelFormat);
    } else {
        auto factory = gst_element_factory_find(codec.toStdString().c_str());

        if (!factory) {
            gst_caps_unref(rawCaps);

            return false;
        }

        auto feature = gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory));

        if (!feature) {
            gst_object_unref(factory);
            gst_caps_unref(rawCaps);

            return false;
        }

        QVariantList supportedPixelFormats;
        QVariantList supportedFramerates;
        SizeList supportedFrameSizes;

        auto pads = gst_element_factory_get_static_pad_templates(GST_ELEMENT_FACTORY(feature));

        for (auto padItem = pads; padItem; padItem = g_list_next(padItem)) {
            auto padtemplate =
                    reinterpret_cast<GstStaticPadTemplate *>(padItem->data);

            if (padtemplate->direction == GST_PAD_SINK
                && padtemplate->presence == GST_PAD_ALWAYS) {
                auto caps = gst_caps_from_string(padtemplate->static_caps.string);

                for (guint i = 0; i < gst_caps_get_size(caps); i++) {
                    auto capsStructure = gst_caps_get_structure(caps, i);
                    auto structureStr = gst_structure_to_string(capsStructure);
                    auto compCaps = gst_caps_from_string(structureStr);

                    if (gst_caps_can_intersect(compCaps, rawCaps)) {
                        // Get supported formats
                        if (gst_structure_has_field(capsStructure, "format")) {
                            GType fieldType = gst_structure_get_field_type(capsStructure, "format");

                            if (fieldType == G_TYPE_STRING) {
                                auto format = gst_structure_get_string(capsStructure, "format");
                                auto formatFF =
                                        MediaWriterGStreamerPrivate::gstToPixelFormat()
                                        .key(format, AkVideoCaps::Format_none);

                                if (formatFF != AkVideoCaps::Format_none
                                    && !supportedPixelFormats.contains(int(formatFF)))
                                    supportedPixelFormats << int(formatFF);
                            } else if (fieldType == GST_TYPE_LIST) {
                                auto formats = gst_structure_get_value(capsStructure, "format");

                                for (guint i = 0; i < gst_value_list_get_size(formats); i++) {
                                    auto format = gst_value_list_get_value(formats, i);
                                    auto formatId = g_value_get_string(format);
                                    auto formatFF =
                                            MediaWriterGStreamerPrivate::gstToPixelFormat()
                                            .key(formatId, AkVideoCaps::Format_none);

                                    if (formatFF != AkVideoCaps::Format_none
                                        && !supportedPixelFormats.contains(int(formatFF)))
                                        supportedPixelFormats << int(formatFF);
                                }
                            }
                        }

                        // Get supported frame sizes
                        if (gst_structure_has_field(capsStructure, "width")
                            && gst_structure_has_field(capsStructure, "height")) {
                            gint width = 0;
                            gint height = 0;

                            GType fieldType = gst_structure_get_field_type(capsStructure, "width");

                            if (fieldType == G_TYPE_INT)
                                gst_structure_get_int(capsStructure, "width", &width);

                            fieldType = gst_structure_get_field_type(capsStructure, "height");

                            if (fieldType == G_TYPE_INT)
                                gst_structure_get_int(capsStructure, "height", &height);

                            QSize size(width, height);

                            if (!size.isEmpty() && !supportedFrameSizes.contains(size))
                                supportedFrameSizes << size;
                        }

                        // Get supported frame rates
                        if (gst_structure_has_field(capsStructure, "framerate")) {
                            GType fieldType = gst_structure_get_field_type(capsStructure, "framerate");

                            if (fieldType == GST_TYPE_FRACTION_RANGE) {
                            } else if (fieldType == GST_TYPE_LIST) {
                                auto framerates = gst_structure_get_value(capsStructure, "framerate");

                                for (guint i = 0; i < gst_value_list_get_size(framerates); i++) {
                                    auto frate = gst_value_list_get_value(framerates, i);
                                    auto num = gst_value_get_fraction_numerator(frate);
                                    auto den = gst_value_get_fraction_denominator(frate);
                                    AkFrac framerate(num, den);
                                    auto fps = QVariant::fromValue(framerate);

                                     if (!supportedFramerates.contains(fps))
                                         supportedFramerates << fps;
                                }
                            } else if (fieldType == GST_TYPE_FRACTION) {
                                gint num = 0;
                                gint den = 0;
                                gst_structure_get_fraction(capsStructure,
                                                           "framerate",
                                                           &num,
                                                           &den);
                               AkFrac framerate(num, den);
                               auto fps = QVariant::fromValue(framerate);

                                if (!supportedFramerates.contains(fps))
                                    supportedFramerates << fps;
                            }
                        }
                    }

                    gst_caps_unref(compCaps);
                    g_free(structureStr);
                }

                gst_caps_unref(caps);
            }
        }

        auto element =
                gst_element_factory_create(GST_ELEMENT_FACTORY(feature),
                                           nullptr);

        if (!element) {
            gst_object_unref(feature);
            gst_object_unref(factory);
            gst_caps_unref(rawCaps);

            return false;
        }

        // Read default bitrate
        int bitrate = 0;

        const char *propBitrate =
                QRegExp("vp\\d+enc").exactMatch(codec)?
                    "target-bitrate": "bitrate";

        if (g_object_class_find_property(G_OBJECT_GET_CLASS(element), propBitrate))
            g_object_get(G_OBJECT(element), propBitrate, &bitrate, nullptr);

        if (codec == "x264enc"
            || codec == "x265enc"
            || codec == "mpeg2enc"
            || codec == "theoraenc")
            bitrate *= 1000;

        if (bitrate < 1500000)
            bitrate = 1500000;

        // Read default GOP
        int gop = 0;
        QStringList gops {"keyframe-max-dist", "gop-size"};

        for (auto &g: gops)
            if (g_object_class_find_property(G_OBJECT_GET_CLASS(element),
                                             g.toStdString().c_str())) {
                g_object_get(G_OBJECT(element),
                             g.toStdString().c_str(),
                             &gop,
                             nullptr);

                break;
            }

        if (gop < 1)
            gop = 12;

        codecParams["defaultBitRate"] = bitrate;
        codecParams["defaultGOP"] = gop;
        codecParams["supportedFrameRates"] = supportedFramerates;
        codecParams["supportedPixelFormats"] = supportedPixelFormats;
        codecParams["supportedFrameSizes"] = QVariant::fromValue(supportedFrameSizes);
        codecParams["defaultPixelFormat"] = supportedPixelFormats.value(0,
                                                                        int(AkVideoCaps::Format_yuv420p));

        gst_object_unref(element);
        gst_object_unref(feature);
        gst_object_unref(factory);
    }

    return true;
}

void MediaWriterGStreamerPrivate::initAudio(int index,
                                            const QString &optKey,
                                            const AkCaps &streamCaps,
                                            const QVariantMap &codecDefaults,
                                            const QVariantMap &configs,
                                            const QString &codec,
                                            const QString &outputFormat,
                                            GstElement *muxer)
{
    auto sourceName = QString("audio_%1").arg(index);
    auto source = gst_element_factory_make("appsrc", sourceName.toStdString().c_str());
    gst_app_src_set_stream_type(GST_APP_SRC(source), GST_APP_STREAM_TYPE_STREAM);
    g_object_set(G_OBJECT(source), "format", GST_FORMAT_TIME, nullptr);
    g_object_set(G_OBJECT(source), "block", true, nullptr);

    AkAudioCaps audioCaps(streamCaps);

    auto sampleFormat = audioCaps.format();
    auto supportedSampleFormats =
            codecDefaults["supportedSampleFormats"].toList();

    if (!supportedSampleFormats.isEmpty()
        && !supportedSampleFormats.contains(sampleFormat)) {
        auto defaultSampleFormat =
                AkAudioCaps::SampleFormat(codecDefaults["defaultSampleFormat"].toInt());
        audioCaps.setFormat(defaultSampleFormat);
    }

    auto supportedSampleRates = codecDefaults["supportedSampleRates"].toList();
    audioCaps = this->nearestSampleRate(audioCaps, supportedSampleRates);
    auto channelLayout = audioCaps.layout();
    auto supportedChannelLayouts =
            codecDefaults["supportedChannelLayouts"].toList();

    if (!supportedChannelLayouts.isEmpty()
        && !supportedChannelLayouts.contains(channelLayout)) {
        auto defaultChannelLayout =
                AkAudioCaps::ChannelLayout(codecDefaults["defaultChannelLayout"].toInt());
        audioCaps.setLayout(defaultChannelLayout);
    }

    if (outputFormat == "flvmux") {
        audioCaps = this->nearestFLVAudioCaps(audioCaps, codec);
        QStringList codecs {"speexenc", "avenc_nellymoser"};

        if (codecs.contains(codec))
            audioCaps.setLayout(AkAudioCaps::Layout_mono);
    } else if (outputFormat == "avmux_dv") {
        audioCaps.setRate(48000);
    } else if (outputFormat == "avmux_gxf"
             || outputFormat == "avmux_mxf"
             || outputFormat == "avmux_mxf_d10") {
                audioCaps.setRate(qBound(4000, audioCaps.rate(), 96000));
    } else if (codec == "avenc_tta") {
        audioCaps.setRate(qBound(8000, audioCaps.rate(), 96000));
    }

    auto gstFormat =
            MediaWriterGStreamerPrivate::gstToSampleFormat()
            .value(audioCaps.format(), "S16");
    auto gstAudioCaps =
            gst_caps_new_simple("audio/x-raw",
                                "format", G_TYPE_STRING, gstFormat.toStdString().c_str(),
                                "layout", G_TYPE_STRING, "interleaved",
                                "rate", G_TYPE_INT, audioCaps.rate(),
                                "channels", G_TYPE_INT, audioCaps.channels(),
                                nullptr);

    gstAudioCaps = gst_caps_fixate(gstAudioCaps);
    gst_app_src_set_caps(GST_APP_SRC(source), gstAudioCaps);

    auto audioConvert = gst_element_factory_make("audioconvert", nullptr);
    auto audioResample = gst_element_factory_make("audioresample", nullptr);
    auto audioRate = gst_element_factory_make("audiorate", nullptr);
    auto audioCodec = gst_element_factory_make(codec.toStdString().c_str(),
                                               nullptr);

    if (codec.startsWith("avenc_"))
        g_object_set(G_OBJECT(audioCodec), "compliance", -2, nullptr);

    // Set codec options.
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(audioCodec),
                                     "bitrate")) {
        int bitrate = configs["bitrate"].toInt();

        if (codec == "lamemp3enc")
            bitrate /= 1000;

        if (bitrate > 0)
            g_object_set(G_OBJECT(audioCodec), "bitrate", bitrate, NULL);
    }

    auto codecOptions = this->m_codecOptions.value(optKey);
    this->setElementOptions(audioCodec, codecOptions);

    auto queue = gst_element_factory_make("queue", nullptr);

    gst_bin_add_many(GST_BIN(this->m_pipeline),
                     source,
                     audioResample,
                     audioRate,
                     audioConvert,
                     audioCodec,
                     queue,
                     nullptr);

    gst_element_link_many(source,
                          audioResample,
                          audioRate,
                          audioConvert,
                          nullptr);
    gst_element_link_filtered(audioConvert, audioCodec, gstAudioCaps);
    gst_caps_unref(gstAudioCaps);
    gst_element_link_many(audioCodec, queue, muxer, nullptr);
}

void MediaWriterGStreamerPrivate::initVideo(int index,
                                            const QString &optKey,
                                            const AkCaps &streamCaps,
                                            const QVariantMap &codecDefaults,
                                            const QVariantMap &configs,
                                            const QString &codec,
                                            const QString &outputFormat,
                                            GstElement *muxer)
{
    Q_UNUSED(outputFormat)

    auto sourceName = QString("video_%1").arg(index);
    auto source = gst_element_factory_make("appsrc", sourceName.toStdString().c_str());
    gst_app_src_set_stream_type(GST_APP_SRC(source), GST_APP_STREAM_TYPE_STREAM);
    g_object_set(G_OBJECT(source), "format", GST_FORMAT_TIME, nullptr);
    g_object_set(G_OBJECT(source), "block", true, nullptr);

    AkVideoCaps videoCaps(streamCaps);

    auto pixelFormat = videoCaps.format();
    auto supportedPixelFormats = codecDefaults["supportedPixelFormats"].toList();

    if (!supportedPixelFormats.contains(int(pixelFormat))) {
        auto defaultPixelFormat =
                AkVideoCaps::PixelFormat(codecDefaults["defaultPixelFormat"].toInt());
        videoCaps.setFormat(defaultPixelFormat);
    }

    auto supportedFrameSizes = codecDefaults["supportedFrameSizes"].value<SizeList>();
    videoCaps = this->nearestFrameSize(videoCaps, supportedFrameSizes);
    auto supportedFrameRates = codecDefaults["supportedFrameRates"].toList();
    videoCaps = this->nearestFrameRate(videoCaps, supportedFrameRates);

    if (codec == "avenc_dvvideo")
        videoCaps = this->nearestDVCaps(videoCaps);

    auto gstFormat =
            MediaWriterGStreamerPrivate::gstToPixelFormat()
            .value(videoCaps.format(), "I420");
    videoCaps.setWidth(MediaWriterGStreamerPrivate::align(videoCaps.width(), 4));

    auto gstVideoCaps =
            gst_caps_new_simple("video/x-raw",
                                "format", G_TYPE_STRING, gstFormat.toStdString().c_str(),
                                "width", G_TYPE_INT, videoCaps.width(),
                                "height", G_TYPE_INT, videoCaps.height(),
                                "framerate", GST_TYPE_FRACTION,
                                             int(videoCaps.fps().num()),
                                             int(videoCaps.fps().den()),
                                nullptr);

    gstVideoCaps = gst_caps_fixate(gstVideoCaps);
    gst_app_src_set_caps(GST_APP_SRC(source), gstVideoCaps);

    auto videoScale = gst_element_factory_make("videoscale", nullptr);
    auto videoRate = gst_element_factory_make("videorate", nullptr);
    auto videoConvert = gst_element_factory_make("videoconvert", nullptr);
    auto videoCodec = gst_element_factory_make(codec.toStdString().c_str(), nullptr);

    if (codec.startsWith("avenc_"))
        g_object_set(G_OBJECT(videoCodec), "compliance", -2, nullptr);

    // Set codec options.

    // Set bitrate
    const char *propBitrate =
            QRegExp("vp\\d+enc").exactMatch(codec)?
                "target-bitrate": "bitrate";

    if (g_object_class_find_property(G_OBJECT_GET_CLASS(videoCodec),
                                     propBitrate)) {
        int bitrate = configs["bitrate"].toInt();

        if (codec == "x264enc"
            || codec == "x265enc"
            || codec == "mpeg2enc"
            || codec == "theoraenc")
            bitrate /= 1000;

        if (bitrate > 0)
            g_object_set(G_OBJECT(videoCodec),
                         propBitrate,
                         bitrate,
                         NULL);
    }

    // Set GOP
    int gop = configs["gop"].toInt();

    if (gop > 0) {
        QStringList gops {"keyframe-max-dist", "gop-size"};

        for (auto &g: gops)
            if (g_object_class_find_property(G_OBJECT_GET_CLASS(videoCodec),
                                             g.toStdString().c_str())) {
                g_object_set(G_OBJECT(videoCodec),
                             g.toStdString().c_str(),
                             gop,
                             nullptr);

                break;
            }
    }

    auto codecOptions = this->m_codecOptions.value(optKey);

    if ((codec == "vp8enc" || codec == "vp9enc")
        && !codecOptions.contains("deadline"))
        codecOptions["deadline"] = 1;
    else if ((codec == "x264enc" || codec == "x265enc")
             && !codecOptions.contains("speed-preset"))
        codecOptions["speed-preset"] = "ultrafast";

    this->setElementOptions(videoCodec, codecOptions);

    auto queue = gst_element_factory_make("queue", nullptr);

    gst_bin_add_many(GST_BIN(this->m_pipeline),
                     source,
                     videoScale,
                     videoRate,
                     videoConvert,
                     videoCodec,
                     queue,
                     nullptr);

    gst_element_link_many(source,
                          videoScale,
                          videoRate,
                          videoConvert,
                          nullptr);
    gst_element_link_filtered(videoConvert, videoCodec, gstVideoCaps);
    gst_caps_unref(gstVideoCaps);
    gst_element_link_many(videoCodec, queue, muxer, nullptr);
}

void MediaWriterGStreamerPrivate::writeAudioPacket(const AkAudioPacket &packet)
{
    if (!this->m_pipeline)
        return;

    int streamIndex = -1;

    for (int i = 0; i < this->m_streamParams.size(); i++)
        if (this->m_streamParams[i].inputIndex() == packet.index()) {
            streamIndex = i;

            break;
        }

    if (streamIndex < 0)
        return;

    auto souceName = QString("audio_%1").arg(streamIndex);
    auto source = gst_bin_get_by_name(GST_BIN(this->m_pipeline),
                                      souceName.toStdString().c_str());

    if (!source)
        return;

    auto iFormat =
            MediaWriterGStreamerPrivate::gstToSampleFormat()
            .value(packet.caps().format(), "");

    if (iFormat.isEmpty())
        return;

    auto sourceCaps = gst_app_src_get_caps(GST_APP_SRC(source));
    const char *layoutType = packet.caps().planar()?
                                 "non-interleaved":
                                 "interleaved";

    auto inputCaps =
            gst_caps_new_simple("audio/x-raw",
                                "format", G_TYPE_STRING, iFormat.toStdString().c_str(),
                                "layout", G_TYPE_STRING, layoutType,
                                "rate", G_TYPE_INT, packet.caps().rate(),
                                "channels", G_TYPE_INT, packet.caps().channels(),
                                nullptr);
    inputCaps = gst_caps_fixate(inputCaps);

    if (!gst_caps_is_equal(sourceCaps, inputCaps))
        gst_app_src_set_caps(GST_APP_SRC(source), inputCaps);

    gst_caps_unref(inputCaps);
    gst_caps_unref(sourceCaps);

    auto buffer = gst_buffer_new_allocate(nullptr, packet.size(), nullptr);
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);
    memcpy(info.data, packet.constData(), packet.size());
    gst_buffer_unmap(buffer, &info);

    auto pts = qint64(packet.pts() * packet.timeBase().value() * GST_SECOND);

#if 0
    GST_BUFFER_PTS(buffer) = GST_BUFFER_DTS(buffer) = this->m_streamParams[streamIndex].nextPts(pts, packet.id());
    GST_BUFFER_DURATION(buffer) = packet.caps().samples() * packet.timeBase().value() * GST_SECOND;
    GST_BUFFER_OFFSET(buffer) = this->m_streamParams[streamIndex].nFrame();
#else
    GST_BUFFER_PTS(buffer) = this->m_streamParams[streamIndex].nextPts(pts, packet.id());
    GST_BUFFER_DTS(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_OFFSET(buffer) = GST_BUFFER_OFFSET_NONE;
#endif

    this->m_streamParams[streamIndex].nFrame() += quint64(packet.samples());

    if (gst_app_src_push_buffer(GST_APP_SRC(source), buffer) != GST_FLOW_OK)
        qWarning() << "Error pushing buffer to GStreamer pipeline";
}

void MediaWriterGStreamerPrivate::writeVideoPacket(const AkVideoPacket &packet)
{
    if (!this->m_pipeline)
        return;

    int streamIndex = -1;

    for (int i = 0; i < this->m_streamParams.size(); i++)
        if (this->m_streamParams[i].inputIndex() == packet.index()) {
            streamIndex = i;

            break;
        }

    if (streamIndex < 0)
        return;

    this->m_videoConverter.begin();
    auto videoPacket = this->m_videoConverter.convert(packet);
    this->m_videoConverter.end();

    auto souceName = QString("video_%1").arg(streamIndex);
    auto source = gst_bin_get_by_name(GST_BIN(this->m_pipeline),
                                      souceName.toStdString().c_str());

    if (!source)
        return;

    auto sourceCaps = gst_app_src_get_caps(GST_APP_SRC(source));
    auto iFormat = MediaWriterGStreamerPrivate::gstToPixelFormat()
                   .value(videoPacket.caps().format(), "BGR");
    auto inputCaps =
            gst_caps_new_simple("video/x-raw",
                                "format", G_TYPE_STRING, iFormat.toStdString().c_str(),
                                "width", G_TYPE_INT, videoPacket.caps().width(),
                                "height", G_TYPE_INT, videoPacket.caps().height(),
                                "framerate", GST_TYPE_FRACTION,
                                             int(videoPacket.caps().fps().num()),
                                             int(videoPacket.caps().fps().den()),
                                nullptr);
    inputCaps = gst_caps_fixate(inputCaps);

    if (!gst_caps_is_equal(sourceCaps, inputCaps))
        gst_app_src_set_caps(GST_APP_SRC(source), inputCaps);

    auto videoInfo = gst_video_info_new();
    gst_video_info_from_caps(videoInfo, inputCaps);

    auto size = size_t(videoPacket.size());
    auto buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);

    for (int plane = 0; plane < GST_VIDEO_INFO_N_PLANES(videoInfo); ++plane) {
        auto planeData = info.data + GST_VIDEO_INFO_PLANE_OFFSET(videoInfo, plane);
        auto oLineSize = GST_VIDEO_INFO_PLANE_STRIDE(videoInfo, plane);
        auto lineSize = qMin<size_t>(packet.lineSize(plane), oLineSize);
        auto heightDiv = packet.heightDiv(plane);

        for (int y = 0; y < videoInfo->height; ++y) {
            auto ys = y >> heightDiv;
            memcpy(planeData + ys * oLineSize,
                   videoPacket.constLine(plane, y),
                   lineSize);
        }
    }

    gst_video_info_free(videoInfo);
    gst_buffer_unmap(buffer, &info);
    gst_caps_unref(inputCaps);
    gst_caps_unref(sourceCaps);

    auto pts = qint64(videoPacket.pts()
                      * videoPacket.timeBase().value()
                      * GST_SECOND);

#if 0
    GST_BUFFER_PTS(buffer) = GST_BUFFER_DTS(buffer) = this->m_streamParams[streamIndex].nextPts(pts, packet.id());
    GST_BUFFER_DURATION(buffer) = GST_SECOND / packet.caps().fps().value();
    GST_BUFFER_OFFSET(buffer) = this->m_streamParams[streamIndex].nFrame();
#else
    GST_BUFFER_PTS(buffer) = this->m_streamParams[streamIndex].nextPts(pts, videoPacket.id());
    GST_BUFFER_DTS(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_OFFSET(buffer) = GST_BUFFER_OFFSET_NONE;
#endif

    this->m_streamParams[streamIndex].nFrame()++;

    if (gst_app_src_push_buffer(GST_APP_SRC(source), buffer) != GST_FLOW_OK)
        qWarning() << "Error pushing buffer to GStreamer pipeline";
}

void MediaWriterGStreamerPrivate::writeSubtitlePacket(const AkSubtitlePacket &packet)
{
    Q_UNUSED(packet)
}

#include "moc_mediawritergstreamer.cpp"

/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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
#include <QFileInfo>
#include <akutils.h>

#include "mediawritergstreamer.h"

#define MINIMUM_PLUGIN_RANK GST_RANK_PRIMARY

typedef QMap<QString, QString> StringStringMap;

inline StringStringMap initGstToFF()
{
    StringStringMap gstToFF = {
        // Audio
        {"S8"      , "s8"     },
        {"U8"      , "u8"     },
        {"S16LE"   , "s16le"  },
        {"S16BE"   , "s16be"  },
        {"U16LE"   , "u16le"  },
        {"U16BE"   , "u16be"  },
        {"S24_32LE", "s2432le"},
        {"S24_32BE", "s2432be"},
        {"U24_32LE", "u2432le"},
        {"U24_32BE", "u2432be"},
        {"S32LE"   , "s32le"  },
        {"S32BE"   , "s32be"  },
        {"U32LE"   , "u32le"  },
        {"U32BE"   , "u32be"  },
        {"S24LE"   , "s24le"  },
        {"S24BE"   , "s24be"  },
        {"U24LE"   , "u24le"  },
        {"U24BE"   , "u24be"  },
        {"S20LE"   , "s20le"  },
        {"S20BE"   , "s20be"  },
        {"U20LE"   , "u20le"  },
        {"U20BE"   , "u20be"  },
        {"S18LE"   , "s18le"  },
        {"S18BE"   , "s18be"  },
        {"U18LE"   , "u18le"  },
        {"U18BE"   , "u18le"  },
        {"F32LE"   , "fltle"  },
        {"F32BE"   , "fltbe"  },
        {"F64LE"   , "dblle"  },
        {"F64BE"   , "dblbe"  },
        {"S16"     , "s16"    },
        {"U16"     , "u16"    },
        {"S24_32"  , "s2432"  },
        {"U24_32"  , "u2432"  },
        {"S32"     , "s32"    },
        {"U32"     , "u32"    },
        {"S24"     , "s24"    },
        {"U24"     , "u24"    },
        {"S20"     , "s20"    },
        {"U20"     , "u20"    },
        {"S18"     , "s18"    },
        {"U18"     , "u18"    },
        {"F32"     , "flt"    },
        {"F64"     , "dbl"    },

        // Video
        {"I420", "yuv420p"},
        //{"YV12", ""},
        {"YUY2", "yuyv422"},
        {"UYVY", "uyvy422"},
        //{"AYUV", ""},
        {"RGBx", "rgb0"   },
        {"BGRx", "bgr0"   },
        {"xRGB", "0rgb"   },
        {"xBGR", "0bgr"   },
        {"RGBA", "rgba"   },
        {"BGRA", "bgra"   },
        {"ARGB", "argb"   },
        {"ABGR", "abgr"   },
        {"RGB" , "rgb24"  },
        {"BGR" , "bgr24"  },
        {"Y41B", "yuv411p"},
        {"Y42B", "yuv422p"},
        //{"YVYU", ""},
        {"Y444"     , "yuv444p" },
        {"v210"     , "v210"    },
        {"v216"     , "v216"    },
        {"NV12"     , "nv12"    },
        {"NV21"     , "nv21"    },
        {"GRAY8"    , "gray8"   },
        {"GRAY16_BE", "gray16be"},
        {"GRAY16_LE", "gray16le"},
        {"V308"     , "v308"    },
        {"RGB16"    , "rgb565"  },
        {"BGR16"    , "bgr565le"},
        {"RGB15"    , "rgb555"  },
        {"BGR15"    , "rgb555le"},
        //{"UYVP", ""},
        {"A420" , "yuva420p"},
        {"RGB8P", "pal8"    },
        {"YUV9" , "yuv410p" },
        //{"YVU9"  , ""},
        //{"IYU1"  , ""},
        //{"ARGB64", ""},
        {"AYUV64", "ayuv64le"},
        //{"r210", ""},
        {"I420_10BE", "yuv420p10be"},
        {"I420_10LE", "yuv420p10le"},
        {"I422_10BE", "yuv422p10be"},
        {"I422_10LE", "yuv422p10le"},
        {"Y444_10BE", "yuv444p10be"},
        {"Y444_10LE", "yuv444p10le"},
        {"GBR"      , "gbrp"       },
        {"GBR_10BE" , "gbrp10be"   },
        {"GBR_10LE" , "gbrp10le"   },
        {"NV16"     , "nv16"       },
        //{"NV24"      , ""},
        //{"NV12_64Z32", ""},
        {"A420_10BE", "yuva420p10be"},
        {"A420_10LE", "yuva420p10le"},
        {"A422_10BE", "yuva422p10be"},
        {"A422_10LE", "yuva422p10le"},
        {"A444_10BE", "yuva444p10be"},
        {"A444_10LE", "yuva444p10le"},
        //{"NV61", ""},
    };

    return gstToFF;
}

Q_GLOBAL_STATIC_WITH_ARGS(StringStringMap, gstToFF, (initGstToFF()))

typedef QVector<AkVideoCaps> VectorVideoCaps;

inline VectorVideoCaps initDVSupportedCaps()
{
    QStringList supportedCaps = {
        // Digital Video doesn't support height > 576 yet.
        /*"video/x-raw,format=yuv422p,width=1440,height=1080,fps=25/1",
        "video/x-raw,format=yuv422p,width=1280,height=1080,fps=30000/1001",
        "video/x-raw,format=yuv422p,width=960,height=720,fps=60000/1001",
        "video/x-raw,format=yuv422p,width=960,height=720,fps=50/1",*/
        "video/x-raw,format=yuv422p,width=720,height=576,fps=25/1",
        "video/x-raw,format=yuv420p,width=720,height=576,fps=25/1",
        "video/x-raw,format=yuv411p,width=720,height=576,fps=25/1",
        "video/x-raw,format=yuv422p,width=720,height=480,fps=30000/1001",
        "video/x-raw,format=yuv411p,width=720,height=480,fps=30000/1001"
    };

    VectorVideoCaps dvSupportedCaps(supportedCaps.size());

    for (int i = 0; i < dvSupportedCaps.size(); i++)
        dvSupportedCaps[i] = supportedCaps[i];

    return dvSupportedCaps;
}

Q_GLOBAL_STATIC_WITH_ARGS(VectorVideoCaps, dvSupportedCaps, (initDVSupportedCaps()))

typedef QVector<QSize> VectorSize;

inline VectorSize initH263SupportedSize()
{
    QList<QSize> supportedSize = {
        QSize(1408, 1152),
        QSize(704, 576),
        QSize(352, 288),
        QSize(176, 144),
        QSize(128, 96)
    };

    return supportedSize.toVector();
}

Q_GLOBAL_STATIC_WITH_ARGS(VectorSize, h263SupportedSize, (initH263SupportedSize()))

typedef QVector<int> VectorInt;
typedef QMap<QString, VectorInt> StringVectorIntMap;

inline StringVectorIntMap initFLVSupportedSampleRates()
{
    StringVectorIntMap flvSupportedSampleRates = {
        {"avenc_adpcm_swf" , {5512, 11025, 22050, 44100              }},
        {"lamemp3enc"      , {5512, 8000 , 11025, 22050, 44100       }},
        {"faac"            , {                                       }},
        {"avenc_nellymoser", {5512, 8000 , 11025, 16000, 22050, 44100}},
        {"identity"        , {5512, 11025, 22050, 44100              }},
        {"alawenc"         , {5512, 11025, 22050, 44100              }},
        {"mulawenc"        , {5512, 11025, 22050, 44100              }},
        {"speexenc"        , {16000                                  }}
    };

    return flvSupportedSampleRates;
}

Q_GLOBAL_STATIC_WITH_ARGS(StringVectorIntMap, flvSupportedSampleRates, (initFLVSupportedSampleRates()))

typedef QMap<GType, QString> OptionTypeStrMap;

inline OptionTypeStrMap initOptionTypeStrMap()
{
    static const OptionTypeStrMap optionTypeStrMap = {
        {G_TYPE_STRING          , "string"  },
        {G_TYPE_BOOLEAN         , "boolean" },
        {G_TYPE_ULONG           , "integer" },
        {G_TYPE_LONG            , "integer" },
        {G_TYPE_UINT            , "integer" },
        {G_TYPE_INT             , "integer" },
        {G_TYPE_UINT64          , "integer" },
        {G_TYPE_INT64           , "integer" },
        {G_TYPE_FLOAT           , "float"   },
        {G_TYPE_DOUBLE          , "float"   },
        {G_TYPE_CHAR            , "integer" },
        {G_TYPE_UCHAR           , "integer" },
        {G_TYPE_PARAM_ENUM      , "menu"    },
        {G_TYPE_PARAM_FLAGS     , "flags"   },
        {G_TYPE_VALUE_ARRAY     , "array"   },
        {GST_TYPE_CAPS          , "caps"    },
        {GST_TYPE_PARAM_FRACTION, "frac"    },
    };

    return optionTypeStrMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(OptionTypeStrMap, codecOptionTypeToStr, (initOptionTypeStrMap()))

MediaWriterGStreamer::MediaWriterGStreamer(QObject *parent):
    MediaWriter(parent)
{
//    setenv("GST_DEBUG", "2", 1);
    gst_init(NULL, NULL);

    this->m_isRecording = false;
    this->m_pipeline = NULL;
    this->m_mainLoop = NULL;
    this->m_busWatchId = 0;

    QObject::connect(this,
                     &MediaWriterGStreamer::outputFormatChanged,
                     this,
                     &MediaWriterGStreamer::updateStreams);

    codecOptions("webmmux");
}

MediaWriterGStreamer::~MediaWriterGStreamer()
{
    this->uninit();
}

QString MediaWriterGStreamer::location() const
{
    return this->m_location;
}

QString MediaWriterGStreamer::outputFormat() const
{
    return this->m_outputFormat;
}

QVariantList MediaWriterGStreamer::streams() const
{
    QVariantList streams;

    for (const QVariantMap &stream: this->m_streamConfigs)
        streams << stream;

    return streams;
}

QStringList MediaWriterGStreamer::supportedFormats()
{
    QStringList supportedFormats;
    auto factoryList =
            gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_MUXER,
                                                  MINIMUM_PLUGIN_RANK);

    for (GList *featureItem = factoryList; featureItem; featureItem = g_list_next(featureItem)) {
        if (G_UNLIKELY (featureItem->data == NULL))
          continue;

        auto factory = GST_ELEMENT_FACTORY(featureItem->data);

        if (!supportedFormats.contains(GST_OBJECT_NAME(factory)))
            supportedFormats << GST_OBJECT_NAME(factory);
    }

    gst_plugin_list_free(factoryList);

    static const QStringList unsupportedFormats = {
        "avmux_3gp",
        "avmux_aiff",
        "avmux_asf",
        "avmux_avi",
        "avmux_flv",
        "avmux_gxf",
        "avmux_mov",
        "avmux_mpegts",
        "avmux_mp4",
        "avmux_mxf",
        "avmux_mxf_d10"
    };

    // Disable conflictive formats
    for (const QString &format: unsupportedFormats)
        supportedFormats.removeAll(format);

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

    QStringList supportedCaps = this->readCaps(format);
    QStringList extensions;

    for (const QString &formatCaps: supportedCaps) {
        auto caps = gst_caps_from_string(formatCaps.toStdString().c_str());
        caps = gst_caps_fixate(caps);
        auto prof = gst_encoding_container_profile_new(NULL, NULL, caps, NULL);
        gst_caps_unref(caps);

        const gchar *extension =
                gst_encoding_profile_get_file_extension(reinterpret_cast<GstEncodingProfile *>(prof));

        if (extension && !extensions.contains(extension))
            extensions << extension;

        gst_encoding_profile_unref(prof);
    }

    return extensions;
}

QString MediaWriterGStreamer::formatDescription(const QString &format)
{
    auto factory = gst_element_factory_find(format.toStdString().c_str());

    if (!factory)
        return QString();

    factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory)));

    if (!factory)
        return QString();

    auto longName =
            gst_element_factory_get_metadata(factory,
                                             GST_ELEMENT_METADATA_LONGNAME);
    QString description(longName);
    gst_object_unref(factory);

    return description;
}

QVariantList MediaWriterGStreamer::formatOptions(const QString &format)
{
    auto element = gst_element_factory_make(format.toStdString().c_str(),
                                            NULL);

    if (!element)
        return QVariantList();

    auto codecOptions = this->parseOptions(element);
    gst_object_unref(element);

    return codecOptions;
}

QStringList MediaWriterGStreamer::supportedCodecs(const QString &format)
{
    return this->supportedCodecs(format, "");
}

QStringList MediaWriterGStreamer::supportedCodecs(const QString &format,
                                                  const QString &type)
{
    auto factory = gst_element_factory_find(format.toStdString().c_str());

    if (!factory)
        return QStringList();

    factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory)));

    if (!factory)
        return QStringList();

    static GstStaticCaps staticRawCaps =
            GST_STATIC_CAPS("video/x-raw;"
                            "audio/x-raw;"
                            "text/x-raw;"
                            "subpicture/x-dvd;"
                            "subpicture/x-pgs");

    GstCaps *rawCaps = gst_static_caps_get(&staticRawCaps);
    GList *encodersList =
            gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_ENCODER,
                                                  MINIMUM_PLUGIN_RANK);

    const GList *pads = gst_element_factory_get_static_pad_templates(factory);
    QStringList supportedCodecs;

    for (const GList *padItem = pads; padItem; padItem = g_list_next(padItem)) {
        auto padtemplate =
                reinterpret_cast<GstStaticPadTemplate *>(padItem->data);

        if (padtemplate->direction == GST_PAD_SINK) {
            auto caps = gst_caps_from_string(padtemplate->static_caps.string);

            for (guint i = 0; i < gst_caps_get_size(caps); i++) {
                GstStructure *capsStructure = gst_caps_get_structure(caps, i);
                auto structureName = gst_structure_get_name(capsStructure);
                QString structureType(structureName);
                gchar *structureStr = gst_structure_to_string(capsStructure);
                GstCaps *compCaps = gst_caps_from_string(structureStr);

                if (gst_caps_can_intersect(compCaps, rawCaps)) {
                    if (!type.isEmpty() && structureType != type) {
                        gst_caps_unref(compCaps);
                        g_free(structureStr);

                        continue;
                    }

                    QString codecType = structureType.mid(0, type.indexOf('/'));

                    if (gst_structure_has_field(capsStructure, "format")) {
                        GType fieldType = gst_structure_get_field_type(capsStructure, "format");

                        if (fieldType == G_TYPE_STRING) {
                            const gchar *format = gst_structure_get_string(capsStructure, "format");
                            QString codecId = QString("identity/%1/%2").arg(codecType).arg(format);

                            if (!supportedCodecs.contains(codecId))
                                supportedCodecs << codecId;
                        } else if (fieldType == GST_TYPE_LIST) {
                            const GValue *formats = gst_structure_get_value(capsStructure, "format");

                            for (guint i = 0; i < gst_value_list_get_size(formats); i++) {
                                const GValue *format = gst_value_list_get_value(formats, i);
                                auto codecId =
                                        QString("identity/%1/%2")
                                            .arg(codecType)
                                            .arg(g_value_get_string(format));

                                if (!supportedCodecs.contains(codecId))
                                    supportedCodecs << codecId;
                            }
                        }
                    }
                } else {
                    GList *encoders =
                            gst_element_factory_list_filter(encodersList,
                                                            caps,
                                                            GST_PAD_SRC,
                                                            FALSE);

                    for (GList *encoderItem = encoders;
                         encoderItem;
                         encoderItem = g_list_next(encoderItem)) {
                        auto encoder =
                                reinterpret_cast<GstElementFactory *>(encoderItem->data);

                        auto klass =
                                gst_element_factory_get_metadata(encoder,
                                                                 GST_ELEMENT_METADATA_KLASS);
                        QString codecType = !strcmp(klass, "Codec/Encoder/Audio")?
                                                "audio/x-raw":
                                            (strcmp(klass, "Codec/Encoder/Video")
                                             || strcmp(klass, "Codec/Encoder/Image"))?
                                                 "video/x-raw": "";

                        if (!type.isEmpty() && type != codecType)
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
    gst_object_unref(factory);

    // Disable conflictive codecs
    static const QMap<QString, QStringList> unsupportedCodecs = {
        {"mp4mux"     , {"schroenc"              }},
        {"flvmux"     , {"lamemp3enc"            }},
        {"avmux_3gp"  , {"avenc_h263p"           }},
        {"avmux_3g2"  , {"avenc_h263p"           }},
        {"matroskamux", {"avenc_tta"             }},
        {"*"          , {"avenc_alac", "mpeg2enc"}},
    };

    for (const QString &codec: unsupportedCodecs.value(format)
                             + unsupportedCodecs["*"])
        supportedCodecs.removeAll(codec);

    return supportedCodecs;
}

QString MediaWriterGStreamer::defaultCodec(const QString &format,
                                           const QString &type)
{
    QStringList codecs = this->supportedCodecs(format, type);

    if (codecs.isEmpty())
        return QString();

    return codecs.first();
}

QString MediaWriterGStreamer::codecDescription(const QString &codec)
{
    if (codec.startsWith("identity/")) {
        QStringList parts = codec.split("/");

        return QString("%1 (%2)").arg(parts[0]).arg(parts[2]);
    }

    auto factory = gst_element_factory_find(codec.toStdString().c_str());

    if (!factory)
        return QString();

    factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory)));

    if (!factory)
        return QString();

    auto longName =
            gst_element_factory_get_metadata(factory,
                                             GST_ELEMENT_METADATA_LONGNAME);
    QString description(longName);
    gst_object_unref(factory);

    return description;
}

QString MediaWriterGStreamer::codecType(const QString &codec)
{
    if (codec.startsWith("identity/audio"))
        return QString("audio/x-raw");
    else if (codec.startsWith("identity/video"))
        return QString("video/x-raw");
    else if (codec.startsWith("identity/text"))
        return QString("text/x-raw");

    auto factory = gst_element_factory_find(codec.toStdString().c_str());

    if (!factory)
        return QString();

    factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory)));

    if (!factory)
        return QString();

    const gchar *klass = gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS);
    QString codecType = !strcmp(klass, "Codec/Encoder/Audio")?
                            "audio/x-raw":
                        (strcmp(klass, "Codec/Encoder/Video")
                         || strcmp(klass, "Codec/Encoder/Image"))?
                             "video/x-raw": "";

    gst_object_unref(factory);

    return codecType;
}

QVariantMap MediaWriterGStreamer::defaultCodecParams(const QString &codec)
{
    QVariantMap codecParams;
    QString codecType = this->codecType(codec);

    static GstStaticCaps staticRawCaps = GST_STATIC_CAPS("video/x-raw;"
                                                         "audio/x-raw;"
                                                         "text/x-raw;"
                                                         "subpicture/x-dvd;"
                                                         "subpicture/x-pgs");

    GstCaps *rawCaps = gst_static_caps_get(&staticRawCaps);

    if (codecType == "audio/x-raw") {
        if (codec.startsWith("identity/audio")) {
            QString sampleFormat = gstToFF->value(codec.split("/").at(2), "s16");
            codecParams["defaultBitRate"] = 128000;
            codecParams["supportedSampleFormats"] = QStringList {sampleFormat};
            codecParams["supportedChannelLayouts"] = QStringList {"mono", "stereo"};
            codecParams["supportedSampleRates"] = QVariantList();
            codecParams["defaultSampleFormat"] = sampleFormat;
            codecParams["defaultChannelLayout"] = "stereo";
            codecParams["defaultChannels"] = 2;
            codecParams["defaultSampleRate"] = 44100;
        } else {
            auto factory =
                    gst_element_factory_find(codec.toStdString().c_str());

            if (!factory) {
                gst_caps_unref(rawCaps);

                return QVariantMap();
            }

            factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory)));

            if (!factory) {
                gst_object_unref(factory);
                gst_caps_unref(rawCaps);

                return QVariantMap();
            }

            QStringList supportedSampleFormats;
            QVariantList supportedSamplerates;
            QStringList supportedChannelLayouts;

            auto pads = gst_element_factory_get_static_pad_templates(factory);

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
                                    const gchar *format = gst_structure_get_string(capsStructure, "format");
                                    QString formatFF = gstToFF->value(format, "");

                                    if (!formatFF.isEmpty() && !supportedSampleFormats.contains(formatFF))
                                        supportedSampleFormats << formatFF;
                                } else if (fieldType == GST_TYPE_LIST) {
                                    const GValue *formats = gst_structure_get_value(capsStructure, "format");

                                    for (guint i = 0; i < gst_value_list_get_size(formats); i++) {
                                        auto format = gst_value_list_get_value(formats, i);
                                        auto formatId = g_value_get_string(format);
                                        QString formatFF = gstToFF->value(formatId, "");

                                        if (!formatFF.isEmpty() && !supportedSampleFormats.contains(formatFF))
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
                                    QString layout = AkAudioCaps::defaultChannelLayoutString(channels);

                                    if (!supportedChannelLayouts.contains(layout))
                                        supportedChannelLayouts << layout;
                                } else if (fieldType == GST_TYPE_INT_RANGE) {
                                    const GValue *channels = gst_structure_get_value(capsStructure, "channels");

                                    int min = gst_value_get_int_range_min(channels);
                                    int max = gst_value_get_int_range_max(channels) + 1;
                                    int step = gst_value_get_int_range_step(channels);

                                    for (int i = min; i < max; i += step) {
                                        QString layout = AkAudioCaps::defaultChannelLayoutString(i);

                                        if (!supportedChannelLayouts.contains(layout))
                                            supportedChannelLayouts << layout;
                                    }
                                } else if (fieldType == GST_TYPE_LIST) {
                                    const GValue *channels = gst_structure_get_value(capsStructure, "channels");

                                    for (guint i = 0; i < gst_value_list_get_size(channels); i++) {
                                        const GValue *nchannels = gst_value_list_get_value(channels, i);
                                        gint nchannelsId = g_value_get_int(nchannels);
                                        QString layout = AkAudioCaps::defaultChannelLayoutString(nchannelsId);

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

            GstElement *element = gst_element_factory_create(factory, NULL);

            if (!element) {
                gst_object_unref(factory);
                gst_caps_unref(rawCaps);

                return QVariantMap();
            }

            int bitrate = 0;

            if (g_object_class_find_property(G_OBJECT_GET_CLASS(element), "bitrate"))
                g_object_get(G_OBJECT(element), "bitrate", &bitrate, NULL);

            if (codec == "lamemp3enc")
                bitrate *= 1000;

            if (bitrate < 1)
                bitrate = 128000;

            codecParams["defaultBitRate"] = bitrate;
            codecParams["supportedSampleFormats"] = supportedSampleFormats;
            codecParams["supportedChannelLayouts"] = supportedChannelLayouts;
            codecParams["supportedSampleRates"] = supportedSamplerates;

            codecParams["defaultSampleFormat"] = supportedSampleFormats.isEmpty()?
                                                     QString("s16"): supportedSampleFormats.at(0);

            QString channelLayout = supportedChannelLayouts.isEmpty()?
                                        QString("stereo"): supportedChannelLayouts.at(0);
            codecParams["defaultChannelLayout"] = channelLayout;
            codecParams["defaultChannels"] = AkAudioCaps::channelCount(channelLayout);
            codecParams["defaultSampleRate"] = supportedSamplerates.isEmpty()?
                                                 44100: supportedSamplerates.at(0);

            gst_object_unref(element);
            gst_object_unref(factory);
        }
    } else if (codecType == "video/x-raw") {
        if (codec.startsWith("identity/video")) {
            QString pixelFormat = gstToFF->value(codec.split("/").at(2), "yuv420p");
            codecParams["defaultBitRate"] = 200000;
            codecParams["defaultGOP"] = 12;
            codecParams["supportedFrameRates"] = QVariantList();
            codecParams["supportedPixelFormats"] = QStringList() << pixelFormat;
            codecParams["defaultPixelFormat"] = pixelFormat;
        } else {
            GstElementFactory *factory = gst_element_factory_find(codec.toStdString().c_str());

            if (!factory) {
                gst_caps_unref(rawCaps);

                return QVariantMap();
            }

            factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory)));

            if (!factory) {
                gst_object_unref(factory);
                gst_caps_unref(rawCaps);

                return QVariantMap();
            }

            QStringList supportedPixelFormats;
            QVariantList supportedFramerates;

            auto pads = gst_element_factory_get_static_pad_templates(factory);

            for (auto padItem = pads; padItem; padItem = g_list_next(padItem)) {
                auto padtemplate =
                        reinterpret_cast<GstStaticPadTemplate *>(padItem->data);

                if (padtemplate->direction == GST_PAD_SINK
                    && padtemplate->presence == GST_PAD_ALWAYS) {
                    GstCaps *caps = gst_caps_from_string(padtemplate->static_caps.string);

                    for (guint i = 0; i < gst_caps_get_size(caps); i++) {
                        GstStructure *capsStructure = gst_caps_get_structure(caps, i);
                        gchar *structureStr = gst_structure_to_string(capsStructure);
                        GstCaps *compCaps = gst_caps_from_string(structureStr);

                        if (gst_caps_can_intersect(compCaps, rawCaps)) {
                            // Get supported formats
                            if (gst_structure_has_field(capsStructure, "format")) {
                                GType fieldType = gst_structure_get_field_type(capsStructure, "format");

                                if (fieldType == G_TYPE_STRING) {
                                    const gchar *format = gst_structure_get_string(capsStructure, "format");
                                    QString formatFF = gstToFF->value(format, "");

                                    if (!formatFF.isEmpty() && !supportedPixelFormats.contains(formatFF))
                                        supportedPixelFormats << formatFF;
                                } else if (fieldType == GST_TYPE_LIST) {
                                    const GValue *formats = gst_structure_get_value(capsStructure, "format");

                                    for (guint i = 0; i < gst_value_list_get_size(formats); i++) {
                                        const GValue *format = gst_value_list_get_value(formats, i);
                                        const gchar *formatId = g_value_get_string(format);
                                        QString formatFF = gstToFF->value(formatId, "");

                                        if (!formatFF.isEmpty() && !supportedPixelFormats.contains(formatFF))
                                            supportedPixelFormats << formatFF;
                                    }
                                }
                            }

                            // Get supported frame rates
                            if (gst_structure_has_field(capsStructure, "framerate")) {
                                GType fieldType = gst_structure_get_field_type(capsStructure, "framerate");

                                if (fieldType == GST_TYPE_FRACTION_RANGE) {
                                } else if (fieldType == GST_TYPE_LIST) {
                                    const GValue *framerates = gst_structure_get_value(capsStructure, "framerate");

                                    for (guint i = 0; i < gst_value_list_get_size(framerates); i++) {
                                        const GValue *frate = gst_value_list_get_value(framerates, i);
                                        gint num = gst_value_get_fraction_numerator(frate);
                                        gint den = gst_value_get_fraction_denominator(frate);
                                        AkFrac framerate(num, den);
                                        QVariant fps = QVariant::fromValue(framerate);

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
                                   QVariant fps = QVariant::fromValue(framerate);

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

            GstElement *element = gst_element_factory_create(factory, NULL);

            if (!element) {
                gst_object_unref(factory);
                gst_caps_unref(rawCaps);

                return QVariantMap();
            }

            int bitrate = 0;

            if (g_object_class_find_property(G_OBJECT_GET_CLASS(element), "bitrate"))
                g_object_get(G_OBJECT(element), "bitrate", &bitrate, NULL);

            if (codec == "x264enc"
                || codec == "x265enc"
                || codec == "mpeg2enc"
                || codec == "theoraenc")
                bitrate *= 1000;

            if (bitrate < 1)
                bitrate = 200000;

            codecParams["defaultBitRate"] = bitrate;
            codecParams["defaultGOP"] = 12;
            codecParams["supportedFrameRates"] = supportedFramerates;
            codecParams["supportedPixelFormats"] = supportedPixelFormats;
            codecParams["defaultPixelFormat"] = supportedPixelFormats.isEmpty()?
                                                  "yuv420p": supportedPixelFormats.at(0);

            gst_object_unref(element);
            gst_object_unref(factory);
        }
    } else if (codecType == "text/x-raw") {
    }

    gst_caps_unref(rawCaps);

    return codecParams;
}

QVariantList MediaWriterGStreamer::codecOptions(const QString &codec)
{
    auto element = gst_element_factory_make(codec.toStdString().c_str(),
                                            NULL);

    if (!element)
        return QVariantList();

    auto codecOptions = this->parseOptions(element);
    gst_object_unref(element);

    return codecOptions;
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
            this->supportedFormats().contains(this->m_outputFormat)?
                this->m_outputFormat: guessFormat(this->m_location);

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

        AkAudioCaps audioCaps(streamCaps);
        QString sampleFormat = AkAudioCaps::sampleFormatToString(audioCaps.format());
        QStringList supportedSampleFormats = codecDefaults["supportedSampleFormats"].toStringList();

        if (!supportedSampleFormats.isEmpty() && !supportedSampleFormats.contains(sampleFormat)) {
            QString defaultSampleFormat = codecDefaults["defaultSampleFormat"].toString();
            audioCaps.format() = AkAudioCaps::sampleFormatFromString(defaultSampleFormat);
            audioCaps.bps() = AkAudioCaps::bitsPerSample(defaultSampleFormat);
        }

        auto supportedSampleRates = codecDefaults["supportedSampleRates"].toList();

        if (!supportedSampleRates.isEmpty()) {
            int sampleRate = 0;
            int maxDiff = std::numeric_limits<int>::max();

            for (const QVariant &rate: supportedSampleRates) {
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

        auto channelLayout = AkAudioCaps::channelLayoutToString(audioCaps.layout());
        auto supportedChannelLayouts = codecDefaults["supportedChannelLayouts"].toStringList();

        if (!supportedChannelLayouts.isEmpty() && !supportedChannelLayouts.contains(channelLayout)) {
            QString defaultChannelLayout = codecDefaults["defaultChannelLayout"].toString();
            audioCaps.layout() = AkAudioCaps::channelLayoutFromString(defaultChannelLayout);
            audioCaps.channels() = AkAudioCaps::channelCount(defaultChannelLayout);
        };

        if (outputFormat == "flvmux") {
            audioCaps = this->nearestFLVAudioCaps(audioCaps, codec);

            if (codec == "speexenc"
                || codec == "avenc_nellymoser")
                audioCaps.channels() = 1;
        } else if (outputFormat == "avmux_dv") {
            audioCaps.rate() = 48000;
        } else if (outputFormat == "avmux_gxf"
                 || outputFormat == "avmux_mxf"
                 || outputFormat == "avmux_mxf_d10") {
                    audioCaps.rate() = qBound(4000, audioCaps.rate(), 96000);
        } else if (codec == "avenc_tta") {
            audioCaps.rate() = qBound(8000, audioCaps.rate(), 96000);
        }

        outputParams["caps"] = QVariant::fromValue(audioCaps.toCaps());
        outputParams["timeBase"] = QVariant::fromValue(AkFrac(1, audioCaps.rate()));
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

        AkVideoCaps videoCaps(streamCaps);
        QString pixelFormat = AkVideoCaps::pixelFormatToString(videoCaps.format());
        QStringList supportedPixelFormats = codecDefaults["supportedPixelFormats"].toStringList();

        if (!supportedPixelFormats.isEmpty() && !supportedPixelFormats.contains(pixelFormat)) {
            QString defaultPixelFormat = codecDefaults["defaultPixelFormat"].toString();
            videoCaps.format() = AkVideoCaps::pixelFormatFromString(defaultPixelFormat);
            videoCaps.bpp() = AkVideoCaps::bitsPerPixel(videoCaps.format());
        }

        QVariantList supportedFrameRates = codecDefaults["supportedFrameRates"].toList();

        if (!supportedFrameRates.isEmpty()) {
            AkFrac frameRate;
            qreal maxDiff = std::numeric_limits<qreal>::max();

            for (const QVariant &rate: supportedFrameRates) {
                qreal diff = qAbs(videoCaps.fps().value() - rate.value<AkFrac>().value());

                if (diff < maxDiff) {
                    frameRate = rate.value<AkFrac>();

                    if (qIsNull(diff))
                        break;

                    maxDiff = diff;
                }
            }

            videoCaps.fps() = frameRate;
        }

        if (codec == "avenc_h263")
            videoCaps = this->nearestH263Caps(videoCaps);
        else if (codec == "avenc_dvvideo")
            videoCaps = this->nearestDVCaps(videoCaps);

        outputParams["caps"] = QVariant::fromValue(videoCaps.toCaps());
        outputParams["timeBase"] = QVariant::fromValue(videoCaps.fps().invert());
    } else if (streamCaps.mimeType() == "text/x-raw") {
        outputParams["caps"] = QVariant::fromValue(streamCaps);
    }

    this->m_streamConfigs << outputParams;
    this->streamsChanged(this->streams());

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

    if (this->supportedFormats().contains(this->m_outputFormat))
        outputFormat = this->m_outputFormat;
    else
        outputFormat = guessFormat(this->m_location);

    if (outputFormat.isEmpty())
        return QVariantMap();

    if (codecParams.contains("label"))
        this->m_streamConfigs[index]["label"] = codecParams["label"];

    AkCaps streamCaps = this->m_streamConfigs[index]["caps"].value<AkCaps>();
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
            AkAudioCaps audioCaps(streamCaps);
            QString sampleFormat = AkAudioCaps::sampleFormatToString(audioCaps.format());
            QStringList supportedSampleFormats = codecDefaults["supportedSampleFormats"].toStringList();

            if (!supportedSampleFormats.isEmpty()
                && !supportedSampleFormats.contains(sampleFormat)) {
                QString defaultSampleFormat = codecDefaults["defaultSampleFormat"].toString();
                audioCaps.format() = AkAudioCaps::sampleFormatFromString(defaultSampleFormat);
                audioCaps.bps() = AkAudioCaps::bitsPerSample(defaultSampleFormat);
            }

            QVariantList supportedSampleRates = codecDefaults["supportedSampleRates"].toList();

            if (!supportedSampleRates.isEmpty()) {
                int sampleRate = 0;
                int maxDiff = std::numeric_limits<int>::max();

                for (const QVariant &rate: supportedSampleRates) {
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

            QString channelLayout = AkAudioCaps::channelLayoutToString(audioCaps.layout());
            QStringList supportedChannelLayouts = codecDefaults["supportedChannelLayouts"].toStringList();

            if (!supportedChannelLayouts.isEmpty() && !supportedChannelLayouts.contains(channelLayout)) {
                QString defaultChannelLayout = codecDefaults["defaultChannelLayout"].toString();
                audioCaps.layout() = AkAudioCaps::channelLayoutFromString(defaultChannelLayout);
                audioCaps.channels() = AkAudioCaps::channelCount(defaultChannelLayout);
            }

            if (outputFormat == "flvmux") {
                audioCaps = this->nearestFLVAudioCaps(audioCaps, codec);

                if (codec == "speexenc"
                    || codec == "avenc_nellymoser")
                    audioCaps.channels() = 1;
            } else if (outputFormat == "avmux_dv") {
                audioCaps.rate() = 48000;
            } else if (outputFormat == "avmux_gxf"
                     || outputFormat == "avmux_mxf"
                     || outputFormat == "avmux_mxf_d10") {
                        audioCaps.rate() = qBound(4000, audioCaps.rate(), 96000);
            } else if (codec == "avenc_tta") {
                audioCaps.rate() = qBound(8000, audioCaps.rate(), 96000);
            }

            streamCaps = audioCaps.toCaps();
            this->m_streamConfigs[index]["timeBase"] = QVariant::fromValue(AkFrac(1, audioCaps.rate()));
        } else if (streamCaps.mimeType() == "video/x-raw") {
            AkVideoCaps videoCaps(streamCaps);
            QString pixelFormat = AkVideoCaps::pixelFormatToString(videoCaps.format());
            QStringList supportedPixelFormats = codecDefaults["supportedPixelFormats"].toStringList();

            if (!supportedPixelFormats.isEmpty()
                && !supportedPixelFormats.contains(pixelFormat)) {
                QString defaultPixelFormat = codecDefaults["defaultPixelFormat"].toString();
                videoCaps.format() = AkVideoCaps::pixelFormatFromString(defaultPixelFormat);
                videoCaps.bpp() = AkVideoCaps::bitsPerPixel(videoCaps.format());
            }

            QVariantList supportedFrameRates = codecDefaults["supportedFrameRates"].toList();

            if (!supportedFrameRates.isEmpty()) {
                AkFrac frameRate;
                qreal maxDiff = std::numeric_limits<qreal>::max();

                for (const QVariant &rate: supportedFrameRates) {
                    qreal diff = qAbs(videoCaps.fps().value() - rate.value<AkFrac>().value());

                    if (diff < maxDiff) {
                        frameRate = rate.value<AkFrac>();

                        if (qIsNull(diff))
                            break;

                        maxDiff = diff;
                    }
                }

                videoCaps.fps() = frameRate;
            }

            if (codec == "avenc_h263")
                videoCaps = this->nearestH263Caps(videoCaps);
            else if (codec == "avenc_dvvideo")
                videoCaps = this->nearestDVCaps(videoCaps);

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

QString MediaWriterGStreamer::guessFormat(const QString &fileName)
{
    QString ext = QFileInfo(fileName).suffix();

    for (const QString &format: this->supportedFormats())
        if (this->fileExtensions(format).contains(ext))
            return format;

    return QString();
}

QStringList MediaWriterGStreamer::readCaps(const QString &element)
{
    auto factory = gst_element_factory_find(element.toStdString().c_str());

    if (!factory)
        return QStringList();

    factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory)));

    if (!factory)
        return QStringList();

    auto pads = gst_element_factory_get_static_pad_templates(factory);
    QStringList elementCaps;

    for (auto padItem = pads; padItem; padItem = g_list_next(padItem)) {
        GstStaticPadTemplate *padtemplate =
                reinterpret_cast<GstStaticPadTemplate *>(padItem->data);

        if (padtemplate->direction == GST_PAD_SRC
            && padtemplate->presence == GST_PAD_ALWAYS) {
            auto caps = gst_caps_from_string(padtemplate->static_caps.string);

            for (guint i = 0; i < gst_caps_get_size(caps); i++) {
                GstStructure *capsStructure = gst_caps_get_structure(caps, i);
                gchar *structureCaps = gst_structure_to_string(capsStructure);

                elementCaps << structureCaps;

                g_free(structureCaps);
            }

            gst_caps_unref(caps);
        }
    }

    gst_object_unref(factory);

    return elementCaps;
}

QVariantList MediaWriterGStreamer::parseOptions(const GstElement *element) const
{
    QVariantList options;
    guint nprops = 0;

    auto propSpecs =
            g_object_class_list_properties(G_OBJECT_GET_CLASS(element),
                                           &nprops);

    for (guint i = 0; i < nprops; i++) {
        GParamSpec *param = propSpecs[i];

        if (!(param->flags & G_PARAM_READWRITE))
            continue;

#if 0
        if (param->flags & G_PARAM_DEPRECATED)
            continue;
#endif

        auto name = g_param_spec_get_name(param);

        if (!strcmp(name, "name"))
            continue;

        QVariant defaultValue;
        QVariant value;
        qreal min = 0;
        qreal max = 0;
        qreal step = 0;
        QVariantMap menu;
        auto paramType = codecOptionTypeToStr->value(param->value_type);

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
                    value = g_value_get_enum(&gValue);
                    auto spec = G_PARAM_SPEC_ENUM(param);
                    defaultValue = spec->default_value;
                    auto value = G_ENUM_CLASS(g_type_class_ref(param->value_type))->values;

                    if (value)
                        for (; value->value_name; value++)
                            menu[value->value_nick] = value->value;

                    paramType = "menu";
                } else if (G_IS_PARAM_SPEC_FLAGS(param)) {
                    value = g_value_get_flags(&gValue);
                    auto spec = G_PARAM_SPEC_FLAGS(param);
                    defaultValue = spec->default_value;
                    auto value = spec->flags_class->values;

                    if (value)
                        for (; value->value_name; value++)
                            menu[value->value_nick] = value->value;

                    paramType = "flags";
                } else if (GST_IS_PARAM_SPEC_FRACTION(param)) {
                    auto num = gst_value_get_fraction_numerator(&gValue);
                    auto den = gst_value_get_fraction_denominator(&gValue);
                    value = QVariant::fromValue(AkFrac(num, den));
                    defaultValue = value;
                    paramType = "frac";
                } else if (param->value_type == GST_TYPE_CAPS) {
                    auto caps = gst_caps_to_string(gst_value_get_caps(&gValue));
                    value = QVariant::fromValue(AkCaps(caps));
                    g_free(caps);
                    defaultValue = value;
                    paramType = "caps";
                } else
                    continue;

                break;
        }

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

void MediaWriterGStreamer::waitState(GstState state)
{
    forever {
        GstState curState;
        auto ret = gst_element_get_state(this->m_pipeline,
                                         &curState,
                                         NULL,
                                         GST_CLOCK_TIME_NONE);

        if (ret == GST_STATE_CHANGE_FAILURE)
            break;

        if (ret == GST_STATE_CHANGE_SUCCESS
            && curState == state)
            break;
    }
}

gboolean MediaWriterGStreamer::busCallback(GstBus *bus,
                                GstMessage *message,
                                gpointer userData)
{
    Q_UNUSED(bus)
    auto self = static_cast<MediaWriterGStreamer *>(userData);

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        GError *err = NULL;
        gchar *debug = NULL;
        gst_message_parse_error(message, &err, &debug);

        qDebug() << "ERROR: from element"
                 << GST_MESSAGE_SRC_NAME(message)
                 << ":"
                 << err->message;

        if (debug)
            qDebug() << "Additional debug info:\n"
                     << debug;

        GstElement *element = GST_ELEMENT(GST_MESSAGE_SRC(message));

        for (const GList *padItem = GST_ELEMENT_PADS(element); padItem; padItem = g_list_next(padItem)) {
            GstPad *pad = GST_PAD_CAST(padItem->data);
            GstCaps *curCaps = gst_pad_get_current_caps(pad);
            gchar *curCapsStr = gst_caps_to_string(curCaps);

            qDebug() << "    Current caps:" << curCapsStr;

            g_free(curCapsStr);
            gst_caps_unref(curCaps);

            GstCaps *allCaps = gst_pad_get_allowed_caps(pad);
            gchar *allCapsStr = gst_caps_to_string(allCaps);

            qDebug() << "    Allowed caps:" << allCapsStr;

            g_free(allCapsStr);
            gst_caps_unref(allCaps);
        }

        g_error_free(err);
        g_free(debug);
        g_main_loop_quit(self->m_mainLoop);

        break;
    }
    case GST_MESSAGE_EOS:
        g_main_loop_quit(self->m_mainLoop);
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
        GstElement *owner = NULL;
        gst_message_parse_stream_status(message, &type, &owner);
        qDebug() << "Stream Status:"
                 << GST_ELEMENT_NAME(owner)
                 << "is"
                 << type;

        break;
    }
    case GST_MESSAGE_LATENCY: {
        qDebug() << "Recalculating latency";
        gst_bin_recalculate_latency(GST_BIN(self->m_pipeline));
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
        GstClock *clock = NULL;
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
        GstTagList *tagList = NULL;
        gst_message_parse_tag(message, &tagList);
        gchar *tags = gst_tag_list_to_string(tagList);
//        qDebug() << "Tags:" << tags;
        g_free(tags);
        gst_tag_list_unref(tagList);
        break;
    }
    case GST_MESSAGE_ELEMENT: {
        const GstStructure *messageStructure = gst_message_get_structure(message);
        gchar *structure = gst_structure_to_string(messageStructure);
//        qDebug() << structure;
        g_free(structure);
        break;
    }
    case GST_MESSAGE_QOS: {
        qDebug() << QString("Received QOS from element %1:")
                        .arg(GST_MESSAGE_SRC_NAME(message)).toStdString().c_str();

        GstFormat format;
        guint64 processed;
        guint64 dropped;
        gst_message_parse_qos_stats(message, &format, &processed, &dropped);
        const gchar *formatStr = gst_format_get_name(format);
        qDebug() << "    Processed" << processed << formatStr;
        qDebug() << "    Dropped" << dropped << formatStr;

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
        qDebug() << "    Is live stream =" << (live? true: false);
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

void MediaWriterGStreamer::setElementOptions(GstElement *element,
                                             const QVariantMap &options)
{
    for (const QString &key: options.keys()) {
        auto paramSpec =
                g_object_class_find_property(G_OBJECT_GET_CLASS(element),
                                             key.toStdString().c_str());

        if (!paramSpec)
            continue;

        GValue gValue;
        memset(&gValue, 0, sizeof(GValue));
        g_value_init(&gValue, paramSpec->value_type);
        QString value = options[key].toString();

        if (!gst_value_deserialize(&gValue, value.toStdString().c_str()))
            continue;

        g_object_set_property(G_OBJECT(element),
                              key.toStdString().c_str(),
                              &gValue);
    }
}

AkVideoCaps MediaWriterGStreamer::nearestDVCaps(const AkVideoCaps &caps) const
{
    AkVideoCaps nearestCaps;
    qreal q = std::numeric_limits<qreal>::max();

    for (const AkVideoCaps &sCaps: *dvSupportedCaps) {
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

AkVideoCaps MediaWriterGStreamer::nearestH263Caps(const AkVideoCaps &caps) const
{
    QSize nearestSize;
    qreal q = std::numeric_limits<qreal>::max();

    for (const QSize &size: *h263SupportedSize) {
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
    nearestCaps.width() = nearestSize.width();
    nearestCaps.height() = nearestSize.height();

    return nearestCaps;
}

AkAudioCaps MediaWriterGStreamer::nearestFLVAudioCaps(const AkAudioCaps &caps,
                                                      const QString &codec) const
{
    int nearestSampleRate = caps.rate();
    int q = std::numeric_limits<int>::max();

    for (const int &sampleRate: flvSupportedSampleRates->value(codec)) {
        int k = qAbs(sampleRate - caps.rate());

        if (k < q) {
            nearestSampleRate = sampleRate;
            q = k;

            if (k == 0)
                break;
        }
    }

    AkAudioCaps nearestCaps(caps);
    nearestCaps.rate() = nearestSampleRate;

    return nearestCaps;
}

void MediaWriterGStreamer::setLocation(const QString &location)
{
    if (this->m_location == location)
        return;

    this->m_location = location;
    emit this->locationChanged(location);
}

void MediaWriterGStreamer::setOutputFormat(const QString &outputFormat)
{
    if (this->m_outputFormat == outputFormat)
        return;

    this->m_outputFormat = outputFormat;
    emit this->outputFormatChanged(outputFormat);
}

void MediaWriterGStreamer::setFormatOptions(const QVariantMap &formatOptions)
{
    if (this->m_formatOptions == formatOptions)
        return;

    this->m_formatOptions = formatOptions;
    emit this->formatOptionsChanged(formatOptions);
}

void MediaWriterGStreamer::resetLocation()
{
    this->setLocation("");
}

void MediaWriterGStreamer::resetOutputFormat()
{
    this->setOutputFormat("");
}

void MediaWriterGStreamer::resetFormatOptions()
{
    this->setFormatOptions(QVariantMap());
}

void MediaWriterGStreamer::enqueuePacket(const AkPacket &packet)
{
    if (!this->m_isRecording)
        return;

    if (packet.caps().mimeType() == "audio/x-raw") {
        this->writeAudioPacket(AkAudioPacket(packet));
    } else if (packet.caps().mimeType() == "video/x-raw") {
        this->writeVideoPacket(AkVideoPacket(packet));
    } else if (packet.caps().mimeType() == "text/x-raw") {
        this->writeSubtitlePacket(packet);
    }
}

void MediaWriterGStreamer::clearStreams()
{
    this->m_streamConfigs.clear();
    this->streamsChanged(this->streams());
}

bool MediaWriterGStreamer::init()
{
    QString outputFormat = this->m_outputFormat.isEmpty()?
                               this->guessFormat(this->m_location):
                               this->m_outputFormat;

    this->m_pipeline = gst_pipeline_new(NULL);

    auto muxer = gst_element_factory_make(outputFormat.toStdString().c_str(),
                                          NULL);

    if (!muxer)
        return false;

    // Set format options.
    this->setElementOptions(muxer, this->m_formatOptions);

    GstElement *filesink = gst_element_factory_make("filesink", NULL);
    g_object_set(G_OBJECT(filesink), "location", this->m_location.toStdString().c_str(), NULL);

    gst_bin_add_many(GST_BIN(this->m_pipeline), muxer, filesink, NULL);
    gst_element_link_many(muxer, filesink, NULL);

    QVector<QVariantMap> streamConfigs = this->m_streamConfigs.toVector();

    for (int i = 0; i < streamConfigs.count(); i++) {
        QVariantMap configs = streamConfigs[i];
        AkCaps streamCaps = configs["caps"].value<AkCaps>();
        QString codec = configs["codec"].toString();

        if (codec.startsWith("identity/"))
            codec = "identity";

        if (streamCaps.mimeType() == "audio/x-raw") {
            QString sourceName = QString("audio_%1").arg(i);
            GstElement *source = gst_element_factory_make("appsrc", sourceName.toStdString().c_str());
            gst_app_src_set_stream_type(GST_APP_SRC(source), GST_APP_STREAM_TYPE_STREAM);
            g_object_set(G_OBJECT(source), "format", GST_FORMAT_TIME, NULL);

            AkAudioCaps audioCaps(streamCaps);

            if (outputFormat == "flvmux") {
                audioCaps = this->nearestFLVAudioCaps(audioCaps, codec);

                if (codec == "speexenc"
                    || codec == "avenc_nellymoser")
                    audioCaps.channels() = 1;
            } else if (outputFormat == "avmux_dv") {
                audioCaps.rate() = 48000;
            } else if (outputFormat == "avmux_gxf"
                     || outputFormat == "avmux_mxf"
                     || outputFormat == "avmux_mxf_d10") {
                        audioCaps.rate() = qBound(4000, audioCaps.rate(), 96000);
            } else if (codec == "avenc_tta") {
                audioCaps.rate() = qBound(8000, audioCaps.rate(), 96000);
            }

            auto format = AkAudioCaps::sampleFormatToString(audioCaps.format());
            auto gstFormat = gstToFF->key(format, "S16");

            auto gstAudioCaps =
                    gst_caps_new_simple("audio/x-raw",
                                        "format", G_TYPE_STRING, gstFormat.toStdString().c_str(),
                                        "layout", G_TYPE_STRING, "interleaved",
                                        "rate", G_TYPE_INT, audioCaps.rate(),
                                        "channels", G_TYPE_INT, audioCaps.channels(),
                                        NULL);

            gstAudioCaps = gst_caps_fixate(gstAudioCaps);
            gst_app_src_set_caps(GST_APP_SRC(source), gstAudioCaps);

            auto audioConvert = gst_element_factory_make("audioconvert", NULL);
            auto audioResample = gst_element_factory_make("audioresample", NULL);
            auto audioRate = gst_element_factory_make("audiorate", NULL);
            auto audioCodec = gst_element_factory_make(codec.toStdString().c_str(), NULL);

            if (codec.startsWith("avenc_"))
                g_object_set(G_OBJECT(audioCodec), "compliance", -2, NULL);

            // Set codec options.
#if 0 // NOTE: Disabled because GStreamer crash when setting an invalid bitrate.
            if (g_object_class_find_property(G_OBJECT_GET_CLASS(audioCodec),
                                             "bitrate")) {
                int bitrate = configs["bitrate"].toInt();

                if (codec == "lamemp3enc")
                    bitrate /= 1000;

                if (bitrate > 0)
                    g_object_set(G_OBJECT(audioCodec), "bitrate", G_TYPE_INT, bitrate, NULL);
            }
#endif
            QVariantMap codecOptions = configs.value("codecOptions").toMap();
            this->setElementOptions(audioCodec, codecOptions);

            GstElement *queue = gst_element_factory_make("queue", NULL);

            gst_bin_add_many(GST_BIN(this->m_pipeline),
                             source,
                             audioResample,
                             audioRate,
                             audioConvert,
                             audioCodec,
                             queue,
                             NULL);

            gst_element_link_many(source, audioResample, audioRate, audioConvert, NULL);
            gst_element_link_filtered(audioConvert, audioCodec, gstAudioCaps);
            gst_caps_unref(gstAudioCaps);
            gst_element_link_many(audioCodec, queue, muxer, NULL);
        } else if (streamCaps.mimeType() == "video/x-raw") {
            QString sourceName = QString("video_%1").arg(i);
            GstElement *source = gst_element_factory_make("appsrc", sourceName.toStdString().c_str());
            gst_app_src_set_stream_type(GST_APP_SRC(source), GST_APP_STREAM_TYPE_STREAM);
            g_object_set(G_OBJECT(source), "format", GST_FORMAT_TIME, NULL);

            AkVideoCaps videoCaps(streamCaps);

            if (codec == "avenc_h263")
                videoCaps = this->nearestH263Caps(videoCaps);
            else if (codec == "avenc_dvvideo")
                videoCaps = this->nearestDVCaps(videoCaps);

            auto format = AkVideoCaps::pixelFormatToString(videoCaps.format());
            auto gstFormat = gstToFF->key(format, "I420");

            auto gstVideoCaps =
                    gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, gstFormat.toStdString().c_str(),
                                        "width", G_TYPE_INT, videoCaps.width(),
                                        "height", G_TYPE_INT, videoCaps.height(),
                                        "framerate", GST_TYPE_FRACTION,
                                                     (int) videoCaps.fps().num(),
                                                     (int) videoCaps.fps().den(),
                                        NULL);

            gstVideoCaps = gst_caps_fixate(gstVideoCaps);
            gst_app_src_set_caps(GST_APP_SRC(source), gstVideoCaps);

            auto videoScale = gst_element_factory_make("videoscale", NULL);
            auto videoRate = gst_element_factory_make("videorate", NULL);
            auto videoConvert = gst_element_factory_make("videoconvert", NULL);
            auto videoCodec = gst_element_factory_make(codec.toStdString().c_str(), NULL);

            if (codec.startsWith("avenc_"))
                g_object_set(G_OBJECT(videoCodec), "compliance", -2, NULL);

            // Set codec options.
#if 0
            if (g_object_class_find_property(G_OBJECT_GET_CLASS(videoCodec),
                                             "bitrate")) {
                int bitrate = configs["bitrate"].toInt();

                if (codec == "x264enc"
                    || codec == "x265enc"
                    || codec == "mpeg2enc"
                    || codec == "theoraenc")
                    bitrate /= 1000;

                if (bitrate > 0)
                    g_object_set(G_OBJECT(videoCodec), "bitrate", G_TYPE_INT, bitrate, NULL);
            }
#endif
            QVariantMap codecOptions = configs.value("codecOptions").toMap();
            this->setElementOptions(videoCodec, codecOptions);

            GstElement *queue = gst_element_factory_make("queue", NULL);

            gst_bin_add_many(GST_BIN(this->m_pipeline),
                             source,
                             videoScale,
                             videoRate,
                             videoConvert,
                             videoCodec,
                             queue,
                             NULL);

            gst_element_link_many(source, videoScale, videoRate, videoConvert, NULL);
            gst_element_link_filtered(videoConvert, videoCodec, gstVideoCaps);
            gst_caps_unref(gstVideoCaps);
            gst_element_link_many(videoCodec, queue, muxer, NULL);
        }

        this->m_streamParams << OutputParams(configs["index"].toInt());
    }

    // Configure the message bus.
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->m_pipeline));
    this->m_busWatchId = gst_bus_add_watch(bus, this->busCallback, this);
    gst_object_unref(bus);

    // Run the main GStreamer loop.
    this->m_mainLoop = g_main_loop_new(NULL, FALSE);
    QtConcurrent::run(&this->m_threadPool, g_main_loop_run, this->m_mainLoop);
    gst_element_set_state(this->m_pipeline, GST_STATE_PLAYING);
    this->m_isRecording = true;

    return true;
}

void MediaWriterGStreamer::uninit()
{
    this->m_isRecording = false;
    this->m_streamParams.clear();

    if (this->m_pipeline) {
        GstIterator *sources = gst_bin_iterate_sources(GST_BIN(this->m_pipeline));
        GValue sourceItm = G_VALUE_INIT;
        gboolean done = FALSE;

        while (!done) {
            switch (gst_iterator_next(sources, &sourceItm)) {
            case GST_ITERATOR_OK: {
                    GstElement *source = GST_ELEMENT(g_value_get_object(&sourceItm));

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

        gst_element_send_event(this->m_pipeline, gst_event_new_eos());

        gst_element_set_state(this->m_pipeline, GST_STATE_NULL);
        this->waitState(GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(this->m_pipeline));
        g_source_remove(this->m_busWatchId);
        this->m_pipeline = NULL;
        this->m_busWatchId = 0;
    }

    if (this->m_mainLoop) {
        g_main_loop_quit(this->m_mainLoop);
        g_main_loop_unref(this->m_mainLoop);
        this->m_mainLoop = NULL;
    }
}

void MediaWriterGStreamer::writeAudioPacket(const AkAudioPacket &packet)
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

    QString souceName = QString("audio_%1").arg(streamIndex);
    GstElement *source = gst_bin_get_by_name(GST_BIN(this->m_pipeline),
                                             souceName.toStdString().c_str());
    GstCaps *sourceCaps = gst_app_src_get_caps(GST_APP_SRC(source));

    QString iFormat = AkAudioCaps::sampleFormatToString(packet.caps().format());
    iFormat = gstToFF->key(iFormat, "S16");

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    QString fEnd = "LE";
#elif Q_BYTE_ORDER == Q_BIG_ENDIAN
    QString fEnd = "BE";
#endif

    if (!iFormat.endsWith(fEnd))
        iFormat += fEnd;

    auto inputCaps =
            gst_caps_new_simple("audio/x-raw",
                                "format", G_TYPE_STRING, iFormat.toStdString().c_str(),
                                "layout", G_TYPE_STRING, "interleaved",
                                "rate", G_TYPE_INT, packet.caps().rate(),
                                "channels", G_TYPE_INT, packet.caps().channels(),
                                NULL);
    inputCaps = gst_caps_fixate(inputCaps);

    if (!gst_caps_is_equal(sourceCaps, inputCaps))
        gst_app_src_set_caps(GST_APP_SRC(source), inputCaps);

    gst_caps_unref(inputCaps);
    gst_caps_unref(sourceCaps);

    size_t size = size_t(packet.buffer().size());

    GstBuffer *buffer = gst_buffer_new_allocate(NULL, size, NULL);
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);
    memcpy(info.data, packet.buffer().constData(), size);
    gst_buffer_unmap(buffer, &info);

    qint64 pts = qint64(packet.pts() * packet.timeBase().value() * GST_SECOND);

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

    this->m_streamParams[streamIndex].nFrame() += quint64(packet.caps().samples());

    if (gst_app_src_push_buffer(GST_APP_SRC(source), buffer) != GST_FLOW_OK)
        qWarning() << "Error pushing buffer to GStreamer pipeline";
}

void MediaWriterGStreamer::writeVideoPacket(const AkVideoPacket &packet)
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

    AkVideoPacket videoPacket = AkUtils::roundSizeTo(packet.toPacket(), 4);
    videoPacket = AkUtils::convertVideo(videoPacket, AkVideoCaps::Format_rgb24);

    QString souceName = QString("video_%1").arg(streamIndex);
    GstElement *source = gst_bin_get_by_name(GST_BIN(this->m_pipeline),
                                             souceName.toStdString().c_str());
    GstCaps *sourceCaps = gst_app_src_get_caps(GST_APP_SRC(source));

    QString iFormat = AkVideoCaps::pixelFormatToString(videoPacket.caps().format());
    iFormat = gstToFF->key(iFormat, "BGR");
    auto inputCaps =
            gst_caps_new_simple("video/x-raw",
                                "format", G_TYPE_STRING, iFormat.toStdString().c_str(),
                                "width", G_TYPE_INT, videoPacket.caps().width(),
                                "height", G_TYPE_INT, videoPacket.caps().height(),
                                "framerate", GST_TYPE_FRACTION,
                                             int(videoPacket.caps().fps().num()),
                                             int(videoPacket.caps().fps().den()),
                                NULL);
    inputCaps = gst_caps_fixate(inputCaps);

    if (!gst_caps_is_equal(sourceCaps, inputCaps))
        gst_app_src_set_caps(GST_APP_SRC(source), inputCaps);

    gst_caps_unref(inputCaps);
    gst_caps_unref(sourceCaps);

    size_t size = size_t(videoPacket.buffer().size());

    GstBuffer *buffer = gst_buffer_new_allocate(NULL, size, NULL);
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);
    memcpy(info.data, videoPacket.buffer().constData(), size);
    gst_buffer_unmap(buffer, &info);

    qint64 pts = qint64(videoPacket.pts() * videoPacket.timeBase().value() * GST_SECOND);

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

void MediaWriterGStreamer::writeSubtitlePacket(const AkPacket &packet)
{
    Q_UNUSED(packet)
}

void MediaWriterGStreamer::updateStreams()
{
    QList<QVariantMap> streamConfigs = this->m_streamConfigs;
    this->clearStreams();

    for (const QVariantMap &configs: streamConfigs) {
        AkCaps caps = configs["caps"].value<AkCaps>();
        int index = configs["index"].toInt();
        this->addStream(index, caps, configs);
    }
}

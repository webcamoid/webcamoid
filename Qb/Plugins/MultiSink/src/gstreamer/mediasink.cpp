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
#include <QFileInfo>

#include "mediasink.h"

typedef QMap<QString, QString> StringStringMap;

inline StringStringMap initGstToFF()
{
    StringStringMap gstToFF;

    // Audio
    gstToFF["S8"] = "s8";
    gstToFF["U8"] = "u8";
    gstToFF["S16LE"] = "s16le";
    gstToFF["S16BE"] = "s16be";
    gstToFF["U16LE"] = "u16le";
    gstToFF["U16BE"] = "u16be";
    gstToFF["S24_32LE"] = "s2432le";
    gstToFF["S24_32BE"] = "s2432be";
    gstToFF["U24_32LE"] = "u2432le";
    gstToFF["U24_32BE"] = "u2432be";
    gstToFF["S32LE"] = "s32le";
    gstToFF["S32BE"] = "s32be";
    gstToFF["U32LE"] = "u32le";
    gstToFF["U32BE"] = "u32be";
    gstToFF["S24LE"] = "s24le";
    gstToFF["S24BE"] = "s24be";
    gstToFF["U24LE"] = "u24le";
    gstToFF["U24BE"] = "u24be";
    gstToFF["S20LE"] = "s20le";
    gstToFF["S20BE"] = "s20be";
    gstToFF["U20LE"] = "u20le";
    gstToFF["U20BE"] = "u20be";
    gstToFF["S18LE"] = "s18le";
    gstToFF["S18BE"] = "s18be";
    gstToFF["U18LE"] = "u18le";
    gstToFF["U18BE"] = "u18le";
    gstToFF["F32LE"] = "fltle";
    gstToFF["F32BE"] = "fltbe";
    gstToFF["F64LE"] = "dblle";
    gstToFF["F64BE"] = "dblbe";
    gstToFF["S16"] = "s16";
    gstToFF["U16"] = "u16";
    gstToFF["S24_32"] = "s2432";
    gstToFF["U24_32"] = "u2432";
    gstToFF["S32"] = "s32";
    gstToFF["U32"] = "u32";
    gstToFF["S24"] = "s24";
    gstToFF["U24"] = "u24";
    gstToFF["S20"] = "s20";
    gstToFF["U20"] = "u20";
    gstToFF["S18"] = "s18";
    gstToFF["U18"] = "u18";
    gstToFF["F32"] = "flt";
    gstToFF["F64"] = "dbl";

    // Video
    gstToFF["I420"] = "yuv420p";
//    gstToFF["YV12"] = "";
    gstToFF["YUY2"] = "yuyv422";
    gstToFF["UYVY"] = "uyvy422";
//    gstToFF["AYUV"] = "";
    gstToFF["RGBx"] = "rgb0";
    gstToFF["BGRx"] = "bgr0";
    gstToFF["xRGB"] = "0rgb";
    gstToFF["xBGR"] = "0bgr";
    gstToFF["RGBA"] = "rgba";
    gstToFF["BGRA"] = "bgra";
    gstToFF["ARGB"] = "argb";
    gstToFF["ABGR"] = "abgr";
    gstToFF["RGB"] = "rgb24";
    gstToFF["BGR"] = "bgr24";
    gstToFF["Y41B"] = "yuv411p";
    gstToFF["Y42B"] = "yuv422p";
//    gstToFF["YVYU"] = "";
    gstToFF["Y444"] = "yuv444p";
//    gstToFF["v210"] = "";
//    gstToFF["v216"] = "";
    gstToFF["NV12"] = "nv12";
    gstToFF["NV21"] = "nv21";
    gstToFF["GRAY8"] = "gray8";
    gstToFF["GRAY16_BE"] = "gray16be";
    gstToFF["GRAY16_LE"] = "gray16le";
//    gstToFF["v308"] = "";
    gstToFF["RGB16"] = "rgb565";
    gstToFF["BGR16"] = "bgr565le";
    gstToFF["RGB15"] = "rgb555";
    gstToFF["BGR15"] = "rgb555le";
//    gstToFF["UYVP"] = "";
    gstToFF["A420"] = "yuva420p";
    gstToFF["RGB8P"] = "pal8";
    gstToFF["YUV9"] = "yuv410p";
//    gstToFF["YVU9"] = "";
//    gstToFF["IYU1"] = "";
//    gstToFF["ARGB64"] = "";
    gstToFF["AYUV64"] = "ayuv64le";
//    gstToFF["r210"] = "";
    gstToFF["I420_10BE"] = "yuv420p10be";
    gstToFF["I420_10LE"] = "yuv420p10le";
    gstToFF["I422_10BE"] = "yuv422p10be";
    gstToFF["I422_10LE"] = "yuv422p10le";
    gstToFF["Y444_10BE"] = "yuv444p10be";
    gstToFF["Y444_10LE"] = "yuv444p10le";
    gstToFF["GBR"] = "gbrp";
    gstToFF["GBR_10BE"] = "gbrp10be";
    gstToFF["GBR_10LE"] = "gbrp10le";
    gstToFF["NV16"] = "nv16";
//    gstToFF["NV24"] = "";
//    gstToFF["NV12_64Z32"] = "";
    gstToFF["A420_10BE"] = "yuva420p10be";
    gstToFF["A420_10LE"] = "yuva420p10le";
    gstToFF["A422_10BE"] = "yuva422p10be";
    gstToFF["A422_10LE"] = "yuva422p10le";
    gstToFF["A444_10BE"] = "yuva444p10be";
    gstToFF["A444_10LE"] = "yuva444p10le";
//    gstToFF["NV61"] = "";

    return gstToFF;
}

Q_GLOBAL_STATIC_WITH_ARGS(StringStringMap, gstToFF, (initGstToFF()))

MediaSink::MediaSink(QObject *parent): QObject(parent)
{
    gst_init(NULL, NULL);

    this->m_pipeline = NULL;
    this->m_mainLoop = NULL;
    this->m_busWatchId = 0;

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
    QStringList supportedFormats;
    GList *plugins = gst_registry_get_plugin_list(gst_registry_get());

    for (GList *pluginItem = plugins; pluginItem; pluginItem = g_list_next(pluginItem)) {
        GstPlugin *plugin = (GstPlugin *) pluginItem->data;

        if (GST_OBJECT_FLAG_IS_SET(plugin, GST_PLUGIN_FLAG_BLACKLISTED))
          continue;

        const gchar *pluginName = gst_plugin_get_name(plugin);

        GList *features = gst_registry_get_feature_list_by_plugin(gst_registry_get(),
                                                                  pluginName);

        for (GList *featureItem = features; featureItem; featureItem = g_list_next(featureItem)) {
            if (G_UNLIKELY (featureItem->data == NULL))
              continue;

            GstPluginFeature *feature = GST_PLUGIN_FEATURE(featureItem->data);

            if (GST_IS_ELEMENT_FACTORY(feature)) {
                GstElementFactory *factory = GST_ELEMENT_FACTORY(feature);

                factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory)));

                if (!factory)
                    continue;

                const gchar *klass = gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS);

                if (!strcmp(klass, "Codec/Muxer"))
                    supportedFormats << GST_OBJECT_NAME(factory);

                gst_object_unref(factory);
            }
        }

        gst_plugin_list_free(features);
    }

    gst_plugin_list_free(plugins);

    return supportedFormats;
}

QStringList MediaSink::fileExtensions(const QString &format)
{
    GstElementFactory *factory = gst_element_factory_find(format.toStdString().c_str());

    if (!factory)
        return QStringList();

    factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory)));

    if (!factory)
        return QStringList();

    const GList *pads = gst_element_factory_get_static_pad_templates(factory);
    QStringList extensions;

    for (const GList *padItem = pads; padItem; padItem = g_list_next(padItem)) {
        GstStaticPadTemplate *padtemplate = (GstStaticPadTemplate *) padItem->data;

        if (padtemplate->direction == GST_PAD_SRC
            && padtemplate->presence == GST_PAD_ALWAYS) {
            GstCaps *caps = gst_caps_from_string(padtemplate->static_caps.string);
            GstEncodingContainerProfile *prof = gst_encoding_container_profile_new(NULL, NULL, caps, NULL);
            gst_caps_unref(caps);

            const gchar *extension = gst_encoding_profile_get_file_extension((GstEncodingProfile *) prof);

            if (extension && !extensions.contains(extension))
                extensions << extension;

            gst_encoding_profile_unref(prof);
        }
    }

    gst_object_unref(factory);

    return extensions;
}

QString MediaSink::formatDescription(const QString &format)
{
    GstElementFactory *factory = gst_element_factory_find(format.toStdString().c_str());

    if (!factory)
        return QString();

    factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory)));

    if (!factory)
        return QString();

    const gchar *longName = gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_LONGNAME);
    QString description(longName);

    gst_object_unref(factory);

    return description;
}

QStringList MediaSink::supportedCodecs(const QString &format,
                                       const QString &type)
{
    GstElementFactory *factory = gst_element_factory_find(format.toStdString().c_str());

    if (!factory)
        return QStringList();

    factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory)));

    if (!factory)
        return QStringList();

    static GstStaticCaps staticRawCaps = GST_STATIC_CAPS("video/x-raw;"
                                                         "audio/x-raw;"
                                                         "text/x-raw;"
                                                         "subpicture/x-dvd;"
                                                         "subpicture/x-pgs");

    GstCaps *rawCaps = gst_static_caps_get(&staticRawCaps);

    GList *encodersList = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_ENCODER,
                                                                GST_RANK_MARGINAL);

    const GList *pads = gst_element_factory_get_static_pad_templates(factory);
    QStringList supportedCodecs;

    for (const GList *padItem = pads; padItem; padItem = g_list_next(padItem)) {
        GstStaticPadTemplate *padtemplate = (GstStaticPadTemplate *) padItem->data;

        if (padtemplate->direction == GST_PAD_SINK) {
            GstCaps *caps = gst_caps_from_string(padtemplate->static_caps.string);

            for (guint i = 0; i < gst_caps_get_size(caps); i++) {
                GstStructure *capsStructure = gst_caps_get_structure(caps, i);
                const gchar *structureName = gst_structure_get_name(capsStructure);
                QString structureType(structureName);
                gchar *structureStr = gst_structure_to_string(capsStructure);
                GstCaps *compCaps = gst_caps_from_string(structureStr);

                if (gst_caps_can_intersect(compCaps, rawCaps)) {
                    if (!type.isEmpty() && structureType != type)
                        continue;

                    QString codecType = structureType.mid(0, type.indexOf('/'));

                    if (gst_structure_has_field(capsStructure, "format")) {
                        GType fieldType = gst_structure_get_field_type(capsStructure, "format");

                        if (fieldType == G_TYPE_STRING) {
                            const gchar *format = gst_structure_get_string(capsStructure, "format");
                            QString codecId = QString("identity/%1/%2").arg(codecType).arg(format);

                            if (!supportedCodecs.contains(codecId))
                                supportedCodecs << codecId;
                        }
                        else if (fieldType == GST_TYPE_LIST) {
                            const GValue *formats = gst_structure_get_value(capsStructure, "format");

                            for (guint i = 0; i < gst_value_list_get_size(formats); i++) {
                                const GValue *format = gst_value_list_get_value(formats, i);
                                QString codecId = QString("identity/%1/%2").arg(codecType).arg(g_value_get_string(format));

                                if (!supportedCodecs.contains(codecId))
                                    supportedCodecs << codecId;
                            }
                        }
                    }
                } else {
                    GList *encoders = gst_element_factory_list_filter(encodersList,
                                                                      caps,
                                                                      GST_PAD_SRC,
                                                                      FALSE);

                    for (GList *encoderItem = encoders; encoderItem; encoderItem = g_list_next(encoderItem)) {
                        GstElementFactory *encoder = (GstElementFactory *) encoderItem->data;

                        const gchar *klass = gst_element_factory_get_metadata(encoder, GST_ELEMENT_METADATA_KLASS);
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

    gst_caps_unref(rawCaps);
    gst_object_unref(factory);

    return supportedCodecs;
}

QString MediaSink::defaultCodec(const QString &format, const QString &type)
{
    QStringList codecs = this->supportedCodecs(format, type);

    if (codecs.isEmpty())
        return QString();

    return codecs.at(0);
}

QString MediaSink::codecDescription(const QString &codec)
{
    if (codec.startsWith("identity/")) {
        QStringList parts = codec.split("/");

        return QString("%1 (%2)").arg(parts[0]).arg(parts[2]);
    }

    GstElementFactory *factory = gst_element_factory_find(codec.toStdString().c_str());

    if (!factory)
        return QString();

    factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory)));

    if (!factory)
        return QString();

    const gchar *longName = gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_LONGNAME);
    QString description(longName);

    gst_object_unref(factory);

    return description;
}

QString MediaSink::codecType(const QString &codec)
{
    if (codec.startsWith("identity/audio"))
        return QString("audio/x-raw");
    else if (codec.startsWith("identity/video"))
        return QString("video/x-raw");
    else if (codec.startsWith("identity/text"))
        return QString("text/x-raw");

    GstElementFactory *factory = gst_element_factory_find(codec.toStdString().c_str());

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

QVariantMap MediaSink::defaultCodecParams(const QString &codec)
{
    QVariantMap codecParams;
    QString codecType = this->codecType(codec);

    qDebug() << codec;

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
            codecParams["supportedSampleFormats"] = QStringList() << sampleFormat;
            codecParams["supportedChannelLayouts"] = QStringList() << "mono" << "stereo";
            codecParams["supportedSampleRates"] = QVariantList();
            codecParams["defaultSampleFormat"] = sampleFormat;
            codecParams["defaultChannelLayout"] = "stereo";
            codecParams["defaultChannels"] = 2;
            codecParams["defaultSampleRate"] = 44100;
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

            QStringList supportedSampleFormats;
            QVariantList supportedSamplerates;
            QStringList supportedChannelLayouts;

            const GList *pads = gst_element_factory_get_static_pad_templates(factory);

            for (const GList *padItem = pads; padItem; padItem = g_list_next(padItem)) {
                GstStaticPadTemplate *padtemplate = (GstStaticPadTemplate *) padItem->data;

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

                                    if (!formatFF.isEmpty() && !supportedSampleFormats.contains(formatFF))
                                        supportedSampleFormats << formatFF;
                                } else if (fieldType == GST_TYPE_LIST) {
                                    const GValue *formats = gst_structure_get_value(capsStructure, "format");

                                    for (guint i = 0; i < gst_value_list_get_size(formats); i++) {
                                        const GValue *format = gst_value_list_get_value(formats, i);
                                        const gchar *formatId = g_value_get_string(format);
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
                                    const GValue *rates = gst_structure_get_value(capsStructure, "rate");

                                    for (guint i = 0; i < gst_value_list_get_size(rates); i++) {
                                        const GValue *rate = gst_value_list_get_value(rates, i);
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
                                    QString layout = QbAudioCaps::defaultChannelLayoutString(channels);

                                    if (!supportedChannelLayouts.contains(layout))
                                        supportedChannelLayouts << layout;
                                } else if (fieldType == GST_TYPE_INT_RANGE) {
                                    const GValue *channels = gst_structure_get_value(capsStructure, "channels");

                                    int min = gst_value_get_int_range_min(channels);
                                    int max = gst_value_get_int_range_max(channels) + 1;
                                    int step = gst_value_get_int_range_step(channels);

                                    for (int i = min; i < max; i += step) {
                                        QString layout = QbAudioCaps::defaultChannelLayoutString(i);

                                        if (!supportedChannelLayouts.contains(layout))
                                            supportedChannelLayouts << layout;
                                    }
                                } else if (fieldType == GST_TYPE_LIST) {
                                    const GValue *channels = gst_structure_get_value(capsStructure, "channels");

                                    for (guint i = 0; i < gst_value_list_get_size(channels); i++) {
                                        const GValue *nchannels = gst_value_list_get_value(channels, i);
                                        gint nchannelsId = g_value_get_int(nchannels);
                                        QString layout = QbAudioCaps::defaultChannelLayoutString(nchannelsId);

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
            codecParams["defaultChannels"] = QbAudioCaps::channelCount(channelLayout);
            codecParams["defaultSampleRate"] = supportedSamplerates.isEmpty()?
                                                 44100: supportedSamplerates.at(0);

            gst_object_unref (element);
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

            const GList *pads = gst_element_factory_get_static_pad_templates(factory);

            for (const GList *padItem = pads; padItem; padItem = g_list_next(padItem)) {
                GstStaticPadTemplate *padtemplate = (GstStaticPadTemplate *) padItem->data;

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
                                        QbFrac framerate(num, den);
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
                                   QbFrac framerate(num, den);
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

            if (bitrate < 1)
                bitrate = 200000;

            codecParams["defaultBitRate"] = bitrate;
            codecParams["defaultGOP"] = 12;
            codecParams["supportedFrameRates"] = supportedFramerates;
            codecParams["supportedPixelFormats"] = supportedPixelFormats;
            codecParams["defaultPixelFormat"] = supportedPixelFormats.isEmpty()?
                                                  "yuv420p": supportedPixelFormats.at(0);

            gst_object_unref(factory);
        }
    } else if (codecType == "text/x-raw") {
    }

    gst_caps_unref(rawCaps);

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
            audioCaps.bps() = QbAudioCaps::bitsPerSample(defaultSampleFormat);
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
            audioCaps.channels() = QbAudioCaps::channelCount(defaultChannelLayout);
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
                audioCaps.bps() = QbAudioCaps::bitsPerSample(defaultSampleFormat);
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
                audioCaps.channels() = QbAudioCaps::channelCount(defaultChannelLayout);
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
    QString ext = QFileInfo(fileName).suffix();

    foreach (QString format, this->supportedFormats())
        if (this->fileExtensions(format).contains(ext))
            return format;

    return QString();
}

void MediaSink::waitState(GstState state)
{
    forever {
        GstState curState;

        if (gst_element_get_state(this->m_pipeline,
                                  &curState,
                                  NULL,
                                  GST_CLOCK_TIME_NONE) == GST_STATE_CHANGE_SUCCESS
            && curState == state)
            break;
    }
}

gboolean MediaSink::busCallback(GstBus *bus,
                                GstMessage *message,
                                gpointer userData)
{
    Q_UNUSED(bus)
    MediaSink *self = static_cast<MediaSink *>(userData);

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;
            gst_message_parse_error(message, &err, &debug);
            qDebug() << "Error: " << err->message;
            g_error_free(err);
            g_free(debug);
            g_main_loop_quit(self->m_mainLoop);

            break;
        }

        case GST_MESSAGE_EOS:
            g_main_loop_quit(self->m_mainLoop);
        break;

        default:
        break;
    }

    return TRUE;
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
    if (this->m_pipeline) {
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

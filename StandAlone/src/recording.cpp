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

#include <QFile>
#include <QSettings>
#include <QQuickItem>
#include <QQmlProperty>
#include <akutils.h>

#include "recording.h"

#define DEFAULT_RECORD_AUDIO false
#define AUDIO_RECORDING_KEY "Enable audio recording"

Recording::Recording(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent),
    m_engine(nullptr)
{
    this->m_recordAudio = DEFAULT_RECORD_AUDIO;
    this->m_state = AkElement::ElementStateNull;
    this->setQmlEngine(engine);
    this->m_pipeline = AkElement::create("Bin", "pipeline");

    if (this->m_pipeline) {
        QFile jsonFile(":/Webcamoid/share/recordingpipeline.json");
        jsonFile.open(QFile::ReadOnly);
        QString description(jsonFile.readAll());
        jsonFile.close();

        this->m_pipeline->setProperty("description", description);

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_videoGen),
                                  Q_ARG(QString, "videoGen"));
        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element",
                                  Q_RETURN_ARG(AkElementPtr, this->m_record),
                                  Q_ARG(QString, "record"));
    }

    if (this->m_record) {
        QObject::connect(this->m_record.data(),
                         SIGNAL(outputFormatChanged(const QString &)),
                         this,
                         SIGNAL(formatChanged(const QString &)));
        QObject::connect(this->m_record.data(),
                         SIGNAL(outputFormatChanged(const QString &)),
                         this,
                         SLOT(saveOutputFormat(const QString &)));
        QObject::connect(this->m_record.data(),
                         SIGNAL(outputFormatChanged(const QString &)),
                         this,
                         SLOT(loadFormatOptions(const QString &)));
        QObject::connect(this->m_record.data(),
                         SIGNAL(outputFormatChanged(const QString &)),
                         this,
                         SLOT(loadStreams(const QString &)));
        QObject::connect(this->m_record.data(),
                         SIGNAL(formatOptionsChanged(const QVariantMap &)),
                         this,
                         SLOT(saveFormatOptions(const QVariantMap &)));
        QObject::connect(this->m_record.data(),
                         SIGNAL(streamsChanged(const QVariantList &)),
                         this,
                         SLOT(saveStreams(const QVariantList &)));
        QObject::connect(this->m_record.data(),
                         SIGNAL(streamsChanged(const QVariantList &)),
                         this,
                         SLOT(loadCodecOptions(const QVariantList &)));
        QObject::connect(this->m_record.data(),
                         SIGNAL(codecOptionsChanged(const QString &, const QVariantMap &)),
                         this,
                         SLOT(saveCodecOptions()));
        QObject::connect(this->m_record.data(),
                         SIGNAL(userControlsValuesChanged(const QVariantMap &)),
                         this,
                         SLOT(userControlsUpdated(const QVariantMap &)));
        QObject::connect(this->m_record.data(),
                         SIGNAL(supportedFormatsChanged(const QStringList &)),
                         this,
                         SLOT(supportedFormatsUpdated(const QStringList &)));
        QObject::connect(this->m_record.data(),
                         SIGNAL(supportedFormatsChanged(const QStringList &)),
                         this,
                         SLOT(updateFormat()));
        QObject::connect(this->m_record.data(),
                         SIGNAL(codecLibChanged(const QString &)),
                         this,
                         SLOT(saveMultiSinkCodecLib(const QString &)));
    }

    this->m_availableFormats = this->recordingFormats();
    this->loadProperties();

    if (this->m_record) {
        QVariantList controls {
            QVariant(QVariantList {
                tr(AUDIO_RECORDING_KEY),
                "boolean",
                0,
                1,
                1,
                DEFAULT_RECORD_AUDIO,
                this->m_recordAudio,
                QStringList()
            })
        };

        this->m_record->setProperty("userControls", controls);
    }

    QObject::connect(this,
                     &Recording::recordAudioChanged,
                     this,
                     &Recording::saveRecordAudio);
    QObject::connect(this,
                     &Recording::audioCapsChanged,
                     this,
                     &Recording::capsUpdated);
    QObject::connect(this,
                     &Recording::videoCapsChanged,
                     this,
                     &Recording::capsUpdated);
    QObject::connect(this,
                     &Recording::recordAudioChanged,
                     this,
                     &Recording::capsUpdated);
}

Recording::~Recording()
{
    this->setState(AkElement::ElementStateNull);
    this->saveProperties();
}

QStringList Recording::availableFormats() const
{
    return this->m_availableFormats;
}

QString Recording::format() const
{
    if (this->m_record)
        return this->m_record->property("outputFormat").toString();

    return QString();
}

AkCaps Recording::audioCaps() const
{
    return this->m_audioCaps;
}

AkCaps Recording::videoCaps() const
{
    return this->m_videoCaps;
}

bool Recording::recordAudio() const
{
    return this->m_recordAudio;
}

QString Recording::videoFileName() const
{
    return this->m_videoFileName;
}

AkElement::ElementState Recording::state() const
{
    return this->m_state;
}

QString Recording::formatDescription(const QString &formatId) const
{
    QString description;

    if (this->m_record)
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "formatDescription",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, formatId));

    return description;
}

QStringList Recording::formatSuffix(const QString &formatId) const
{
    QStringList suffix;

    if (this->m_record)
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "fileExtensions",
                                  Q_RETURN_ARG(QStringList, suffix),
                                  Q_ARG(QString, formatId));

    return suffix;
}

bool Recording::embedControls(const QString &where,
                              const QString &format,
                              const QString &name)
{
    if (!this->m_record)
        return false;

    auto ctrlInterface = this->m_record->controlInterface(this->m_engine,
                                                          format);

    if (!ctrlInterface)
        return false;

    if (!name.isEmpty())
        ctrlInterface->setObjectName(name);

    for (auto obj: this->m_engine->rootObjects()) {
        // First, find where to embed the UI.
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        // Create an item with the plugin context.
        auto interfaceItem = qobject_cast<QQuickItem *>(ctrlInterface);

        // Finally, embed the plugin item UI in the desired place.
        interfaceItem->setParentItem(item);
        interfaceItem->setParent(item);

        QQmlProperty::write(interfaceItem,
                            "anchors.fill",
                            qVariantFromValue(item));

        return true;
    }

    return false;
}

void Recording::removeInterface(const QString &where)
{
    if (!this->m_engine)
        return;

    for (const QObject *obj: this->m_engine->rootObjects()) {
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        QList<decltype(item)> childItems = item->childItems();

        for (auto child: childItems) {
            child->setParentItem(NULL);
            child->setParent(NULL);

            delete child;
        }
    }
}

QStringList Recording::recordingFormats() const
{
    if (!this->m_record)
        return QStringList();

    QStringList formats;
    QStringList supportedFormats;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "supportedFormats",
                              Q_RETURN_ARG(QStringList, supportedFormats));

    for (const QString &format: supportedFormats) {
        QStringList audioCodecs;
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "supportedCodecs",
                                  Q_RETURN_ARG(QStringList, audioCodecs),
                                  Q_ARG(QString, format),
                                  Q_ARG(QString, "audio/x-raw"));

        QStringList videoCodecs;
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "supportedCodecs",
                                  Q_RETURN_ARG(QStringList, videoCodecs),
                                  Q_ARG(QString, format),
                                  Q_ARG(QString, "video/x-raw"));

        QStringList extensions;
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "fileExtensions",
                                  Q_RETURN_ARG(QStringList, extensions),
                                  Q_ARG(QString, format));

        if ((format == "gif" || !audioCodecs.isEmpty())
            && !videoCodecs.isEmpty()
            && !extensions.isEmpty())
            formats << format;
    }

    return formats;
}

void Recording::setFormat(const QString &format)
{
    if (this->m_record)
        this->m_record->setProperty("outputFormat", format);
}

void Recording::setAudioCaps(const AkCaps &audioCaps)
{
    if (this->m_audioCaps == audioCaps)
        return;

    this->m_audioCaps = audioCaps;
    emit this->audioCapsChanged(audioCaps);
}

void Recording::setVideoCaps(const AkCaps &videoCaps)
{
    if (this->m_videoCaps == videoCaps)
        return;

    this->m_videoCaps = videoCaps;
    emit this->videoCapsChanged(videoCaps);
}

void Recording::setRecordAudio(bool recordAudio)
{
    if (this->m_recordAudio == recordAudio)
        return;

    this->m_recordAudio = recordAudio;
    emit this->recordAudioChanged(recordAudio);
}

void Recording::setVideoFileName(const QString &videoFileName)
{
    if (this->m_videoFileName == videoFileName)
        return;

    this->m_videoFileName = videoFileName;
    emit this->videoFileNameChanged(videoFileName);

    if (this->m_record)
        this->m_record->setProperty("location", videoFileName);
}

void Recording::setState(AkElement::ElementState state)
{
    if (this->m_state == state)
        return;

    this->m_mutex.lock();
    this->m_record->setState(state);
    this->m_mutex.unlock();
    this->m_state = state;
    emit this->stateChanged(state);
}

void Recording::resetFormat()
{
    QString defaultFormat;

    if (this->m_record)
        defaultFormat =
                this->m_record->property("codecLib").toString() == "gstreamer"?
                    "webmmux": "webm";

    this->setFormat(defaultFormat);
}

void Recording::resetAudioCaps()
{
    this->setAudioCaps(AkCaps());
}

void Recording::resetVideoCaps()
{
    this->setVideoCaps(AkCaps());
}

void Recording::resetRecordAudio()
{
    this->setRecordAudio(DEFAULT_RECORD_AUDIO);
}

void Recording::resetVideoFileName()
{
    this->setVideoFileName("");
}

void Recording::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void Recording::takePhoto()
{
    this->m_mutex.lock();
    this->m_photo = AkUtils::packetToImage(this->m_curPacket).copy();
    this->m_mutex.unlock();
}

void Recording::savePhoto(const QString &fileName)
{
    QString path = fileName;
    path.replace("file://", "");

    if (path.isEmpty())
        return;

    this->m_photo.save(path);
}

AkPacket Recording::iStream(const AkPacket &packet)
{
    this->m_mutex.lock();
    this->m_curPacket = packet;

    if (this->m_state == AkElement::ElementStatePlaying) {
        if (packet.caps().mimeType() == "video/x-raw")
            (*this->m_videoGen)(packet);
        else
            (*this->m_record)(packet);
    }

    this->m_mutex.unlock();

    return AkPacket();
}

void Recording::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->m_engine == engine)
        return;

    this->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("Recording", this);
}

void Recording::supportedFormatsUpdated(const QStringList &availableFormats)
{
    Q_UNUSED(availableFormats)

    auto recordingFormats = this->recordingFormats();

    if (this->m_availableFormats == recordingFormats)
        return;

    this->m_availableFormats = recordingFormats;
    emit this->availableFormatsChanged(recordingFormats);
}

void Recording::userControlsUpdated(const QVariantMap &userControls)
{
    if (userControls.contains(AUDIO_RECORDING_KEY))
        this->setRecordAudio(userControls[AUDIO_RECORDING_KEY].toBool());
}

void Recording::capsUpdated()
{
    if (!this->m_record)
        return;

    auto format = this->m_record->property("outputFormat").toString();

    if (format.isEmpty())
        return;

    this->loadStreams(format);
}

void Recording::updateFormat()
{
    if (!this->m_record)
        return;

    auto codecLib = this->m_record->property("codecLib").toString();

    if (codecLib.isEmpty())
        return;

    this->setFormat(codecLib == "gstreamer"? "webmmux": "webm");
}

void Recording::loadProperties()
{
    QSettings config;

    config.beginGroup("Libraries");

    if (this->m_record)
        this->m_record->setProperty("codecLib",
                                    config.value("MultiSink.codecLib",
                                                 this->m_record->property("codecLib")));

    config.endGroup();

    QString defaultFormat;

    if (this->m_record)
        defaultFormat =
                this->m_record->property("codecLib").toString() == "gstreamer"?
                    "webmmux": "webm";

    QString preferredFormat =
            this->m_availableFormats.isEmpty()
            || this->m_availableFormats.contains(defaultFormat)?
                defaultFormat: this->m_availableFormats.first();

    config.beginGroup("RecordConfigs");

    this->m_recordAudio = config.value("recordAudio", DEFAULT_RECORD_AUDIO).toBool();
    auto format = config.value("format").toString();

    if (format.isEmpty()
        || !this->m_availableFormats.contains(format))
        format = preferredFormat;

    if (this->m_record)
        this->m_record->setProperty("outputFormat", format);

    config.endGroup();
}

void Recording::loadFormatOptions(const QString &format)
{
    if (!this->m_record || format.isEmpty())
        return;

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_options").arg(format));
    QVariantMap formatOptions;

    for (auto &key: config.allKeys())
        formatOptions[key] = config.value(key);

    QMetaObject::invokeMethod(this->m_record.data(),
                              "setFormatOptions",
                              Q_ARG(QVariantMap, formatOptions));

    config.endGroup();
}

void Recording::loadStreams(const QString &format)
{
    if (!this->m_record || format.isEmpty())
        return;

    QStringList audioCodecs;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "supportedCodecs",
                              Q_RETURN_ARG(QStringList, audioCodecs),
                              Q_ARG(QString, format),
                              Q_ARG(QString, "audio/x-raw"));

    QStringList videoCodecs;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "supportedCodecs",
                              Q_RETURN_ARG(QStringList, videoCodecs),
                              Q_ARG(QString, format),
                              Q_ARG(QString, "video/x-raw"));

    QSettings config;

    config.beginGroup(QString("RecordConfigs_%1").arg(format));
    auto audioCodec = config.value("audio", audioCodecs.value(0)).toString();
    auto videoCodec = config.value("video", videoCodecs.value(0)).toString();
    config.endGroup();

    QVector<AkCaps> streamCaps = {this->m_videoCaps};

    if (this->m_recordAudio
        && this->m_audioCaps
        && !audioCodec.isEmpty())
        streamCaps << this->m_audioCaps;

    QMetaObject::invokeMethod(this->m_record.data(), "clearStreams");

    const QMap<QString, QString> labels = {
        {"audio/x-raw", tr("Audio")   },
        {"video/x-raw", tr("Video")   },
        {"text/x-raw" , tr("Subtitle")}
    };

    for (int stream = 0; stream < streamCaps.size(); stream++)
        if (streamCaps[stream]) {
            config.beginGroup(QString("RecordConfigs_%1_stream_%2_%3")
                              .arg(format)
                              .arg(stream? "audio": "video")
                              .arg(stream? audioCodec: videoCodec));

            auto codec = stream? audioCodec: videoCodec;
            QVariantMap defaultParams;
            QMetaObject::invokeMethod(this->m_record.data(),
                                      "defaultCodecParams",
                                      Q_RETURN_ARG(QVariantMap, defaultParams),
                                      Q_ARG(QString, codec));

            QVariantMap streamParams;
            QString mimeType = streamCaps[stream].mimeType();
            auto label = config.value("label").toString();

            if (label.isEmpty())
                label = labels[mimeType];

            streamParams = {
                {"codec"  , codec                                                  },
                {"bitrate", config.value("bitrate", defaultParams.value("bitrate"))},
                {"label"  , label                                                  }
            };

            if (!stream)
                streamParams["gop"] = config.value("gop", defaultParams.value("gop"));

            QMetaObject::invokeMethod(this->m_record.data(),
                                      "addStream",
                                      Q_RETURN_ARG(QVariantMap, streamParams),
                                      Q_ARG(int, stream),
                                      Q_ARG(AkCaps, streamCaps[stream]),
                                      Q_ARG(QVariantMap, streamParams));

            config.endGroup();
        }

    this->m_videoGen->setProperty("fps", this->m_videoCaps.property("fps"));
}

void Recording::loadCodecOptions(const QVariantList &streams)
{
    if (!this->m_record || streams.isEmpty())
        return;

    auto format = this->m_record->property("outputFormat").toString();

    if (format.isEmpty())
        return;

    const QMap<QString, QString> sType = {
        {"audio/x-raw", tr("audio")   },
        {"video/x-raw", tr("video")   },
        {"text/x-raw" , tr("subtitle")}
    };

    QSettings config;

    for (int i = 0; i < streams.size(); i++) {
        auto stream = streams[i].toMap();
        auto type = stream["caps"].value<AkCaps>().mimeType();
        auto codec = stream["codec"].toString();

        config.beginGroup(QString("RecordConfigs_%1_stream_%2_%3_options")
                          .arg(format).arg(sType[type]).arg(codec));

        QVariantMap codecOptions;

        for (auto &key: config.allKeys())
            codecOptions[key] = config.value(key);

        QMetaObject::invokeMethod(this->m_record.data(),
                                  "setCodecOptions",
                                  Q_ARG(int, i),
                                  Q_ARG(QVariantMap, codecOptions));

        config.endGroup();
    }
}

void Recording::saveOutputFormat(const QString &format)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("format", format);
    config.endGroup();
}

void Recording::saveFormatOptions(const QVariantMap &formatOptions)
{
    Q_UNUSED(formatOptions)

    if (!this->m_record)
        return;

    auto format = this->m_record->property("outputFormat").toString();

    if (format.isEmpty())
        return;

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_options").arg(format));

    QVariantList options;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "formatOptions",
                              Q_RETURN_ARG(QVariantList, options));

    for (auto &option: options) {
        auto optList = option.toList();
        config.setValue(optList[0].toString(), optList[7]);
    }

    config.endGroup();
}

void Recording::saveStreams(const QVariantList &streams)
{
    if (!this->m_record || streams.isEmpty())
        return;

    auto format = this->m_record->property("outputFormat").toString();

    if (format.isEmpty())
        return;

    const QMap<QString, QString> sType = {
        {"audio/x-raw", "audio"   },
        {"video/x-raw", "video"   },
        {"text/x-raw" , "subtitle"}
    };

    QSettings config;

    for (auto &stream: streams) {
        auto streamMap = stream.toMap();
        auto codec = streamMap["codec"].toString();
        auto type = streamMap["caps"].value<AkCaps>().mimeType();
        auto streamType = sType[type];

        config.beginGroup(QString("RecordConfigs_%1").arg(format));
        config.setValue(streamType, codec);
        config.endGroup();

        config.beginGroup(QString("RecordConfigs_%1_stream_%2_%3")
                          .arg(format).arg(streamType).arg(codec));

        config.setValue("bitrate", streamMap.value("bitrate"));
        config.setValue("label", streamMap.value("label"));

        if (type == "video/x-raw")
            config.setValue("gop", streamMap.value("gop"));

        config.endGroup();
    }
}

void Recording::saveCodecParams()
{
    if (!this->m_record)
        return;

    auto format = this->m_record->property("outputFormat").toString();

    if (format.isEmpty())
        return;

    auto streams = this->m_record->property("streams").toList();

    if (streams.isEmpty())
        return;

    const QMap<QString, QString> sType = {
        {"audio/x-raw", "audio"   },
        {"video/x-raw", "video"   },
        {"text/x-raw" , "subtitle"}
    };

    QSettings config;

    for (int i = 0; i < streams.size(); i++) {
        auto stream = streams[i].toMap();
        auto type = stream["caps"].value<AkCaps>().mimeType();
        auto codec = stream["codec"].toString();

        config.beginGroup(QString("RecordConfigs_%1_stream_%2_%3")
                          .arg(format).arg(sType[type]).arg(codec));

        if (type == "audio/x-raw") {
            config.setValue("bitrate", stream["bitrate"]);
            config.setValue("label", stream["label"]);
        } else {
            config.setValue("bitrate", stream["bitrate"]);
            config.setValue("gop", stream["gop"]);
            config.setValue("label", stream["label"]);
        }

        config.endGroup();
    }
}

void Recording::saveCodecOptions()
{
    if (!this->m_record)
        return;

    auto format = this->m_record->property("outputFormat").toString();

    if (format.isEmpty())
        return;

    auto streams = this->m_record->property("streams").toList();

    if (streams.isEmpty())
        return;

    const QMap<QString, QString> sType = {
        {"audio/x-raw", tr("audio")   },
        {"video/x-raw", tr("video")   },
        {"text/x-raw" , tr("subtitle")}
    };

    QSettings config;

    for (int i = 0; i < streams.size(); i++) {
        auto stream = streams[i].toMap();
        auto type = stream["caps"].value<AkCaps>().mimeType();
        auto codec = stream["codec"].toString();

        config.beginGroup(QString("RecordConfigs_%1_stream_%2_%3_options")
                          .arg(format).arg(sType[type]).arg(codec));

        QVariantList codecOptions;
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "codecOptions",
                                  Q_RETURN_ARG(QVariantList, codecOptions),
                                  Q_ARG(int, i));

        for (auto &option: codecOptions) {
            auto opt = option.toList();
            config.setValue(opt[0].toString(), opt[7]);
        }

        config.endGroup();
    }
}

void Recording::saveRecordAudio(bool recordAudio)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("recordAudio", recordAudio);
    config.endGroup();
}

void Recording::saveMultiSinkCodecLib(const QString &codecLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("MultiSink.codecLib", codecLib);
    config.endGroup();
}

void Recording::saveProperties()
{
    QSettings config;
    config.beginGroup("Libraries");

    if (this->m_record)
        config.setValue("MultiSink.codecLib", this->m_record->property("codecLib"));

    config.endGroup();
}

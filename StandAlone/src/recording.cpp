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

#include <QtGlobal>
#include <QDir>
#include <QImage>
#include <QMutex>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QQmlContext>
#include <QQuickItem>
#include <QQmlProperty>
#include <QQmlApplicationEngine>
#include <akcaps.h>
#include <akaudiocaps.h>
#include <akvideocaps.h>
#include <akpacket.h>
#include <akvideopacket.h>
#include <akplugin.h>

#include "recording.h"

#define DEFAULT_RECORD_AUDIO true
#define AUDIO_RECORDING_KEY "Enable audio recording"

using ObjectPtr = QSharedPointer<QObject>;

class RecordingPrivate
{
    public:
        Recording *self;
        QQmlApplicationEngine *m_engine {nullptr};
        QStringList m_availableFormats;
        AkAudioCaps m_audioCaps;
        AkVideoCaps m_videoCaps;
        QString m_videoFileName;
        QString m_imageFormat {"png"};
        QString m_imagesDirectory;
        QString m_videoDirectory;
        QString m_lastPhotoPreview;
        AkElementPtr m_record {AkElement::create("MultiSink")};
        ObjectPtr m_recordSettings {
             AkElement::create<QObject>("MultiSink",
                                        AK_PLUGIN_TYPE_ELEMENT_SETTINGS)
        };
        QMutex m_mutex;
        AkVideoPacket m_curPacket;
        QImage m_photo;
        QMap<QString, QString> m_imageFormats;
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        int m_imageSaveQuality {-1};
        bool m_recordAudio {DEFAULT_RECORD_AUDIO};

        explicit RecordingPrivate(Recording *self);
        void saveImageFormat(const QString &imageFormat);
        void saveImagesDirectory(const QString &imagesDirectory);
        void saveVideoDirectory(const QString &videoDirectory);
        void saveImageSaveQuality(int imageSaveQuality);
        void updatePreviews();
        QStringList recordingFormats() const;
};

Recording::Recording(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new RecordingPrivate(this);
    this->setQmlEngine(engine);
    this->d->m_imagesDirectory =
            QDir(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first())
            .filePath(qApp->applicationName());
    this->d->m_videoDirectory =
            QDir(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).first())
            .filePath(qApp->applicationName());

    if (this->d->m_record) {
        QObject::connect(this->d->m_record.data(),
                         SIGNAL(outputFormatChanged(const QString &)),
                         this,
                         SIGNAL(formatChanged(const QString &)));
        QObject::connect(this->d->m_record.data(),
                         SIGNAL(outputFormatChanged(const QString &)),
                         this,
                         SLOT(saveOutputFormat(const QString &)));
        QObject::connect(this->d->m_record.data(),
                         SIGNAL(outputFormatChanged(const QString &)),
                         this,
                         SLOT(loadFormatOptions(const QString &)));
        QObject::connect(this->d->m_record.data(),
                         SIGNAL(outputFormatChanged(const QString &)),
                         this,
                         SLOT(loadStreams(const QString &)));
        QObject::connect(this->d->m_record.data(),
                         SIGNAL(formatOptionsChanged(const QVariantMap &)),
                         this,
                         SLOT(saveFormatOptions(const QVariantMap &)));
        QObject::connect(this->d->m_record.data(),
                         SIGNAL(streamsChanged(const QVariantList &)),
                         this,
                         SLOT(saveStreams(const QVariantList &)));
        QObject::connect(this->d->m_record.data(),
                         SIGNAL(streamsChanged(const QVariantList &)),
                         this,
                         SLOT(loadCodecOptions(const QVariantList &)));
        QObject::connect(this->d->m_record.data(),
                         SIGNAL(codecOptionsChanged(const QString &, const QVariantMap &)),
                         this,
                         SLOT(saveCodecOptions()));
        QObject::connect(this->d->m_record.data(),
                         SIGNAL(userControlsValuesChanged(const QVariantMap &)),
                         this,
                         SLOT(userControlsUpdated(const QVariantMap &)));
        QObject::connect(this->d->m_record.data(),
                         SIGNAL(supportedFormatsChanged(const QStringList &)),
                         this,
                         SLOT(supportedFormatsUpdated(const QStringList &)));
        QObject::connect(this->d->m_record.data(),
                         SIGNAL(supportedFormatsChanged(const QStringList &)),
                         this,
                         SLOT(updateFormat()));
    }

    if (this->d->m_recordSettings) {
        QObject::connect(this->d->m_recordSettings.data(),
                         SIGNAL(codecLibChanged(const QString &)),
                         this,
                         SLOT(saveMultiSinkCodecLib(const QString &)));
    }

    this->d->m_availableFormats = this->d->recordingFormats();
    this->loadProperties();
    this->d->updatePreviews();

    if (this->d->m_record) {
        QVariantList controls {
            QVariant(QVariantList {
                tr(AUDIO_RECORDING_KEY),
                "boolean",
                0,
                1,
                1,
                DEFAULT_RECORD_AUDIO,
                this->d->m_recordAudio,
                QStringList()
            })
        };

        this->d->m_record->setProperty("userControls", controls);
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
    delete this->d;
}

QStringList Recording::availableFormats() const
{
    return this->d->m_availableFormats;
}

QString Recording::format() const
{
    if (this->d->m_record)
        return this->d->m_record->property("outputFormat").toString();

    return QString();
}

AkAudioCaps Recording::audioCaps() const
{
    return this->d->m_audioCaps;
}

AkVideoCaps Recording::videoCaps() const
{
    return this->d->m_videoCaps;
}

bool Recording::recordAudio() const
{
    return this->d->m_recordAudio;
}

QString Recording::videoFileName() const
{
    return this->d->m_videoFileName;
}

QString Recording::imagesDirectory() const
{
    return this->d->m_imagesDirectory;
}

QString Recording::videoDirectory() const
{
    return this->d->m_videoDirectory;
}

QString Recording::imageFormat() const
{
    return this->d->m_imageFormat;
}

int Recording::imageSaveQuality() const
{
    return this->d->m_imageSaveQuality;
}

QStringList Recording::availableImageFormats() const
{
    return this->d->m_imageFormats.keys();
}

QString Recording::imageFormatDescription(const QString &format) const
{
    return this->d->m_imageFormats.value(format);
}

QString Recording::lastPhotoPreview() const
{
    return this->d->m_lastPhotoPreview;
}

AkElement::ElementState Recording::state() const
{
    return this->d->m_state;
}

QString Recording::formatDescription(const QString &formatId) const
{
    QString description;

    if (this->d->m_record)
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "formatDescription",
                                  Q_RETURN_ARG(QString, description),
                                  Q_ARG(QString, formatId));

    return description;
}

QStringList Recording::formatSuffix(const QString &formatId) const
{
    QStringList suffix;

    if (this->d->m_record)
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "fileExtensions",
                                  Q_RETURN_ARG(QStringList, suffix),
                                  Q_ARG(QString, formatId));

    return suffix;
}

bool Recording::embedControls(const QString &where,
                              const QString &format,
                              const QString &name)
{
    if (!this->d->m_record)
        return false;

    auto ctrlInterface =
            this->d->m_record->controlInterface(this->d->m_engine, format);

    if (!ctrlInterface)
        return false;

    if (!name.isEmpty())
        ctrlInterface->setObjectName(name);

    for (auto &obj: this->d->m_engine->rootObjects()) {
        // First, find where to embed the UI.
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        // Create an item with the plugin context.
        auto interfaceItem = qobject_cast<QQuickItem *>(ctrlInterface);

        // Finally, embed the plugin item UI in the desired place.
        interfaceItem->setParentItem(item);

        return true;
    }

    return false;
}

void Recording::removeInterface(const QString &where)
{
    if (!this->d->m_engine)
        return;

    for (auto &obj: this->d->m_engine->rootObjects()) {
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        QList<decltype(item)> childItems = item->childItems();

        for (auto child: childItems) {
            child->setParentItem(nullptr);
            child->setParent(nullptr);

            delete child;
        }
    }
}

RecordingPrivate::RecordingPrivate(Recording *self):
    self(self)
{
    this->m_imageFormats = {
        {"png", "PNG" },
        {"jpg", "JPEG"},
        {"bmp", "BMP" },
        {"gif", "GIF" },
    };
}

void RecordingPrivate::saveImageFormat(const QString &imageFormat)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("imageFormat", imageFormat);
    config.endGroup();
}

void RecordingPrivate::saveImagesDirectory(const QString &imagesDirectory)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("imagesDirectory", imagesDirectory);
    config.endGroup();
}

void RecordingPrivate::saveVideoDirectory(const QString &videoDirectory)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("videoDirectory", videoDirectory);
    config.endGroup();
}

void RecordingPrivate::saveImageSaveQuality(int imageSaveQuality)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("imageSaveQuality", imageSaveQuality);
    config.endGroup();
}

void RecordingPrivate::updatePreviews()
{
    QStringList nameFilters;

    for (auto it = this->m_imageFormats.begin();
         it != this->m_imageFormats.end();
         it++) {
        nameFilters += "*." + it.key();
    }

    QDir dir(this->m_imagesDirectory);
    auto photos = dir.entryList(nameFilters,
                                QDir::Files | QDir::Readable,
                                QDir::Time);

    if (!photos.isEmpty())
        this->m_lastPhotoPreview = dir.filePath(photos.first());
}

QStringList RecordingPrivate::recordingFormats() const
{
    if (!this->m_record)
        return QStringList();

    QStringList formats;
    QStringList supportedFormats;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "supportedFormats",
                              Q_RETURN_ARG(QStringList, supportedFormats));

    for (auto &format: supportedFormats) {
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

#ifdef Q_OS_ANDROID
        if (!videoCodecs.isEmpty() && !extensions.isEmpty())
            formats << format;
#else
        if ((format == "gif" || !audioCodecs.isEmpty())
            && !videoCodecs.isEmpty()
            && !extensions.isEmpty())
            formats << format;
#endif
    }

    return formats;
}

void Recording::setFormat(const QString &format)
{
    if (this->d->m_record)
        this->d->m_record->setProperty("outputFormat", format);
}

void Recording::setAudioCaps(const AkAudioCaps &audioCaps)
{
    if (this->d->m_audioCaps == audioCaps)
        return;

    this->d->m_audioCaps = audioCaps;
    emit this->audioCapsChanged(audioCaps);
}

void Recording::setVideoCaps(const AkVideoCaps &videoCaps)
{
    if (this->d->m_videoCaps == videoCaps)
        return;

    this->d->m_videoCaps = videoCaps;
    emit this->videoCapsChanged(videoCaps);
}

void Recording::setRecordAudio(bool recordAudio)
{
    if (this->d->m_recordAudio == recordAudio)
        return;

    this->d->m_recordAudio = recordAudio;
    emit this->recordAudioChanged(recordAudio);
}

void Recording::setVideoFileName(const QString &videoFileName)
{
    if (this->d->m_videoFileName == videoFileName)
        return;

    this->d->m_videoFileName = videoFileName;
    emit this->videoFileNameChanged(videoFileName);

    if (this->d->m_record)
        this->d->m_record->setProperty("location", videoFileName);
}

void Recording::setImageFormat(const QString &imageFormat)
{
    if (this->d->m_imageFormat == imageFormat)
        return;

    this->d->m_imageFormat = imageFormat;
    emit this->imageFormatChanged(this->d->m_imageFormat);
    this->d->saveImageFormat(this->d->m_imageFormat);
}

void Recording::setImagesDirectory(const QString &imagesDirectory)
{
    if (this->d->m_imagesDirectory == imagesDirectory)
        return;

    this->d->m_imagesDirectory = imagesDirectory;
    emit this->imagesDirectoryChanged(this->d->m_imagesDirectory);
    this->d->saveImagesDirectory(this->d->m_imagesDirectory);
}

void Recording::setVideoDirectory(const QString &videoDirectory)
{
    if (this->d->m_videoDirectory == videoDirectory)
        return;

    this->d->m_videoDirectory = videoDirectory;
    emit this->videoDirectoryChanged(this->d->m_videoDirectory);
    this->d->saveVideoDirectory(this->d->m_videoDirectory);
}

void Recording::setImageSaveQuality(int imageSaveQuality)
{
    if (this->d->m_imageSaveQuality == imageSaveQuality)
        return;

    this->d->m_imageSaveQuality = imageSaveQuality;
    emit this->imageSaveQualityChanged(this->d->m_imageSaveQuality);
    this->d->saveImageSaveQuality(this->d->m_imageSaveQuality);
}

void Recording::setState(AkElement::ElementState state)
{
    if (this->d->m_state == state)
        return;

    this->d->m_record->setState(state);
    this->d->m_state = state;
    emit this->stateChanged(state);
}

void Recording::resetFormat()
{
    QString defaultFormat;

    if (this->d->m_record)
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "defaultFormat",
                                  Q_RETURN_ARG(QString, defaultFormat));

    this->setFormat(defaultFormat);
}

void Recording::resetAudioCaps()
{
    this->setAudioCaps({});
}

void Recording::resetVideoCaps()
{
    this->setVideoCaps({});
}

void Recording::resetRecordAudio()
{
    this->setRecordAudio(DEFAULT_RECORD_AUDIO);
}

void Recording::resetVideoFileName()
{
    this->setVideoFileName("");
}

void Recording::resetImageFormat()
{
    this->setImageFormat("png");
}

void Recording::resetImagesDirectory()
{
    auto dir =
            QDir(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)
                 .first())
            .filePath(qApp->applicationName());
    this->setImagesDirectory(dir);
}

void Recording::resetVideoDirectory()
{
    auto dir =
            QDir(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                 .first())
            .filePath(qApp->applicationName());
    this->setVideoDirectory(dir);
}

void Recording::resetImageSaveQuality()
{
    this->setImageSaveQuality(-1);
}

void Recording::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void Recording::takePhoto()
{
    this->d->m_mutex.lock();
    this->d->m_photo = this->d->m_curPacket.toImage().copy();
    this->d->m_mutex.unlock();
}

void Recording::savePhoto(const QString &fileName)
{
    QString path = fileName;
    path.replace("file://", "");

    if (path.isEmpty())
        return;

    if (QDir().mkpath(this->d->m_imagesDirectory)) {
        this->d->m_photo.save(path, nullptr, this->d->m_imageSaveQuality);
        this->d->m_lastPhotoPreview = path;
        emit this->lastPhotoPreviewChanged(path);
    }
}

AkPacket Recording::iStream(const AkPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw") {
        this->d->m_mutex.lock();
        this->d->m_curPacket = packet;
        this->d->m_mutex.unlock();
    }

    if (this->d->m_state == AkElement::ElementStatePlaying)
        (*this->d->m_record)(packet);

    return AkPacket();
}

void Recording::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("Recording", this);
}

void Recording::supportedFormatsUpdated(const QStringList &availableFormats)
{
    Q_UNUSED(availableFormats)

    auto recordingFormats = this->d->recordingFormats();

    if (this->d->m_availableFormats == recordingFormats)
        return;

    this->d->m_availableFormats = recordingFormats;
    emit this->availableFormatsChanged(recordingFormats);
}

void Recording::userControlsUpdated(const QVariantMap &userControls)
{
    if (userControls.contains(AUDIO_RECORDING_KEY))
        this->setRecordAudio(userControls[AUDIO_RECORDING_KEY].toBool());
}

void Recording::capsUpdated()
{
    if (!this->d->m_record)
        return;

    auto format = this->d->m_record->property("outputFormat").toString();

    if (format.isEmpty())
        return;

    this->loadStreams(format);
}

void Recording::updateFormat()
{
    if (!this->d->m_record)
        return;

    QString defaultFormat;
    QMetaObject::invokeMethod(this->d->m_record.data(),
                              "defaultFormat",
                              Q_RETURN_ARG(QString, defaultFormat));
    this->setFormat(defaultFormat);
}

void Recording::loadProperties()
{
    QSettings config;

    config.beginGroup("Libraries");

    if (this->d->m_recordSettings) {
        auto codecLib =
                config.value("MultiSink.codecLib",
                             this->d->m_recordSettings->property("codecLib"));
        this->d->m_recordSettings->setProperty("codecLib", codecLib);
    }

    config.endGroup();

    QString defaultFormat;

    if (this->d->m_record) {
        QString defaultFormat;
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "defaultFormat",
                                  Q_RETURN_ARG(QString, defaultFormat));
    }

    QString preferredFormat =
            this->d->m_availableFormats.isEmpty()
            || this->d->m_availableFormats.contains(defaultFormat)?
                defaultFormat: this->d->m_availableFormats.first();

    config.beginGroup("RecordConfigs");

    this->d->m_imagesDirectory =
            config.value("imagesDirectory",
                         this->d->m_imagesDirectory).toString();
    this->d->m_videoDirectory =
            config.value("videoDirectory",
                         this->d->m_videoDirectory).toString();
    this->d->m_imageFormat = config.value("imageFormat", "png").toString();
    this->d->m_imageSaveQuality = config.value("imageSaveQuality", -1).toInt();
    this->d->m_recordAudio =
            config.value("recordAudio", DEFAULT_RECORD_AUDIO).toBool();
    auto format = config.value("format").toString();

    if (format.isEmpty()
        || !this->d->m_availableFormats.contains(format))
        format = preferredFormat;

    if (this->d->m_record)
        this->d->m_record->setProperty("outputFormat", format);

    config.endGroup();
}

void Recording::loadFormatOptions(const QString &format)
{
    if (!this->d->m_record || format.isEmpty())
        return;

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_options").arg(format));
    QVariantMap formatOptions;

    for (auto &key: config.allKeys())
        formatOptions[key] = config.value(key);

    QMetaObject::invokeMethod(this->d->m_record.data(),
                              "setFormatOptions",
                              Q_ARG(QVariantMap, formatOptions));

    config.endGroup();
}

void Recording::loadStreams(const QString &format)
{
    if (!this->d->m_record || format.isEmpty())
        return;

    QStringList audioCodecs;
    QMetaObject::invokeMethod(this->d->m_record.data(),
                              "supportedCodecs",
                              Q_RETURN_ARG(QStringList, audioCodecs),
                              Q_ARG(QString, format),
                              Q_ARG(QString, "audio/x-raw"));

    QStringList videoCodecs;
    QMetaObject::invokeMethod(this->d->m_record.data(),
                              "supportedCodecs",
                              Q_RETURN_ARG(QStringList, videoCodecs),
                              Q_ARG(QString, format),
                              Q_ARG(QString, "video/x-raw"));

    QSettings config;

    config.beginGroup(QString("RecordConfigs_%1").arg(format));
    auto audioCodec = config.value("audio", audioCodecs.value(0)).toString();
    auto videoCodec = config.value("video", videoCodecs.value(0)).toString();
    config.endGroup();

    QVector<AkCaps> streamCaps {this->d->m_videoCaps};

    if (this->d->m_recordAudio
        && this->d->m_audioCaps
        && !audioCodec.isEmpty())
        streamCaps << this->d->m_audioCaps;

    QMetaObject::invokeMethod(this->d->m_record.data(), "clearStreams");

    const QMap<QString, QString> labels = {
        {"audio/x-raw", tr("Audio")   },
        {"video/x-raw", tr("Video")   },
        {"text/x-raw" , tr("Subtitle")}
    };

    int i = 0;

    for (auto &caps: streamCaps) {
        if (caps) {
            QString streamType;

            if (i > 0)
                streamType = "audio";
            else
                streamType = "video";

            auto codec = i > 0? audioCodec: videoCodec;
            config.beginGroup(QString("RecordConfigs_%1_stream_%2_%3")
                              .arg(format, streamType, codec));

            QVariantMap defaultParams;
            QMetaObject::invokeMethod(this->d->m_record.data(),
                                      "defaultCodecParams",
                                      Q_RETURN_ARG(QVariantMap, defaultParams),
                                      Q_ARG(QString, codec));

            QVariantMap streamParams;
            QString mimeType = caps.mimeType();
            auto label = config.value("label").toString();

            if (label.isEmpty())
                label = labels[mimeType];

            streamParams = {
                {"codec"  , codec                                                  },
                {"bitrate", config.value("bitrate", defaultParams.value("bitrate"))},
                {"label"  , label                                                  }
            };

            if (i < 1)
                streamParams["gop"] = config.value("gop", defaultParams.value("gop"));

            QMetaObject::invokeMethod(this->d->m_record.data(),
                                      "addStream",
                                      Q_RETURN_ARG(QVariantMap, streamParams),
                                      Q_ARG(int, i),
                                      Q_ARG(AkCaps, caps),
                                      Q_ARG(QVariantMap, streamParams));

            config.endGroup();
        }

        i++;
    }
}

void Recording::loadCodecOptions(const QVariantList &streams)
{
    if (!this->d->m_record || streams.isEmpty())
        return;

    auto format = this->d->m_record->property("outputFormat").toString();

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
                          .arg(format, sType[type], codec));

        QVariantMap codecOptions;

        for (auto &key: config.allKeys())
            codecOptions[key] = config.value(key);

        QMetaObject::invokeMethod(this->d->m_record.data(),
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

    if (!this->d->m_record)
        return;

    auto format = this->d->m_record->property("outputFormat").toString();

    if (format.isEmpty())
        return;

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_options").arg(format));

    QVariantList options;
    QMetaObject::invokeMethod(this->d->m_record.data(),
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
    if (!this->d->m_record || streams.isEmpty())
        return;

    auto format = this->d->m_record->property("outputFormat").toString();

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
                          .arg(format, streamType, codec));

        config.setValue("bitrate", streamMap.value("bitrate"));
        config.setValue("label", streamMap.value("label"));

        if (type == "video/x-raw")
            config.setValue("gop", streamMap.value("gop"));

        config.endGroup();
    }
}

void Recording::saveCodecParams()
{
    if (!this->d->m_record)
        return;

    auto format = this->d->m_record->property("outputFormat").toString();

    if (format.isEmpty())
        return;

    auto streams = this->d->m_record->property("streams").toList();

    if (streams.isEmpty())
        return;

    const QMap<QString, QString> sType = {
        {"audio/x-raw", "audio"   },
        {"video/x-raw", "video"   },
        {"text/x-raw" , "subtitle"}
    };

    QSettings config;

    for (auto &stream: streams) {
        auto streamMap = stream.toMap();
        auto type = streamMap["caps"].value<AkCaps>().mimeType();
        auto codec = streamMap["codec"].toString();

        config.beginGroup(QString("RecordConfigs_%1_stream_%2_%3")
                          .arg(format, sType[type], codec));

        if (type == "audio/x-raw") {
            config.setValue("bitrate", streamMap["bitrate"]);
            config.setValue("label", streamMap["label"]);
        } else {
            config.setValue("bitrate", streamMap["bitrate"]);
            config.setValue("gop", streamMap["gop"]);
            config.setValue("label", streamMap["label"]);
        }

        config.endGroup();
    }
}

void Recording::saveCodecOptions()
{
    if (!this->d->m_record)
        return;

    auto format = this->d->m_record->property("outputFormat").toString();

    if (format.isEmpty())
        return;

    auto streams = this->d->m_record->property("streams").toList();

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
                          .arg(format, sType[type], codec));

        QVariantList codecOptions;
        QMetaObject::invokeMethod(this->d->m_record.data(),
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

    if (this->d->m_recordSettings) {
        auto codecLib = this->d->m_recordSettings->property("codecLib");
        config.setValue("MultiSink.codecLib", codecLib);
    }

    config.endGroup();
}

#include "moc_recording.cpp"

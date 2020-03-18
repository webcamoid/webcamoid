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
#include <QDateTime>
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

using ObjectPtr = QSharedPointer<QObject>;

class RecordingPrivate
{
    public:
        Recording *self;
        QQmlApplicationEngine *m_engine {nullptr};
        QStringList m_availableVideoFormats;
        QStringList m_availableVideoFormatExtensions;
        QStringList m_availableVideoCodecs;
        QStringList m_availableAudioCodecs;
        QString m_videoFormatExtension;
        AkAudioCaps m_audioCaps;
        AkVideoCaps m_videoCaps;
        QString m_videoCodec;
        QString m_audioCodec;
        QString m_imageFormat {"png"};
        QString m_imagesDirectory;
        QString m_videoDirectory;
        QString m_lastVideoPreview;
        QString m_lastPhotoPreview;
        QString m_videoFilePath;
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
        void updateFormats();
        void updateExtensions();
        void updateCodecs();
        void saveVideoFormat(const QString &videoFormat);
        void saveVideoFormatExtension(const QString &videoFormatExtension);
        void saveMultiSinkCodecLib(const QString &codecLib);
        void saveRecordAudio(bool recordAudio);
        void loadProperties();
        void loadFormatOptions(const QString &videoFormat);
        void loadStreams(const QString &videoFormat);
};

Recording::Recording(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new RecordingPrivate(this);
    this->setQmlEngine(engine);

    if (this->d->m_record) {
        QObject::connect(this->d->m_record.data(),
                         SIGNAL(outputFormatChanged(const QString &)),
                         this,
                         SLOT(outputFormatChanged(const QString &)));
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
    }

    if (this->d->m_recordSettings) {
        QObject::connect(this->d->m_recordSettings.data(),
                         SIGNAL(codecLibChanged(const QString &)),
                         this,
                         SLOT(codecLibChanged(const QString &)));
    }

    this->d->loadProperties();
    this->d->updateFormats();
    this->d->updatePreviews();

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
    delete this->d;
}

AkAudioCaps Recording::audioCaps() const
{
    return this->d->m_audioCaps;
}

AkVideoCaps Recording::videoCaps() const
{
    return this->d->m_videoCaps;
}

AkElement::ElementState Recording::state() const
{
    return this->d->m_state;
}

QString Recording::videoDirectory() const
{
    return this->d->m_videoDirectory;
}

QStringList Recording::availableVideoFormats() const
{
    return this->d->m_availableVideoFormats;
}

QStringList Recording::availableVideoFormatExtensions() const
{
    return this->d->m_availableVideoFormatExtensions;
}

QStringList Recording::availableVideoCodecs() const
{
    return this->d->m_availableVideoCodecs;
}

QStringList Recording::availableAudioCodecs() const
{
    return this->d->m_availableAudioCodecs;
}

QString Recording::videoFormat() const
{
    if (this->d->m_record)
        return this->d->m_record->property("outputFormat").toString();

    return {};
}

QString Recording::videoFormatExtension() const
{
    return this->d->m_videoFormatExtension;
}

QString Recording::videoCodec() const
{
    return this->d->m_videoCodec;
}

QString Recording::audioCodec() const
{
    return this->d->m_audioCodec;
}

QString Recording::videoFormatDescription(const QString &formatId) const
{
    if (!this->d->m_record)
        return {};

    QString description;
    QMetaObject::invokeMethod(this->d->m_record.data(),
                              "formatDescription",
                              Q_RETURN_ARG(QString, description),
                              Q_ARG(QString, formatId));

    return description;
}

QString Recording::codecDescription(const QString &codec) const
{
    if (!this->d->m_record)
        return {};

    QString description;
    QMetaObject::invokeMethod(this->d->m_record.data(),
                              "codecDescription",
                              Q_RETURN_ARG(QString, description),
                              Q_ARG(QString, codec));

    return description;
}

bool Recording::recordAudio() const
{
    return this->d->m_recordAudio;
}

QString Recording::lastVideoPreview() const
{
    return this->d->m_lastVideoPreview;
}

QString Recording::imagesDirectory() const
{
    return this->d->m_imagesDirectory;
}

QStringList Recording::availableImageFormats() const
{
    return this->d->m_imageFormats.keys();
}

QString Recording::imageFormat() const
{
    return this->d->m_imageFormat;
}

QString Recording::imageFormatDescription(const QString &format) const
{
    return this->d->m_imageFormats.value(format);
}

QString Recording::lastPhotoPreview() const
{
    return this->d->m_lastPhotoPreview;
}

int Recording::imageSaveQuality() const
{
    return this->d->m_imageSaveQuality;
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

void RecordingPrivate::updateFormats()
{
    if (!this->m_record)
        return;

    QStringList videoFormats;
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
            this->m_availableVideoFormats << format;
#else
        if ((format == "gif" || !audioCodecs.isEmpty())
            && !videoCodecs.isEmpty()
            && !extensions.isEmpty())
            videoFormats << format;
#endif
    }

    if (this->m_availableVideoFormats != videoFormats) {
        this->m_availableVideoFormats = videoFormats;
        emit self->availableVideoFormatsChanged(videoFormats);

        QString defaultFormat;
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "defaultFormat",
                                  Q_RETURN_ARG(QString, defaultFormat));

        QSettings config;
        config.beginGroup("RecordConfigs");
        self->setVideoFormat(config.value("format", defaultFormat).toString());
        config.endGroup();
    }
}

void RecordingPrivate::updateExtensions()
{
    if (!this->m_record)
        return;

    QStringList extensions;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "fileExtensions",
                              Q_RETURN_ARG(QStringList, extensions),
                              Q_ARG(QString, self->videoFormat()));

    if (this->m_availableVideoFormatExtensions != extensions) {
        this->m_availableVideoFormatExtensions = extensions;
        emit self->availableVideoFormatExtensionsChanged(extensions);

        QSettings config;
        config.beginGroup("RecordConfigs");
        self->setVideoFormatExtension(config.value("videoFormatExtension",
                                                   extensions.value(0))
                                      .toString());
        config.endGroup();
    }
}

void RecordingPrivate::updateCodecs()
{
    if (!this->m_record)
        return;

    auto format = self->videoFormat();

    QStringList audioCodecs;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "supportedCodecs",
                              Q_RETURN_ARG(QStringList,
                                           audioCodecs),
                              Q_ARG(QString, format),
                              Q_ARG(QString, "audio/x-raw"));

    QStringList videoCodecs;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "supportedCodecs",
                              Q_RETURN_ARG(QStringList,
                                           videoCodecs),
                              Q_ARG(QString, format),
                              Q_ARG(QString, "video/x-raw"));

    if (this->m_availableAudioCodecs != audioCodecs) {
        this->m_availableAudioCodecs = audioCodecs;
        emit self->availableAudioCodecsChanged(audioCodecs);

        QString defaultAudioCodec;
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "defaultCodec",
                                  Q_RETURN_ARG(QString, defaultAudioCodec),
                                  Q_ARG(QString, format),
                                  Q_ARG(QString, "audio/x-raw"));

        QSettings config;
        config.beginGroup(QString("RecordConfigs_%1").arg(format));
        self->setAudioCodec(config.value("audio", defaultAudioCodec).toString());
        config.endGroup();
    }

    if (this->m_availableVideoCodecs != videoCodecs) {
        this->m_availableVideoCodecs = videoCodecs;
        emit self->availableVideoCodecsChanged(videoCodecs);

        QString defaultVideoCodec;
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "defaultCodec",
                                  Q_RETURN_ARG(QString, defaultVideoCodec),
                                  Q_ARG(QString, format),
                                  Q_ARG(QString, "video/x-raw"));

        QSettings config;
        config.beginGroup(QString("RecordConfigs_%1").arg(format));
        self->setVideoCodec(config.value("video", defaultVideoCodec).toString());
        config.endGroup();
    }
}

void RecordingPrivate::saveVideoFormat(const QString &videoFormat)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("format", videoFormat);
    config.endGroup();
}

void RecordingPrivate::saveVideoFormatExtension(const QString &videoFormatExtension)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("videoFormatExtension", videoFormatExtension);
    config.endGroup();
}

void RecordingPrivate::saveMultiSinkCodecLib(const QString &codecLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("MultiSink.codecLib", codecLib);
    config.endGroup();
}

void RecordingPrivate::saveRecordAudio(bool recordAudio)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("recordAudio", recordAudio);
    config.endGroup();
}

void RecordingPrivate::loadProperties()
{
    QSettings config;

    config.beginGroup("Libraries");

    if (this->m_recordSettings) {
        auto codecLib =
                config.value("MultiSink.codecLib",
                             this->m_recordSettings->property("codecLib"));
        this->m_recordSettings->setProperty("codecLib", codecLib);
    }

    config.endGroup();

    config.beginGroup("RecordConfigs");

    auto defaultImagesDirectory =
            QDir(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first())
            .filePath(qApp->applicationName());
    auto defaultVideoDirectory =
            QDir(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).first())
            .filePath(qApp->applicationName());
    this->m_imagesDirectory =
            config.value("imagesDirectory", defaultImagesDirectory).toString();
    this->m_videoDirectory =
            config.value("videoDirectory", defaultVideoDirectory).toString();
    this->m_imageFormat = config.value("imageFormat", "png").toString();
    this->m_imageSaveQuality = config.value("imageSaveQuality", -1).toInt();
    this->m_recordAudio =
            config.value("recordAudio", DEFAULT_RECORD_AUDIO).toBool();

    config.endGroup();
}

void RecordingPrivate::loadFormatOptions(const QString &videoFormat)
{
    if (!this->m_record || videoFormat.isEmpty())
        return;

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_options").arg(videoFormat));
    QVariantMap formatOptions;

    for (auto &key: config.allKeys())
        formatOptions[key] = config.value(key);

    QMetaObject::invokeMethod(this->m_record.data(),
                              "setFormatOptions",
                              Q_ARG(QVariantMap, formatOptions));

    config.endGroup();
}

void RecordingPrivate::loadStreams(const QString &videoFormat)
{
    if (!this->m_record || videoFormat.isEmpty())
        return;

    QVector<AkCaps> streamCaps {this->m_videoCaps};

    if (this->m_recordAudio
        && this->m_audioCaps
        && !this->m_audioCodec.isEmpty())
        streamCaps << this->m_audioCaps;

    QMetaObject::invokeMethod(this->m_record.data(), "clearStreams");
    QSettings config;
    int i = 0;

    for (auto &caps: streamCaps) {
        if (caps) {
            QString streamType;

            if (i > 0)
                streamType = "audio";
            else
                streamType = "video";

            auto codec = i > 0? this->m_audioCodec: this->m_videoCodec;
            config.beginGroup(QString("RecordConfigs_%1_stream_%2_%3")
                              .arg(videoFormat, streamType, codec));

            QVariantMap defaultParams;
            QMetaObject::invokeMethod(this->m_record.data(),
                                      "defaultCodecParams",
                                      Q_RETURN_ARG(QVariantMap, defaultParams),
                                      Q_ARG(QString, codec));

            auto bitrate =
                    config.value("bitrate", defaultParams.value("bitrate"));

            QVariantMap streamParams {
                {"codec"  , codec  },
                {"bitrate", bitrate},
            };

            if (i < 1)
                streamParams["gop"] = config.value("gop", defaultParams.value("gop"));

            QMetaObject::invokeMethod(this->m_record.data(),
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

void Recording::setState(AkElement::ElementState state)
{
    if (!this->d->m_record || this->d->m_state == state)
        return;

    if (state == AkElement::ElementStatePlaying) {
        if (!QDir().mkpath(this->d->m_videoDirectory))
            return;

        auto currentTime =
                QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
        this->d->m_videoFilePath =
                tr("%1/Video %2.%3")
                    .arg(this->d->m_videoDirectory)
                    .arg(currentTime)
                    .arg(this->d->m_videoFormatExtension);
        this->d->m_record->setProperty("location", this->d->m_videoFilePath);
    }

    this->d->m_record->setState(state);
    this->d->m_state = state;
    emit this->stateChanged(state);

    if (state == AkElement::ElementStateNull) {/*
        QString path = this->d->m_videoFilePath;
        path.replace("file://", "");

        if (!path.isEmpty())
            emit this->lastPhotoPreviewChanged(path);*/
    }
}

void Recording::setVideoDirectory(const QString &videoDirectory)
{
    if (this->d->m_videoDirectory == videoDirectory)
        return;

    this->d->m_videoDirectory = videoDirectory;
    emit this->videoDirectoryChanged(this->d->m_videoDirectory);
    this->d->saveVideoDirectory(this->d->m_videoDirectory);
}

void Recording::setVideoFormat(const QString &videoFormat)
{
    if (this->d->m_record)
        this->d->m_record->setProperty("outputFormat", videoFormat);
}

void Recording::setVideoFormatExtension(const QString &videoFormatExtension)
{
    if (this->d->m_videoFormatExtension == videoFormatExtension)
        return;

    this->d->m_videoFormatExtension = videoFormatExtension;
    emit this->videoFormatExtensionChanged(this->d->m_videoFormatExtension);
    this->d->saveVideoFormatExtension(this->d->m_videoFormatExtension);
}

void Recording::setVideoCodec(const QString &videoCodec)
{
    if (this->d->m_videoCodec == videoCodec)
        return;

    this->d->m_videoCodec = videoCodec;
    emit this->videoCodecChanged(this->d->m_videoCodec);
}

void Recording::setAudioCodec(const QString &audioCodec)
{
    if (this->d->m_audioCodec == audioCodec)
        return;

    this->d->m_audioCodec = audioCodec;
    emit this->audioCodecChanged(this->d->m_audioCodec);
}

void Recording::setRecordAudio(bool recordAudio)
{
    if (this->d->m_recordAudio == recordAudio)
        return;

    this->d->m_recordAudio = recordAudio;
    emit this->recordAudioChanged(recordAudio);
    this->d->saveRecordAudio(recordAudio);
}

void Recording::setImagesDirectory(const QString &imagesDirectory)
{
    if (this->d->m_imagesDirectory == imagesDirectory)
        return;

    this->d->m_imagesDirectory = imagesDirectory;
    emit this->imagesDirectoryChanged(this->d->m_imagesDirectory);
    this->d->saveImagesDirectory(this->d->m_imagesDirectory);
}

void Recording::setImageFormat(const QString &imageFormat)
{
    if (this->d->m_imageFormat == imageFormat)
        return;

    this->d->m_imageFormat = imageFormat;
    emit this->imageFormatChanged(this->d->m_imageFormat);
    this->d->saveImageFormat(this->d->m_imageFormat);
}

void Recording::setImageSaveQuality(int imageSaveQuality)
{
    if (this->d->m_imageSaveQuality == imageSaveQuality)
        return;

    this->d->m_imageSaveQuality = imageSaveQuality;
    emit this->imageSaveQualityChanged(this->d->m_imageSaveQuality);
    this->d->saveImageSaveQuality(this->d->m_imageSaveQuality);
}

void Recording::resetAudioCaps()
{
    this->setAudioCaps({});
}

void Recording::resetVideoCaps()
{
    this->setVideoCaps({});
}

void Recording::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void Recording::resetVideoDirectory()
{
    auto dir =
            QDir(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                 .first())
            .filePath(qApp->applicationName());
    this->setVideoDirectory(dir);
}

void Recording::resetVideoFormat()
{
    QString defaultFormat;

    if (this->d->m_record)
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "defaultFormat",
                                  Q_RETURN_ARG(QString, defaultFormat));

    this->setVideoFormat(defaultFormat);
}

void Recording::resetVideoFormatExtension()
{
    this->setVideoFormatExtension(this->d->m_availableVideoFormatExtensions.value(0));
}

void Recording::resetVideoCodec()
{
    QString defaultVideoCodec;

    if (!this->d->m_record)
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "defaultCodec",
                                  Q_RETURN_ARG(QString, defaultVideoCodec),
                                  Q_ARG(QString, this->videoFormat()),
                                  Q_ARG(QString, "video/x-raw"));

    this->setVideoCodec(defaultVideoCodec);
}

void Recording::resetAudioCodec()
{
    QString defaultAudioCodec;

    if (!this->d->m_record)
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "defaultCodec",
                                  Q_RETURN_ARG(QString, defaultAudioCodec),
                                  Q_ARG(QString, this->videoFormat()),
                                  Q_ARG(QString, "audio/x-raw"));

    this->setAudioCodec(defaultAudioCodec);
}

void Recording::resetRecordAudio()
{
    this->setRecordAudio(DEFAULT_RECORD_AUDIO);
}

void Recording::resetImagesDirectory()
{
    auto dir =
            QDir(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)
                 .first())
            .filePath(qApp->applicationName());
    this->setImagesDirectory(dir);
}

void Recording::resetImageFormat()
{
    this->setImageFormat("png");
}

void Recording::resetImageSaveQuality()
{
    this->setImageSaveQuality(-1);
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

void Recording::codecLibChanged(const QString &codecLib)
{
    this->d->saveMultiSinkCodecLib(codecLib);
    this->d->updateFormats();
}

void Recording::outputFormatChanged(const QString &outputFormat)
{
    emit this->videoFormatChanged(outputFormat);
    this->d->saveVideoFormat(outputFormat);
    this->d->updateExtensions();
    this->d->loadFormatOptions(outputFormat);
    this->d->updateCodecs();
    this->d->loadStreams(outputFormat);
}

void Recording::capsUpdated()
{
    this->d->loadStreams(this->videoFormat());
}

void Recording::loadCodecOptions(const QVariantList &streams)
{
    if (!this->d->m_record || streams.isEmpty())
        return;

    auto format = this->videoFormat();

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

void Recording::saveFormatOptions(const QVariantMap &formatOptions)
{
    Q_UNUSED(formatOptions)

    if (!this->d->m_record)
        return;

    auto format = this->videoFormat();

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

    auto format = this->videoFormat();

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

        if (type == "video/x-raw")
            config.setValue("gop", streamMap.value("gop"));

        config.endGroup();
    }
}

void Recording::saveCodecOptions()
{
    if (!this->d->m_record)
        return;

    auto format = this->videoFormat();

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

#include "moc_recording.cpp"

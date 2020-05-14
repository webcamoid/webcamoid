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
#include <QAbstractEventDispatcher>
#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QMutex>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
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
        AkAudioCaps m_audioCaps;
        AkVideoCaps m_videoCaps;
        QStringList m_availableVideoFormats;
        QStringList m_availableVideoFormatExtensions;
        QVariantList m_availableVideoFormatOptions;
        QStringList m_availableVideoCodecs;
        QStringList m_availableAudioCodecs;
        QVariantList m_availableVideoCodecOptions;
        QVariantList m_availableAudioCodecOptions;
        QString m_videoFormat;
        QString m_videoFormatExtension;
        QVariantMap m_videoFormatOptions;
        QString m_videoCodec;
        QString m_audioCodec;
        QVariantMap m_videoCodecParams;
        QVariantMap m_audioCodecParams;
        QVariantMap m_videoCodecOptions;
        QVariantMap m_audioCodecOptions;
        QString m_imageFormat {"png"};
        QString m_imagesDirectory;
        QString m_videoDirectory;
        QString m_lastVideoPreview;
        QString m_lastVideo;
        QString m_lastPhotoPreview;
        AkElementPtr m_record {AkElement::create("MultiSink")};
        ObjectPtr m_recordSettings {
             AkElement::create<QObject>("MultiSink",
                                        AK_PLUGIN_TYPE_ELEMENT_SETTINGS)
        };
        AkElementPtr m_thumbnailer {AkElement::create("MultiSrc")};
        ObjectPtr m_thumbnailerSettings {
             AkElement::create<QObject>("MultiSrc",
                                        AK_PLUGIN_TYPE_ELEMENT_SETTINGS)
        };
        QMutex m_mutex;
        AkVideoPacket m_curPacket;
        QImage m_photo;
        QImage m_thumbnail;
        QMap<QString, QString> m_imageFormats;
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        int m_imageSaveQuality {-1};
        bool m_recordAudio {DEFAULT_RECORD_AUDIO};

        explicit RecordingPrivate(Recording *self);
        void loadProperties();
        void updateProperties();
        void updatePreviews();
        void updateAvailableVideoFormats(bool save=false);
        void updateAvailableVideoFormatExtensions(bool save=false);
        void updateAvailableVideoFormatOptions(bool save=false);
        void updateAvailableVideoCodecs(bool save=false);
        void updateAvailableAudioCodecs(bool save=false);
        void updateAvailableVideoCodecsOptions(bool save=false);
        void updateAvailableAudioCodecsOptions(bool save=false);
        void updateVideoFormat(bool save=false);
        void updateVideoFormatExtension(bool save=false);
        void updateVideoFormatOptions(bool save=false);
        void updateVideoCodec(bool save=false);
        void updateAudioCodec(bool save=false);
        void updateVideoCodecParams(bool save=false);
        void updateAudioCodecParams(bool save=false);
        void updateStreams(bool save=false);
        void updateVideoCodecOptions(bool save=false);
        void updateAudioCodecOptions(bool save=false);
        void saveMultiSinkCodecLib(const QString &codecLib);
        void saveImageFormat(const QString &imageFormat);
        void saveImagesDirectory(const QString &imagesDirectory);
        void saveVideoDirectory(const QString &videoDirectory);
        void saveImageSaveQuality(int imageSaveQuality);
        void saveVideoFormat(const QString &videoFormat);
        void saveVideoFormatExtension(const QString &videoFormatExtension);
        void saveVideoFormatOptions(const QVariantMap &formatOptions);
        void saveVideoCodec(const QString &videoCodec);
        void saveAudioCodec(const QString &audioCodec);
        void saveVideoCodecParams(const QVariantMap &videoCodecParams);
        void saveAudioCodecParams(const QVariantMap &audioCodecParams);
        void saveVideoCodecOptions(const QVariantMap &videoCodecOptions);
        void saveAudioCodecOptions(const QVariantMap &audioCodecOptions);
        void saveRecordAudio(bool recordAudio);
        QString readThumbnail(const QString &videoFile);
};

Recording::Recording(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new RecordingPrivate(this);
    this->setQmlEngine(engine);

    if (this->d->m_recordSettings) {
        QObject::connect(this->d->m_recordSettings.data(),
                         SIGNAL(codecLibChanged(const QString &)),
                         this,
                         SLOT(codecLibChanged(const QString &)));
    }

    if (this->d->m_thumbnailer) {
        QObject::connect(this->d->m_thumbnailer.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         this,
                         SLOT(thumbnailUpdated(const AkPacket &)),
                         Qt::DirectConnection);
    }

    this->d->loadProperties();
    this->d->updateProperties();
    this->d->updateAvailableVideoFormats();
    this->d->updatePreviews();
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

QVariantList Recording::availableVideoFormatOptions() const
{
    return this->d->m_availableVideoFormatOptions;
}

QStringList Recording::availableVideoCodecs() const
{
    return this->d->m_availableVideoCodecs;
}

QStringList Recording::availableAudioCodecs() const
{
    return this->d->m_availableAudioCodecs;
}

QVariantList Recording::availableVideoCodecOptions() const
{
    return this->d->m_availableVideoCodecOptions;
}

QVariantList Recording::availableAudioCodecOptions() const
{
    return this->d->m_availableAudioCodecOptions;
}

QString Recording::videoFormat() const
{
    return this->d->m_videoFormat;
}

QString Recording::videoFormatExtension() const
{
    return this->d->m_videoFormatExtension;
}

QVariantMap Recording::videoFormatOptions() const
{
    return this->d->m_videoFormatOptions;
}

QString Recording::videoCodec() const
{
    return this->d->m_videoCodec;
}

QString Recording::audioCodec() const
{
    return this->d->m_audioCodec;
}

QVariantMap Recording::videoCodecParams() const
{
    return this->d->m_videoCodecParams;
}

QVariantMap Recording::audioCodecParams() const
{
    return this->d->m_audioCodecParams;
}

QVariantMap Recording::videoCodecOptions() const
{
    return this->d->m_videoCodecOptions;
}

QVariantMap Recording::audioCodecOptions() const
{
    return this->d->m_audioCodecOptions;
}

bool Recording::recordAudio() const
{
    return this->d->m_recordAudio;
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

QString Recording::lastVideoPreview() const
{
    return this->d->m_lastVideoPreview;
}

QString Recording::lastVideo() const
{
    return this->d->m_lastVideo;
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

void Recording::setAudioCaps(const AkAudioCaps &audioCaps)
{
    if (this->d->m_audioCaps == audioCaps)
        return;

    this->d->m_audioCaps = audioCaps;
    emit this->audioCapsChanged(audioCaps);
    this->d->updateStreams(true);
}

void Recording::setVideoCaps(const AkVideoCaps &videoCaps)
{
    if (this->d->m_videoCaps == videoCaps)
        return;

    this->d->m_videoCaps = videoCaps;
    emit this->videoCapsChanged(videoCaps);
    this->d->updateStreams(true);
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
        this->d->m_lastVideo =
                tr("%1/Video %2.%3")
                    .arg(this->d->m_videoDirectory)
                    .arg(currentTime)
                    .arg(this->d->m_videoFormatExtension);
        this->d->m_record->setProperty("location", this->d->m_lastVideo);
    }

    this->d->m_record->setState(state);
    this->d->m_state = state;
    emit this->stateChanged(state);

    if (state == AkElement::ElementStateNull) {
        auto thumbnail = this->d->readThumbnail(this->d->m_lastVideo);

        if (!thumbnail.isEmpty()) {
            this->d->m_lastVideoPreview = thumbnail;
            emit this->lastVideoPreviewChanged(thumbnail);
            emit this->lastVideoChanged(this->d->m_lastVideo);
        }
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
    if (this->d->m_videoFormat == videoFormat)
        return;

    if (this->d->m_record)
        this->d->m_record->setProperty("outputFormat", videoFormat);

    this->d->m_videoFormat = videoFormat;
    emit this->videoFormatChanged(videoFormat);
    this->d->saveVideoFormat(videoFormat);
    this->d->updateAvailableVideoFormatExtensions(true);
    this->d->updateAvailableVideoFormatOptions(true);
    this->d->updateAvailableVideoCodecs(true);
    this->d->updateAvailableAudioCodecs(true);
}

void Recording::setVideoFormatExtension(const QString &videoFormatExtension)
{
    if (this->d->m_videoFormatExtension == videoFormatExtension)
        return;

    this->d->m_videoFormatExtension = videoFormatExtension;
    emit this->videoFormatExtensionChanged(this->d->m_videoFormatExtension);
    this->d->saveVideoFormatExtension(this->d->m_videoFormatExtension);
}

void Recording::setVideoFormatOptions(const QVariantMap &videoFormatOptions)
{
    if (this->d->m_videoFormatOptions == videoFormatOptions)
        return;

    if (this->d->m_record)
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "setFormatOptions",
                                  Q_ARG(QVariantMap, videoFormatOptions));

    this->d->m_videoFormatOptions = videoFormatOptions;
    emit this->videoFormatOptionsChanged(videoFormatOptions);
    this->d->saveVideoFormatOptions(videoFormatOptions);
}

void Recording::setVideoCodec(const QString &videoCodec)
{
    if (this->d->m_videoCodec == videoCodec)
        return;

    this->d->m_videoCodec = videoCodec;
    emit this->videoCodecChanged(this->d->m_videoCodec);
    this->d->saveVideoCodec(videoCodec);
    this->d->updateVideoCodecParams(true);
}

void Recording::setAudioCodec(const QString &audioCodec)
{
    if (this->d->m_audioCodec == audioCodec)
        return;

    this->d->m_audioCodec = audioCodec;
    emit this->audioCodecChanged(this->d->m_audioCodec);
    this->d->saveAudioCodec(audioCodec);
    this->d->updateAudioCodecParams(true);
}

void Recording::setVideoCodecParams(const QVariantMap &videoCodecParams)
{
    if (this->d->m_videoCodecParams == videoCodecParams)
        return;

    this->d->m_videoCodecParams = videoCodecParams;
    emit this->videoCodecParamsChanged(this->d->m_videoCodecParams);
    this->d->saveVideoCodecParams(videoCodecParams);
    this->d->updateStreams(true);
}

void Recording::setAudioCodecParams(const QVariantMap &audioCodecParams)
{
    if (this->d->m_audioCodecParams == audioCodecParams)
        return;

    this->d->m_audioCodecParams = audioCodecParams;
    emit this->audioCodecParamsChanged(this->d->m_audioCodecParams);
    this->d->saveAudioCodecParams(audioCodecParams);
    this->d->updateStreams(true);
}

void Recording::setVideoCodecOptions(const QVariantMap &videoCodecOptions)
{
    if (this->d->m_videoCodecOptions == videoCodecOptions)
        return;

    if (this->d->m_record)
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "setCodecOptions",
                                  Q_ARG(int, 0),
                                  Q_ARG(QVariantMap, videoCodecOptions));

    this->d->m_videoCodecOptions = videoCodecOptions;
    emit this->videoCodecOptionsChanged(videoCodecOptions);
    this->d->saveVideoCodecOptions(videoCodecOptions);
}

void Recording::setAudioCodecOptions(const QVariantMap &audioCodecOptions)
{
    if (this->d->m_audioCodecOptions == audioCodecOptions)
        return;

    if (this->d->m_record)
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "setCodecOptions",
                                  Q_ARG(int, 1),
                                  Q_ARG(QVariantMap, audioCodecOptions));

    this->d->m_audioCodecOptions = audioCodecOptions;
    emit this->audioCodecOptionsChanged(audioCodecOptions);
    this->d->saveAudioCodecOptions(audioCodecOptions);
}

void Recording::setRecordAudio(bool recordAudio)
{
    if (this->d->m_recordAudio == recordAudio)
        return;

    this->d->m_recordAudio = recordAudio;
    emit this->recordAudioChanged(recordAudio);
    this->d->saveRecordAudio(recordAudio);
    this->d->updateStreams(true);
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

void Recording::resetVideoFormatOptions()
{
    this->setVideoFormatOptions({});
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

void Recording::resetVideoCodecParams()
{
    QVariantMap streamParams;

    if (this->d->m_record && !this->d->m_videoCodec.isEmpty()) {
        QVariantMap defaultParams;
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "defaultCodecParams",
                                  Q_RETURN_ARG(QVariantMap, defaultParams),
                                  Q_ARG(QString, this->d->m_videoCodec));

        streamParams = {
            {"codec"         , this->d->m_videoCodec                },
            {"bitrate"       , defaultParams.value("defaultBitRate")},
            {"gop"           , defaultParams.value("defaultGOP")    },
            {"defaultBitrate", defaultParams.value("defaultBitRate")},
            {"defaultGOP"    , defaultParams.value("defaultGOP")    },
        };
    }

    this->setVideoCodecParams(streamParams);
}

void Recording::resetAudioCodecParams()
{
    QVariantMap streamParams;

    if (this->d->m_record && !this->d->m_audioCodec.isEmpty()) {
        QVariantMap defaultParams;
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "defaultCodecParams",
                                  Q_RETURN_ARG(QVariantMap, defaultParams),
                                  Q_ARG(QString, this->d->m_audioCodec));

        streamParams = {
            {"codec"         , this->d->m_audioCodec                },
            {"bitrate"       , defaultParams.value("defaultBitRate")},
            {"defaultBitrate", defaultParams.value("defaultBitRate")},
        };
    }

    this->setAudioCodecParams(streamParams);
}

void Recording::resetVideoCodecOptions()
{
    QVariantMap codecOptionsMap;

    if (this->d->m_record) {
        QVariantList defaultCodecOptions;
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "codecOptions",
                                  Q_RETURN_ARG(QVariantList, defaultCodecOptions),
                                  Q_ARG(int, 0));

        for (auto &option: defaultCodecOptions) {
            auto optionParams = option.toList();
            codecOptionsMap[optionParams[0].toString()] = optionParams[6];
        }
    }

    this->setVideoCodecOptions(codecOptionsMap);
}

void Recording::resetAudioCodecOptions()
{
    QVariantMap codecOptionsMap;

    if (this->d->m_record) {
        QVariantList defaultCodecOptions;
        QMetaObject::invokeMethod(this->d->m_record.data(),
                                  "codecOptions",
                                  Q_RETURN_ARG(QVariantList, defaultCodecOptions),
                                  Q_ARG(int, 1));

        for (auto &option: defaultCodecOptions) {
            auto optionParams = option.toList();
            codecOptionsMap[optionParams[0].toString()] = optionParams[6];
        }
    }

    this->setAudioCodecOptions(codecOptionsMap);
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
        engine->rootContext()->setContextProperty("recording", this);
}

void Recording::codecLibChanged(const QString &codecLib)
{
    this->d->saveMultiSinkCodecLib(codecLib);
    this->d->updateAvailableVideoFormats();
}

void Recording::thumbnailUpdated(const AkPacket &packet)
{
    AkVideoPacket videoPacket(packet);
    auto thumbnail = videoPacket.toImage();

    if (thumbnail.isNull())
        return;

    this->d->m_thumbnail = thumbnail;
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

    if (this->m_thumbnailerSettings) {
        auto codecLib =
                config.value("MultiSrc.codecLib",
                             this->m_thumbnailerSettings->property("codecLib"));
        this->m_thumbnailer->setProperty("codecLib", codecLib);
    }

    config.endGroup();
}

void RecordingPrivate::updateProperties()
{
    QSettings config;
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

void RecordingPrivate::updatePreviews()
{
    // Update photo preview

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

    // Update video preview

    nameFilters.clear();
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
            for (auto extension: extensions)
                nameFilters += "*." + extension;
#else
        if ((format == "gif" || !audioCodecs.isEmpty())
            && !videoCodecs.isEmpty()
            && !extensions.isEmpty()) {
            for (auto extension: extensions)
                nameFilters += "*." + extension;
        }
#endif
    }

    dir = QDir(this->m_videoDirectory);
    auto videos = dir.entryList(nameFilters,
                                QDir::Files | QDir::Readable,
                                QDir::Time);

    if (!videos.isEmpty()) {
        this->m_lastVideoPreview =
            this->readThumbnail(dir.filePath(videos.first()));
        this->m_lastVideo = dir.filePath(videos.first());
    }
}

void RecordingPrivate::updateAvailableVideoFormats(bool save)
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
            videoFormats << format;
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
        this->updateVideoFormat(save);
    }
}

void RecordingPrivate::updateAvailableVideoFormatExtensions(bool save)
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
        this->updateVideoFormatExtension(save);
    }
}

void RecordingPrivate::updateAvailableVideoFormatOptions(bool save)
{
    if (!this->m_record)
        return;

    QVariantList defaultFormatOptions;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "formatOptions",
                              Q_RETURN_ARG(QVariantList, defaultFormatOptions));

    QVariantList formatOptions;

    for (auto &option: defaultFormatOptions) {
        auto optionParams = option.toList();
        optionParams[7] = optionParams[6];
        formatOptions << QVariant(optionParams);
    }

    if (this->m_availableVideoFormatOptions != formatOptions) {
        this->m_availableVideoFormatOptions = formatOptions;
        emit self->availableVideoFormatOptionsChanged(formatOptions);
        this->updateVideoFormatOptions(save);
    }
}

void RecordingPrivate::updateAvailableVideoCodecs(bool save)
{
    QStringList videoCodecs;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "supportedCodecs",
                              Q_RETURN_ARG(QStringList, videoCodecs),
                              Q_ARG(QString, this->m_videoFormat),
                              Q_ARG(QString, "video/x-raw"));

    if (this->m_availableVideoCodecs != videoCodecs) {
        this->m_availableVideoCodecs = videoCodecs;
        emit self->availableVideoCodecsChanged(videoCodecs);
        this->updateVideoCodec(save);
    }
}

void RecordingPrivate::updateAvailableAudioCodecs(bool save)
{
    if (!this->m_record)
        return;

    QStringList audioCodecs;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "supportedCodecs",
                              Q_RETURN_ARG(QStringList, audioCodecs),
                              Q_ARG(QString, this->m_videoFormat),
                              Q_ARG(QString, "audio/x-raw"));

    if (this->m_availableAudioCodecs != audioCodecs) {
        this->m_availableAudioCodecs = audioCodecs;
        emit self->availableAudioCodecsChanged(audioCodecs);
        this->updateAudioCodec(save);
    }
}

void RecordingPrivate::updateAvailableVideoCodecsOptions(bool save)
{
    if (!this->m_record)
        return;

    QVariantList defaultCodecOptions;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "codecOptions",
                              Q_RETURN_ARG(QVariantList, defaultCodecOptions),
                              Q_ARG(int, 0));

    QVariantList codecOptions;

    for (auto &option: defaultCodecOptions) {
        auto optionParams = option.toList();
        optionParams[7] = optionParams[6];
        codecOptions << QVariant(optionParams);
    }

    if (this->m_availableVideoCodecOptions != codecOptions) {
        this->m_availableVideoCodecOptions = codecOptions;
        emit self->availableVideoCodecOptionsChanged(codecOptions);
        this->updateVideoCodecOptions(save);
    }
}

void RecordingPrivate::updateAvailableAudioCodecsOptions(bool save)
{
    if (!this->m_record)
        return;

    QVariantList defaultCodecOptions;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "codecOptions",
                              Q_RETURN_ARG(QVariantList, defaultCodecOptions),
                              Q_ARG(int, 1));

    QVariantList codecOptions;

    for (auto &option: defaultCodecOptions) {
        auto optionParams = option.toList();
        optionParams[7] = optionParams[6];
        codecOptions << QVariant(optionParams);
    }

    if (this->m_availableAudioCodecOptions != codecOptions) {
        this->m_availableAudioCodecOptions = codecOptions;
        emit self->availableAudioCodecOptionsChanged(codecOptions);
        this->updateAudioCodecOptions(save);
    }
}

void RecordingPrivate::updateVideoFormat(bool save)
{
    if (!this->m_record)
        return;

    QString defaultFormat;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "defaultFormat",
                              Q_RETURN_ARG(QString, defaultFormat));

    QSettings config;
    config.beginGroup("RecordConfigs");
    auto videoFormat = config.value("format").toString();

    if (!this->m_availableVideoFormats.contains(videoFormat))
        videoFormat = defaultFormat;

    if (this->m_videoFormat != videoFormat) {
        this->m_record->setProperty("outputFormat", videoFormat);
        this->m_videoFormat = videoFormat;
        emit self->videoFormatChanged(videoFormat);

        if (save)
            this->saveVideoFormat(videoFormat);

        this->updateAvailableVideoFormatExtensions(save);
        this->updateAvailableVideoFormatOptions(save);
        this->updateAvailableVideoCodecs(save);
        this->updateAvailableAudioCodecs(save);
    }

    config.endGroup();
}

void RecordingPrivate::updateVideoFormatExtension(bool save)
{
    QSettings config;
    config.beginGroup("RecordConfigs");

    auto &extensions =
            this->m_availableVideoFormatExtensions;
    auto extension =
            config.value("videoFormatExtension").toString();

    if (!extensions.contains(extension))
        extension = extensions.value(0);

    config.endGroup();

    if (this->m_videoFormatExtension != extension) {
        this->m_videoFormatExtension = extension;
        emit self->videoFormatExtensionChanged(extension);

        if (save)
            this->saveVideoFormatExtension(extension);
    }
}

void RecordingPrivate::updateVideoFormatOptions(bool save)
{
    if (!this->m_record)
        return;

    QVariantList defaultFormatOptions;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "formatOptions",
                              Q_RETURN_ARG(QVariantList, defaultFormatOptions));

    QVariantMap formatOptions;

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_options")
                      .arg(this->m_videoFormat));

    for (auto &option: defaultFormatOptions) {
        auto optionParams = option.toList();
        auto key = optionParams[0].toString();
        formatOptions[key] = config.value(key, optionParams[6]);

        if (optionParams[2] == "flags" && !formatOptions[key].isValid())
            formatOptions[key] = QStringList();
    }

    config.endGroup();

    if (this->m_videoFormatOptions != formatOptions) {
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "setFormatOptions",
                                  Q_ARG(QVariantMap, formatOptions));

        this->m_videoFormatOptions = formatOptions;
        emit self->videoFormatOptionsChanged(formatOptions);

        if (save)
            this->saveVideoFormatOptions(formatOptions);
    }
}

void RecordingPrivate::updateVideoCodec(bool save)
{
    QString defaultVideoCodec;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "defaultCodec",
                              Q_RETURN_ARG(QString, defaultVideoCodec),
                              Q_ARG(QString, this->m_videoFormat),
                              Q_ARG(QString, "video/x-raw"));

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1").arg(this->m_videoFormat));
    auto videoCodec = config.value("video").toString();

    if (!this->m_availableVideoCodecs.contains(videoCodec))
        videoCodec = defaultVideoCodec;

    config.endGroup();

    if (this->m_videoCodec != videoCodec) {
        this->m_videoCodec = videoCodec;
        emit self->videoCodecChanged(videoCodec);

        if (save)
            this->saveVideoCodec(videoCodec);

        this->updateVideoCodecParams(save);
    }
}

void RecordingPrivate::updateAudioCodec(bool save)
{
    QString defaultAudioCodec;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "defaultCodec",
                              Q_RETURN_ARG(QString, defaultAudioCodec),
                              Q_ARG(QString, this->m_videoFormat),
                              Q_ARG(QString, "audio/x-raw"));

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1").arg(this->m_videoFormat));
    auto audioCodec = config.value("audio").toString();

    if (!this->m_availableAudioCodecs.contains(audioCodec))
        audioCodec = defaultAudioCodec;

    config.endGroup();

    if (this->m_audioCodec != audioCodec) {
        this->m_audioCodec = audioCodec;
        emit self->audioCodecChanged(audioCodec);

        if (save)
            this->saveAudioCodec(audioCodec);

        this->updateAudioCodecParams(save);
    }
}

void RecordingPrivate::updateVideoCodecParams(bool save)
{
    if (!this->m_record)
        return;

    QVariantMap streamParams;

    if (!this->m_videoCodec.isEmpty()) {
        QSettings config;
        config.beginGroup(QString("RecordConfigs_%1_stream_video_%2")
                          .arg(this->m_videoFormat, this->m_videoCodec));

        QVariantMap defaultParams;
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "defaultCodecParams",
                                  Q_RETURN_ARG(QVariantMap, defaultParams),
                                  Q_ARG(QString, this->m_videoCodec));

        auto bitrate =
                config.value("bitrate", defaultParams.value("defaultBitRate"));
        auto gop = config.value("gop", defaultParams.value("defaultGOP"));
        config.endGroup();

        streamParams = {
            {"codec"         , this->m_videoCodec                   },
            {"bitrate"       , bitrate                              },
            {"gop"           , gop                                  },
            {"defaultBitrate", defaultParams.value("defaultBitRate")},
            {"defaultGOP"    , defaultParams.value("defaultGOP")    },
        };
    }

    if (this->m_videoCodecParams != streamParams) {
        this->m_videoCodecParams = streamParams;
        emit self->videoCodecParamsChanged(streamParams);

        if (save)
            this->saveVideoCodecParams(streamParams);

        this->updateStreams(save);
    }
}

void RecordingPrivate::updateAudioCodecParams(bool save)
{
    if (!this->m_record)
        return;

    QVariantMap streamParams;

    if (!this->m_audioCodec.isEmpty()) {
        QSettings config;
        config.beginGroup(QString("RecordConfigs_%1_stream_audio_%2")
                          .arg(this->m_videoFormat, this->m_audioCodec));

        QVariantMap defaultParams;
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "defaultCodecParams",
                                  Q_RETURN_ARG(QVariantMap, defaultParams),
                                  Q_ARG(QString, this->m_audioCodec));

        auto bitrate =
                config.value("bitrate", defaultParams.value("defaultBitRate"));
        config.endGroup();

        streamParams = {
            {"codec"         , this->m_audioCodec                   },
            {"bitrate"       , bitrate                              },
            {"defaultBitrate", defaultParams.value("defaultBitRate")},
            {"defaultGOP"    , defaultParams.value("defaultGOP")    },
        };
    }

    if (this->m_audioCodecParams != streamParams) {
        this->m_audioCodecParams = streamParams;
        emit self->audioCodecParamsChanged(this->m_audioCodecParams);

        if (save)
            this->saveAudioCodecParams(streamParams);

        this->updateStreams(save);
    }
}

void RecordingPrivate::updateStreams(bool save)
{
    if (!this->m_record)
        return;

    QMetaObject::invokeMethod(this->m_record.data(), "clearStreams");

    if (this->m_videoCaps && !this->m_videoCodec.isEmpty())
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "addStream",
                                  Q_ARG(int, 0),
                                  Q_ARG(AkCaps, this->m_videoCaps),
                                  Q_ARG(QVariantMap, this->m_videoCodecParams));

    if (this->m_recordAudio
        && this->m_audioCaps
        && !this->m_audioCodec.isEmpty()) {
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "addStream",
                                  Q_ARG(int, 1),
                                  Q_ARG(AkCaps, this->m_audioCaps),
                                  Q_ARG(QVariantMap, this->m_audioCodecParams));
    }

    this->updateAvailableVideoCodecsOptions(save);
    this->updateAvailableAudioCodecsOptions(save);
}

void RecordingPrivate::updateVideoCodecOptions(bool save)
{
    if (!this->m_record)
        return;

    QVariantList defaultCodecOptions;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "codecOptions",
                              Q_RETURN_ARG(QVariantList, defaultCodecOptions),
                              Q_ARG(int, 0));

    QVariantMap codecOptions;

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_stream_video_%2_options")
                      .arg(this->m_videoFormat, this->m_videoCodec));

    for (auto &option: defaultCodecOptions) {
        auto optionParams = option.toList();
        auto key = optionParams[0].toString();
        codecOptions[key] = config.value(key, optionParams[6]);

        if (optionParams[2] == "flags" && !codecOptions[key].isValid())
            codecOptions[key] = QStringList();
    }

    config.endGroup();

    if (this->m_videoCodecOptions != codecOptions) {
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "setCodecOptions",
                                  Q_ARG(int, 0),
                                  Q_ARG(QVariantMap, codecOptions));

        this->m_videoCodecOptions = codecOptions;
        emit self->videoCodecOptionsChanged(codecOptions);

        if (save)
            this->saveVideoCodecOptions(codecOptions);
    }
}

void RecordingPrivate::updateAudioCodecOptions(bool save)
{
    if (!this->m_record)
        return;

    QVariantList defaultCodecOptions;
    QMetaObject::invokeMethod(this->m_record.data(),
                              "codecOptions",
                              Q_RETURN_ARG(QVariantList, defaultCodecOptions),
                              Q_ARG(int, 1));

    QVariantMap codecOptions;

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_stream_audio_%2_options")
                      .arg(this->m_videoFormat, this->m_audioCodec));

    for (auto &option: defaultCodecOptions) {
        auto optionParams = option.toList();
        auto key = optionParams[0].toString();
        codecOptions[key] = config.value(key, optionParams[6]);

        if (optionParams[2] == "flags" && !codecOptions[key].isValid())
            codecOptions[key] = QStringList();
    }

    config.endGroup();

    if (this->m_audioCodecOptions != codecOptions) {
        QMetaObject::invokeMethod(this->m_record.data(),
                                  "setCodecOptions",
                                  Q_ARG(int, 1),
                                  Q_ARG(QVariantMap, codecOptions));

        this->m_audioCodecOptions = codecOptions;
        emit self->audioCodecOptionsChanged(codecOptions);

        if (save)
            this->saveAudioCodecOptions(codecOptions);
    }
}

void RecordingPrivate::saveMultiSinkCodecLib(const QString &codecLib)
{
    QSettings config;
    config.beginGroup("Libraries");
    config.setValue("MultiSink.codecLib", codecLib);
    config.endGroup();
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

void RecordingPrivate::saveVideoFormatOptions(const QVariantMap &formatOptions)
{
    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_options")
                      .arg(this->m_videoFormat));

    for (auto it = formatOptions.begin();
         it != formatOptions.end();
         it++) {
        config.setValue(it.key(), it.value());
    }

    config.endGroup();
}

void RecordingPrivate::saveVideoCodec(const QString &videoCodec)
{
    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1").arg(this->m_videoFormat));
    config.setValue("video", videoCodec);
    config.endGroup();
}

void RecordingPrivate::saveAudioCodec(const QString &audioCodec)
{
    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1").arg(this->m_videoFormat));
    config.setValue("audio", audioCodec);
    config.endGroup();
}

void RecordingPrivate::saveVideoCodecParams(const QVariantMap &videoCodecParams)
{
    if (this->m_videoCodec.isEmpty())
        return;

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_stream_video_%2")
                      .arg(this->m_videoFormat, this->m_videoCodec));
    config.setValue("bitrate", videoCodecParams.value("bitrate"));
    config.setValue("gop", videoCodecParams.value("gop"));
    config.endGroup();
}

void RecordingPrivate::saveAudioCodecParams(const QVariantMap &audioCodecParams)
{
    if (this->m_audioCodec.isEmpty())
        return;

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_stream_audio_%2")
                      .arg(this->m_videoFormat, this->m_audioCodec));
    config.setValue("bitrate", audioCodecParams.value("bitrate"));
    config.endGroup();
}

void RecordingPrivate::saveVideoCodecOptions(const QVariantMap &videoCodecOptions)
{
    if (this->m_videoCodec.isEmpty())
        return;

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_stream_video_%2_options")
                      .arg(this->m_videoFormat, this->m_videoCodec));

    for (auto it = videoCodecOptions.begin();
         it != videoCodecOptions.end();
         it++) {
        config.setValue(it.key(), it.value());
    }

    config.endGroup();
}

void RecordingPrivate::saveAudioCodecOptions(const QVariantMap &audioCodecOptions)
{
    if (this->m_audioCodec.isEmpty())
        return;

    QSettings config;
    config.beginGroup(QString("RecordConfigs_%1_stream_audio_%2_options")
                      .arg(this->m_videoFormat, this->m_audioCodec));

    for (auto it = audioCodecOptions.begin();
         it != audioCodecOptions.end();
         it++) {
        config.setValue(it.key(), it.value());
    }

    config.endGroup();
}

void RecordingPrivate::saveRecordAudio(bool recordAudio)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("recordAudio", recordAudio);
    config.endGroup();
}

QString RecordingPrivate::readThumbnail(const QString &videoFile)
{
    if (!this->m_thumbnailer || videoFile.isEmpty())
        return {};

    this->m_thumbnailer->setProperty("media", videoFile);
    this->m_thumbnailer->setProperty("sync", false);
    int videoStream = -1;
    QMetaObject::invokeMethod(this->m_thumbnailer.data(),
                              "defaultStream",
                              Q_RETURN_ARG(int, videoStream),
                              Q_ARG(QString, "video/x-raw"));

    if (videoStream < 0)
        return {};

    QList<int> streams {videoStream};
    QMetaObject::invokeMethod(this->m_thumbnailer.data(),
                              "setStreams",
                              Q_ARG(QList<int>, streams));

    this->m_thumbnail = {};
    this->m_thumbnailer->setState(AkElement::ElementStatePaused);
    auto duration = this->m_thumbnailer->property("durationMSecs").value<qint64>();
    QMetaObject::invokeMethod(this->m_thumbnailer.data(),
                              "seek",
                              Q_ARG(qint64, qint64(0.1 * duration)));
    this->m_thumbnailer->setState(AkElement::ElementStatePlaying);

    while (this->m_thumbnail.isNull()
           && this->m_thumbnailer->state() != AkElement::ElementStateNull) {
        auto eventDispatcher = QThread::currentThread()->eventDispatcher();

        if (eventDispatcher)
            eventDispatcher->processEvents(QEventLoop::AllEvents);
    }

    this->m_thumbnailer->setState(AkElement::ElementStateNull);
    auto thumnailDir =
            QDir(QStandardPaths::standardLocations(QStandardPaths::TempLocation).first())
            .filePath(qApp->applicationName());

    if (this->m_thumbnail.isNull() || !QDir().mkpath(thumnailDir))
        return {};

    auto defaultTempDirectory =
            QStandardPaths::standardLocations(QStandardPaths::TempLocation).first();
    auto baseName = QFileInfo(videoFile).baseName();
    auto thumnailPath = QString("%1/%2.%3")
                        .arg(thumnailDir)
                        .arg(baseName)
                        .arg(this->m_imageFormat);

    if (!this->m_thumbnail.save(thumnailPath,
                                nullptr,
                                this->m_imageSaveQuality))
        return {};

    return thumnailPath;
}

#include "moc_recording.cpp"

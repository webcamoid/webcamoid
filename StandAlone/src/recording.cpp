/* Webcamoid, camera capture application.
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

#include <QAbstractEventDispatcher>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QImageWriter>
#include <QMutex>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickItem>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QtConcurrent>
#include <QtGlobal>

#include <ak.h>
#include <akaudiocaps.h>
#include <akcaps.h>
#include <akcompressedcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>
#include <iak/akelement.h>
#include <iak/akaudioencoder.h>
#include <iak/akvideoencoder.h>
#include <iak/akvideomuxer.h>

#include "recording.h"
#include "encoding.h"

#define DEFAULT_RECORD_AUDIO true

struct FormatInfo
{
    QString name;
    QString description;
    QString extension;
    QStringList pluginIDs; // Muxer plugins providing the format, best first.
};

using ObjectPtr = QSharedPointer<QObject>;

class RecordingPrivate
{
    public:
        Recording *self;
        QQmlApplicationEngine *m_engine {nullptr};
        AkAudioCaps m_audioCaps;
        AkVideoCaps m_videoCaps;
        EncodingPtr m_encoding;
        QVector<FormatInfo> m_supportedFormats;
        QMap<QString, AkVideoMuxerPtr> m_muxerPlugins;
        QString m_format;
        QString m_audioCodec;
        QString m_videoCodec;
        AkVideoMuxerPtr m_muxer;
        QString m_muxerPluginID;
        AkAudioEncoderPtr m_audioEncoder;
        AkVideoEncoderPtr m_videoEncoder;
        QMetaObject::Connection m_audioHeadersChangedConnection;
        QMetaObject::Connection m_videoHeadersChangedConnection;
        QString m_imageFormat {"png"};
        QString m_imagesDirectory;
        QString m_videoDirectory;
        QString m_lastVideoPreview;
        QString m_lastVideo;
        QString m_lastPhotoPreview;
        QString m_latestVideoUri;
        QString m_latestPhotoUri;
        AkElementPtr m_thumbnailer {akPluginManager->create<AkElement>("MultimediaSource/MultiSrc")};
        QReadWriteLock m_thumbnailMutex;
        QMutex m_thumbnailerMutex;
        QThreadPool m_threadPool;
        QImage m_photo;
        QImage m_thumbnail;
        QMap<QString, QString> m_imageFormats;
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        int m_imageSaveQuality {-1};
        bool m_useFlash {true};
        bool m_useVideoFlash {true};
        int m_photoTimeout {0};
        bool m_recordAudio {DEFAULT_RECORD_AUDIO};
        bool m_isRecording {false};
        bool m_pause {false};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit RecordingPrivate(Recording *self);
        void initSupportedFormats();
        const FormatInfo *findFormat(const QString &format) const;
        QStringList supportedCodecsFor(const QString &format,
                                       AkCaps::CapsType type) const;
        QString defaultCodec(const QString &format, AkCaps::CapsType type) const;
        QString defaultFormat() const;
        bool muxerSupportsCodecs(const QString &pluginID,
                                 const QString &format) const;
        void updateMuxer();
        void loadFormatCodecs();
        void refreshCodecs();
        void printRecordingParameters();
        bool init();
        void uninit();
        static QString normatizePluginID(const QString &pluginID);
        void loadConfigs();
        void loadFormatOptions();
        void updatePreviews();
        void readThumbnail(const QString &videoFile);
        void thumbnailReady();

        // General options
        void saveAudioCaps(const AkAudioCaps &audioCaps);
        void saveVideoCaps(const AkVideoCaps &videoCaps);

        // Video
        void saveVideoDirectory(const QString &videoDirectory);
        void saveVideoFormat(const QString &videoFormat);
        void saveCodec(AkCaps::CapsType type, const QString &codec);
        void saveVideoFormatOptionValue(const QString &option,
                                        const QVariant &value);
        void saveRecordAudio(bool recordAudio);
        void saveUseVideoFlash(bool useVideoFlash);

        // Picture
        void saveImagesDirectory(const QString &imagesDirectory);
        void saveImageFormat(const QString &imageFormat);
        void saveImageSaveQuality(int imageSaveQuality);
        void saveUseFlash(bool useFlash);
        void savePhotoTimeout(int photoTimeout);
};

Recording::Recording(const EncodingPtr &encoding,
                     QQmlApplicationEngine *engine,
                     QObject *parent):
    QObject(parent)
{
    this->d = new RecordingPrivate(this);
    this->d->m_encoding = encoding;
    this->setQmlEngine(engine);

    if (this->d->m_thumbnailer) {
        QObject::connect(this->d->m_thumbnailer.data(),
                         SIGNAL(oStream(AkPacket)),
                         this,
                         SLOT(thumbnailUpdated(AkPacket)),
                         Qt::DirectConnection);
        QObject::connect(this->d->m_thumbnailer.data(),
                         SIGNAL(mediaLoaded(QString)),
                         this,
                         SLOT(mediaLoaded(QString)));
    }

    this->d->initSupportedFormats();
    this->d->loadConfigs();
    this->d->updatePreviews();

    QObject::connect(this->d->m_encoding.data(),
                     &Encoding::encoderChanged,
                     this,
                     [this] () {
        this->d->refreshCodecs();
    });
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

QString Recording::videoFormat() const
{
    return this->d->m_format;
}

QStringList Recording::videoFormats() const
{
    QStringList formats;

    for (auto &format: this->d->m_supportedFormats)
        if (!this->d->supportedCodecsFor(format.name, AkCaps::CapsAudio).isEmpty()
            && !this->d->supportedCodecsFor(format.name, AkCaps::CapsVideo).isEmpty())
            formats << format.name;

    return formats;
}

QString Recording::defaultVideoFormat() const
{
    return this->d->defaultFormat();
}

QString Recording::formatDescription(const QString &format) const
{
    auto fmt = this->d->findFormat(format);

    return fmt? fmt->description: QString();
}

QString Recording::codec(AkCaps::CapsType type) const
{
    switch (type) {
    case AkCaps::CapsAudio:
        return this->d->m_audioCodec;

    case AkCaps::CapsVideo:
        return this->d->m_videoCodec;

    default:
        break;
    }

    return {};
}

QString Recording::defaultCodec(const QString &format,
                                AkCaps::CapsType type) const
{
    return this->d->defaultCodec(format, type);
}

QStringList Recording::supportedCodecs(const QString &format,
                                       AkCaps::CapsType type) const
{
    QStringList codecs;

    if (type == AkCaps::CapsAudio || type == AkCaps::CapsAny)
        codecs << this->d->supportedCodecsFor(format, AkCaps::CapsAudio);

    if (type == AkCaps::CapsVideo || type == AkCaps::CapsAny)
        codecs << this->d->supportedCodecsFor(format, AkCaps::CapsVideo);

    return codecs;
}

QString Recording::codecDescription(const QString &codec) const
{
    return this->d->m_encoding->codecDescription(codec);
}

AkPropertyOptions Recording::videoFormatOptions() const
{
    if (!this->d->m_muxer)
        return {};

    return this->d->m_muxer->options();
}

QVariant Recording::videoFormatOptionValue(const QString &option) const
{
    if (!this->d->m_muxer)
        return {};

    return this->d->m_muxer->optionValue(option);
}

bool Recording::recordAudio() const
{
    return this->d->m_recordAudio;
}

QString Recording::lastVideoPreview() const
{
    return this->d->m_lastVideoPreview;
}

QString Recording::lastVideo() const
{
    return this->d->m_lastVideo;
}

QString Recording::latestVideoUri() const
{
    return this->d->m_latestVideoUri;
}

bool Recording::useVideoFlash() const
{
    return this->d->m_useVideoFlash;
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

QString Recording::latestPhotoUri() const
{
    return this->d->m_latestPhotoUri;
}

int Recording::imageSaveQuality() const
{
    return this->d->m_imageSaveQuality;
}

bool Recording::useFlash() const
{
    return this->d->m_useFlash;
}

int Recording::photoTimeout() const
{
    return this->d->m_photoTimeout;
}

void Recording::setAudioCaps(const AkAudioCaps &audioCaps)
{
    if (this->d->m_audioCaps == audioCaps)
        return;

    this->d->m_audioCaps = audioCaps;
    emit this->audioCapsChanged(audioCaps);
    this->d->saveAudioCaps(audioCaps);
}

void Recording::setVideoCaps(const AkVideoCaps &videoCaps)
{
    if (this->d->m_videoCaps == videoCaps)
        return;

    this->d->m_videoCaps = videoCaps;
    emit this->videoCapsChanged(videoCaps);
    this->d->saveVideoCaps(videoCaps);
}

bool Recording::setState(AkElement::ElementState state)
{
    switch (this->d->m_state) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            this->d->m_pause = true;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        case AkElement::ElementStatePlaying:
            if (!this->d->init())
                return false;

            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->uninit();
            this->d->m_pause = false;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        case AkElement::ElementStatePlaying:
            this->d->m_pause = false;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->uninit();
            this->d->m_pause = false;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        case AkElement::ElementStatePaused:
            this->d->m_pause = true;
            this->d->m_state = state;
            emit this->stateChanged(state);

            return true;
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
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
    if (this->d->m_isRecording
        || videoFormat == this->d->m_format
        || !this->d->findFormat(videoFormat))
        return;

    this->d->m_format = videoFormat;
    this->d->loadFormatCodecs();
    this->d->updateMuxer();
    emit this->videoFormatChanged(videoFormat);
    this->d->saveVideoFormat(videoFormat);
}

void Recording::setCodec(AkCaps::CapsType type, const QString &codec)
{
    if (this->d->m_isRecording
        || !this->d->supportedCodecsFor(this->d->m_format, type).contains(codec))
        return;

    switch (type) {
    case AkCaps::CapsAudio:
        if (this->d->m_audioCodec == codec)
            return;

        this->d->m_audioCodec = codec;

        break;

    case AkCaps::CapsVideo:
        if (this->d->m_videoCodec == codec)
            return;

        this->d->m_videoCodec = codec;

        break;

    default:
        return;
    }

    this->d->updateMuxer();
    emit this->codecChanged(type, codec);
    this->d->saveCodec(type, codec);
}

void Recording::setVideoFormatOptionValue(const QString &option,
                                          const QVariant &value)
{
    if (!this->d->m_muxer)
        return;

    if (this->d->m_muxer->optionValue(option) == value)
        return;

    this->d->m_muxer->setOptionValue(option, value);
    emit this->videoFormatOptionValueChanged(option, value);
    this->d->saveVideoFormatOptionValue(option, value);
}

void Recording::setRecordAudio(bool recordAudio)
{
    if (this->d->m_recordAudio == recordAudio)
        return;

    this->d->m_recordAudio = recordAudio;
    emit this->recordAudioChanged(recordAudio);
    this->d->saveRecordAudio(recordAudio);
}

void Recording::setUseVideoFlash(bool useVideoFlash)
{
    if (this->d->m_useVideoFlash == useVideoFlash)
        return;

    this->d->m_useVideoFlash = useVideoFlash;
    emit this->useVideoFlashChanged(this->d->m_useVideoFlash);
    this->d->saveUseVideoFlash(this->d->m_useVideoFlash);
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

void Recording::setUseFlash(bool useFlash)
{
    if (this->d->m_useFlash == useFlash)
        return;

    this->d->m_useFlash = useFlash;
    emit this->useFlashChanged(this->d->m_useFlash);
    this->d->saveUseFlash(this->d->m_useFlash);
}

void Recording::setPhotoTimeout(int photoTimeout)
{
    if (this->d->m_photoTimeout == photoTimeout)
        return;

    this->d->m_photoTimeout = photoTimeout;
    emit this->photoTimeoutChanged(this->d->m_photoTimeout);
    this->d->savePhotoTimeout(this->d->m_photoTimeout);
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
    auto moviesPath =
            QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    QString dir = moviesPath.isEmpty()?
                      "":
                      QDir(moviesPath).filePath(qApp->applicationName());
    this->setVideoDirectory(dir);
}

void Recording::resetVideoFormat()
{
    this->setVideoFormat(this->d->defaultFormat());
}

void Recording::resetCodec(AkCaps::CapsType type)
{
    this->setCodec(type, this->d->defaultCodec(this->videoFormat(), type));
}

void Recording::resetVideoFormatOptionValue(const QString &option)
{
    this->setVideoFormatOptionValue(option, this->videoFormatOptionValue(option));
}

void Recording::resetVideoFormatOptions()
{
    for (auto &option: this->videoFormatOptions())
        this->resetVideoFormatOptionValue(option.name());
}

void Recording::resetRecordAudio()
{
    this->setRecordAudio(DEFAULT_RECORD_AUDIO);
}

void Recording::resetUseVideoFlash()
{
    this->setUseVideoFlash(true);
}

void Recording::resetImagesDirectory()
{
    auto picturesPath =
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QString dir = picturesPath.isEmpty()?
                      "":
                      QDir(picturesPath).filePath(qApp->applicationName());
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

void Recording::resetUseFlash()
{
    this->setUseFlash(true);
}

void Recording::resetPhotoTimeout()
{
    this->setPhotoTimeout(0);
}

void Recording::takePhoto()
{
    this->d->m_photo = QImage();
    emit this->requestPhotoFrame();
}

void Recording::savePhoto(const QString &fileName)
{
    QString path = fileName;

#ifdef Q_OS_WIN32
    path.replace("file:///", "");
#else
    path.replace("file://", "");
#endif

    if (path.isEmpty())
        return;

    auto saveDirectory = QFileInfo(path).absolutePath();

    if (!QDir().exists(saveDirectory) && !QDir().mkpath(saveDirectory)) {
        qCritical() << "Failed creatng the Images directory" << saveDirectory;

        return;
    }

    if (this->d->m_photo.isNull()) {
        qCritical() << "The image to save is Null";

        return;
    }

    if (!this->d->m_photo.save(path, nullptr, this->d->m_imageSaveQuality)) {
        qCritical() << "Failed saving the photo to" << path;

        return;
    }

    this->d->m_lastPhotoPreview = path;
    emit this->lastPhotoPreviewChanged(path);
}

void Recording::setPhotoFrame(const QImage &frame)
{
    this->d->m_photo = frame.convertedTo(QImage::Format_ARGB32);

    emit this->photoReady();
}

bool Recording::copyToClipboard()
{
    if (!this->d->m_photo.isNull()) {
        QApplication::clipboard()->setImage(this->d->m_photo, QClipboard::Clipboard);

        return true;
    }

    return false;
}

AkPacket Recording::iStream(const AkPacket &packet)
{
    if (this->d->m_isRecording) {
        switch (packet.type()) {
        case AkPacket::PacketAudio:
            if (this->d->m_audioEncoder)
                this->d->m_audioEncoder->iStream(packet);

            break;

        case AkPacket::PacketVideo:
            if (this->d->m_videoEncoder)
                this->d->m_videoEncoder->iStream(packet);

            break;

        default:
            break;
        }
    }

    return {};
}

void Recording::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("recording", this);
}

void Recording::thumbnailUpdated(const AkPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return;

    QImage thumbnail(src.caps().width(),
                     src.caps().height(),
                     QImage::Format_ARGB32);
    auto lineSize =
            qMin<size_t>(src.lineSize(0), thumbnail.bytesPerLine());

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = src.constLine(0, y);
        auto dstLine = thumbnail.scanLine(y);
        memcpy(dstLine, srcLine, lineSize);
    }

    this->d->m_thumbnailMutex.lockForWrite();
    this->d->m_thumbnail = thumbnail;
    this->d->m_thumbnailMutex.unlock();
    auto result =
            QtConcurrent::run(&this->d->m_threadPool,
                              &RecordingPrivate::thumbnailReady,
                              this->d);
    Q_UNUSED(result)
}

void Recording::mediaLoaded(const QString &media)
{
    int videoStream = -1;
    QMetaObject::invokeMethod(this->d->m_thumbnailer.data(),
                              "defaultStream",
                              Q_RETURN_ARG(int, videoStream),
                              Q_ARG(AkCaps::CapsType, AkCaps::CapsVideo));

    if (videoStream < 0)
        return;

    QList<int> streams {videoStream};
    QMetaObject::invokeMethod(this->d->m_thumbnailer.data(),
                              "setStreams",
                              Q_ARG(QList<int>, streams));

    this->d->m_thumbnailMutex.lockForWrite();
    this->d->m_thumbnail = {};
    this->d->m_thumbnailMutex.unlock();
    this->d->m_thumbnailer->setState(AkElement::ElementStatePaused);
    auto duration = this->d->m_thumbnailer->property("durationMSecs").value<qint64>();

    if (duration < 1)
        return;

    QMetaObject::invokeMethod(this->d->m_thumbnailer.data(),
                              "seek",
                              Q_ARG(qint64, qint64(0.1 * duration)));
    this->d->m_thumbnailerMutex.lock();
    this->d->m_thumbnailer->setState(AkElement::ElementStatePlaying);
    this->d->m_thumbnailerMutex.unlock();
}

RecordingPrivate::RecordingPrivate(Recording *self):
    self(self)
{
    static const QMap<QString, QString> formatsDescription {
        {"avif", "AV1 Image File Format (AVIF)"                      },
        {"jpg" , "Joint Photographic Experts Group (JPEG)"    },
        {"png" , "Portable Network Graphics (PNG)"            },
        {"webp", "WebP (WEBP)"                                },
    };

    static const QMap<QString, QString> recordingFormatsMapping {
        {"jpeg", "jpg" },
    };

    for (auto &format: QImageWriter::supportedImageFormats()) {
        QString fmt = format;

        if (recordingFormatsMapping.contains(fmt))
            fmt = recordingFormatsMapping[fmt];

        if (this->m_imageFormats.contains(fmt))
            continue;

        if (formatsDescription.contains(fmt))
            this->m_imageFormats[fmt] = formatsDescription[fmt];
    }
}

void RecordingPrivate::initSupportedFormats()
{
    this->m_supportedFormats.clear();
    this->m_muxerPlugins.clear();

    auto muxerPlugins =
            akPluginManager->listPlugins("^VideoMuxer([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);

    // The formats are named after the container, regardless of the plugin
    // that implements it. When many plugins provide the same format, the one
    // with the highest priority goes first.
    std::stable_sort(muxerPlugins.begin(),
                     muxerPlugins.end(),
                     [] (const QString &a, const QString &b) {
        return akPluginManager->pluginInfo(a).priority()
               > akPluginManager->pluginInfo(b).priority();
    });

    for (auto &pluginID: muxerPlugins) {
        auto plugin = akPluginManager->create<AkVideoMuxer>(pluginID);

        if (!plugin)
            continue;

        this->m_muxerPlugins[pluginID] = plugin;

        for (auto &muxer: plugin->muxers()) {
            auto it = std::find_if(this->m_supportedFormats.begin(),
                                   this->m_supportedFormats.end(),
                                   [&muxer] (const FormatInfo &formatInfo) -> bool {
                return formatInfo.name == muxer;
            });

            if (it == this->m_supportedFormats.end())
                this->m_supportedFormats << FormatInfo {muxer,
                                                        plugin->description(muxer),
                                                        plugin->extension(muxer),
                                                        {pluginID}};
            else
                it->pluginIDs << pluginID;
        }
    }

    std::sort(this->m_supportedFormats.begin(),
              this->m_supportedFormats.end(),
              [] (const FormatInfo &fi1, const FormatInfo &fi2) {
        return fi1.description < fi2.description;
    });
}

const FormatInfo *RecordingPrivate::findFormat(const QString &format) const
{
    for (auto &fmt: this->m_supportedFormats)
        if (fmt.name == format)
            return &fmt;

    return nullptr;
}

QStringList RecordingPrivate::supportedCodecsFor(const QString &format,
                                                 AkCaps::CapsType type) const
{
    auto fmt = this->findFormat(format);

    if (!fmt)
        return {};

    auto compressedType = type == AkCaps::CapsAudio?
                              AkCompressedCaps::CapsType_Audio:
                              AkCompressedCaps::CapsType_Video;
    QStringList codecs;

    // A codec is available when the muxer supports the codec ID used by the
    // encoder currently associated to it.
    for (auto &codec: this->m_encoding->codecs(type)) {
        auto id = this->m_encoding->encoderCodecID(type, codec);

        for (auto &pluginID: fmt->pluginIDs) {
            auto plugin = this->m_muxerPlugins.value(pluginID);

            if (plugin
                && plugin->supportedCodecs(fmt->name, compressedType).contains(id)) {
                codecs << codec;

                break;
            }
        }
    }

    return codecs;
}

QString RecordingPrivate::defaultCodec(const QString &format,
                                       AkCaps::CapsType type) const
{
    return this->supportedCodecsFor(format, type).value(0);
}

QString RecordingPrivate::defaultFormat() const
{
    QString defaultFormat;
    int defaultPriority = 0;

    for (auto &format: this->self->videoFormats()) {
        auto fmt = this->findFormat(format);

        if (!fmt)
            continue;

        auto priority =
                akPluginManager->pluginInfo(fmt->pluginIDs.first()).priority();

        if (defaultFormat.isEmpty() || priority > defaultPriority) {
            defaultFormat = format;
            defaultPriority = priority;
        }
    }

    return defaultFormat;
}

bool RecordingPrivate::muxerSupportsCodecs(const QString &pluginID,
                                           const QString &format) const
{
    auto plugin = this->m_muxerPlugins.value(pluginID);

    if (!plugin)
        return false;

    auto supports = [this, &plugin, &format] (AkCaps::CapsType type,
                                              const QString &codec) -> bool {
        if (codec.isEmpty())
            return true;

        auto compressedType = type == AkCaps::CapsAudio?
                                  AkCompressedCaps::CapsType_Audio:
                                  AkCompressedCaps::CapsType_Video;

        return plugin->supportedCodecs(format, compressedType)
                .contains(this->m_encoding->encoderCodecID(type, codec));
    };

    return supports(AkCaps::CapsAudio, this->m_audioCodec)
           && supports(AkCaps::CapsVideo, this->m_videoCodec);
}

void RecordingPrivate::updateMuxer()
{
    auto fmt = this->findFormat(this->m_format);

    if (!fmt) {
        this->m_muxer = {};
        this->m_muxerPluginID.clear();

        return;
    }

    // Use the best plugin that supports the selected codecs.
    QString pluginID;

    for (auto &id: fmt->pluginIDs)
        if (this->muxerSupportsCodecs(id, fmt->name)) {
            pluginID = id;

            break;
        }

    if (pluginID.isEmpty())
        pluginID = fmt->pluginIDs.first();

    if (this->m_muxer
        && this->m_muxerPluginID == pluginID
        && this->m_muxer->muxer() == fmt->name)
        return;

    auto muxer = akPluginManager->create<AkVideoMuxer>(pluginID);

    if (!muxer) {
        qCritical() << "Failed to create the muxer:" << pluginID;
        this->m_muxer = {};
        this->m_muxerPluginID.clear();

        return;
    }

    muxer->setMuxer(fmt->name);
    this->m_muxer = muxer;
    this->m_muxerPluginID = pluginID;
    this->loadFormatOptions();
}

void RecordingPrivate::loadFormatCodecs()
{
    auto fmt = this->findFormat(this->m_format);

    if (!fmt)
        return;

    QSettings config;

    for (auto type: {AkCaps::CapsAudio, AkCaps::CapsVideo}) {
        auto key = type == AkCaps::CapsAudio? "audio": "video";

        config.beginGroup("RecordConfigs_FormatCodecs_"
                          + normatizePluginID(this->m_format));
        auto stored = config.value(key).toString();
        config.endGroup();

        // Previous versions stored them per muxer plugin.
        if (stored.isEmpty()) {
            config.beginGroup("RecordConfigs_FormatCodecs_"
                              + normatizePluginID(fmt->pluginIDs.first()
                                                  + ':'
                                                  + fmt->name));
            stored = config.value(key).toString();
            config.endGroup();
        }

        // Previous versions stored the encoder implementation, not the codec.
        if (stored.contains(':'))
            stored = this->m_encoding->encoderCodec(stored);

        auto supported = this->supportedCodecsFor(this->m_format, type);
        auto codec = supported.contains(stored)? stored: supported.value(0);
        auto &current = type == AkCaps::CapsAudio?
                            this->m_audioCodec:
                            this->m_videoCodec;

        if (current != codec) {
            current = codec;
            emit self->codecChanged(type, codec);
        }
    }
}

void RecordingPrivate::refreshCodecs()
{
    if (!this->m_isRecording) {
        if (!self->videoFormats().contains(this->m_format))
            self->setVideoFormat(this->defaultFormat());
        else
            this->loadFormatCodecs();

        this->updateMuxer();
    }

    emit self->supportedCodecsChanged();
}

void RecordingPrivate::printRecordingParameters()
{
    qInfo() << "Recording parameters:";
    qInfo() << "    Format:" << self->videoFormat();

    if (this->m_recordAudio) {
        qInfo() << "    Audio:";
        qInfo() << "        sample format:" << this->m_audioCaps.format();
        qInfo() << "        channels:" << this->m_audioCaps.channels();
        qInfo() << "        layout:" << this->m_audioCaps.layout();
        qInfo() << "        sample rate:" << this->m_audioCaps.rate();
        qInfo() << "        codec:" << self->codec(AkCaps::CapsAudio);
        qInfo() << "        encoder:" << this->m_encoding->encoder(AkCaps::CapsAudio, this->m_audioCodec);
        qInfo() << "        bitrate:" << this->m_encoding->audioBitrate();
    }

    qInfo() << "    Video:";
    qInfo() << "        pixel format:" << this->m_videoCaps.format();
    qInfo() << "        width:" << this->m_videoCaps.width();
    qInfo() << "        height:" << this->m_videoCaps.height();
    qInfo() << "        frame rate:" << this->m_videoCaps.fps().toString();
    qInfo() << "        codec:" << self->codec(AkCaps::CapsVideo);
    qInfo() << "        encoder:" << this->m_encoding->encoder(AkCaps::CapsVideo, this->m_videoCodec);
    qInfo() << "        bitrate:" << this->m_encoding->videoBitrate();
}

bool RecordingPrivate::init()
{
    qInfo() << "Starting recording";
    this->printRecordingParameters();

    if (!QDir().mkpath(this->m_videoDirectory)) {
        qCritical() << "Failed to create the directory:" << this->m_videoDirectory;

        return false;
    }

    // The settings of the encoders are shared with the other outputs and can
    // change at any time, so create the encoders for each recording.
    this->updateMuxer();

    if (!this->m_muxer) {
        qCritical() << "Muxer not set";

        return false;
    }

    this->m_videoEncoder = this->m_encoding->createVideoEncoder(this->m_videoCodec);

    if (!this->m_videoEncoder) {
        qCritical() << "Video codec not set";

        return false;
    }

    this->m_audioEncoder =
            this->m_audioCodec.isEmpty()?
                AkAudioEncoderPtr():
                this->m_encoding->createAudioEncoder(this->m_audioCodec);
    this->m_audioCaps.setRate(this->m_encoding->audioSampleRate());

    auto currentTime =
            QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
    auto location =
            QObject::tr("%1/Video %2.%3")
                .arg(this->m_videoDirectory,
                     currentTime,
                     this->m_muxer->extension(this->m_muxer->muxer()));
    this->m_muxer->setLocation(location);

    this->m_videoEncoder->setInputCaps(this->m_videoCaps);
    this->m_videoEncoder->setBitrate(this->m_encoding->videoBitrate());
    this->m_videoEncoder->setGop(this->m_encoding->videoGOP());
    this->m_videoEncoder->setFillGaps(!this->m_muxer->gapsAllowed(AkCompressedCaps::CapsType_Video));
    this->m_muxer->setStreamCaps(this->m_videoEncoder->outputCaps());
    this->m_muxer->setStreamBitrate(AkCompressedCaps::CapsType_Video,
                                    this->m_videoEncoder->bitrate());
    this->m_videoEncoder->link(this->m_muxer, Qt::DirectConnection);
    this->m_videoHeadersChangedConnection =
            QObject::connect(this->m_videoEncoder.data(),
                             &AkVideoEncoder::headersChanged,
                             [this] (const QByteArray &headers) {
                                this->m_muxer->setStreamHeaders(AkCompressedCaps::CapsType_Video,
                                                                headers);
                             });

    if (this->m_audioEncoder) {
        this->m_audioEncoder->setInputCaps(this->m_audioCaps);
        this->m_audioEncoder->setBitrate(this->m_encoding->audioBitrate());
        this->m_audioEncoder->setFillGaps(!this->m_muxer->gapsAllowed(AkCompressedCaps::CapsType_Audio));
        this->m_muxer->setStreamCaps(this->m_audioEncoder->outputCaps());
        this->m_muxer->setStreamBitrate(AkCompressedCaps::CapsType_Audio,
                                        this->m_audioEncoder->bitrate());
        this->m_audioEncoder->link(this->m_muxer, Qt::DirectConnection);
        this->m_audioHeadersChangedConnection =
                QObject::connect(this->m_audioEncoder.data(),
                                 &AkAudioEncoder::headersChanged,
                                 [this] (const QByteArray &headers) {
                                    this->m_muxer->setStreamHeaders(AkCompressedCaps::CapsType_Audio,
                                                                    headers);
                                 });

        this->m_audioEncoder->setState(AkElement::ElementStatePaused);
        this->m_muxer->setStreamHeaders(AkCompressedCaps::CapsType_Audio,
                                        this->m_audioEncoder->headers());
    }

    this->m_videoEncoder->setState(AkElement::ElementStatePaused);
    this->m_muxer->setStreamHeaders(AkCompressedCaps::CapsType_Video,
                                    this->m_videoEncoder->headers());
    this->m_muxer->setState(AkElement::ElementStatePlaying);

    if (this->m_audioEncoder)
        this->m_audioEncoder->setState(AkElement::ElementStatePlaying);

    this->m_videoEncoder->setState(AkElement::ElementStatePlaying);
    qInfo() << "Recording started";
    this->m_isRecording = true;

    return true;
}

void RecordingPrivate::uninit()
{
    if (!this->m_isRecording)
        return;

    qInfo() << "Stopping recording";
    this->m_isRecording = false;
    qint64 videoDuration = 0;
    qreal videoTime = 0.0;

    if (this->m_videoEncoder) {
        this->m_videoEncoder->setState(AkElement::ElementStateNull);
        videoDuration = this->m_videoEncoder->encodedTimePts();
        auto fps = this->m_videoEncoder->outputCaps().rawCaps().fps();
        videoTime = videoDuration / fps.value();
        QObject::disconnect(this->m_videoHeadersChangedConnection);
    }

    qint64 audioDuration = 0;
    qreal audioTime = 0.0;

    if (this->m_audioEncoder) {
        this->m_audioEncoder->setState(AkElement::ElementStateNull);
        audioDuration = this->m_audioEncoder->encodedTimePts();
        audioTime = qreal(audioDuration)
                    / this->m_audioEncoder->outputCaps().rawCaps().rate();
        QObject::disconnect(this->m_audioHeadersChangedConnection);
    }

    if (this->m_muxer) {
        if (audioDuration > 0)
            this->m_muxer->setStreamDuration(AkCompressedCaps::CapsType_Audio,
                                             audioDuration);

        if (videoDuration > 0)
            this->m_muxer->setStreamDuration(AkCompressedCaps::CapsType_Video,
                                             videoDuration);

        this->m_muxer->setState(AkElement::ElementStateNull);
    }

    auto duration = qMax(audioTime, videoTime);
    qInfo() << QString("Video duration: %1 (a: %2, v: %3)")
               .arg(duration)
               .arg(audioTime)
               .arg(videoTime)
               .toStdString().c_str();
    qInfo() << "Recording stopped";

    auto location = this->m_muxer->location();

    if (this->m_lastVideo != location) {
        this->readThumbnail(location);
        this->m_lastVideo = location;
        emit self->lastVideoChanged(location);
    }
}

QString RecordingPrivate::normatizePluginID(const QString &pluginID)
{
    static char const *videoRecordingValidPluginIDChars =
            "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    QString normalized;

    for (auto &c: pluginID) {
        auto count =
                std::count(videoRecordingValidPluginIDChars,
                           videoRecordingValidPluginIDChars
                           + strnlen(videoRecordingValidPluginIDChars, 64),
                           c);
        normalized += count > 0? c: '_';
    }

    return normalized;
}

void RecordingPrivate::loadConfigs()
{
    QSettings config;

    config.beginGroup("RecordConfigs");
    this->m_useFlash = config.value("useFlash", true).toBool();
    this->m_useVideoFlash = config.value("useVideoFlash", true).toBool();
    this->m_photoTimeout = config.value("photoTimeout", 0).toInt();
    config.endGroup();

    config.beginGroup("RecordConfigs");

    auto picturesPath =
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    auto moviesPath =
            QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);

    QString defaultImagesDirectory =
        picturesPath.isEmpty()?
            "":
            QDir(picturesPath).filePath(qApp->applicationName());
    QString defaultVideoDirectory =
        moviesPath.isEmpty()?
            "":
            QDir(moviesPath).filePath(qApp->applicationName());
    this->m_imagesDirectory =
            config.value("imagesDirectory", defaultImagesDirectory).toString();
    this->m_videoDirectory =
            config.value("videoDirectory", defaultVideoDirectory).toString();
    this->m_imageFormat = config.value("imageFormat", "png").toString();
    this->m_imageSaveQuality = config.value("imageSaveQuality", -1).toInt();
    this->m_recordAudio =
            config.value("recordAudio", DEFAULT_RECORD_AUDIO).toBool();

    // Configure the recording formats

    auto outputWidth = qMax(config.value("outputWidth", 1280).toInt(), 160);
    auto outputHeight = qMax(config.value("outputHeight", 720).toInt(), 90);
    auto outputFPS = qMax(config.value("outputFPS", 30).toInt(), 1);

    this->m_videoCaps = {AkVideoCaps::Format_yuv420p,
                         outputWidth,
                         outputHeight,
                         {outputFPS, 1}};
    this->m_audioCaps = {AkAudioCaps::SampleFormat_s16,
                         AkAudioCaps::Layout_stereo,
                         false,
                         this->m_encoding->audioSampleRate()};

    // Configure the format. Previous versions stored the muxer plugin too.

    auto videoFormat = config.value("format").toString();
    videoFormat = videoFormat.mid(videoFormat.lastIndexOf(':') + 1);
    config.endGroup();

    if (!self->videoFormats().contains(videoFormat))
        videoFormat = this->defaultFormat();

    if (!videoFormat.isEmpty()) {
        this->m_format = videoFormat;
        this->loadFormatCodecs();
        this->updateMuxer();
    }
}

void RecordingPrivate::loadFormatOptions()
{
    if (!this->m_muxer)
        return;

    emit self->videoFormatOptionsChanged(this->m_muxer->options());

    QSettings config;
    auto pluginID =
            this->normatizePluginID(this->m_muxerPluginID
                                    + ':'
                                    + this->m_muxer->muxer());
    config.beginGroup("RecordConfigs_FormatOptions_" + pluginID);

    for (auto &option: this->m_muxer->options())
        if (config.contains(option.name()))
            this->m_muxer->setOptionValue(option.name(),
                                          config.value(option.name()));

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

    for (auto &format: this->m_supportedFormats)
        nameFilters += "*." + format.extension;

    dir = QDir(this->m_videoDirectory);
    auto videos = dir.entryList(nameFilters,
                                QDir::Files | QDir::Readable,
                                QDir::Time);

    if (!videos.isEmpty()) {
        this->m_lastVideo = dir.filePath(videos.first());
        this->readThumbnail(this->m_lastVideo);
    }
}

void RecordingPrivate::readThumbnail(const QString &videoFile)
{
    if (!this->m_thumbnailer || videoFile.isEmpty())
        return;

    this->m_thumbnailer->setProperty("media", videoFile);
    this->m_thumbnailer->setProperty("sync", false);
}

void RecordingPrivate::thumbnailReady()
{
    this->m_thumbnailerMutex.lock();
    this->m_thumbnailer->setState(AkElement::ElementStateNull);
    this->m_thumbnailerMutex.unlock();

    auto tempPaths =
            QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    auto thumnailDir =
            tempPaths.isEmpty()?
                "":
                QDir(tempPaths).filePath(qApp->applicationName());

    if (thumnailDir.isEmpty())
        return;

    this->m_thumbnailMutex.lockForRead();
    auto thumbnail = this->m_thumbnail;
    this->m_thumbnailMutex.unlock();

    if (thumbnail.isNull() || !QDir().mkpath(thumnailDir))
        return;

    auto media = this->m_thumbnailer->property("media").toString();
    auto baseName = QFileInfo(media).baseName();

    /* NOTE: Saving in formats other than BMP can result in broken files that
     * can cause Qml to crash the whole app.
     */
    auto thumbnailPath = QString("%1/%2.%3")
                         .arg(thumnailDir,
                              baseName,
                              "bmp");

    if (!thumbnail.save(thumbnailPath,
                        nullptr,
                        this->m_imageSaveQuality))
        return;

    this->m_lastVideoPreview = thumbnailPath;
    emit self->lastVideoPreviewChanged(thumbnailPath);
}

void RecordingPrivate::saveAudioCaps(const AkAudioCaps &audioCaps)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("audioSampleRate", audioCaps.rate());
    config.endGroup();
}

void RecordingPrivate::saveVideoCaps(const AkVideoCaps &videoCaps)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("outputWidth", videoCaps.width());
    config.setValue("outputHeight", videoCaps.height());
    config.setValue("outputFPS", qRound(videoCaps.fps().value()));
    config.endGroup();
}

void RecordingPrivate::saveVideoDirectory(const QString &videoDirectory)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("videoDirectory", videoDirectory);
    config.endGroup();
}

void RecordingPrivate::saveVideoFormat(const QString &videoFormat)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("format", videoFormat);
    config.endGroup();
}

void RecordingPrivate::saveCodec(AkCaps::CapsType type, const QString &codec)
{
    QSettings config;
    config.beginGroup("RecordConfigs_FormatCodecs_"
                      + normatizePluginID(this->m_format));

    switch (type) {
    case AkCaps::CapsAudio:
        config.setValue("audio", codec);

        break;

    case AkCaps::CapsVideo:
        config.setValue("video", codec);

        break;

    default:
        break;
    }

    config.endGroup();
}

void RecordingPrivate::saveVideoFormatOptionValue(const QString &option,
                                                  const QVariant &value)
{

    QSettings config;
    auto pluginID = normatizePluginID(self->videoFormat());
    config.beginGroup("RecordConfigs_FormatOptions_" + pluginID);
    config.setValue(option, value);
    config.endGroup();
}

void RecordingPrivate::saveRecordAudio(bool recordAudio)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("recordAudio", recordAudio);
    config.endGroup();
}

void RecordingPrivate::saveUseVideoFlash(bool useVideoFlash)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("useVideoFlash", useVideoFlash);
    config.endGroup();
}

void RecordingPrivate::saveImagesDirectory(const QString &imagesDirectory)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("imagesDirectory", imagesDirectory);
    config.endGroup();
}

void RecordingPrivate::saveImageFormat(const QString &imageFormat)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("imageFormat", imageFormat);
    config.endGroup();
}

void RecordingPrivate::saveImageSaveQuality(int imageSaveQuality)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("imageSaveQuality", imageSaveQuality);
    config.endGroup();
}

void RecordingPrivate::saveUseFlash(bool useFlash)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("useFlash", useFlash);
    config.endGroup();
}

void RecordingPrivate::savePhotoTimeout(int photoTimeout)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("photoTimeout", photoTimeout);
    config.endGroup();
}

#include "moc_recording.cpp"

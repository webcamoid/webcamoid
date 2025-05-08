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

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/private/qandroidextras_p.h>

#define PERMISSION_GRANTED  0
#define PERMISSION_DENIED  -1
#endif

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

#define DEFAULT_AUDIO_BITRATE 128000
#define DEFAULT_VIDEO_BITRATE 1500000
#define DEFAULT_VIDEO_GOP 1000
#define DEFAULT_RECORD_AUDIO true

struct CodecInfo
{
    QString pluginID;
    AkCaps::CapsType type;
    AkCodecID codecID;
    QString name;
    QString description;
    int priority;
};

struct FormatInfo
{
    QString pluginID;
    AkVideoMuxer::FormatID formatID;
    QString name;
    QString description;
    QString extension;
    QStringList audioPluginsID;
    QStringList videoPluginsID;
    QString defaultAudioPluginID;
    QString defaultVideoPluginID;
};

struct PluginPriority
{
    QString pluginID;
    int priority;
};

using ObjectPtr = QSharedPointer<QObject>;

class RecordingPrivate
{
    public:
        Recording *self;
        QQmlApplicationEngine *m_engine {nullptr};
        AkAudioCaps m_audioCaps;
        AkVideoCaps m_videoCaps;
        int m_audioBitrate {DEFAULT_AUDIO_BITRATE};
        int m_videoBitrate {DEFAULT_VIDEO_BITRATE};
        int m_videoGOP {1000};
        QVector<CodecInfo> m_supportedCodecs;
        QVector<FormatInfo> m_supportedFormats;
        QString m_defaultFormat;
        AkVideoMuxerPtr m_muxer;
        QString m_muxerPluginID;
        AkAudioEncoderPtr m_audioEncoder;
        QString m_audioPluginID;
        AkVideoEncoderPtr m_videoEncoder;
        QString m_videoPluginID;
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
        QMutex m_mutex;
        QReadWriteLock m_thumbnailMutex;
        QMutex m_thumbnailerMutex;
        QThreadPool m_threadPool;
        AkVideoPacket m_curPacket;
        QImage m_photo;
        QImage m_thumbnail;
        QMap<QString, QString> m_imageFormats;
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        int m_imageSaveQuality {-1};
        bool m_recordAudio {DEFAULT_RECORD_AUDIO};
        bool m_isRecording {false};
        bool m_pause {false};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit RecordingPrivate(Recording *self);
        static bool canAccessStorage();
        inline void initSupportedCodecs();
        inline void initSupportedFormats();
        QString defaultCodec(const QString &format, AkCaps::CapsType type) const;
        void printRecordingParameters();
        bool init();
        void uninit();
        static QString normatizePluginID(const QString &pluginID);
        void loadConfigs();
        void loadFormatOptions();
        void loadCodecOptions(AkCaps::CapsType type);
        void updatePreviews();
        void readThumbnail(const QString &videoFile);
        void thumbnailReady();

#ifdef Q_OS_ANDROID
        static QString androidCopyUriToTemp(const QString &uri,
                                            const QString &outputFileName={});
        static QJniObject createContentValues(const QString &filePath,
                                              bool isVideo);
        static QString createDirectoryForLegacyStorage(const QString &appFolderPath,
                                                       const QString &outputFileName);
        static QJniObject getExternalContentUri(bool isVideo);
        static bool copyFileToMediaStore(QFile &file, QJniObject &outputStream);
        static bool updateIsPending(QJniObject &contentResolver,
                                    const QJniObject &uri);
#endif

        static bool saveMediaFileToGallery(const QString &filePath,
                                           bool isVideo);
        static QString getLatestMediaUri(bool isVideo);

        // General options
        void saveAudioCaps(const AkAudioCaps &audioCaps);
        void saveVideoCaps(const AkVideoCaps &videoCaps);

        // Video
        void saveVideoDirectory(const QString &videoDirectory);
        void saveVideoFormat(const QString &videoFormat);
        void saveCodec(AkCaps::CapsType type, const QString &codec);
        void saveVideoFormatOptionValue(const QString &option,
                                        const QVariant &value);
        void saveCodecOptionValue(AkCaps::CapsType type,
                                  const QString &option,
                                  const QVariant &value);
        void saveBitrate(AkCaps::CapsType type, int bitrate);
        void saveVideoGOP(int gop);
        void saveRecordAudio(bool recordAudio);

        // Picture
        void saveImagesDirectory(const QString &imagesDirectory);
        void saveImageFormat(const QString &imageFormat);
        void saveImageSaveQuality(int imageSaveQuality);
};

Recording::Recording(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new RecordingPrivate(this);
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

    this->d->loadConfigs();
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

QString Recording::videoFormat() const
{
    if (!this->d->m_muxer)
        return {};

    return this->d->m_muxerPluginID + ':' + this->d->m_muxer->muxer();
}

QStringList Recording::videoFormats() const
{
    QStringList formats;

    for (auto &format: this->d->m_supportedFormats)
        formats << format.pluginID + ':' + format.name;

    return formats;
}

QString Recording::defaultVideoFormat() const
{
    return this->d->m_defaultFormat;
}

QString Recording::formatDescription(const QString &format) const
{
    auto formatParts = format.split(':');

    if (formatParts.size() < 2)
        return {};

    auto pluginID = formatParts[0];
    auto muxerID = formatParts[1];

    auto it = std::find_if(this->d->m_supportedFormats.begin(),
                           this->d->m_supportedFormats.end(),
                           [&pluginID, &muxerID] (const FormatInfo &formatInfo) -> bool {
        return formatInfo.pluginID == pluginID && formatInfo.name == muxerID;
    });

    if (it == this->d->m_supportedFormats.end())
        return {};

    return it->description;
}

QString Recording::codec(AkCaps::CapsType type) const
{
    switch (type) {
    case AkCaps::CapsAudio:
        if (!this->d->m_audioEncoder)
            return {};

        return this->d->m_audioPluginID + ':' + this->d->m_audioEncoder->codec();

    case AkCaps::CapsVideo:
        if (!this->d->m_videoEncoder)
            return {};

        return this->d->m_videoPluginID + ':' + this->d->m_videoEncoder->codec();

    default:
        break;
    }

    return {};
}

QString Recording::defaultCodec(const QString &format,
                                AkCaps::CapsType type) const
{
    auto formatParts = format.split(':');

    if (formatParts.size() < 2)
        return {};

    auto pluginID = formatParts[0];
    auto muxerID = formatParts[1];

    auto it = std::find_if(this->d->m_supportedFormats.begin(),
                           this->d->m_supportedFormats.end(),
                           [&pluginID, &muxerID] (const FormatInfo &formatInfo) -> bool {
        return formatInfo.pluginID == pluginID && formatInfo.name == muxerID;
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

QStringList Recording::supportedCodecs(const QString &format,
                                       AkCaps::CapsType type) const
{
    auto formatParts = format.split(':');

    if (formatParts.size() < 2)
        return {};

    auto pluginID = formatParts[0];
    auto muxerID = formatParts[1];

    auto it = std::find_if(this->d->m_supportedFormats.begin(),
                           this->d->m_supportedFormats.end(),
                           [&pluginID, &muxerID] (const FormatInfo &formatInfo) -> bool {
        return formatInfo.pluginID == pluginID && formatInfo.name == muxerID;
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

QString Recording::codecDescription(const QString &codec) const
{
    auto codecParts = codec.split(':');

    if (codecParts.size() < 2)
        return {};

    auto pluginID = codecParts[0];
    auto codecID = codecParts[1];

    auto it = std::find_if(this->d->m_supportedCodecs.begin(),
                           this->d->m_supportedCodecs.end(),
                           [&pluginID, &codecID] (const CodecInfo &codecInfo) -> bool {
        return codecInfo.pluginID == pluginID && codecInfo.name == codecID;
    });

    if (it == this->d->m_supportedCodecs.end())
        return {};

    return it->description;
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

AkPropertyOptions Recording::codecOptions(AkCaps::CapsType type) const
{
    switch (type) {
    case AkCaps::CapsAudio:
        if (!this->d->m_audioEncoder)
            return {};

        return this->d->m_audioEncoder->options();

    case AkCaps::CapsVideo:
        if (!this->d->m_videoEncoder)
            return {};

        return this->d->m_videoEncoder->options();

    default:
        break;
    }

    return {};
}

QVariant Recording::codecOptionValue(AkCaps::CapsType type,
                                     const QString &option) const
{
    switch (type) {
    case AkCaps::CapsAudio:
        if (!this->d->m_audioEncoder)
            return {};

        return this->d->m_audioEncoder->optionValue(option);

    case AkCaps::CapsVideo:
        if (!this->d->m_videoEncoder)
            return {};

        return this->d->m_videoEncoder->optionValue(option);

    default:
        break;
    }

    return {};
}

int Recording::bitrate(AkCaps::CapsType type) const
{
    switch (type) {
    case AkCaps::CapsAudio:
        return this->d->m_audioBitrate;

    case AkCaps::CapsVideo:
        return this->d->m_videoBitrate;

    default:
        break;
    }

    return {};
}

int Recording::defaultBitrate(AkCaps::CapsType type) const
{
    switch (type) {
    case AkCaps::CapsAudio:
        return DEFAULT_AUDIO_BITRATE;

    case AkCaps::CapsVideo:
        return DEFAULT_VIDEO_BITRATE;

    default:
        break;
    }

    return 0;
}

int Recording::videoGOP() const
{
    return this->d->m_videoGOP;
}

int Recording::defaultVideoGOP() const
{
    return DEFAULT_VIDEO_GOP;
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
    auto curFormat =
            this->d->m_muxer?
                this->d->m_muxerPluginID + ':' + this->d->m_muxer->muxer():
                QString();

    if (videoFormat == curFormat)
        return;

    auto formatParts = videoFormat.split(':');
    auto formatPluginID = formatParts.value(0);
    auto formatName = formatParts.value(1);

    auto muxer = akPluginManager->create<AkVideoMuxer>(formatPluginID);

    if (muxer)
        muxer->setMuxer(formatName);
    else
        qCritical() << "Failed to create the muxer:" << formatPluginID;

    this->d->m_muxer = muxer;
    this->d->m_muxerPluginID = formatPluginID;
    emit this->videoFormatChanged(videoFormat);
    this->d->saveVideoFormat(videoFormat);
    this->d->loadFormatOptions();
}

void Recording::setCodec(AkCaps::CapsType type, const QString &codec)
{
    switch (type) {
    case AkCaps::CapsAudio: {
        auto curCodec =
                this->d->m_audioEncoder?
                    this->d->m_audioPluginID + ':' + this->d->m_audioEncoder->codec():
                    QString();

        if (codec == curCodec)
            return;

        auto codecParts = codec.split(':');
        auto codecPluginID = codecParts.value(0);
        auto codecName = codecParts.value(1);

        auto encoder = akPluginManager->create<AkAudioEncoder>(codecPluginID);

        if (encoder)
            encoder->setCodec(codecName);
        else
            qDebug() << "Failed to create the muxer:" << codecPluginID;

        this->d->m_audioEncoder = encoder;
        this->d->m_audioPluginID = codecPluginID;
        emit this->codecChanged(type, codec);
        this->d->saveCodec(type, codec);
        this->d->loadCodecOptions(AkCaps::CapsAudio);

        break;
    }

    case AkCaps::CapsVideo: {
        auto curCodec =
                this->d->m_videoEncoder?
                    this->d->m_videoPluginID + ':' + this->d->m_videoEncoder->codec():
                    QString();

        if (codec == curCodec)
            return;

        auto codecParts = codec.split(':');
        auto codecPluginID = codecParts.value(0);
        auto codecName = codecParts.value(1);

        auto encoder = akPluginManager->create<AkVideoEncoder>(codecPluginID);

        if (encoder)
            encoder->setCodec(codecName);
        else
            qDebug() << "Failed to create the muxer:" << codecPluginID;

        this->d->m_videoEncoder = encoder;
        this->d->m_videoPluginID = codecPluginID;
        emit this->codecChanged(type, codec);
        this->d->saveCodec(type, codec);
        this->d->loadCodecOptions(AkCaps::CapsVideo);

        break;
    }

    default:
        break;
    }
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

void Recording::setCodecOptionValue(AkCaps::CapsType type,
                                    const QString &option,
                                    const QVariant &value)
{
    switch (type) {
    case AkCaps::CapsAudio:
        if (!this->d->m_audioEncoder)
            return;

        if (this->d->m_audioEncoder->optionValue(option) == value)
            return;

        this->d->m_audioEncoder->setOptionValue(option, value);
        emit this->codecOptionValueChanged(type, option, value);
        this->d->saveCodecOptionValue(type, option, value);

        break;

    case AkCaps::CapsVideo:
        if (!this->d->m_videoEncoder)
            return;

        if (this->d->m_videoEncoder->optionValue(option) == value)
            return;

        this->d->m_videoEncoder->setOptionValue(option, value);
        emit this->codecOptionValueChanged(type, option, value);
        this->d->saveCodecOptionValue(type, option, value);

        break;

    default:
        break;
    }
}

void Recording::setBitrate(AkCaps::CapsType type, int bitrate)
{
    switch (type) {
    case AkCaps::CapsAudio:
        if (this->d->m_audioBitrate == bitrate)
            return;

        this->d->m_audioBitrate = bitrate;
        emit this->bitrateChanged(type, bitrate);
        this->d->saveBitrate(type, bitrate);

        break;

    case AkCaps::CapsVideo:
        if (!this->d->m_videoEncoder)
            return;

        if (this->d->m_videoBitrate == bitrate)
            return;

        this->d->m_videoBitrate = bitrate;
        emit this->bitrateChanged(type, bitrate);
        this->d->saveBitrate(type, bitrate);

        break;

    default:
        break;
    }
}

void Recording::setVideoGOP(int gop)
{
    if (this->d->m_videoGOP == gop)
        return;

    this->d->m_videoGOP = gop;
    emit this->videoGOPChanged(gop);
    this->d->saveVideoGOP(gop);
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
    auto moviesPath =
            QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    QString dir = moviesPath.isEmpty()?
                      "":
                      QDir(moviesPath).filePath(qApp->applicationName());
    this->setVideoDirectory(dir);
}

void Recording::resetVideoFormat()
{
    this->setVideoFormat(this->d->m_defaultFormat);
}

void Recording::resetCodec(AkCaps::CapsType type)
{
    this->setCodec(type, this->d->defaultCodec(this->videoFormat(), type));
}

void Recording::resetVideoFormatOptionValue(const QString &option)
{
    this->setVideoFormatOptionValue(option, this->videoFormatOptionValue(option));
}

void Recording::resetCodecOptionValue(AkCaps::CapsType type,
                                      const QString &option)
{
    this->setCodecOptionValue(type, option, this->codecOptionValue(type,
                                                                   option));
}

void Recording::resetVideoFormatOptions()
{
    for (auto &option: this->videoFormatOptions())
        this->resetVideoFormatOptionValue(option.name());
}

void Recording::resetCodecOptions(AkCaps::CapsType type)
{
    for (auto &option: this->codecOptions(type))
        this->resetCodecOptionValue(type, option.name());
}

void Recording::resetBitrate(AkCaps::CapsType type)
{
    int bitrate = type == AkCaps::CapsVideo?
                      DEFAULT_VIDEO_BITRATE:
                      DEFAULT_AUDIO_BITRATE;

    this->setBitrate(type, bitrate);
}

void Recording::resetVideoGOP()
{
    this->setVideoGOP(DEFAULT_VIDEO_GOP);
}

void Recording::resetRecordAudio()
{
    this->setRecordAudio(DEFAULT_RECORD_AUDIO);
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

void Recording::takePhoto()
{
    this->d->m_mutex.lock();

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(this->d->m_curPacket);
    this->d->m_videoConverter.end();

    this->d->m_photo = QImage(src.caps().width(),
                              src.caps().height(),
                              QImage::Format_ARGB32);
    auto lineSize =
            qMin<size_t>(src.lineSize(0), this->d->m_photo.bytesPerLine());

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = src.constLine(0, y);
        auto dstLine = this->d->m_photo.scanLine(y);
        memcpy(dstLine, srcLine, lineSize);
    }

    this->d->m_mutex.unlock();
}

void Recording::savePhoto(const QString &fileName)
{
    if (!this->d->canAccessStorage())
        return;

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

#ifdef Q_OS_ANDROID
    if (RecordingPrivate::saveMediaFileToGallery(path, false)) {
        this->d->m_latestPhotoUri = RecordingPrivate::getLatestMediaUri(false);

        if (!this->d->m_latestPhotoUri.isEmpty()) {
            auto latestPhoto =
                    RecordingPrivate::androidCopyUriToTemp(this->d->m_latestPhotoUri,
                                                           "photo");

            if (!latestPhoto.isEmpty()) {
                this->d->m_lastPhotoPreview = latestPhoto;
                emit this->lastPhotoPreviewChanged(latestPhoto);
                emit this->latestPhotoUriChanged(this->d->m_latestPhotoUri);
            } else {
                this->d->m_lastPhotoPreview = {};
                emit this->lastPhotoPreviewChanged(this->d->m_lastPhotoPreview);
                this->d->m_latestPhotoUri = {};
                emit this->latestPhotoUriChanged(this->d->m_latestPhotoUri);
            }
        }
    } else {
        this->d->m_lastPhotoPreview = {};
        emit this->lastPhotoPreviewChanged(this->d->m_lastPhotoPreview);
        this->d->m_latestPhotoUri = {};
        emit this->latestPhotoUriChanged(this->d->m_latestPhotoUri);
    }

    if (QFile::exists(path))
        QFile::remove(path);
#else
    this->d->m_lastPhotoPreview = path;
    emit this->lastPhotoPreviewChanged(path);
#endif
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
    if (packet.type() == AkPacket::PacketVideo) {
        this->d->m_mutex.lock();
        this->d->m_curPacket = packet;
        this->d->m_mutex.unlock();
    }

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
        {"bmp" , "Windows Bitmap (BMP)"                       },
        {"cur" , "Microsoft Windows Cursor (CUR)"             },
        {"icns", "Apple Icon Image (ICNS)"                    },
        {"ico" , "Microsoft Windows Icon (ICO)"               },
        {"jp2" , "Joint Photographic Experts Group 2000 (JP2)"},
        {"jpg" , "Joint Photographic Experts Group (JPEG)"    },
        {"pbm" , "Portable Bitmap (PBM)"                      },
        {"pgm" , "Portable Graymap (PGM)"                     },
        {"png" , "Portable Network Graphics (PNG)"            },
        {"ppm" , "Portable Pixmap (PPM)"                      },
        {"tiff", "Tagged Image File Format (TIFF)"            },
        {"wbmp", "Wireless Bitmap (WBMP)"                     },
        {"webp", "WebP (WEBP)"                                },
        {"xbm" , "X11 Bitmap (XBM)"                           },
        {"xpm" , "X11 Pixmap (XPM)"                           },
    };

    static const QMap<QString, QString> recordingFormatsMapping {
        {"jpeg", "jpg" },
        {"tif" , "tiff"},
    };

    for (auto &format: QImageWriter::supportedImageFormats()) {
        QString fmt = format;

        if (recordingFormatsMapping.contains(fmt))
            fmt = recordingFormatsMapping[fmt];

        if (this->m_imageFormats.contains(fmt))
            continue;

        if (formatsDescription.contains(fmt))
            this->m_imageFormats[fmt] = formatsDescription[fmt];
        else
            this->m_imageFormats[fmt] = fmt.toUpper();
    }

    this->initSupportedCodecs();
    this->initSupportedFormats();
}

bool RecordingPrivate::canAccessStorage()
{
#ifdef Q_OS_ANDROID
    static bool done = false;
    static bool result = false;

    if (done)
        return result;

    if (android_get_device_api_level() >= 29) {
        done = true;
        result = true;

        return result;
    }

    QJniObject context =
        qApp->nativeInterface<QNativeInterface::QAndroidApplication>()->context();

    if (!context.isValid()) {
        done = false;

        return result;
    }

    QStringList permissions {
        "android.permission.WRITE_EXTERNAL_STORAGE"
    };
    QStringList neededPermissions;

    for (auto &permission: permissions) {
        auto permissionStr = QJniObject::fromString(permission);
        auto result =
            context.callMethod<jint>("checkSelfPermission",
                                     "(Ljava/lang/String;)I",
                                     permissionStr.object());

        if (result != PERMISSION_GRANTED)
            neededPermissions << permission;
    }

    if (!neededPermissions.isEmpty()) {
        QJniEnvironment jniEnv;
        jobjectArray permissionsArray =
            jniEnv->NewObjectArray(permissions.size(),
                                   jniEnv->FindClass("java/lang/String"),
                                   nullptr);
        int i = 0;

        for (auto &permission: permissions) {
            auto permissionObject = QJniObject::fromString(permission);
            jniEnv->SetObjectArrayElement(permissionsArray,
                                          i,
                                          permissionObject.object());
            i++;
        }

        context.callMethod<void>("requestPermissions",
                                 "([Ljava/lang/String;I)V",
                                 permissionsArray,
                                 jint(Ak::id()));
        QElapsedTimer timer;
        timer.start();
        static const int timeout = 5000;

        while (timer.elapsed() < timeout) {
            bool permissionsGranted = true;

            for (auto &permission: permissions) {
                auto permissionStr = QJniObject::fromString(permission);
                auto result =
                    context.callMethod<jint>("checkSelfPermission",
                                             "(Ljava/lang/String;)I",
                                             permissionStr.object());

                if (result != PERMISSION_GRANTED) {
                    permissionsGranted = false;

                    break;
                }
            }

            if (permissionsGranted)
                break;

            auto eventDispatcher = QThread::currentThread()->eventDispatcher();

            if (eventDispatcher)
                eventDispatcher->processEvents(QEventLoop::AllEvents);
        }
    }

    done = true;
    result = true;
#endif

    return true;
}

void RecordingPrivate::initSupportedCodecs()
{
    this->m_supportedCodecs.clear();

    auto audioEncoders =
            akPluginManager->listPlugins("^AudioEncoder([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);

    for (auto &encoder: audioEncoders) {
        auto codecPlugin = akPluginManager->create<AkAudioEncoder>(encoder);
        auto codecInfo = akPluginManager->pluginInfo(encoder);

        for (auto &codec: codecPlugin->codecs())
            this->m_supportedCodecs << CodecInfo {encoder,
                                                  AkCaps::CapsAudio,
                                                  codecPlugin->codecID(codec),
                                                  codec,
                                                  codecPlugin->codecDescription(codec),
                                                  codecInfo.priority()};
    }

    auto videoEncoders =
            akPluginManager->listPlugins("^VideoEncoder([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);

    for (auto &encoder: videoEncoders) {
        auto codecPlugin = akPluginManager->create<AkVideoEncoder>(encoder);
        auto codecInfo = akPluginManager->pluginInfo(encoder);

        for (auto &codec: codecPlugin->codecs())
            this->m_supportedCodecs << CodecInfo {encoder,
                                                  AkCaps::CapsVideo,
                                                  codecPlugin->codecID(codec),
                                                  codec,
                                                  codecPlugin->codecDescription(codec),
                                                  codecInfo.priority()};
    }

    std::sort(this->m_supportedCodecs.begin(),
              this->m_supportedCodecs.end(),
              [] (const CodecInfo &ci1, const CodecInfo &ci2) {
        return ci1.description < ci2.description;
    });
}

void RecordingPrivate::initSupportedFormats()
{
    this->m_supportedFormats.clear();

    auto muxerPlugins =
            akPluginManager->listPlugins("^VideoMuxer([/]([0-9a-zA-Z_])+)+$",
                                         {},
                                         AkPluginManager::FilterEnabled
                                         | AkPluginManager::FilterRegexp);
    QVector<PluginPriority> formatsPriority;

    for (auto &muxerPluginId: muxerPlugins) {
        auto muxerInfo = akPluginManager->pluginInfo(muxerPluginId);
        auto muxerPlugin = akPluginManager->create<AkVideoMuxer>(muxerPluginId);

        for (auto &muxer: muxerPlugin->muxers()) {
            QVector<PluginPriority> codecsPriority;
            QVector<QString> audioPluginsID;
            auto supportedAudioCodecs =
                    muxerPlugin->supportedCodecs(muxer,
                                                 AkCompressedCaps::CapsType_Audio);
            auto defaultAudioCodec =
                    muxerPlugin->defaultCodec(muxer,
                                              AkCompressedCaps::CapsType_Audio);

            for (auto &codec: this->m_supportedCodecs)
                if (supportedAudioCodecs.contains(codec.codecID)
                    && codec.type == AkCaps::CapsAudio) {
                    auto id = codec.pluginID + ':' + codec.name;
                    audioPluginsID << id;

                    if (codec.codecID == defaultAudioCodec)
                        codecsPriority << PluginPriority {id, codec.priority};
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
                    muxerPlugin->supportedCodecs(muxer,
                                                 AkCompressedCaps::CapsType_Video);
            auto defaultVideoCodec =
                    muxerPlugin->defaultCodec(muxer,
                                              AkCompressedCaps::CapsType_Video);

            for (auto &codec: this->m_supportedCodecs)
                if (supportedVideoCodecs.contains(codec.codecID)
                    && codec.type == AkCaps::CapsVideo) {
                    auto id = codec.pluginID + ':' + codec.name;
                    videoPluginsID << id;

                    if (codec.codecID == defaultVideoCodec)
                        codecsPriority << PluginPriority {id, codec.priority};
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
                muxerPluginId,
                muxerPlugin->formatID(muxer),
                muxer,
                muxerPlugin->description(muxer),
                muxerPlugin->extension(muxer),
                audioPluginsID,
                videoPluginsID,
                defaultAudioPluginID,
                defaultVideoPluginID
            };

            formatsPriority << PluginPriority {muxerPluginId + ':' + muxer,
                                               muxerInfo.priority()};
        }
    }

    std::sort(this->m_supportedFormats.begin(),
              this->m_supportedFormats.end(),
              [] (const FormatInfo &fi1, const FormatInfo &fi2) {
        return fi1.description < fi2.description;
    });

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

QString RecordingPrivate::defaultCodec(const QString &format, AkCaps::CapsType type) const
{
    auto formatParts = format.split(':');

    if (formatParts.size() < 2)
        return {};

    auto pluginID = formatParts[0];
    auto muxerID = formatParts[1];

    auto it = std::find_if(this->m_supportedFormats.begin(),
                           this->m_supportedFormats.end(),
                           [&pluginID, &muxerID] (const FormatInfo &formatInfo) -> bool {
        return formatInfo.pluginID == pluginID && formatInfo.name == muxerID;
    });

    if (it == this->m_supportedFormats.end())
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
        qInfo() << "        bitrate:" << this->m_audioBitrate;
    }

    qInfo() << "    Video:";
    qInfo() << "        pixel format:" << this->m_videoCaps.format();
    qInfo() << "        width:" << this->m_videoCaps.width();
    qInfo() << "        height:" << this->m_videoCaps.height();
    qInfo() << "        frame rate:" << this->m_videoCaps.fps().toString();
    qInfo() << "        codec:" << self->codec(AkCaps::CapsVideo);
    qInfo() << "        bitrate:" << this->m_videoBitrate;
}

bool RecordingPrivate::init()
{
    qInfo() << "Starting recording";
    this->printRecordingParameters();

    if (!QDir().mkpath(this->m_videoDirectory)) {
        qCritical() << "Failed to create the directory:" << this->m_videoDirectory;

        return false;
    }

    if (!this->m_muxer) {
        qCritical() << "Muxer not set";

        return false;
    }

    if (!this->m_videoEncoder) {
        qCritical() << "Video codec not set";

        return false;
    }

    auto currentTime =
            QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
    auto location =
            QObject::tr("%1/Video %2.%3")
                .arg(this->m_videoDirectory,
                     currentTime,
                     this->m_muxer->extension(this->m_muxer->muxer()));
    this->m_muxer->setLocation(location);

    this->m_videoEncoder->setInputCaps(this->m_videoCaps);
    this->m_videoEncoder->setBitrate(this->m_videoBitrate);
    this->m_videoEncoder->setGop(this->m_videoGOP);
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
        this->m_audioEncoder->setBitrate(this->m_audioBitrate);
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

#ifdef Q_OS_ANDROID
    if (RecordingPrivate::saveMediaFileToGallery(location, true)) {
        this->m_latestVideoUri = RecordingPrivate::getLatestMediaUri(true);

        if (!this->m_latestVideoUri.isEmpty()) {
            auto latestVideo =
                    RecordingPrivate::androidCopyUriToTemp(this->m_latestVideoUri,
                                                           "video");

            if (!latestVideo.isEmpty()) {
                this->readThumbnail(latestVideo);
                this->m_lastVideo = latestVideo;
                emit self->lastVideoChanged(latestVideo);
                emit self->latestVideoUriChanged(this->m_latestVideoUri);
            } else {
                this->readThumbnail({});
                this->m_lastVideo = {};
                emit self->lastVideoChanged(this->m_lastVideo);
                this->m_latestVideoUri = {};
                emit self->latestVideoUriChanged(this->m_latestVideoUri);
            }
        }
    } else {
        this->readThumbnail({});
        this->m_lastVideo = {};
        emit self->lastVideoChanged(this->m_lastVideo);
        this->m_latestVideoUri = {};
        emit self->latestVideoUriChanged(this->m_latestVideoUri);
    }

    if (QFile::exists(location))
        QFile::remove(location);
#else
    if (this->m_lastVideo != location) {
        this->readThumbnail(location);
        this->m_lastVideo = location;
        emit self->lastVideoChanged(location);
    }
#endif
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
    auto audioSampleRate = qMax(config.value("audioSampleRate", 48000).toInt(), 8000);

    this->m_videoCaps = {AkVideoCaps::Format_yuv420p,
                         outputWidth,
                         outputHeight,
                         {outputFPS, 1}};
    this->m_audioCaps = {AkAudioCaps::SampleFormat_s16,
                         AkAudioCaps::Layout_stereo,
                         false,
                         audioSampleRate};

    this->m_audioBitrate = qMax(config.value("audioBitrate", DEFAULT_AUDIO_BITRATE).toInt(), 1000);
    this->m_videoBitrate = qMax(config.value("videoBitrate", DEFAULT_VIDEO_BITRATE).toInt(), 100000);
    this->m_videoGOP = qMax(config.value("videoGOP", DEFAULT_VIDEO_GOP).toInt(), 1);

    // Configure the format

    auto videoFormat =
            config.value("format", this->m_defaultFormat).toString();
    auto formatParts = videoFormat.split(':');
    auto formatPluginID = formatParts.value(0);
    auto formatName = formatParts.value(1);

    if (!formatPluginID.isEmpty() && !formatName.isEmpty()) {
        auto muxer = akPluginManager->create<AkVideoMuxer>(formatPluginID);

        if (muxer && muxer->muxers().contains(formatName)) {
            muxer->setMuxer(formatName);
            this->m_muxer = muxer;
            this->m_muxerPluginID = formatPluginID;
            this->loadFormatOptions();
        }
    }

    config.endGroup();

    // Configure the codecs

    auto videoFormatID = normatizePluginID(videoFormat);
    config.beginGroup("RecordConfigs_FormatCodecs_" + videoFormatID);

    auto audioCodec =
            config.value("audio",
                         this->defaultCodec(videoFormat,
                                            AkCaps::CapsAudio)).toString();
    auto audioCodecParts = audioCodec.split(':');
    auto audioCodecPluginID = audioCodecParts.value(0);
    auto audioCodecName = audioCodecParts.value(1);

    if (!audioCodecPluginID.isEmpty() && !audioCodecName.isEmpty()) {
        auto encoder = akPluginManager->create<AkAudioEncoder>(audioCodecPluginID);

        if (encoder && encoder->codecs().contains(audioCodecName)) {
            encoder->setCodec(audioCodecName);
            this->m_audioEncoder = encoder;
            this->m_audioPluginID = audioCodecPluginID;
            this->loadCodecOptions(AkCaps::CapsAudio);
        }
    }

    auto videoCodec =
            config.value("video",
                         this->defaultCodec(videoFormat,
                                            AkCaps::CapsVideo)).toString();
    auto videoCodecParts = videoCodec.split(':');
    auto videoCodecPluginID = videoCodecParts.value(0);
    auto videoCodecName = videoCodecParts.value(1);

    if (!videoCodecPluginID.isEmpty() && !videoCodecName.isEmpty()) {
        auto encoder = akPluginManager->create<AkVideoEncoder>(videoCodecPluginID);

        if (encoder && encoder->codecs().contains(videoCodecName)) {
            encoder->setCodec(videoCodecName);
            this->m_videoEncoder = encoder;
            this->m_videoPluginID = videoCodecPluginID;
            this->loadCodecOptions(AkCaps::CapsVideo);
        }
    }

    config.endGroup();
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

void RecordingPrivate::loadCodecOptions(AkCaps::CapsType type)
{
    switch (type) {
    case AkCaps::CapsAudio: {
        if (!this->m_audioEncoder)
            return;

        emit self->codecOptionsChanged(type, this->m_audioEncoder->options());

        QSettings config;
        auto pluginID =
                this->normatizePluginID(this->m_audioPluginID
                                        + ':'
                                        + this->m_audioEncoder->codec());
        config.beginGroup("RecordConfigs_AudioCodecOptions_" + pluginID);

        for (auto &option: this->m_audioEncoder->options())
            if (config.contains(option.name()))
                this->m_audioEncoder->setOptionValue(option.name(),
                                                     config.value(option.name()));

        config.endGroup();

        return;
    }

    case AkCaps::CapsVideo: {
        if (!this->m_videoEncoder)
            return;

        emit self->codecOptionsChanged(type, this->m_videoEncoder->options());

        QSettings config;
        auto pluginID =
                this->normatizePluginID(this->m_videoPluginID
                                        + ':'
                                        + this->m_videoEncoder->codec());
        config.beginGroup("RecordConfigs_VideoCodecOptions_" + pluginID);

        for (auto &option: this->m_videoEncoder->options())
            if (config.contains(option.name()))
                this->m_videoEncoder->setOptionValue(option.name(),
                                                     config.value(option.name()));

        config.endGroup();

        return;
    }

    default:
        break;
    }
}

void RecordingPrivate::updatePreviews()
{
    if (!this->canAccessStorage())
        return;

    // Update photo preview

#ifdef Q_OS_ANDROID
    this->m_latestPhotoUri = getLatestMediaUri(false);

    if (!this->m_latestPhotoUri.isEmpty()) {
        auto latestPhoto = androidCopyUriToTemp(this->m_latestPhotoUri, "photo");

        if (!latestPhoto.isEmpty())
            this->m_lastPhotoPreview = latestPhoto;
    }
#else
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
#endif

    // Update video preview

#ifdef Q_OS_ANDROID
    this->m_latestVideoUri = getLatestMediaUri(true);

    if (!this->m_latestVideoUri.isEmpty()) {
        auto latestVideo = androidCopyUriToTemp(this->m_latestVideoUri, "video");

        if (!latestVideo.isEmpty()) {
            this->m_lastVideo = latestVideo;
            this->readThumbnail(this->m_lastVideo);
        }
    }
#else
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
#endif
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

#ifdef Q_OS_ANDROID
QString RecordingPrivate::androidCopyUriToTemp(const QString &uri,
                                               const QString &outputFileName)
{
    if (!uri.startsWith("content://"))
        return {};

    auto urlStr = QJniObject::fromString(uri);

    // Parse the URI

    auto mediaUri =
            QJniObject::callStaticObjectMethod("android/net/Uri",
                                               "parse",
                                               "(Ljava/lang/String;)Landroid/net/Uri;",
                                               urlStr.object());

    // Get ContentResolver

    auto context = QJniObject(QNativeInterface::QAndroidApplication::context());
    auto contentResolver =
            context.callObjectMethod("getContentResolver",
                                     "()Landroid/content/ContentResolver;");

    // Get the file name and the extension

    auto cursor = contentResolver.callObjectMethod("query",
                                                   "(Landroid/net/Uri;"
                                                   "[Ljava/lang/String;"
                                                   "Ljava/lang/String;"
                                                   "[Ljava/lang/String;"
                                                   "Ljava/lang/String;)"
                                                   "Landroid/database/Cursor;",
                                                   mediaUri.object(),
                                                   nullptr,
                                                   nullptr,
                                                   nullptr,
                                                   nullptr);

    if (!cursor.isValid())
        return {};

    if (!cursor.callMethod<jboolean>("moveToFirst", "()Z")) {
        cursor.callMethod<void>("close", "()V");

        return {};
    }

    auto displayName =
        QJniObject::getStaticObjectField("android/provider/OpenableColumns",
                                         "DISPLAY_NAME",
                                         "Ljava/lang/String;");
    auto nameIndex =
            cursor.callMethod<jint>("getColumnIndex",
                                    "(Ljava/lang/String;)I",
                                    displayName);
    QString fileName;

    if (nameIndex >= 0) {
        fileName = cursor.callObjectMethod("getString",
                                           "(I)Ljava/lang/String;",
                                           nameIndex).toString();
    }

    cursor.callMethod<void>("close", "()V");

    if (fileName.isEmpty())
        return {};

    QString fileBaseName;
    QFileInfo fileInfo(fileName);

    if (outputFileName.isEmpty()) {
        fileBaseName = fileInfo.baseName();
    } else {
        QFileInfo outputFileInfo(outputFileName);
        fileBaseName = outputFileInfo.baseName();
    }

    auto fileSuffix = fileInfo.completeSuffix();

    // Copy the file to the cache

    auto cacheDir =
            QStandardPaths::writableLocation(QStandardPaths::TempLocation)
            + "/Thumnails";
    QDir().mkpath(cacheDir);
    auto filePath = QString("%1/%2.%3")
                    .arg(cacheDir)
                    .arg(fileBaseName)
                    .arg(fileSuffix);

    // Copy file

    auto inputStream =
            contentResolver.callObjectMethod("openInputStream",
                                             "(Landroid/net/Uri;)Ljava/io/InputStream;",
                                             mediaUri.object());

    if (!inputStream.isValid()) {
        qCritical() << "Failed to open the file from the URI:" << uri;

        return {};
    }

    QJniObject outputStream("java/io/FileOutputStream",
                            "(Ljava/lang/String;)V",
                            QJniObject::fromString(filePath).object());

    QJniEnvironment env;
    auto buffer = env->NewByteArray(8192);

    forever {
        auto bytesRead = inputStream.callMethod<jint>("read", "([B)I", buffer);

        if (bytesRead <= 0)
            break;

        outputStream.callMethod<void>("write", "([BII)V", buffer, 0, bytesRead);
    }

    outputStream.callMethod<void>("close", "()V");
    inputStream.callMethod<void>("close", "()V");
    env->DeleteLocalRef(buffer);

    return filePath;
}

QJniObject RecordingPrivate::createContentValues(const QString &filePath,
                                                 bool isVideo)
{
    // Create ContentValues
    QJniObject contentValues("android/content/ContentValues");

    if (!contentValues.isValid()) {
        qCritical() << "Could not create ContentValues";

        return  {};
    }

    auto outputFile = QFileInfo(filePath).fileName();

    // Configure DISPLAY_NAME
    auto displayNameConst =
            QJniObject::getStaticObjectField("android/provider/MediaStore$MediaColumns",
                                             "DISPLAY_NAME",
                                             "Ljava/lang/String;");
    contentValues.callMethod<void>("put",
                                   "(Ljava/lang/String;Ljava/lang/String;)V",
                                   displayNameConst.object(),
                                   QJniObject::fromString(outputFile).object());

    // Configure MIME_TYPE
    auto mimeTypeConst =
            QJniObject::getStaticObjectField("android/provider/MediaStore$MediaColumns",
                                             "MIME_TYPE",
                                             "Ljava/lang/String;");
    QMimeDatabase mimeDb;
    auto mimeType = mimeDb.mimeTypeForFile(filePath).name();
    contentValues.callMethod<void>("put",
                                   "(Ljava/lang/String;Ljava/lang/String;)V",
                                   mimeTypeConst.object(),
                                   QJniObject::fromString(mimeType).object());

    // Configure folder and storage options

    auto moviesDirectory =
        QJniObject::getStaticObjectField("android/os/Environment",
                                         "DIRECTORY_MOVIES",
                                         "Ljava/lang/String;").toString();
    auto picturesDirectory =
        QJniObject::getStaticObjectField("android/os/Environment",
                                         "DIRECTORY_PICTURES",
                                         "Ljava/lang/String;").toString();

    auto folderPath =
            QString("%1/" COMMONS_APPNAME).arg(isVideo?
                                                    moviesDirectory:
                                                    picturesDirectory);

    if (android_get_device_api_level() >= 29) {
        // Android 10+: Use Scoped Storage with RELATIVE_PATH and IS_PENDING
        auto relativePathConst =
                QJniObject::getStaticObjectField("android/provider/MediaStore$MediaColumns",
                                                 "RELATIVE_PATH",
                                                 "Ljava/lang/String;");
        contentValues.callMethod<void>("put",
                                       "(Ljava/lang/String;Ljava/lang/String;)V",
                                       relativePathConst.object(),
                                       QJniObject::fromString(folderPath).object());
        auto isPendingConst =
                QJniObject::getStaticObjectField("android/provider/MediaStore$MediaColumns",
                                                 "IS_PENDING",
                                                 "Ljava/lang/String;");
        contentValues.callMethod<void>("put",
                                       "(Ljava/lang/String;Ljava/lang/Integer;)V",
                                       isPendingConst.object(),
                                       QJniObject("java/lang/Integer",
                                                  "(I)V",
                                                  1).object());
    } else {
        // Android < 10: Use legacy storage with DATA field
        QString baseDir;
        auto dirObj =
                QJniObject::callStaticObjectMethod("android/os/Environment",
                                                   "getExternalStoragePublicDirectory",
                                                   "(Ljava/lang/String;)Ljava/io/File;",
                                                   QJniObject::fromString(isVideo?
                                                                              moviesDirectory:
                                                                              picturesDirectory).object());
        if (dirObj.isValid()) {
            baseDir = dirObj.callObjectMethod("getAbsolutePath",
                                              "()Ljava/lang/String;").toString();
        } else {
            qWarning() << "Could not get external storage directory, falling back to /sdcard";
            baseDir = "/sdcard/" + (isVideo?
                                        moviesDirectory:
                                        picturesDirectory);
        }

        auto appFolderPath = QString("%1/" COMMONS_APPNAME).arg(baseDir);
        auto fullPath =
                createDirectoryForLegacyStorage(appFolderPath, outputFile);

        if (fullPath.isEmpty())
            return  {};

        auto dataConst =
                QJniObject::getStaticObjectField("android/provider/MediaStore$MediaColumns",
                                                 "DATA",
                                                 "Ljava/lang/String;");
        contentValues.callMethod<void>("put",
                                       "(Ljava/lang/String;Ljava/lang/String;)V",
                                       dataConst.object(),
                                       QJniObject::fromString(fullPath).object());
    }

    return contentValues;
}

QString RecordingPrivate::createDirectoryForLegacyStorage(const QString &appFolderPath,
                                                          const QString &outputFileName)
{
    QDir dir;

    if (!dir.exists(appFolderPath) && !dir.mkpath(appFolderPath)) {
        qWarning() << "Could not create directory" << appFolderPath;

        return {};
    }

    return QString("%1/%2").arg(appFolderPath, outputFileName);
}

QJniObject RecordingPrivate::getExternalContentUri(bool isVideo)
{
    auto uri =
            QJniObject::getStaticObjectField(isVideo?
                                                 "android/provider/MediaStore$Video$Media":
                                                 "android/provider/MediaStore$Images$Media",
                                             "EXTERNAL_CONTENT_URI",
                                             "Landroid/net/Uri;");

    if (!uri.isValid())
        qWarning() << "Could not obtain EXTERNAL_CONTENT_URI";

    return uri;
}

bool RecordingPrivate::copyFileToMediaStore(QFile &file,
                                            QJniObject &outputStream)
{
    QJniEnvironment env;

    // 1 MB per chunk
    constexpr qsizetype chunkSize = 1024 * 1024;

    auto buffer = new char [chunkSize];

    if (!buffer)
        return false;

    auto jChunk = env->NewByteArray(chunkSize);

    if (!jChunk) {
        delete [] buffer;

        return false;
    }

    while (!file.atEnd()) {
        auto bytesRead = file.read(buffer, chunkSize);

        if (bytesRead <= 0) {
            qWarning() << "Error reading the file or unexpected end";
            env->DeleteLocalRef(jChunk);
            delete [] buffer;

            return false;
        }

        // Write the chunk to the output stream,
        env->SetByteArrayRegion(jChunk,
                                0,
                                bytesRead,
                                reinterpret_cast<const jbyte *>(buffer));
        outputStream.callMethod<void>("write",
                                      "([BII)V",
                                      jChunk,
                                      0,
                                      static_cast<jint>(bytesRead));
    }

    env->DeleteLocalRef(jChunk);
    delete [] buffer;

    return true;
}

bool RecordingPrivate::updateIsPending(QJniObject &contentResolver, const QJniObject &uri)
{
    // Not needed for Android < 10
    if (android_get_device_api_level() < 29)
        return true;

    QJniObject contentValues("android/content/ContentValues");

    if (!contentValues.isValid()) {
        qWarning() << "Could not create ContentValues for IS_PENDING update";

        return false;
    }

    auto isPendingConst =
            QJniObject::getStaticObjectField("android/provider/MediaStore$MediaColumns",
                                             "IS_PENDING",
                                             "Ljava/lang/String;");
    contentValues.callMethod<void>("put",
                                   "(Ljava/lang/String;Ljava/lang/Integer;)V",
                                   isPendingConst.object(),
                                   QJniObject("java/lang/Integer",
                                              "(I)V",
                                              0).object());

    auto result =
            contentResolver.callMethod<jint>("update",
                                             "(Landroid/net/Uri;"
                                             "Landroid/content/ContentValues;"
                                             "Ljava/lang/String;"
                                             "[Ljava/lang/String;)I",
                                             uri.object(),
                                             contentValues.object(),
                                             nullptr,
                                             nullptr);

    if (result <= 0) {
        qWarning() << "Could not update IS_PENDING";

        return false;
    }

    return true;
}
#endif

bool RecordingPrivate::saveMediaFileToGallery(const QString &filePath,
                                              bool isVideo)
{
#ifdef Q_OS_ANDROID
    // Open the input file
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open the file" << filePath;

        return false;
    }

    // Get the application context
    QJniObject context =
            qApp->nativeInterface<QNativeInterface::QAndroidApplication>()->context();

    if (!context.isValid()) {
        qCritical() << "Could not obtain the Context";
        file.close();

        return false;
    }

    // Create ContentValues
    auto contentValues = createContentValues(filePath, isVideo);

    if (!contentValues.isValid()) {
        file.close();

        return false;
    }

    // Get ContentResolver
    auto contentResolver =
            context.callObjectMethod("getContentResolver",
                                     "()Landroid/content/ContentResolver;");

    if (!contentResolver.isValid()) {
        qCritical() << "Could not obtain ContentResolver";
        file.close();

        return false;
    }

    // Get EXTERNAL_CONTENT_URI
    auto externalContentUri = getExternalContentUri(isVideo);

    if (!externalContentUri.isValid()) {
        file.close();

        return false;
    }

    // Insert the media into MediaStore
    auto uri =
            contentResolver.callObjectMethod("insert",
                                             "(Landroid/net/Uri;Landroid/content/ContentValues;)Landroid/net/Uri;",
                                             externalContentUri.object(),
                                             contentValues.object());

    if (!uri.isValid()) {
        qWarning() << "Could not insert into MediaStore";
        file.close();

        return false;
    }

    // Open OutputStream
    auto outputStream =
            contentResolver.callObjectMethod("openOutputStream",
                                             "(Landroid/net/Uri;)Ljava/io/OutputStream;",
                                             uri.object());

    if (!outputStream.isValid()) {
        qWarning() << "Could not open OutputStream";
        file.close();

        return false;
    }

    // Copy file to MediaStore
    if (!copyFileToMediaStore(file, outputStream)) {
        outputStream.callMethod<void>("close", "()V");
        file.close();

        return false;
    }

    // Close OutputStream
    outputStream.callMethod<void>("close", "()V");

    // Update IS_PENDING for Android 10+
    if (!updateIsPending(contentResolver, uri)) {
        file.close();

        return false;
    }

    // Close the file
    file.close();

    return true;
#else
    Q_UNUSED(filePath)
    Q_UNUSED(isVideo)

    return false;
#endif
}

QString RecordingPrivate::getLatestMediaUri(bool isVideo)
{
#ifdef Q_OS_ANDROID
    // Get the Android application context
    QJniObject context =
    qApp->nativeInterface<QNativeInterface::QAndroidApplication>()->context();

    if (!context.isValid()) {
        qCritical() << "getLatestMediaUri: Failed to obtain Android context";

        return {};
    }

    // Get the content resolver to perform queries
    auto contentResolver =
    context.callObjectMethod("getContentResolver",
                             "()Landroid/content/ContentResolver;");

    if (!contentResolver.isValid()) {
        qCritical() << "getLatestMediaUri: Failed to obtain ContentResolver";

        return {};
    }

    // Get the appropriate MediaStore URI (images or videos)
    auto externalContentUri =
        QJniObject::getStaticObjectField(isVideo?
                                            "android/provider/MediaStore$Video$Media":
                                            "android/provider/MediaStore$Images$Media",
                                         "EXTERNAL_CONTENT_URI",
                                         "Landroid/net/Uri;");

    if (!externalContentUri.isValid()) {
        qCritical() << "getLatestMediaUri: Failed to obtain external content URI";

        return {};
    }

    QJniEnvironment env;

    // Prepare projection to retrieve only the _id field
    auto projection = env->NewObjectArray(1,
                                          env->FindClass("java/lang/String"),
                                          nullptr);
    env->SetObjectArrayElement(projection,
                               0,
                               QJniObject::fromString("_id").object());

    // Sort results by most recently added
    auto sortOrder = QJniObject::fromString("date_added DESC");

    auto moviesDirectory =
        QJniObject::getStaticObjectField("android/os/Environment",
                                        "DIRECTORY_MOVIES",
                                        "Ljava/lang/String;").toString();
    auto picturesDirectory =
        QJniObject::getStaticObjectField("android/os/Environment",
                                        "DIRECTORY_PICTURES",
                                        "Ljava/lang/String;").toString();

    // Build base paths for fallback (only used for Android < 10)
    QStringList basePaths;

    if (android_get_device_api_level() < 29) {
        basePaths << "/sdcard/"
                     + (isVideo?
                            moviesDirectory + "/" COMMONS_APPNAME "/":
                            picturesDirectory + "/" COMMONS_APPNAME "/");

        // Look for possible SD card mount points
        QDir storageDir("/storage");

        for (auto &entry: storageDir.entryList(QDir::Dirs
                                               | QDir::NoDotAndDotDot)) {
            if (entry != "emulated" && entry != "self") {
                basePaths << QString("/storage/%1/%2")
                             .arg(entry)
                             .arg(isVideo?
                                    moviesDirectory + "/" COMMONS_APPNAME "/":
                                    picturesDirectory + "/" COMMONS_APPNAME "/");
            }
        }
    } else {
        // No filtering by path for Android 10+
        basePaths << "";
    }

    // Perform a single query attempt per path (fallback-friendly)
    for (auto &path: basePaths) {
        QJniObject selection;
        jobjectArray selectionArgs = nullptr;

        // If a path is specified, filter with DATA LIKE 'path%'
        if (!path.isEmpty()) {
            selection = QJniObject::fromString("DATA LIKE ?");
            selectionArgs =
                env->NewObjectArray(1,
                                    env->FindClass("java/lang/String"),
                                    QJniObject::fromString(path + "%").object());
        }

        // Query MediaStore for matching media entries
        auto cursor =
            contentResolver.callObjectMethod("query",
                                            "(Landroid/net/Uri;"
                                            "[Ljava/lang/String;"
                                            "Ljava/lang/String;"
                                            "[Ljava/lang/String;"
                                            "Ljava/lang/String;)"
                                            "Landroid/database/Cursor;",
                                            externalContentUri.object(),
                                            projection,
                                            selection.isValid()?
                                                selection.object():
                                                nullptr,
                                            selectionArgs,
                                            sortOrder.object());

        if (!cursor.isValid()) {
            qWarning() << "getLatestMediaUri: MediaStore query returned null for path:" << path;

            continue;
        }

        // If a result is found, return its content URI
        if (cursor.callMethod<jboolean>("moveToFirst", "()Z")) {
            int id = cursor.callMethod<jint>("getInt", "(I)I", 0);
            cursor.callMethod<void>("close", "()V");
            auto uriWithId = QJniObject::callStaticObjectMethod("android/content/ContentUris",
                                                                "withAppendedId",
                                                                "(Landroid/net/Uri;J)Landroid/net/Uri;",
                                                                externalContentUri.object(),
                                                                static_cast<jlong>(id));

            if (!uriWithId.isValid()) {
                qWarning() << "getLatestMediaUri: Failed to create URI from id:" << id;

                return {};
            }

            return uriWithId.toString();
        }

        // No results in this query, clean up cursor
        cursor.callMethod<void>("close", "()V");
    }

    qWarning() << "getLatestMediaUri: No media URI found";

    return {};
#else
    return {};
#endif
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
    auto videoFormatID = normatizePluginID(self->videoFormat());
    config.beginGroup("RecordConfigs_FormatCodecs_" + videoFormatID);

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

void RecordingPrivate::saveCodecOptionValue(AkCaps::CapsType type,
                                            const QString &option,
                                            const QVariant &value)
{
    QSettings config;
    auto pluginID = this->normatizePluginID(self->codec(type));

    switch (type) {
    case AkCaps::CapsAudio: {
        config.beginGroup("RecordConfigs_AudioCodecOptions_" + pluginID);
        config.setValue(option, value);
        config.endGroup();

        return;
    }

    case AkCaps::CapsVideo: {
        QSettings config;
        auto pluginID = this->normatizePluginID(self->codec(type));
        config.beginGroup("RecordConfigs_VideoCodecOptions_" + pluginID);
        config.setValue(option, value);
        config.endGroup();

        return;
    }

    default:
        break;
    }
}

void RecordingPrivate::saveBitrate(AkCaps::CapsType type, int bitrate)
{
    QSettings config;
    config.beginGroup("RecordConfigs");

    switch (type) {
    case AkCaps::CapsAudio:
        config.setValue("audioBitrate", bitrate);
        break;

    case AkCaps::CapsVideo:
        config.setValue("videoBitrate", bitrate);
        break;

    default:
        break;
    }

    config.endGroup();
}

void RecordingPrivate::saveVideoGOP(int gop)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("videoGOP", gop);
    config.endGroup();
}

void RecordingPrivate::saveRecordAudio(bool recordAudio)
{
    QSettings config;
    config.beginGroup("RecordConfigs");
    config.setValue("recordAudio", recordAudio);
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

#include "moc_recording.cpp"

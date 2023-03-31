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

#ifndef RECORDING_H
#define RECORDING_H

#include <akelement.h>

class RecordingPrivate;
class Recording;
class AkAudioCaps;
class AkVideoCaps;
class QQmlApplicationEngine;

using RecordingPtr = QSharedPointer<Recording>;

class Recording: public QObject
{
    Q_OBJECT
    Q_PROPERTY(AkAudioCaps audioCaps
               READ audioCaps
               WRITE setAudioCaps
               RESET resetAudioCaps
               NOTIFY audioCapsChanged)
    Q_PROPERTY(AkVideoCaps videoCaps
               READ videoCaps
               WRITE setVideoCaps
               RESET resetVideoCaps
               NOTIFY videoCapsChanged)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)
    Q_PROPERTY(QString videoDirectory
               READ videoDirectory
               WRITE setVideoDirectory
               RESET resetVideoDirectory
               NOTIFY videoDirectoryChanged)
    Q_PROPERTY(QStringList availableVideoFormats
               READ availableVideoFormats
               NOTIFY availableVideoFormatsChanged)
    Q_PROPERTY(QStringList availableVideoFormatExtensions
               READ availableVideoFormatExtensions
               NOTIFY availableVideoFormatExtensionsChanged)
    Q_PROPERTY(QVariantList availableVideoFormatOptions
               READ availableVideoFormatOptions
               NOTIFY availableVideoFormatOptionsChanged)
    Q_PROPERTY(QStringList availableVideoCodecs
               READ availableVideoCodecs
               NOTIFY availableVideoCodecsChanged)
    Q_PROPERTY(QStringList availableAudioCodecs
               READ availableAudioCodecs
               NOTIFY availableAudioCodecsChanged)
    Q_PROPERTY(QVariantList availableVideoCodecOptions
               READ availableVideoCodecOptions
               NOTIFY availableVideoCodecOptionsChanged)
    Q_PROPERTY(QVariantList availableAudioCodecOptions
               READ availableAudioCodecOptions
               NOTIFY availableAudioCodecOptionsChanged)
    Q_PROPERTY(QString videoFormat
               READ videoFormat
               WRITE setVideoFormat
               RESET resetVideoFormat
               NOTIFY videoFormatChanged)
    Q_PROPERTY(QString videoFormatExtension
               READ videoFormatExtension
               WRITE setVideoFormatExtension
               RESET resetVideoFormatExtension
               NOTIFY videoFormatExtensionChanged)
    Q_PROPERTY(QVariantMap videoFormatOptions
               READ videoFormatOptions
               WRITE setVideoFormatOptions
               RESET resetVideoFormatOptions
               NOTIFY videoFormatOptionsChanged)
    Q_PROPERTY(QString videoCodec
               READ videoCodec
               WRITE setVideoCodec
               RESET resetVideoCodec
               NOTIFY videoCodecChanged)
    Q_PROPERTY(QString audioCodec
               READ audioCodec
               WRITE setAudioCodec
               RESET resetAudioCodec
               NOTIFY audioCodecChanged)
    Q_PROPERTY(QVariantMap videoCodecParams
               READ videoCodecParams
               WRITE setVideoCodecParams
               RESET resetVideoCodecParams
               NOTIFY videoCodecParamsChanged)
    Q_PROPERTY(QVariantMap audioCodecParams
               READ audioCodecParams
               WRITE setAudioCodecParams
               RESET resetAudioCodecParams
               NOTIFY audioCodecParamsChanged)
    Q_PROPERTY(QVariantMap videoCodecOptions
               READ videoCodecOptions
               WRITE setVideoCodecOptions
               RESET resetVideoCodecOptions
               NOTIFY videoCodecOptionsChanged)
    Q_PROPERTY(QVariantMap audioCodecOptions
               READ audioCodecOptions
               WRITE setAudioCodecOptions
               RESET resetAudioCodecOptions
               NOTIFY audioCodecOptionsChanged)
    Q_PROPERTY(bool recordAudio
               READ recordAudio
               WRITE setRecordAudio
               RESET resetRecordAudio
               NOTIFY recordAudioChanged)
    Q_PROPERTY(QString lastVideoPreview
               READ lastVideoPreview
               NOTIFY lastVideoPreviewChanged)
    Q_PROPERTY(QString lastVideo
               READ lastVideo
               NOTIFY lastVideoChanged)
    Q_PROPERTY(QString imagesDirectory
               READ imagesDirectory
               WRITE setImagesDirectory
               RESET resetImagesDirectory
               NOTIFY imagesDirectoryChanged)
    Q_PROPERTY(QStringList availableImageFormats
               READ availableImageFormats
               CONSTANT)
    Q_PROPERTY(QString imageFormat
               READ imageFormat
               WRITE setImageFormat
               RESET resetImageFormat
               NOTIFY imageFormatChanged)
    Q_PROPERTY(QString lastPhotoPreview
               READ lastPhotoPreview
               NOTIFY lastPhotoPreviewChanged)
    Q_PROPERTY(int imageSaveQuality
               READ imageSaveQuality
               WRITE setImageSaveQuality
               RESET resetImageSaveQuality
               NOTIFY imageSaveQualityChanged)

    public:
        Recording(QQmlApplicationEngine *engine=nullptr,
                  QObject *parent=nullptr);
        ~Recording();

        // General options
        Q_INVOKABLE AkAudioCaps audioCaps() const;
        Q_INVOKABLE AkVideoCaps videoCaps() const;
        Q_INVOKABLE AkElement::ElementState state() const;

        // Video
        Q_INVOKABLE QString videoDirectory() const;
        Q_INVOKABLE QStringList availableVideoFormats() const;
        Q_INVOKABLE QStringList availableVideoFormatExtensions() const;
        Q_INVOKABLE QVariantList availableVideoFormatOptions() const;
        Q_INVOKABLE QStringList availableVideoCodecs() const;
        Q_INVOKABLE QStringList availableAudioCodecs() const;
        Q_INVOKABLE QVariantList availableVideoCodecOptions() const;
        Q_INVOKABLE QVariantList availableAudioCodecOptions() const;
        Q_INVOKABLE QString videoFormat() const;
        Q_INVOKABLE QString videoFormatExtension() const;
        Q_INVOKABLE QVariantMap videoFormatOptions() const;
        Q_INVOKABLE QString videoCodec() const;
        Q_INVOKABLE QString audioCodec() const;
        Q_INVOKABLE QVariantMap videoCodecParams() const;
        Q_INVOKABLE QVariantMap audioCodecParams() const;
        Q_INVOKABLE QVariantMap videoCodecOptions() const;
        Q_INVOKABLE QVariantMap audioCodecOptions() const;
        Q_INVOKABLE bool recordAudio() const;
        Q_INVOKABLE QString videoFormatDescription(const QString &formatId) const;
        Q_INVOKABLE QString codecDescription(const QString &codec) const;
        Q_INVOKABLE QString lastVideoPreview() const;
        Q_INVOKABLE QString lastVideo() const;

        // Picture
        Q_INVOKABLE QString imagesDirectory() const;
        Q_INVOKABLE QStringList availableImageFormats() const;
        Q_INVOKABLE QString imageFormat() const;
        Q_INVOKABLE QString imageFormatDescription(const QString &videoFormat) const;
        Q_INVOKABLE QString lastPhotoPreview() const;
        Q_INVOKABLE int imageSaveQuality() const;

    private:
        RecordingPrivate *d;

    signals:
        void audioCapsChanged(const AkAudioCaps &audioCaps);
        void videoCapsChanged(const AkVideoCaps &videoCaps);
        void stateChanged(AkElement::ElementState state);
        void videoDirectoryChanged(const QString &videoDirectory);
        void availableVideoFormatsChanged(const QStringList &availableVideoFormats);
        void availableVideoFormatExtensionsChanged(const QStringList &availableVideoFormatExtensions);
        void availableVideoFormatOptionsChanged(const QVariantList &availableVideoFormatOptions);
        void availableVideoCodecsChanged(const QStringList &availableVideoCodecs);
        void availableAudioCodecsChanged(const QStringList &availableAudioCodecs);
        void availableVideoCodecOptionsChanged(const QVariantList &availableVideoCodecOptions);
        void availableAudioCodecOptionsChanged(const QVariantList &availableAudioCodecOptions);
        void videoFormatChanged(const QString &videoFormat);
        void videoFormatExtensionChanged(const QString &videoFormatExtension);
        void videoFormatOptionsChanged(const QVariantMap &videoFormatOptions);
        void videoCodecChanged(const QString &videoCodec);
        void audioCodecChanged(const QString &audioCodec);
        void videoCodecParamsChanged(const QVariantMap &videoCodecParams);
        void audioCodecParamsChanged(const QVariantMap &audioCodecParams);
        void videoCodecOptionsChanged(const QVariantMap &videoCodecOptions);
        void audioCodecOptionsChanged(const QVariantMap &audioCodecOptions);
        void recordAudioChanged(bool recordAudio);
        void lastVideoPreviewChanged(const QString &lastVideoPreview);
        void lastVideoChanged(const QString &lastVideo);
        void imagesDirectoryChanged(const QString &imagesDirectory);
        void imageFormatChanged(const QString &imageFormat);
        void lastPhotoPreviewChanged(const QString &lastPhotoPreview);
        void imageSaveQualityChanged(int imageSaveQuality);

    public slots:
        void setAudioCaps(const AkAudioCaps &audioCaps);
        void setVideoCaps(const AkVideoCaps &videoCaps);
        void setState(AkElement::ElementState state);
        void setVideoDirectory(const QString &videoDirectory);
        void setVideoFormat(const QString &videoFormat);
        void setVideoFormatExtension(const QString &videoFormatExtension);
        void setVideoFormatOptions(const QVariantMap &videoFormatOptions);
        void setVideoCodec(const QString &videoCodec);
        void setAudioCodec(const QString &audioCodec);
        void setVideoCodecParams(const QVariantMap &videoCodecParams);
        void setAudioCodecParams(const QVariantMap &audioCodecParams);
        void setVideoCodecOptions(const QVariantMap &videoCodecOptions);
        void setAudioCodecOptions(const QVariantMap &audioCodecOptions);
        void setRecordAudio(bool recordAudio);
        void setImagesDirectory(const QString &imagesDirectory);
        void setImageFormat(const QString &imageFormat);
        void setImageSaveQuality(int imageSaveQuality);
        void resetAudioCaps();
        void resetVideoCaps();
        void resetState();
        void resetVideoDirectory();
        void resetVideoFormat();
        void resetVideoFormatExtension();
        void resetVideoFormatOptions();
        void resetVideoCodec();
        void resetAudioCodec();
        void resetVideoCodecParams();
        void resetAudioCodecParams();
        void resetVideoCodecOptions();
        void resetAudioCodecOptions();
        void resetRecordAudio();
        void resetImagesDirectory();
        void resetImageFormat();
        void resetImageSaveQuality();
        void takePhoto();
        void savePhoto(const QString &fileName);
		bool copyToClipboard();
        AkPacket iStream(const AkPacket &packet);
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);

    private slots:
        void thumbnailUpdated(const AkPacket &packet);
        void mediaLoaded(const QString &media);
};

#endif // RECORDING_H

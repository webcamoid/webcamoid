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

#include <akcaps.h>
#include <akpropertyoption.h>
#include <iak/akelement.h>

class RecordingPrivate;
class Recording;
class AkAudioCaps;
class AkVideoCaps;
class QQmlApplicationEngine;

using RecordingPtr = QSharedPointer<Recording>;

class Recording: public QObject
{
    Q_OBJECT

    // General options
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

    // Video
    Q_PROPERTY(QString videoDirectory
               READ videoDirectory
               WRITE setVideoDirectory
               RESET resetVideoDirectory
               NOTIFY videoDirectoryChanged)
    Q_PROPERTY(QString videoFormat
               READ videoFormat
               WRITE setVideoFormat
               RESET resetVideoFormat
               NOTIFY videoFormatChanged)
    Q_PROPERTY(QStringList videoFormats
               READ videoFormats
               CONSTANT)
    Q_PROPERTY(QString defaultVideoFormat
               READ defaultVideoFormat
               CONSTANT)
    Q_PROPERTY(AkPropertyOptions videoFormatOptions
               READ videoFormatOptions
               RESET resetVideoFormatOptions
               NOTIFY videoFormatOptionsChanged)
    Q_PROPERTY(int videoGOP
               READ videoGOP
               WRITE setVideoGOP
               RESET resetVideoGOP
               NOTIFY videoGOPChanged)
    Q_PROPERTY(int defaultVideoGOP
               READ defaultVideoGOP
               CONSTANT)
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

    // Picture
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
        Q_INVOKABLE QString videoFormat() const;
        Q_INVOKABLE QStringList videoFormats() const;
        Q_INVOKABLE QString defaultVideoFormat() const;
        Q_INVOKABLE QString formatDescription(const QString &format) const;
        Q_INVOKABLE QString codec(AkCaps::CapsType type) const;
        Q_INVOKABLE QString defaultCodec(const QString &format,
                                         AkCaps::CapsType type) const;
        Q_INVOKABLE QStringList supportedCodecs(const QString &format,
                                                AkCaps::CapsType type) const;
        Q_INVOKABLE QString codecDescription(const QString &codec) const;
        Q_INVOKABLE AkPropertyOptions videoFormatOptions() const;
        Q_INVOKABLE QVariant videoFormatOptionValue(const QString &option) const;
        Q_INVOKABLE AkPropertyOptions codecOptions(AkCaps::CapsType type) const;
        Q_INVOKABLE QVariant codecOptionValue(AkCaps::CapsType type,
                                              const QString &option) const;
        Q_INVOKABLE int bitrate(AkCaps::CapsType type) const;
        Q_INVOKABLE int defaultBitrate(AkCaps::CapsType type) const;
        Q_INVOKABLE int videoGOP() const;
        Q_INVOKABLE int defaultVideoGOP() const;
        Q_INVOKABLE bool recordAudio() const;
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
        // General options
        void audioCapsChanged(const AkAudioCaps &audioCaps);
        void videoCapsChanged(const AkVideoCaps &videoCaps);
        void stateChanged(AkElement::ElementState state);

        // Video
        void videoDirectoryChanged(const QString &videoDirectory);
        void videoFormatChanged(const QString &videoFormat);
        void codecChanged(AkCaps::CapsType type,
                          const QString &codec);
        void videoFormatOptionsChanged(const AkPropertyOptions &options);
        void videoFormatOptionValueChanged(const QString &option,
                                           const QVariant &value);
        void codecOptionsChanged(AkCaps::CapsType type,
                                 const AkPropertyOptions &options);
        void codecOptionValueChanged(AkCaps::CapsType type,
                                     const QString &option,
                                     const QVariant &value);
        void bitrateChanged(AkCaps::CapsType type, int bitrate);
        void videoGOPChanged(int gop);
        void recordAudioChanged(bool recordAudio);
        void lastVideoPreviewChanged(const QString &lastVideoPreview);
        void lastVideoChanged(const QString &lastVideo);

        // Picture
        void imagesDirectoryChanged(const QString &imagesDirectory);
        void imageFormatChanged(const QString &imageFormat);
        void lastPhotoPreviewChanged(const QString &lastPhotoPreview);
        void imageSaveQualityChanged(int imageSaveQuality);

    public slots:
        // General options
        void setAudioCaps(const AkAudioCaps &audioCaps);
        void setVideoCaps(const AkVideoCaps &videoCaps);
        bool setState(AkElement::ElementState state);

        // Video
        void setVideoDirectory(const QString &videoDirectory);
        void setVideoFormat(const QString &format);
        void setCodec(AkCaps::CapsType type, const QString &codec);
        void setVideoFormatOptionValue(const QString &option,
                                       const QVariant &value);
        void setCodecOptionValue(AkCaps::CapsType type,
                                 const QString &option,
                                 const QVariant &value);
        void setBitrate(AkCaps::CapsType type, int bitrate);
        void setVideoGOP(int gop);
        void setRecordAudio(bool recordAudio);

        // Picture
        void setImagesDirectory(const QString &imagesDirectory);
        void setImageFormat(const QString &imageFormat);
        void setImageSaveQuality(int imageSaveQuality);

        // General options
        void resetAudioCaps();
        void resetVideoCaps();
        void resetState();

        // Video
        void resetVideoDirectory();
        void resetVideoFormat();
        void resetCodec(AkCaps::CapsType type);
        void resetVideoFormatOptionValue(const QString &option);
        void resetCodecOptionValue(AkCaps::CapsType type,
                                   const QString &option);
        void resetVideoFormatOptions();
        void resetCodecOptions(AkCaps::CapsType type);
        void resetBitrate(AkCaps::CapsType type);
        void resetVideoGOP();
        void resetRecordAudio();

        // Picture
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

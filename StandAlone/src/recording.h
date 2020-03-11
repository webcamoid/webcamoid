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
    Q_PROPERTY(QStringList availableFormats
               READ availableFormats
               NOTIFY availableFormatsChanged)
    Q_PROPERTY(QString format
               READ format
               WRITE setFormat
               RESET resetFormat
               NOTIFY formatChanged)
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
    Q_PROPERTY(bool recordAudio
               READ recordAudio
               WRITE setRecordAudio
               RESET resetRecordAudio
               NOTIFY recordAudioChanged)
    Q_PROPERTY(QString videoFileName
               READ videoFileName
               WRITE setVideoFileName
               RESET resetVideoFileName
               NOTIFY videoFileNameChanged)
    Q_PROPERTY(QString imageFormat
               READ imageFormat
               WRITE setImageFormat
               RESET resetImageFormat
               NOTIFY imageFormatChanged)
    Q_PROPERTY(QString imagesDirectory
               READ imagesDirectory
               WRITE setImagesDirectory
               RESET resetImagesDirectory
               NOTIFY imagesDirectoryChanged)
    Q_PROPERTY(QString videoDirectory
               READ videoDirectory
               WRITE setVideoDirectory
               RESET resetVideoDirectory
               NOTIFY videoDirectoryChanged)
    Q_PROPERTY(int imageSaveQuality
               READ imageSaveQuality
               WRITE setImageSaveQuality
               RESET resetImageSaveQuality
               NOTIFY imageSaveQualityChanged)
    Q_PROPERTY(QStringList availableImageFormats
               READ availableImageFormats
               CONSTANT)
    Q_PROPERTY(QString lastPhotoPreview
               READ lastPhotoPreview
               NOTIFY lastPhotoPreviewChanged)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)

    public:
        Recording(QQmlApplicationEngine *engine=nullptr,
                  QObject *parent=nullptr);
        ~Recording();

        Q_INVOKABLE QStringList availableFormats() const;
        Q_INVOKABLE QString format() const;
        Q_INVOKABLE AkAudioCaps audioCaps() const;
        Q_INVOKABLE AkVideoCaps videoCaps() const;
        Q_INVOKABLE bool recordAudio() const;
        Q_INVOKABLE QString videoFileName() const;
        Q_INVOKABLE QString imagesDirectory() const;
        Q_INVOKABLE QString videoDirectory() const;
        Q_INVOKABLE QString imageFormat() const;
        Q_INVOKABLE int imageSaveQuality() const;
        Q_INVOKABLE QStringList availableImageFormats() const;
        Q_INVOKABLE QString imageFormatDescription(const QString &format) const;
        Q_INVOKABLE QString lastPhotoPreview() const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE QString formatDescription(const QString &formatId) const;
        Q_INVOKABLE QStringList formatSuffix(const QString &formatId) const;
        Q_INVOKABLE bool embedControls(const QString &where,
                                       const QString &format="",
                                       const QString &name="");
        Q_INVOKABLE void removeInterface(const QString &where);

    private:
        RecordingPrivate *d;

    signals:
        void availableFormatsChanged(const QStringList &availableFormats);
        void formatChanged(const QString &format);
        void audioCapsChanged(const AkAudioCaps &audioCaps);
        void videoCapsChanged(const AkVideoCaps &videoCaps);
        void recordAudioChanged(bool recordAudio);
        void videoFileNameChanged(const QString &videoFileName);
        void imageFormatChanged(const QString &imageFormat);
        void imagesDirectoryChanged(const QString &imagesDirectory);
        void videoDirectoryChanged(const QString &videoDirectory);
        void imageSaveQualityChanged(int imageSaveQuality);
        void lastPhotoPreviewChanged(const QString &lastPhotoPreview);
        void stateChanged(AkElement::ElementState state);

    public slots:
        void setFormat(const QString &format);
        void setAudioCaps(const AkAudioCaps &audioCaps);
        void setVideoCaps(const AkVideoCaps &videoCaps);
        void setRecordAudio(bool recordAudio);
        void setVideoFileName(const QString &videoFileName);
        void setImageFormat(const QString &imageFormat);
        void setImagesDirectory(const QString &imagesDirectory);
        void setVideoDirectory(const QString &videoDirectory);
        void setImageSaveQuality(int imageSaveQuality);
        void setState(AkElement::ElementState state);
        void resetFormat();
        void resetAudioCaps();
        void resetVideoCaps();
        void resetRecordAudio();
        void resetVideoFileName();
        void resetImageFormat();
        void resetImagesDirectory();
        void resetVideoDirectory();
        void resetImageSaveQuality();
        void resetState();
        void takePhoto();
        void savePhoto(const QString &fileName);
        AkPacket iStream(const AkPacket &packet);
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);

    private slots:
        void supportedFormatsUpdated(const QStringList &availableFormats);
        void userControlsUpdated(const QVariantMap &userControls);
        void capsUpdated();
        void updateFormat();
        void loadProperties();
        void loadFormatOptions(const QString &format);
        void loadStreams(const QString &format);
        void loadCodecOptions(const QVariantList &streams);
        void saveOutputFormat(const QString &format);
        void saveFormatOptions(const QVariantMap &formatOptions);
        void saveStreams(const QVariantList &streams);
        void saveCodecParams();
        void saveCodecOptions();
        void saveRecordAudio(bool recordAudio);
        void saveMultiSinkCodecLib(const QString &codecLib);
        void saveProperties();
};

#endif // RECORDING_H

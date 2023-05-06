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

#ifndef VIDEOLAYER_H
#define VIDEOLAYER_H

#include <akelement.h>
#include <akvideocaps.h>

#include "downloadmanager.h"

class VideoLayerPrivate;
class VideoLayer;
class CliOptions;
class AkAudioCaps;
class QQmlApplicationEngine;

using VideoLayerPtr = QSharedPointer<VideoLayer>;

class VideoLayer: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString inputError
               READ inputError
               NOTIFY inputErrorChanged)
    Q_PROPERTY(QString outputError
               READ outputError
               NOTIFY outputErrorChanged)
    Q_PROPERTY(QStringList videoSourceFileFilters
               READ videoSourceFileFilters
               CONSTANT)
    Q_PROPERTY(QString videoInput
               READ videoInput
               WRITE setVideoInput
               RESET resetVideoInput
               NOTIFY videoInputChanged)
    Q_PROPERTY(QStringList videoOutput
               READ videoOutput
               WRITE setVideoOutput
               RESET resetVideoOutput
               NOTIFY videoOutputChanged)
    Q_PROPERTY(QStringList inputs
               READ inputs
               NOTIFY inputsChanged)
    Q_PROPERTY(QStringList outputs
               READ outputs
               NOTIFY outputsChanged)
    Q_PROPERTY(AkAudioCaps inputAudioCaps
               READ inputAudioCaps
               NOTIFY inputAudioCapsChanged)
    Q_PROPERTY(AkVideoCaps inputVideoCaps
               READ inputVideoCaps
               NOTIFY inputVideoCapsChanged)
    Q_PROPERTY(AkVideoCaps::PixelFormatList supportedOutputPixelFormats
               READ supportedOutputPixelFormats
               CONSTANT)
    Q_PROPERTY(AkVideoCaps::PixelFormat defaultOutputPixelFormat
               READ defaultOutputPixelFormat
               CONSTANT)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)
    Q_PROPERTY(FlashMode flashMode
               READ flashMode
               WRITE setFlashMode
               RESET resetFlashMode
               NOTIFY flashModeChanged)
    Q_PROPERTY(bool playOnStart
               READ playOnStart
               WRITE setPlayOnStart
               RESET resetPlayOnStart
               NOTIFY playOnStartChanged)
    Q_PROPERTY(bool outputsAsInputs
               READ outputsAsInputs
               WRITE setOutputsAsInputs
               RESET resetOutputsAsInputs
               NOTIFY outputsAsInputsChanged)
    Q_PROPERTY(QList<quint64> clientsPids
               READ clientsPids
               CONSTANT)
    Q_PROPERTY(bool driverInstalled
               READ driverInstalled
               CONSTANT)
    Q_PROPERTY(QString picture
               READ picture
               WRITE setPicture
               RESET resetPicture
               NOTIFY pictureChanged)
    Q_PROPERTY(QString rootMethod
               READ rootMethod
               WRITE setRootMethod
               RESET resetRootMethod
               NOTIFY rootMethodChanged)
    Q_PROPERTY(QStringList availableRootMethods
               READ availableRootMethods
               CONSTANT)
    Q_PROPERTY(bool isVCamSupported
               READ isVCamSupported
               CONSTANT)
    Q_PROPERTY(VCamStatus vcamInstallStatus
               READ vcamInstallStatus
               CONSTANT)
    Q_PROPERTY(QString vcamDriver
               READ vcamDriver
               NOTIFY vcamDriverChanged)
    Q_PROPERTY(QString currentVCamVersion
               READ currentVCamVersion
               NOTIFY currentVCamVersionChanged)
    Q_PROPERTY(bool isCurrentVCamInstalled
               READ isCurrentVCamInstalled
               NOTIFY currentVCamInstalledChanged)
    Q_PROPERTY(QString vcamUpdateUrl
               READ vcamUpdateUrl
               CONSTANT)
    Q_PROPERTY(QString vcamDownloadUrl
               READ vcamDownloadUrl
               CONSTANT)
    Q_PROPERTY(QString defaultVCamDriver
               READ defaultVCamDriver
               CONSTANT)

    public:
        enum InputType {
            InputUnknown,
            InputCamera,
            InputDesktop,
            InputImage,
            InputStream
        };
        Q_ENUM(InputType)

        enum OutputType {
            OutputUnknown,
            OutputVirtualCamera,
        };
        Q_ENUM(OutputType)

        enum VCamStatus
        {
            VCamNotInstalled,
            VCamInstalled,
            VCamInstalledOther
        };
        Q_ENUM(VCamStatus)

        enum FlashMode
        {
            FlashMode_Off,
            FlashMode_On,
            FlashMode_Auto,
            FlashMode_Torch,
            FlashMode_RedEye,
            FlashMode_External,
        };
        Q_ENUM(FlashMode)
        using FlashModeList = QList<FlashMode>;

        VideoLayer(QQmlApplicationEngine *engine=nullptr,
                   QObject *parent=nullptr);
        ~VideoLayer();

        Q_INVOKABLE QStringList videoSourceFileFilters() const;
        Q_INVOKABLE QString videoInput() const;
        Q_INVOKABLE QStringList videoOutput() const;
        Q_INVOKABLE QStringList inputs() const;
        Q_INVOKABLE QStringList outputs() const;
        Q_INVOKABLE AkAudioCaps inputAudioCaps() const;
        Q_INVOKABLE AkVideoCaps inputVideoCaps() const;
        Q_INVOKABLE AkVideoCaps::PixelFormatList supportedOutputPixelFormats() const;
        Q_INVOKABLE AkVideoCaps::PixelFormat defaultOutputPixelFormat() const;
        Q_INVOKABLE AkVideoCapsList supportedOutputVideoCaps(const QString &device) const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE FlashModeList supportedFlashModes(const QString &videoInput) const;
        Q_INVOKABLE FlashMode flashMode() const;
        Q_INVOKABLE bool playOnStart() const;
        Q_INVOKABLE bool outputsAsInputs() const;
        Q_INVOKABLE InputType deviceType(const QString &device) const;
        Q_INVOKABLE QStringList devicesByType(InputType type) const;
        Q_INVOKABLE QString description(const QString &device) const;
        Q_INVOKABLE QString createOutput(OutputType type,
                                         const QString &description,
                                         const AkVideoCapsList &formats);
        Q_INVOKABLE QString createOutput(OutputType type,
                                         const QString &description,
                                         const QVariantList &formats);
        Q_INVOKABLE bool editOutput(const QString &output,
                                    const QString &description,
                                    const AkVideoCapsList &formats);
        Q_INVOKABLE bool removeOutput(const QString &output);
        Q_INVOKABLE bool removeAllOutputs();
        Q_INVOKABLE QString inputError() const;
        Q_INVOKABLE QString outputError() const;
        Q_INVOKABLE bool embedInputControls(const QString &where,
                                            const QString &device,
                                            const QString &name={}) const;
        Q_INVOKABLE bool embedOutputControls(const QString &where,
                                             const QString &device,
                                             const QString &name={}) const;
        Q_INVOKABLE void removeInterface(const QString &where) const;
        Q_INVOKABLE QList<quint64> clientsPids() const;
        Q_INVOKABLE QString clientExe(quint64 pid) const;
        Q_INVOKABLE bool driverInstalled() const;
        Q_INVOKABLE QString picture() const;
        Q_INVOKABLE QString rootMethod() const;
        Q_INVOKABLE QStringList availableRootMethods() const;
        Q_INVOKABLE bool isVCamSupported() const;
        Q_INVOKABLE VCamStatus vcamInstallStatus() const;
        Q_INVOKABLE QString vcamDriver() const;
        Q_INVOKABLE QString currentVCamVersion() const;
        Q_INVOKABLE bool isCurrentVCamInstalled() const;
        Q_INVOKABLE QString vcamUpdateUrl() const;
        Q_INVOKABLE QString vcamDownloadUrl() const;
        Q_INVOKABLE QString defaultVCamDriver() const;

    private:
        VideoLayerPrivate *d;

    signals:
        void videoInputChanged(const QString &videoInput);
        void videoOutputChanged(const QStringList &videoOutput);
        void inputsChanged(const QStringList &inputs);
        void outputsChanged(const QStringList &outputs);
        void inputAudioCapsChanged(const AkAudioCaps &inputAudioCaps);
        void inputVideoCapsChanged(const AkVideoCaps &inputVideoCaps);
        void stateChanged(AkElement::ElementState state);
        void flashModeChanged(FlashMode mode);
        void playOnStartChanged(bool playOnStart);
        void outputsAsInputsChanged(bool outputsAsInputs);
        void oStream(const AkPacket &packet);
        void inputErrorChanged(const QString &inputError);
        void outputErrorChanged(const QString &outputError);
        void pictureChanged(const QString &picture);
        void rootMethodChanged(const QString &rootMethod);
        void vcamDriverChanged(const QString &vcamDriver);
        void currentVCamVersionChanged(const QString &currentVCamVersion);
        void currentVCamInstalledChanged(bool installed);
        void startVCamDownload(const QString &title,
                               const QString &fromUrl,
                               const QString &toFile);
        void vcamDownloadReady(const QString &filePath);
        void vcamDownloadFailed(const QString &error);
        void vcamInstallFinished(int exitCode, const QString &error);

    public slots:
        bool applyPicture();
        void setLatestVCamVersion(const QString &version);
        bool downloadVCam();
        bool executeVCamInstaller(const QString &installer);
        void checkVCamDownloadReady(const QString &url,
                                    const QString &filePath,
                                    DownloadManager::DownloadStatus status,
                                    const QString &error);
        void setInputStream(const QString &stream, const QString &description);
        void removeInputStream(const QString &stream);
        void setVideoInput(const QString &videoInput);
        void setVideoOutput(const QStringList &videoOutput);
        void setState(AkElement::ElementState state);
        void setFlashMode(FlashMode mode);
        void setPlayOnStart(bool playOnStart);
        void setOutputsAsInputs(bool outputsAsInputs);
        void setPicture(const QString &picture);
        void setRootMethod(const QString &rootMethod);
        void resetVideoInput();
        void resetVideoOutput();
        void resetState();
        void resetFlashMode();
        void resetPlayOnStart();
        void resetOutputsAsInputs();
        void resetPicture();
        void resetRootMethod();
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);

    private slots:
        void updateCaps();
        void updateInputs();
        void saveVirtualCameraRootMethod(const QString &rootMethod);
        AkPacket iStream(const AkPacket &packet);

        friend class VideoLayerPrivate;
};

Q_DECLARE_METATYPE(VideoLayer::InputType)
Q_DECLARE_METATYPE(VideoLayer::OutputType)
Q_DECLARE_METATYPE(VideoLayer::VCamStatus)
Q_DECLARE_METATYPE(VideoLayer::FlashMode)
Q_DECLARE_METATYPE(VideoLayer::FlashModeList)

#endif // VIDEOLAYER_H

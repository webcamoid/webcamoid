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

#ifndef VIDEOLAYER_H
#define VIDEOLAYER_H

#include <akvideocaps.h>
#include <iak/akelement.h>

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
    Q_PROPERTY(QStringList videoSourceFileFilters
               READ videoSourceFileFilters
               CONSTANT)
    Q_PROPERTY(QString videoInput
               READ videoInput
               WRITE setVideoInput
               RESET resetVideoInput
               NOTIFY videoInputChanged)
    Q_PROPERTY(QStringList inputs
               READ inputs
               NOTIFY inputsChanged)
    Q_PROPERTY(AkAudioCaps inputAudioCaps
               READ inputAudioCaps
               NOTIFY inputAudioCapsChanged)
    Q_PROPERTY(AkVideoCaps inputVideoCaps
               READ inputVideoCaps
               NOTIFY inputVideoCapsChanged)
    Q_PROPERTY(QStringList screens
               READ screens
               NOTIFY screensChanged)
    Q_PROPERTY(QStringList windows
               READ windows
               NOTIFY windowsChanged)
    Q_PROPERTY(bool canCaptureWindows
               READ canCaptureWindows
               NOTIFY canCaptureWindowsChanged)
    Q_PROPERTY(QStringList supportedFileFormats
               READ supportedFileFormats
               CONSTANT)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)
    Q_PROPERTY(bool isTorchSupported
               READ isTorchSupported
               NOTIFY isTorchSupportedChanged)
    Q_PROPERTY(TorchMode torchMode
               READ torchMode
               WRITE setTorchMode
               RESET resetTorchMode
               NOTIFY torchModeChanged)
    Q_PROPERTY(PermissionStatus cameraPermissionStatus
               READ cameraPermissionStatus
               NOTIFY cameraPermissionStatusChanged)
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

    public:
        enum InputType {
            InputUnknown,
            InputCamera,
            InputScreen,
            InputImage,
            InputStream
        };
        Q_ENUM(InputType)

        enum TorchMode
        {
            Torch_Off,
            Torch_On,
        };
        Q_ENUM(TorchMode)

        enum PermissionStatus
        {
            PermissionStatus_Undetermined,
            PermissionStatus_Granted,
            PermissionStatus_Denied,
        };
        Q_ENUM(PermissionStatus)

        VideoLayer(QQmlApplicationEngine *engine=nullptr,
                   QObject *parent=nullptr);
        ~VideoLayer();

        Q_INVOKABLE QStringList videoSourceFileFilters() const;
        Q_INVOKABLE QString videoInput() const;
        Q_INVOKABLE QStringList inputs() const;
        Q_INVOKABLE AkAudioCaps inputAudioCaps() const;
        Q_INVOKABLE AkVideoCaps inputVideoCaps() const;
        Q_INVOKABLE QStringList screens() const;
        Q_INVOKABLE QStringList windows() const;
        Q_INVOKABLE bool canCaptureWindows() const;
        Q_INVOKABLE QStringList supportedFileFormats() const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE bool isTorchSupported() const;
        Q_INVOKABLE TorchMode torchMode() const;
        Q_INVOKABLE PermissionStatus cameraPermissionStatus() const;
        Q_INVOKABLE bool playOnStart() const;
        Q_INVOKABLE bool outputsAsInputs() const;
        Q_INVOKABLE InputType deviceType(const QString &device) const;
        Q_INVOKABLE QStringList devicesByType(InputType type) const;
        Q_INVOKABLE QString description(const QString &device) const;
        Q_INVOKABLE QString inputError() const;
        Q_INVOKABLE bool embedInputControls(const QString &where,
                                            const QString &device,
                                            const QString &name={}) const;
        Q_INVOKABLE void removeInterface(const QString &where) const;

    private:
        VideoLayerPrivate *d;

    signals:
        void videoInputChanged(const QString &videoInput);
        void inputsChanged(const QStringList &inputs);
        void inputAudioCapsChanged(const AkAudioCaps &inputAudioCaps);
        void inputVideoCapsChanged(const AkVideoCaps &inputVideoCaps);
        void screensChanged(const QStringList &screens);
        void windowsChanged(const QStringList &windows);
        void canCaptureWindowsChanged(bool canCaptureWindows);
        void stateChanged(AkElement::ElementState state);
        void isTorchSupportedChanged(bool torchSupported);
        void torchModeChanged(TorchMode mode);
        void cameraPermissionStatusChanged(PermissionStatus status);
        void playOnStartChanged(bool playOnStart);
        void outputsAsInputsChanged(bool outputsAsInputs);
        void oStream(const AkPacket &packet);
        void inputErrorChanged(const QString &inputError);

    public slots:
        void setInputStream(const QString &stream, const QString &description);
        void removeInputStream(const QString &stream);
        bool addScreenSource(const QString &source);
        void removeScreenSource(const QString &source);
        void setVideoInput(const QString &videoInput);
        void setState(AkElement::ElementState state);
        void setTorchMode(TorchMode mode);
        void setPlayOnStart(bool playOnStart);
        void setOutputsAsInputs(bool outputsAsInputs);
        void resetVideoInput();
        void resetState();
        void resetTorchMode();
        void resetPlayOnStart();
        void resetOutputsAsInputs();
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);
        void updateInputs();

    private slots:
        void updateCaps();

        friend class VideoLayerPrivate;
};

Q_DECLARE_METATYPE(VideoLayer::InputType)
Q_DECLARE_METATYPE(VideoLayer::TorchMode)
Q_DECLARE_METATYPE(VideoLayer::PermissionStatus)

#endif // VIDEOLAYER_H

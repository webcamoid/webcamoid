/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#ifndef VIDEOLAYERCOMPAT_H
#define VIDEOLAYERCOMPAT_H

#include <akaudiocaps.h>
#include <akvideocaps.h>
#include <iak/akelement.h>

#include "videolayer.h"

class VideoLayerCompatPrivate;
class VideoLayerCompat;

using VideoLayerCompatPtr = QSharedPointer<VideoLayerCompat>;

// Thin, throwaway adapter: reproduces the OLD single-source VideoLayer
// public API (properties/signals/slots) on top of the NEW multi-source
// VideoLayer, so the legacy QML tree can keep running unmodified while
// the new backend gets tested and QML is migrated at its own pace.
//
// Registered in QML under the name "videoLayer" INSTEAD OF the real
// VideoLayer, which MediaTools keeps but no longer exposes directly.
//
// This class is meant to be deleted once QML migration is done -- do not
// add anything here beyond what old QML actually needs, and do not let
// new QML code start depending on it.
class VideoLayerCompat: public QObject
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
    Q_PROPERTY(QStringList cameras
               READ cameras
               NOTIFY camerasChanged)
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
    Q_PROPERTY(VideoLayer::TorchMode torchMode
               READ torchMode
               WRITE setTorchMode
               RESET resetTorchMode
               NOTIFY torchModeChanged)
    Q_PROPERTY(VideoLayer::PermissionStatus cameraPermissionStatus
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
        // InputType/TorchMode/PermissionStatus are NOT redeclared here --
        // old QML refers to them as "VideoLayer.InputCamera" etc.
        // (qmlRegisterType still registers the real VideoLayer's enums
        // under that name), so this class only needs the enum TYPES for
        // its own property/method signatures, not fresh declarations.

        explicit VideoLayerCompat(VideoLayer *backend, QObject *parent=nullptr);
        ~VideoLayerCompat();

        Q_INVOKABLE QStringList videoSourceFileFilters() const;
        Q_INVOKABLE QString videoInput() const;
        Q_INVOKABLE QStringList inputs() const;
        Q_INVOKABLE AkAudioCaps inputAudioCaps() const;
        Q_INVOKABLE AkVideoCaps inputVideoCaps() const;
        Q_INVOKABLE QStringList cameras() const;
        Q_INVOKABLE QStringList screens() const;
        Q_INVOKABLE QStringList windows() const;
        Q_INVOKABLE bool canCaptureWindows() const;
        Q_INVOKABLE QStringList supportedFileFormats() const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE bool isTorchSupported() const;
        Q_INVOKABLE VideoLayer::TorchMode torchMode() const;
        Q_INVOKABLE VideoLayer::PermissionStatus cameraPermissionStatus() const;
        Q_INVOKABLE bool playOnStart() const;
        Q_INVOKABLE bool outputsAsInputs() const;
        Q_INVOKABLE VideoLayer::InputType deviceType(const QString &device) const;
        Q_INVOKABLE QStringList devicesByType(VideoLayer::InputType type) const;
        Q_INVOKABLE QString description(const QString &device) const;
        Q_INVOKABLE QString inputError() const;
        Q_INVOKABLE bool embedInputControls(const QString &where,
                                            const QString &device,
                                            const QString &name={}) const;
        Q_INVOKABLE void removeInterface(const QString &where) const;

    private:
        VideoLayerCompatPrivate *d;

    signals:
        void videoInputChanged(const QString &videoInput);
        void inputsChanged(const QStringList &inputs);
        void inputAudioCapsChanged(const AkAudioCaps &inputAudioCaps);
        void inputVideoCapsChanged(const AkVideoCaps &inputVideoCaps);
        void camerasChanged(const QStringList &cameras);
        void screensChanged(const QStringList &screens);
        void windowsChanged(const QStringList &windows);
        void canCaptureWindowsChanged(bool canCaptureWindows);
        void stateChanged(AkElement::ElementState state);
        void isTorchSupportedChanged(bool torchSupported);
        void torchModeChanged(VideoLayer::TorchMode mode);
        void cameraPermissionStatusChanged(VideoLayer::PermissionStatus status);
        void playOnStartChanged(bool playOnStart);
        void outputsAsInputsChanged(bool outputsAsInputs);
        void oStream(const AkPacket &packet);
        void inputErrorChanged(const QString &inputError);

    public slots:
        void setInputStream(const QString &stream, const QString &description);
        void removeInputStream(const QString &stream);
        bool addCameraSource(const QString &source);
        void removeCameraSource(const QString &source);
        bool addScreenSource(const QString &source);
        void removeScreenSource(const QString &source);

        void setVideoInput(const QString &videoInput);
        void setState(AkElement::ElementState state);
        void setTorchMode(VideoLayer::TorchMode mode);
        void setPlayOnStart(bool playOnStart);
        void setOutputsAsInputs(bool outputsAsInputs);
        void resetVideoInput();
        void resetState();
        void resetTorchMode();
        void resetPlayOnStart();
        void resetOutputsAsInputs();
        void updateInputs();

    private slots:
        void refreshActiveSource();
        void handleSourceAudioCapsChanged(qint64 id, const AkAudioCaps &caps);
        void handleSourceVideoCapsChanged(qint64 id, const AkVideoCaps &caps);
        void handleSourceErrorChanged(qint64 id, const QString &error);
        void handleIsTorchSupportedChanged(qint64 id, bool torchSupported);
        void handleTorchModeChanged(qint64 id, VideoLayer::TorchMode mode);
        void handleCameraPermissionStatusChanged(qint64 id,
                                                  VideoLayer::PermissionStatus status);
};

#endif // VIDEOLAYERCOMPAT_H

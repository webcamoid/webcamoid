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
    Q_PROPERTY(QStringList videoSourceFileFilters
               READ videoSourceFileFilters
               CONSTANT)
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

        enum TorchMode {
            Torch_Off,
            Torch_On,
        };
        Q_ENUM(TorchMode)

        enum PermissionStatus {
            PermissionStatus_Undetermined,
            PermissionStatus_Granted,
            PermissionStatus_Denied,
        };
        Q_ENUM(PermissionStatus)

        VideoLayer(QQmlApplicationEngine *engine=nullptr,
                   QObject *parent=nullptr);
        ~VideoLayer();

        // Discovery (unchanged concept, reads from dedicated instances)
        Q_INVOKABLE QStringList videoSourceFileFilters() const;
        Q_INVOKABLE QStringList cameras() const;
        Q_INVOKABLE QStringList screens() const;
        Q_INVOKABLE QStringList windows() const;
        Q_INVOKABLE bool canCaptureWindows() const;
        Q_INVOKABLE QStringList supportedFileFormats() const;
        Q_INVOKABLE InputType deviceType(const QString &device) const;
        Q_INVOKABLE InputType deviceType(qint64 id) const;
        Q_INVOKABLE QStringList devicesByType(InputType type) const;
        Q_INVOKABLE QString description(const QString &device) const;

        // Global state -- the "instant play/stop everything" master switch.
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE bool playOnStart() const;
        Q_INVOKABLE bool outputsAsInputs() const;

        // Per-source queries. NOTE: no per-source state getter/setter --
        // effective playback = enabled(id) && state() == Playing. Only
        // "enabled" is independently controllable per source.
        Q_INVOKABLE QVariantList sourceIds() const;
        Q_INVOKABLE QString sourceDevice(qint64 id) const;
        Q_INVOKABLE QString sourceLabel(qint64 id) const;
        Q_INVOKABLE bool sourceEnabled(qint64 id) const;
        Q_INVOKABLE int sourceZOrder(qint64 id) const;
        Q_INVOKABLE AkAudioCaps sourceAudioCaps(qint64 id) const;
        Q_INVOKABLE AkVideoCaps sourceVideoCaps(qint64 id) const;
        Q_INVOKABLE QString sourceError(qint64 id) const;
        Q_INVOKABLE InputType sourceType(qint64 id) const;

        // Camera-specific per-source
        Q_INVOKABLE bool isTorchSupported(qint64 id) const;
        Q_INVOKABLE TorchMode torchMode(qint64 id) const;
        Q_INVOKABLE PermissionStatus cameraPermissionStatus(qint64 id) const;

        Q_INVOKABLE bool embedControls(const QString &where,
                                       qint64 id,
                                       const QString &name={}) const;
        Q_INVOKABLE void removeInterface(const QString &where) const;

    private:
        VideoLayerPrivate *d;

    signals:
        void camerasChanged(const QStringList &cameras);
        void screensChanged(const QStringList &screens);
        void windowsChanged(const QStringList &windows);
        void canCaptureWindowsChanged(bool canCaptureWindows);
        void stateChanged(AkElement::ElementState state);
        void playOnStartChanged(bool playOnStart);
        void outputsAsInputsChanged(bool outputsAsInputs);

        // Per-source signals
        void sourceAdded(qint64 id, const QString &device);
        void sourceRemoved(qint64 id);
        void sourceEnabledChanged(qint64 id, bool enabled);
        void sourceLabelChanged(qint64 id, const QString &label);
        void sourceZOrderChanged(qint64 id, int zOrder);
        void sourceAudioCapsChanged(qint64 id, const AkAudioCaps &audioCaps);
        void sourceVideoCapsChanged(qint64 id, const AkVideoCaps &videoCaps);
        void sourceErrorChanged(qint64 id, const QString &error);
        void isTorchSupportedChanged(qint64 id, bool torchSupported);
        void torchModeChanged(qint64 id, TorchMode mode);
        void cameraPermissionStatusChanged(qint64 id, PermissionStatus status);

        // oStream carries the source id in the packet
        void oStream(const AkPacket &packet);

    public slots:
        qint64 addSource(const QString &device);
        void removeSource(qint64 id);
        void setSourceEnabled(qint64 id, bool enabled);
        void setSourceLabel(qint64 id, const QString &label);
        void setSourceZOrder(qint64 id, int zOrder);
        void setState(AkElement::ElementState state);
        void setTorchMode(qint64 id, TorchMode mode);
        void setPlayOnStart(bool playOnStart);
        void setOutputsAsInputs(bool outputsAsInputs);
        void resetState();
        void resetTorchMode(qint64 id);
        void resetPlayOnStart();
        void resetOutputsAsInputs();
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);
        void updateInputs();

    private slots:
        void updateSourceCaps(qint64 id);
        void handleDevicesChanged();
        void handleSourceError(const QString &error);
        void handleStreamsChanged(const QList<int> &streams);
        void handleIsTorchSupportedChanged(bool torchSupported);
        void handleTorchModeChanged(TorchMode mode);
        void handlePermissionStatusChanged(PermissionStatus status);

        friend class VideoLayerPrivate;
};

Q_DECLARE_METATYPE(VideoLayer::InputType)
Q_DECLARE_METATYPE(VideoLayer::TorchMode)
Q_DECLARE_METATYPE(VideoLayer::PermissionStatus)

#endif // VIDEOLAYER_H

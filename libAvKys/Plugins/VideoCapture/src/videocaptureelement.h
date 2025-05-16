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

#ifndef VIDEOCAPTUREELEMENT_H
#define VIDEOCAPTUREELEMENT_H

#include <iak/akmultimediasourceelement.h>

class VideoCaptureElementPrivate;

class VideoCaptureElement: public AkMultimediaSourceElement
{
    Q_OBJECT
    Q_PROPERTY(QString error
               READ error
               NOTIFY errorChanged)
    Q_PROPERTY(QStringList medias
               READ medias
               NOTIFY mediasChanged)
    Q_PROPERTY(QString media
               READ media
               WRITE setMedia
               RESET resetMedia
               NOTIFY mediaChanged)
    Q_PROPERTY(QList<int> streams
               READ streams
               WRITE setStreams
               RESET resetStreams
               NOTIFY streamsChanged)
    Q_PROPERTY(bool loop
               READ loop
               WRITE setLoop
               RESET resetLoop
               NOTIFY loopChanged)
    Q_PROPERTY(QString ioMethod
               READ ioMethod
               WRITE setIoMethod
               RESET resetIoMethod
               NOTIFY ioMethodChanged)
    Q_PROPERTY(int nBuffers
               READ nBuffers
               WRITE setNBuffers
               RESET resetNBuffers
               NOTIFY nBuffersChanged)
    Q_PROPERTY(bool isTorchSupported
               READ isTorchSupported
               NOTIFY isTorchSupportedChanged)
    Q_PROPERTY(TorchMode torchMode
               READ torchMode
               WRITE setTorchMode
               RESET resetTorchMode
               NOTIFY torchModeChanged)
    Q_PROPERTY(PermissionStatus permissionStatus
               READ permissionStatus
               NOTIFY permissionStatusChanged)

    public:
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

        VideoCaptureElement();
        ~VideoCaptureElement();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE QStringList medias() override;
        Q_INVOKABLE QString media() const override;
        Q_INVOKABLE QList<int> streams() override;
        Q_INVOKABLE QList<int> listTracks(AkCaps::CapsType type=AkCaps::CapsUnknown);
        Q_INVOKABLE int defaultStream(AkCaps::CapsType type) override;
        Q_INVOKABLE QString description(const QString &media) override;
        Q_INVOKABLE AkCaps caps(int stream) override;
        Q_INVOKABLE AkCaps rawCaps(int stream) const;
        Q_INVOKABLE QString streamDescription(int stream) const;
        Q_INVOKABLE QStringList listCapsDescription() const;
        Q_INVOKABLE QString ioMethod() const;
        Q_INVOKABLE int nBuffers() const;
        Q_INVOKABLE QVariantList imageControls() const;
        Q_INVOKABLE bool setImageControls(const QVariantMap &imageControls);
        Q_INVOKABLE bool resetImageControls();
        Q_INVOKABLE QVariantList cameraControls() const;
        Q_INVOKABLE bool setCameraControls(const QVariantMap &cameraControls);
        Q_INVOKABLE bool resetCameraControls();
        Q_INVOKABLE bool isTorchSupported() const;
        Q_INVOKABLE TorchMode torchMode() const;
        Q_INVOKABLE PermissionStatus permissionStatus() const;

        // Specialized methods for Qml

        Q_INVOKABLE QStringList listFormats(const QString &device) const;
        Q_INVOKABLE QStringList listResolutions(const QString &device,
                                                int formatIndex) const;
        Q_INVOKABLE QStringList listFps(const QString &device,
                                        int formatIndex,
                                        int resolutionIndex) const;
        Q_INVOKABLE int formatIndex(const QString &device, int index) const;
        Q_INVOKABLE int resolutionIndex(const QString &device, int index) const;
        Q_INVOKABLE int fpsIndex(const QString &device, int index) const;
        Q_INVOKABLE int streamIndex(const QString &device,
                                    int formatIndex,
                                    int resolutionIndex,
                                    int fpsIndex) const;

    private:
        VideoCaptureElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;

    signals:
        void errorChanged(const QString &error);
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void loopChanged(bool loop);
        void ioMethodChanged(const QString &ioMethod);
        void nBuffersChanged(int nBuffers);
        void imageControlsChanged(const QVariantMap &imageControls);
        void cameraControlsChanged(const QVariantMap &cameraControls);
        void pictureTaken(int index, const AkPacket &picture);
        void isTorchSupportedChanged(bool torchSupported);
        void torchModeChanged(TorchMode mode);
        void permissionStatusChanged(PermissionStatus status);

    public slots:
        void setMedia(const QString &media) override;
        void setStreams(const QList<int> &streams) override;
        void setIoMethod(const QString &ioMethod);
        void setNBuffers(int nBuffers);
        void setTorchMode(TorchMode mode);
        void resetMedia() override;
        void resetStreams() override;
        void resetIoMethod();
        void resetNBuffers();
        void resetTorchMode();
        void reset();
        void takePictures(int count, int delayMsecs=0);
        bool setState(AkElement::ElementState state) override;
};

Q_DECLARE_METATYPE(VideoCaptureElement::TorchMode)
Q_DECLARE_METATYPE(VideoCaptureElement::PermissionStatus)

#endif // VIDEOCAPTUREELEMENT_H

/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

#ifndef CAPTUREANDROIDCAMERA_H
#define CAPTUREANDROIDCAMERA_H

#include "capture.h"

class CaptureAndroidCameraPrivate;

class CaptureAndroidCamera: public Capture
{
    Q_OBJECT

    public:
        CaptureAndroidCamera(QObject *parent=nullptr);
        ~CaptureAndroidCamera();

        Q_INVOKABLE QStringList webcams() const override;
        Q_INVOKABLE QString device() const override;
        Q_INVOKABLE QList<int> streams() override;
        Q_INVOKABLE QList<int> listTracks(AkCaps::CapsType type) override;
        Q_INVOKABLE QString ioMethod() const override;
        Q_INVOKABLE int nBuffers() const override;
        Q_INVOKABLE QString description(const QString &webcam) const override;
        Q_INVOKABLE CaptureVideoCaps caps(const QString &webcam) const override;
        Q_INVOKABLE QVariantList imageControls() const override;
        Q_INVOKABLE bool setImageControls(const QVariantMap &imageControls) override;
        Q_INVOKABLE bool resetImageControls() override;
        Q_INVOKABLE QVariantList cameraControls() const override;
        Q_INVOKABLE bool setCameraControls(const QVariantMap &cameraControls) override;
        Q_INVOKABLE bool resetCameraControls() override;
        Q_INVOKABLE FlashModeList supportedFlashModes(const QString &webcam) const override;
        Q_INVOKABLE FlashMode flashMode() const override;
        Q_INVOKABLE AkPacket readFrame() override;

    private:
        CaptureAndroidCameraPrivate *d;

    public slots:
        bool init() override;
        void uninit() override;
        void setDevice(const QString &device) override;
        void setStreams(const QList<int> &streams) override;
        void setIoMethod(const QString &ioMethod) override;
        void setNBuffers(int nBuffers) override;
        void setFlashMode(FlashMode mode) override;
        void resetDevice() override;
        void resetStreams() override;
        void resetIoMethod() override;
        void resetNBuffers() override;
        void resetFlashMode() override;
        void reset() override;
        void takePictures(int count, int delayMsecs=0) override;
};

#endif // CAPTUREANDROIDCAMERA_H

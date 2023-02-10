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

#ifndef CAPTURE_H
#define CAPTURE_H

#include <akcaps.h>

class Capture;
class CapturePrivate;
class AkPacket;

using CapturePtr = QSharedPointer<Capture>;
using CaptureVideoCaps = QVector<AkCaps>;

class Capture: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString error
               READ error
               NOTIFY errorChanged)
    Q_PROPERTY(QStringList webcams
               READ webcams
               NOTIFY webcamsChanged)
    Q_PROPERTY(QString device
               READ device
               WRITE setDevice
               RESET resetDevice
               NOTIFY deviceChanged)
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
    Q_PROPERTY(FlashMode flashMode
               READ flashMode
               WRITE setFlashMode
               RESET resetFlashMode
               NOTIFY flashModeChanged)

    public:
        enum FlashMode
        {
            FlashMode_Off,
            FlashMode_On,
            FlashMode_Auto,
            FlashMode_Torch,
            FlashMode_RedEye,
            FlashMode_External,
        };
        using FlashModeList = QList<FlashMode>;

        Capture(QObject *parent=nullptr);
        ~Capture();

        Q_INVOKABLE virtual QString error() const;
        Q_INVOKABLE virtual QStringList webcams() const;
        Q_INVOKABLE virtual QString device() const;
        Q_INVOKABLE virtual QList<int> streams();
        Q_INVOKABLE virtual QList<int> listTracks(AkCaps::CapsType type);
        Q_INVOKABLE virtual QString ioMethod() const;
        Q_INVOKABLE virtual int nBuffers() const;
        Q_INVOKABLE virtual QString description(const QString &webcam) const;
        Q_INVOKABLE virtual CaptureVideoCaps caps(const QString &webcam) const;
        Q_INVOKABLE virtual QVariantList imageControls() const;
        Q_INVOKABLE virtual bool setImageControls(const QVariantMap &imageControls);
        Q_INVOKABLE virtual bool resetImageControls();
        Q_INVOKABLE virtual QVariantList cameraControls() const;
        Q_INVOKABLE virtual bool setCameraControls(const QVariantMap &cameraControls);
        Q_INVOKABLE virtual bool resetCameraControls();
        Q_INVOKABLE virtual FlashModeList supportedFlashModes(const QString &webcam) const;
        Q_INVOKABLE virtual FlashMode flashMode() const;
        Q_INVOKABLE virtual AkPacket readFrame();

    private:
        CapturePrivate *d;

    signals:
        void errorChanged(const QString &error);
        void webcamsChanged(const QStringList &webcams);
        void deviceChanged(const QString &device);
        void streamsChanged(const QList<int> &streams);
        void ioMethodChanged(const QString &ioMethod);
        void nBuffersChanged(int nBuffers);
        void imageControlsChanged(const QVariantMap &imageControls);
        void cameraControlsChanged(const QVariantMap &cameraControls);
        void pictureTaken(int index, const AkPacket &picture);
        void flashModeChanged(FlashMode mode);

    public slots:
        virtual bool init();
        virtual void uninit();
        virtual void setDevice(const QString &device);
        virtual void setStreams(const QList<int> &streams);
        virtual void setIoMethod(const QString &ioMethod);
        virtual void setNBuffers(int nBuffers);
        virtual void setFlashMode(FlashMode mode);
        virtual void resetDevice();
        virtual void resetStreams();
        virtual void resetIoMethod();
        virtual void resetNBuffers();
        virtual void resetFlashMode();
        virtual void reset();
        virtual void takePictures(int count, int delayMsecs=0);
};

#endif // CAPTURE_H

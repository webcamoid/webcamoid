/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include <QWaitCondition>
#include <QMutex>
#include <QTimer>
#include <ak.h>

class CapturePrivate;

class Capture: public QObject
{
    Q_OBJECT
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

    public:
        enum IoMethod
        {
            IoMethodUnknown = -1,
            IoMethodReadWrite,
            IoMethodMemoryMap,
            IoMethodUserPointer
        };

        explicit Capture();
        ~Capture();

        Q_INVOKABLE QStringList webcams() const;
        Q_INVOKABLE QString device() const;
        Q_INVOKABLE QList<int> streams() const;
        Q_INVOKABLE QList<int> listTracks(const QString &mimeType);
        Q_INVOKABLE QString ioMethod() const;
        Q_INVOKABLE int nBuffers() const;
        Q_INVOKABLE QString description(const QString &webcam) const;
        Q_INVOKABLE QVariantList caps(const QString &webcam) const;
        Q_INVOKABLE QString capsDescription(const AkCaps &caps) const;
        Q_INVOKABLE QVariantList imageControls() const;
        Q_INVOKABLE bool setImageControls(const QVariantMap &imageControls);
        Q_INVOKABLE bool resetImageControls();
        Q_INVOKABLE QVariantList cameraControls() const;
        Q_INVOKABLE bool setCameraControls(const QVariantMap &cameraControls);
        Q_INVOKABLE bool resetCameraControls();
        Q_INVOKABLE AkPacket readFrame();

        QMutex &mutex();
        QWaitCondition &frameReady();
        void *curFrame();

    private:
        QStringList m_webcams;
        QString m_device;
        QList<int> m_streams;
        IoMethod m_ioMethod;
        int m_nBuffers;
        QMutex m_mutex;
        QMutex m_controlsMutex;
        QWaitCondition m_frameReady;
        AkFrac m_fps;
        AkFrac m_timeBase;
        AkCaps m_caps;
        qint64 m_id;
        CapturePrivate *d;

        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;

        QVariantMap controlStatus(const QVariantList &controls) const;

    signals:
        void webcamsChanged(const QStringList &webcams) const;
        void deviceChanged(const QString &device);
        void streamsChanged(const QList<int> &streams);
        void ioMethodChanged(const QString &ioMethod);
        void nBuffersChanged(int nBuffers);
        void error(const QString &message);
        void imageControlsChanged(const QVariantMap &imageControls) const;
        void cameraControlsChanged(const QVariantMap &cameraControls) const;

    public slots:
        bool init();
        void uninit();
        void setDevice(const QString &device);
        void setStreams(const QList<int> &streams);
        void setIoMethod(const QString &ioMethod);
        void setNBuffers(int nBuffers);
        void resetDevice();
        void resetStreams();
        void resetIoMethod();
        void resetNBuffers();
        void reset();

        void cameraConnected();
        void cameraDisconnected();

    private slots:
        void updateWebcams();
};

#endif // CAPTURE_H

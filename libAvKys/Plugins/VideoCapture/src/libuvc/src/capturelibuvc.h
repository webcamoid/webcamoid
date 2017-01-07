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

#ifndef CAPTURELIBUVC_H
#define CAPTURELIBUVC_H

#include <QtConcurrent>
#include <libuvc/libuvc.h>
#include <ak.h>

#include "capture.h"

/* libuvc requires RW permissions for opening capturing devices, so you must
 * create the following .rules file:
 *
 * /etc/udev/rules.d/99-uvc.rules
 *
 * Then, for each webcam add the following line:
 *
 * SUBSYSTEMS=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="XXXX", ATTRS{idProduct}=="YYYY", MODE="0666"
 *
 * Replace XXXX and YYYY for the 4 hexadecimal characters corresponding to the
 * vendor and product ID of your webcams. Read more at:
 *
 * http://wiki.ros.org/libuvc_camera#Usage
 */

class CaptureLibUVC: public Capture
{
    Q_OBJECT

    public:
        enum IoMethod
        {
            IoMethodUnknown = -1,
            IoMethodReadWrite,
            IoMethodMemoryMap,
            IoMethodUserPointer
        };

        explicit CaptureLibUVC(QObject *parent=NULL);
        ~CaptureLibUVC();

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
        Q_INVOKABLE QString uvcId(quint16 vendorId, quint16 productId) const;

    private:
        QString m_device;
        QList<int> m_streams;
        QMap<quint32, QString> m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, QVariantList> m_devicesCaps;
        QMap<QString, QVariantList> m_imageControls;
        QMap<QString, QVariantList> m_cameraControls;
        QString m_curDevice;
        AkPacket m_curPacket;
        uvc_context_t *m_uvcContext;
        uvc_device_handle_t *m_deviceHnd;
        QThreadPool m_threadPool;
        QWaitCondition m_packetNotReady;
        QMutex m_mutex;
        qint64 m_id;
        AkFrac m_fps;

        QVariantList controlsList(uvc_device_handle_t *deviceHnd,
                                  uint8_t unit,
                                  uint8_t control,
                                  int controlType) const;
        void setControls(uvc_device_handle_t *deviceHnd,
                         uint8_t unit,
                         uint8_t control,
                         int controlType,
                         const QVariantMap &values);
        static void frameCallback(struct uvc_frame *frame, void *userData);

        inline QString fourccToStr(const uint8_t *format) const
        {
            char fourcc[5];
            memcpy(fourcc, format, sizeof(quint32));
            fourcc[4] = 0;

            return QString(fourcc);
        }

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

    private slots:
        void updateDevices();
};

#endif // CAPTURELIBUVC_H

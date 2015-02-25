/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/videodev2.h>
#include <QFileSystemWatcher>
#include <QDir>
#include <QSize>

#include <qb.h>

#include "platform/capturebuffer.h"

class Capture: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList webcams
               READ webcams
               NOTIFY webcamsChanged)
    Q_PROPERTY(QString device
               READ device
               WRITE setDevice
               RESET resetDevice)
    Q_PROPERTY(QString ioMethod
               READ ioMethod
               WRITE setIoMethod
               RESET resetIoMethod)
    Q_PROPERTY(int nBuffers
               READ nBuffers
               WRITE setNBuffers
               RESET resetNBuffers)
    Q_PROPERTY(bool isCompressed
               READ isCompressed)
    Q_PROPERTY(QbCaps caps
               READ caps)

    public:
        enum IoMethod
        {
            IoMethodUnknown = -1,
            IoMethodReadWrite,
            IoMethodMemoryMap,
            IoMethodUserPointer
        };

        explicit Capture();

        Q_INVOKABLE QStringList webcams() const;
        Q_INVOKABLE QString device() const;
        Q_INVOKABLE QString ioMethod() const;
        Q_INVOKABLE int nBuffers() const;
        Q_INVOKABLE bool isCompressed() const;
        Q_INVOKABLE QbCaps caps(v4l2_format *format = NULL, bool *changePxFmt = NULL) const;
        Q_INVOKABLE QString description(const QString &webcam) const;
        Q_INVOKABLE QVariantList availableSizes(const QString &webcam) const;
        Q_INVOKABLE QSize size(const QString &webcam) const;
        Q_INVOKABLE bool setSize(const QString &webcam, const QSize &size);
        Q_INVOKABLE bool resetSize(const QString &webcam);
        Q_INVOKABLE QVariantList imageControls(const QString &webcam) const;
        Q_INVOKABLE bool setImageControls(const QString &webcam, const QVariantMap &imageControls) const;
        Q_INVOKABLE bool resetImageControls(const QString &webcam) const;
        Q_INVOKABLE QVariantList cameraControls(const QString &webcam) const;
        Q_INVOKABLE bool setCameraControls(const QString &webcam, const QVariantMap &cameraControls) const;
        Q_INVOKABLE bool resetCameraControls(const QString &webcam) const;
        Q_INVOKABLE QbPacket readFrame();

    private:
        QStringList m_webcams;
        QString m_device;
        IoMethod m_ioMethod;
        int m_nBuffers;

        QMap<v4l2_ctrl_type, QString> m_ctrlTypeToString;
        QFileSystemWatcher *m_fsWatcher;
        int m_fd;
        QbFrac m_fps;
        QbFrac m_timeBase;
        QbCaps m_caps;
        qint64 m_id;
        QVector<CaptureBuffer> m_buffers;
        QMap<quint32, QString> m_rawToFF;
        QMap<quint32, QString> m_compressedToFF;

        quint32 defaultFormat(int fd, bool compressed) const;
        QString v4l2ToFF(quint32 fmt) const;
        QbFrac fps(int fd) const;
        quint32 format(const QString &webcam, const QSize &size) const;
        QVariantList controls(const QString &webcam, quint32 controlClass) const;
        QVariantList queryControl(int handle, quint32 controlClass, v4l2_queryctrl *queryctrl) const;
        QMap<QString, quint32> findImageControls(int handle) const;
        bool initReadWrite(quint32 bufferSize);
        bool initMemoryMap();
        bool initUserPointer(quint32 bufferSize);
        bool startCapture();
        void stopCapture();
        bool isCompressedFormat(quint32 format);

        inline QString fourccToStr(quint32 format) const
        {
            QString fourcc;

            for (int i = 0; i < 4; i++) {
                fourcc.append(QChar(format & 0xff));
                format >>= 8;
            }

            return fourcc;
        }

        inline int xioctl(int fd, int request, void *arg) const
        {
            int r = -1;

            while (true) {
                r = ioctl(fd, request, arg);

                if (r != -1 || errno != EINTR)
                    break;
            }

            return r;
        }

        inline QbPacket processFrame(char *buffer, quint32 bufferSize, qint64 pts) const
        {
            QbBufferPtr oBuffer(new char[bufferSize]);

            if (!oBuffer)
                return QbPacket();

            memcpy(oBuffer.data(), buffer, bufferSize);

            QbPacket oPacket(this->m_caps,
                             oBuffer,
                             bufferSize);

            oPacket.setPts(pts);
            oPacket.setTimeBase(this->m_timeBase);
            oPacket.setIndex(0);
            oPacket.setId(this->m_id);

            return oPacket;
        }

    signals:
        void error(const QString &message);
        void webcamsChanged(const QStringList &webcams) const;
        void sizeChanged(const QString &webcam, const QSize &size) const;
        void imageControlsChanged(const QString &webcam, const QVariantMap &imageControls) const;
        void cameraControlsChanged(const QString &webcam, const QVariantMap &cameraControls) const;

    public slots:
        bool init();
        void uninit();
        void setDevice(const QString &device);
        void setIoMethod(const QString &ioMethod);
        void setNBuffers(int nBuffers);
        void resetDevice();
        void resetIoMethod();
        void resetNBuffers();
        void reset(const QString &webcam);

    private slots:
        void onDirectoryChanged(const QString &path);
};

#endif // CAPTURE_H

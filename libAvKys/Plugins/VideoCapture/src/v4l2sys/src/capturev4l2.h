/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef CAPTUREV4L2_H
#define CAPTUREV4L2_H

#include <sys/mman.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <QFileSystemWatcher>
#include <ak.h>

#include "capture.h"
#include "capturebuffer.h"

#ifdef HAVE_V4LUTILS
#include <libv4l2.h>

#define x_ioctl v4l2_ioctl
#define x_open v4l2_open
#define x_close v4l2_close
#define x_read v4l2_read
#define x_mmap v4l2_mmap
#define x_munmap v4l2_munmap
#else
#include <unistd.h>
#include <sys/ioctl.h>

#define x_ioctl ioctl
#define x_open open
#define x_close close
#define x_read read
#define x_mmap mmap
#define x_munmap munmap
#endif

class CaptureV4L2: public Capture
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

        explicit CaptureV4L2(QObject *parent=NULL);
        ~CaptureV4L2();

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

    private:
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, QVariantList> m_devicesCaps;
        QMap<QString, QVariantList> m_imageControls;
        QMap<QString, QVariantList> m_cameraControls;
        IoMethod m_ioMethod;
        int m_nBuffers;
        QFileSystemWatcher *m_fsWatcher;
        int m_fd;
        AkFrac m_fps;
        AkFrac m_timeBase;
        AkCaps m_caps;
        qint64 m_id;
        QVector<CaptureBuffer> m_buffers;

        Q_INVOKABLE QVariantList capsFps(int fd,
                                         const v4l2_fmtdesc &format,
                                         __u32 width,
                                         __u32 height) const;
        AkFrac fps(int fd) const;
        void setFps(int fd, const AkFrac &fps);
        QVariantList controls(int fd, quint32 controlClass) const;
        bool setControls(int fd, quint32 controlClass, const QVariantMap &controls) const;
        QVariantList queryControl(int handle, quint32 controlClass, v4l2_queryctrl *queryctrl) const;
        QMap<QString, quint32> findControls(int handle, quint32 controlClass) const;
        bool initReadWrite(quint32 bufferSize);
        bool initMemoryMap();
        bool initUserPointer(quint32 bufferSize);
        bool startCapture();
        void stopCapture();

        inline QString fourccToStr(quint32 format) const
        {
            char fourcc[5];
            memcpy(fourcc, &format, sizeof(quint32));
            fourcc[4] = 0;

            return QString(fourcc);
        }

        inline quint32 strToFourCC(const QString &format) const
        {
            quint32 fourcc;
            memcpy(&fourcc, format.toStdString().c_str(), sizeof(quint32));

            return fourcc;
        }

        inline int xioctl(int fd, ulong request, void *arg) const
        {
            int r = -1;

            forever {
                r = x_ioctl(fd, request, arg);

                if (r != -1 || errno != EINTR)
                    break;
            }

            return r;
        }

        inline AkPacket processFrame(const char *buffer,
                                     size_t bufferSize,
                                     qint64 pts) const
        {
            QByteArray oBuffer(buffer, int(bufferSize));
            AkPacket oPacket(this->m_caps, oBuffer);

            oPacket.setPts(pts);
            oPacket.setTimeBase(this->m_timeBase);
            oPacket.setIndex(0);
            oPacket.setId(this->m_id);

            return oPacket;
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
        void onDirectoryChanged(const QString &path);
        void onFileChanged(const QString &fileName);
};

#endif // CAPTUREV4L2_H

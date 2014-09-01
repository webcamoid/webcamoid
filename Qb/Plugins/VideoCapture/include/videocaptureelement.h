/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef VIDEOCAPTUREELEMENT_H
#define VIDEOCAPTUREELEMENT_H

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/videodev2.h>

#include <qb.h>

#include "capturebuffer.h"

class VideoCaptureElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QStringList webcams READ webcams NOTIFY webcamsChanged)
    Q_PROPERTY(QString device READ device WRITE setDevice RESET resetDevice)
    Q_PROPERTY(QString ioMethod READ ioMethod WRITE setIoMethod RESET resetIoMethod)
    Q_PROPERTY(int nBuffers READ nBuffers WRITE setNBuffers RESET resetNBuffers)
    Q_PROPERTY(bool isCompressed READ isCompressed)
    Q_PROPERTY(QString caps READ caps)

    public:
        enum IoMethod
        {
            IoMethodUnknown = -1,
            IoMethodReadWrite,
            IoMethodMemoryMap,
            IoMethodUserPointer
        };

        explicit VideoCaptureElement();

        Q_INVOKABLE QStringList webcams() const;
        Q_INVOKABLE QString device() const;
        Q_INVOKABLE QString ioMethod() const;
        Q_INVOKABLE int nBuffers() const;
        Q_INVOKABLE bool isCompressed() const;
        Q_INVOKABLE QString caps(v4l2_format *format = NULL, bool *changePxFmt = NULL) const;
        Q_INVOKABLE QString description(const QString &webcam) const;
        Q_INVOKABLE QVariantList availableSizes(const QString &webcam) const;
        Q_INVOKABLE QSize size(const QString &webcam) const;
        Q_INVOKABLE bool setSize(const QString &webcam, const QSize &size) const;
        Q_INVOKABLE bool resetSize(const QString &webcam) const;
        Q_INVOKABLE QVariantList controls(const QString &webcam) const;
        Q_INVOKABLE bool setControls(const QString &webcam, const QVariantMap &controls) const;
        Q_INVOKABLE bool resetControls(const QString &webcam) const;
        bool event(QEvent *event);

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
        QTimer m_timer;

        __u32 format(const QString &webcam, const QSize &size) const;
        QVariantList queryControl(int handle, v4l2_queryctrl *queryctrl) const;
        QMap<QString, uint> findControls(int handle) const;
        int xioctl(int fd, int request, void *arg) const;
        QString v4l2ToFF(quint32 fmt) const;
        quint32 defaultFormat(int fd, bool compressed) const;
        bool isCompressedFormat(quint32 format);
        bool initReadWrite(quint32 bufferSize);
        bool initMemoryMap();
        bool initUserPointer(quint32 bufferSize);
        void stopCapture();
        void uninit();
        bool startCapture();
        bool init();
        void processFrame(char *buffer, quint32 bufferSize, qint64 pts);
        QString fourccToStr(quint32 format) const;
        QbFrac fps(int fd) const;

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    signals:
        void error(const QString &message);
        void webcamsChanged(const QStringList &webcams) const;
        void sizeChanged(const QString &webcam, const QSize &size) const;
        void controlsChanged(const QString &webcam, const QVariantMap &controls) const;

    public slots:
        void setDevice(const QString &device);
        void setIoMethod(const QString &ioMethod);
        void setNBuffers(int nBuffers);
        void resetDevice();
        void resetIoMethod();
        void resetNBuffers();
        void reset(const QString &webcam) const;
        void reset() const;

    private slots:
        void onDirectoryChanged(const QString &path);
        bool readFrame();
};

#endif // VIDEOCAPTUREELEMENT_H

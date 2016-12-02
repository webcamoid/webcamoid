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

#ifndef CAMERAOUTV4L2_H
#define CAMERAOUTV4L2_H

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <QFileSystemWatcher>
#include <QDir>
#include <QSize>

#include "cameraout.h"

class CameraOutV4L2: public CameraOut
{
    Q_OBJECT

    public:
        enum RootMethod
        {
            RootMethodSu,
            RootMethodSudo
        };

        explicit CameraOutV4L2(QObject *parent=NULL);
        ~CameraOutV4L2();

        Q_INVOKABLE QString driverPath() const;
        Q_INVOKABLE QStringList webcams() const;
        Q_INVOKABLE QString device() const;
        Q_INVOKABLE int streamIndex() const;
        Q_INVOKABLE AkCaps caps() const;
        Q_INVOKABLE QString description(const QString &webcam) const;
        Q_INVOKABLE void writeFrame(const AkPacket &frame);
        Q_INVOKABLE int maxCameras() const;
        Q_INVOKABLE bool needRoot() const;
        Q_INVOKABLE int passwordTimeout() const;
        Q_INVOKABLE QString rootMethod() const;
        Q_INVOKABLE QString createWebcam(const QString &description,
                                         const QString &password) const;
        Q_INVOKABLE bool changeDescription(const QString &webcam,
                                           const QString &description,
                                           const QString &password) const;
        Q_INVOKABLE bool removeWebcam(const QString &webcam,
                                      const QString &password) const;
        Q_INVOKABLE bool removeAllWebcams(const QString &password) const;

    private:
        QString m_driverPath;
        QStringList m_webcams;
        QString m_device;
        int m_streamIndex;
        AkCaps m_caps;
        int m_passwordTimeout;
        RootMethod m_rootMethod;
        QFileSystemWatcher *m_fsWatcher;
        int m_fd;

        inline int xioctl(int fd, ulong request, void *arg) const
        {
            int r = -1;

            forever {
                r = ioctl(fd, request, arg);

                if (r != -1 || errno != EINTR)
                    break;
            }

            return r;
        }

        bool isModuleLoaded() const;
        bool sudo(const QString &command,
                  const QStringList &argumments,
                  const QString &password) const;
        void rmmod(const QString &password) const;

    public slots:
        bool init(int streamIndex, const AkCaps &caps);
        void uninit();
        void setDriverPath(const QString &driverPath);
        void setDevice(const QString &device);
        void setPasswordTimeout(int passwordTimeout);
        void setRootMethod(const QString &rootMethod);
        void resetDriverPath();
        void resetDevice();
        void resetPasswordTimeout();
        void resetRootMethod();

    private slots:
        void onDirectoryChanged(const QString &path);
};

#endif // CAMERAOUTV4L2_H

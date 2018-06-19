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

#ifndef CAMERAOUT_H
#define CAMERAOUT_H

#include <akcaps.h>

class CameraOut;
class AkPacket;
typedef QSharedPointer<CameraOut> CameraOutPtr;

class CameraOut: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList driverPaths
               READ driverPaths
               WRITE setDriverPaths
               RESET resetDriverPaths
               NOTIFY driverPathsChanged)
    Q_PROPERTY(QStringList webcams
               READ webcams
               NOTIFY webcamsChanged)
    Q_PROPERTY(QString device
               READ device
               WRITE setDevice
               RESET resetDevice
               NOTIFY deviceChanged)
    Q_PROPERTY(int streamIndex
               READ streamIndex)
    Q_PROPERTY(AkCaps caps
               READ caps
               WRITE setCaps
               RESET resetCaps
               NOTIFY capsChanged)
    Q_PROPERTY(int maxCameras
               READ maxCameras)
    Q_PROPERTY(bool needRoot
               READ needRoot
               NOTIFY needRootChanged)
    Q_PROPERTY(int passwordTimeout
               READ passwordTimeout
               WRITE setPasswordTimeout
               RESET resetPasswordTimeout
               NOTIFY passwordTimeoutChanged)
    Q_PROPERTY(QString rootMethod
               READ rootMethod
               WRITE setRootMethod
               RESET resetRootMethod
               NOTIFY rootMethodChanged)

    public:
        explicit CameraOut(QObject *parent=nullptr);
        virtual ~CameraOut();

        Q_INVOKABLE virtual QStringList driverPaths() const;
        Q_INVOKABLE virtual QStringList webcams() const;
        Q_INVOKABLE virtual QString device() const;
        Q_INVOKABLE virtual int streamIndex() const;
        Q_INVOKABLE virtual AkCaps caps() const;
        Q_INVOKABLE virtual QString description(const QString &webcam) const;
        Q_INVOKABLE virtual void writeFrame(const AkPacket &frame);
        Q_INVOKABLE virtual int maxCameras() const;
        Q_INVOKABLE virtual bool needRoot() const;
        Q_INVOKABLE virtual int passwordTimeout() const;
        Q_INVOKABLE virtual QString rootMethod() const;
        Q_INVOKABLE virtual QString createWebcam(const QString &description="",
                                                 const QString &password="");
        Q_INVOKABLE virtual bool changeDescription(const QString &webcam,
                                                   const QString &description="",
                                                   const QString &password="");
        Q_INVOKABLE virtual bool removeWebcam(const QString &webcam,
                                              const QString &password="");
        Q_INVOKABLE virtual bool removeAllWebcams(const QString &password="");

    protected:
        QStringList m_driverPaths;
        QString m_device;
        AkCaps m_caps;
        int m_passwordTimeout;
        QString m_rootMethod;

    signals:
        void driverPathsChanged(const QStringList &driverPaths);
        void webcamsChanged(const QStringList &webcams) const;
        void deviceChanged(const QString &device);
        void capsChanged(const AkCaps &caps);
        void needRootChanged(bool needRoot);
        void passwordTimeoutChanged(int passwordTimeout);
        void rootMethodChanged(const QString &rootMethod);
        void error(const QString &message);

    public slots:
        virtual bool init(int streamIndex);
        virtual void uninit();
        virtual bool setDriverPaths(const QStringList &driverPaths);
        virtual bool addDriverPath(const QString &driverPath);
        virtual bool addDriverPaths(const QStringList &driverPaths);
        virtual bool removeDriverPath(const QString &driverPath);
        virtual bool removeDriverPaths(const QStringList &driverPaths);
        virtual void setDevice(const QString &device);
        virtual void setCaps(const AkCaps &caps);
        virtual void setPasswordTimeout(int passwordTimeout);
        virtual void setRootMethod(const QString &rootMethod);
        virtual void resetDriverPaths();
        virtual void resetDevice();
        virtual void resetCaps();
        virtual void resetPasswordTimeout();
        virtual void resetRootMethod();
};

#endif // CAMERAOUT_H

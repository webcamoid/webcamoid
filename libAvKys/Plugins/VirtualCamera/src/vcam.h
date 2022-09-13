/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#ifndef VCAM_H
#define VCAM_H

#include <QObject>
#include <QVariant>
#include <akvideocaps.h>
#include <akvideopacket.h>

class VCam;
using VCamPtr = QSharedPointer<VCam>;

class VCam: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString error
               READ error
               NOTIFY errorChanged)
    Q_PROPERTY(bool isInstalled
               READ isInstalled
               CONSTANT)
    Q_PROPERTY(QStringList webcams
               READ webcams
               NOTIFY webcamsChanged)
    Q_PROPERTY(QString device
               READ device
               WRITE setDevice
               RESET resetDevice
               NOTIFY deviceChanged)
    Q_PROPERTY(AkVideoCaps currentCaps
               READ currentCaps
               WRITE setCurrentCaps
               RESET resetCurrentCaps
               NOTIFY currentCapsChanged)
    Q_PROPERTY(QString picture
               READ picture
               WRITE setPicture
               RESET resetPicture
               NOTIFY pictureChanged)
    Q_PROPERTY(QString rootMethod
               READ rootMethod
               WRITE setRootMethod
               RESET resetRootMethod
               NOTIFY rootMethodChanged)
    Q_PROPERTY(QStringList availableRootMethods
               READ availableRootMethods
               CONSTANT)

    public:
        VCam(QObject *parent=nullptr);
        virtual ~VCam() = default;

        Q_INVOKABLE virtual QString error() const;
        Q_INVOKABLE virtual bool isInstalled() const;
        Q_INVOKABLE virtual QString installedVersion() const;
        Q_INVOKABLE virtual QStringList webcams() const;
        Q_INVOKABLE virtual QString device() const;
        Q_INVOKABLE virtual QString description(const QString &webcam) const;
        Q_INVOKABLE virtual QList<AkVideoCaps::PixelFormat> supportedOutputPixelFormats() const;
        Q_INVOKABLE virtual AkVideoCaps::PixelFormat defaultOutputPixelFormat() const;
        Q_INVOKABLE virtual AkVideoCapsList caps(const QString &webcam) const;
        Q_INVOKABLE virtual AkVideoCaps currentCaps() const;
        Q_INVOKABLE virtual QVariantList controls() const;
        Q_INVOKABLE virtual bool setControls(const QVariantMap &controls);
        Q_INVOKABLE virtual QList<quint64> clientsPids() const;
        Q_INVOKABLE virtual QString clientExe(quint64 pid) const;
        Q_INVOKABLE virtual QString picture() const;
        Q_INVOKABLE virtual QString rootMethod() const;
        Q_INVOKABLE virtual QStringList availableRootMethods() const;
        Q_INVOKABLE virtual QString deviceCreate(const QString &description,
                                                 const AkVideoCapsList &caps);
        Q_INVOKABLE virtual bool deviceEdit(const QString &deviceId,
                                            const QString &description,
                                            const AkVideoCapsList &caps);
        Q_INVOKABLE virtual bool changeDescription(const QString &deviceId,
                                                   const QString &description);
        Q_INVOKABLE virtual bool deviceDestroy(const QString &deviceId);
        Q_INVOKABLE virtual bool destroyAllDevices();

    signals:
        void errorChanged(const QString &error);
        void webcamsChanged(const QStringList &webcams);
        void deviceChanged(const QString &device);
        void currentCapsChanged(const AkVideoCaps &error);
        void pictureChanged(const QString &picture);
        void rootMethodChanged(const QString &rootMethod);
        void controlsChanged(const QVariantMap &controls);

    public slots:
        virtual bool init();
        virtual void uninit();
        virtual bool write(const AkVideoPacket &packet);
        virtual bool applyPicture();
        virtual void setDevice(const QString &device);
        virtual void setCurrentCaps(const AkVideoCaps &currentCaps);
        virtual void setPicture(const QString &picture);
        virtual void setRootMethod(const QString &rootMethod);
        virtual void resetDevice();
        virtual void resetCurrentCaps();
        virtual void resetPicture();
        virtual void resetRootMethod();
};

#endif // VCAM_H

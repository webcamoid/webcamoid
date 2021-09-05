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

#ifndef VCAMCMIO_H
#define VCAMCMIO_H

#include "vcam.h"

class VCamCMIOPrivate;

class VCamCMIO: public VCam
{
    Q_OBJECT

    public:
        VCamCMIO(QObject *parent=nullptr);
        ~VCamCMIO();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE bool isInstalled() const;
        Q_INVOKABLE QString installedVersion() const;
        Q_INVOKABLE QStringList webcams() const;
        Q_INVOKABLE QString device() const;
        Q_INVOKABLE QString description(const QString &webcam) const;
        Q_INVOKABLE QList<AkVideoCaps::PixelFormat> supportedOutputPixelFormats() const;
        Q_INVOKABLE AkVideoCaps::PixelFormat defaultOutputPixelFormat() const;
        Q_INVOKABLE AkVideoCapsList caps(const QString &webcam) const;
        Q_INVOKABLE AkVideoCaps currentCaps() const;
        Q_INVOKABLE QVariantList controls() const;
        Q_INVOKABLE bool setControls(const QVariantMap &controls);
        Q_INVOKABLE QList<quint64> clientsPids() const;
        Q_INVOKABLE QString clientExe(quint64 pid) const;
        Q_INVOKABLE QString picture() const;
        Q_INVOKABLE QString rootMethod() const;
        Q_INVOKABLE QStringList availableRootMethods() const;
        Q_INVOKABLE QString deviceCreate(const QString &description,
                                         const AkVideoCapsList &caps);
        Q_INVOKABLE bool deviceEdit(const QString &deviceId,
                                    const QString &description,
                                    const AkVideoCapsList &caps);
        Q_INVOKABLE bool changeDescription(const QString &deviceId,
                                           const QString &description);
        Q_INVOKABLE bool deviceDestroy(const QString &deviceId);
        Q_INVOKABLE bool destroyAllDevices();

    private:
        VCamCMIOPrivate *d;

    public slots:
        bool init();
        void uninit();
        void setDevice(const QString &device);
        void setCurrentCaps(const AkVideoCaps &currentCaps);
        void setPicture(const QString &picture);
        void setRootMethod(const QString &rootMethod);
        bool applyPicture();
        bool write(const AkVideoPacket &packet);

    friend class VCamCMIOPrivate;
};

#endif // VCAMCMIO_H

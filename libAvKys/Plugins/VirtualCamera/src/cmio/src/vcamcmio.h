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

        Q_INVOKABLE QString error() const override;
        Q_INVOKABLE bool isInstalled() const override;
        Q_INVOKABLE QString installedVersion() const override;
        Q_INVOKABLE QStringList webcams() const override;
        Q_INVOKABLE QString device() const override;
        Q_INVOKABLE QString description(const QString &webcam) const override;
        Q_INVOKABLE QList<AkVideoCaps::PixelFormat> supportedOutputPixelFormats() const override;
        Q_INVOKABLE AkVideoCaps::PixelFormat defaultOutputPixelFormat() const override;
        Q_INVOKABLE AkVideoCapsList caps(const QString &webcam) const override;
        Q_INVOKABLE AkVideoCaps currentCaps() const override;
        Q_INVOKABLE QVariantList controls() const override;
        Q_INVOKABLE bool setControls(const QVariantMap &controls) override;
        Q_INVOKABLE QList<quint64> clientsPids() const override;
        Q_INVOKABLE QString clientExe(quint64 pid) const override;
        Q_INVOKABLE QString picture() const override;
        Q_INVOKABLE QString rootMethod() const override;
        Q_INVOKABLE QStringList availableRootMethods() const override;
        Q_INVOKABLE QString deviceCreate(const QString &description,
                                         const AkVideoCapsList &caps) override;
        Q_INVOKABLE bool deviceEdit(const QString &deviceId,
                                    const QString &description,
                                    const AkVideoCapsList &caps) override;
        Q_INVOKABLE bool changeDescription(const QString &deviceId,
                                           const QString &description) override;
        Q_INVOKABLE bool deviceDestroy(const QString &deviceId) override;
        Q_INVOKABLE bool destroyAllDevices() override;

    private:
        VCamCMIOPrivate *d;

    public slots:
        bool init() override;
        void uninit() override;
        void setDevice(const QString &device) override;
        void setCurrentCaps(const AkVideoCaps &currentCaps) override;
        void setPicture(const QString &picture) override;
        void setRootMethod(const QString &rootMethod) override;
        bool applyPicture() override;
        bool write(const AkVideoPacket &packet) override;

    friend class VCamCMIOPrivate;
};

#endif // VCAMCMIO_H

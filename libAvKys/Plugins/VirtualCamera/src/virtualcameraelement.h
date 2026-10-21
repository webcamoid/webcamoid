/* Webcamoid, camera capture application.
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

#ifndef VIRTUALCAMERAELEMENT_H
#define VIRTUALCAMERAELEMENT_H

#include <iak/akelement.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>

class VirtualCameraElementPrivate;

class VirtualCameraElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QString error
               READ error
               NOTIFY errorChanged)
    Q_PROPERTY(QString device
               READ device
               NOTIFY deviceChanged)
    Q_PROPERTY(QString description
               READ description
               WRITE setDescription
               RESET resetDescription
               NOTIFY descriptionChanged)
    Q_PROPERTY(AkVideoCaps outputCaps
               READ outputCaps
               WRITE setOutputCaps
               RESET resetOutputCaps
               NOTIFY outputCapsChanged)
    Q_PROPERTY(AkVideoConverter::AspectRatioMode aspectRatioMode
               READ aspectRatioMode
               WRITE setAspectRatioMode
               RESET resetAspectRatioMode
               NOTIFY aspectRatioModeChanged)
    Q_PROPERTY(QList<quint64> clientsPids
               READ clientsPids
               CONSTANT)
    Q_PROPERTY(bool driverInstalled
               READ driverInstalled
               CONSTANT)
    Q_PROPERTY(QString driverVersion
               READ driverVersion
               CONSTANT)
    Q_PROPERTY(QString picture
               READ picture
               WRITE setPicture
               RESET resetPicture
               NOTIFY pictureChanged)
    Q_PROPERTY(bool canEditVCamDescription
               READ canEditVCamDescription
               CONSTANT)

    public:
        VirtualCameraElement();
        ~VirtualCameraElement();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE QString device() const;
        Q_INVOKABLE QString description() const;
        Q_INVOKABLE AkVideoCaps outputCaps() const;
        Q_INVOKABLE AkVideoConverter::AspectRatioMode aspectRatioMode() const;
        Q_INVOKABLE void createWebcam(const QString &description);
        Q_INVOKABLE void changeDescription(const QString &description);
        Q_INVOKABLE void removeWebcam();
        Q_INVOKABLE QList<quint64> clientsPids() const;
        Q_INVOKABLE QString clientExe(quint64 pid) const;
        Q_INVOKABLE bool driverInstalled() const;
        Q_INVOKABLE QString driverVersion() const;
        Q_INVOKABLE QString picture() const;
        Q_INVOKABLE bool canEditVCamDescription() const;

    private:
        VirtualCameraElementPrivate *d;

    protected:
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void errorChanged(const QString &error);
        void deviceChanged(const QString &device);
        void descriptionChanged(const QString &description);
        void outputCapsChanged(const AkVideoCaps &outputCaps);
        void aspectRatioModeChanged(AkVideoConverter::AspectRatioMode aspectRatioMode);
        void pictureChanged(const QString &picture);
        void webcamCreated(bool ok, const QString &msg);
        void descriptionEdited(bool ok, const QString &msg);
        void webcamRemoved(bool ok, const QString &msg);

    public slots:
        bool applyPicture();
        void setDescription(const QString &description);
        void setOutputCaps(const AkVideoCaps &outputCaps);
        void setAspectRatioMode(AkVideoConverter::AspectRatioMode aspectRatioMode);
        void setPicture(const QString &picture);
        void resetDescription();
        void resetOutputCaps();
        void resetAspectRatioMode();
        void resetPicture();
        bool setState(AkElement::ElementState state) override;
};

#endif // VIRTUALCAMERAELEMENT_H

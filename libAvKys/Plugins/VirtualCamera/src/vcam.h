/* Webcamoid, camera capture application.
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
#include <akvideoconverter.h>
#include <akvideopacket.h>

class VCam;
class VCamPrivate;
using VCamPtr = QSharedPointer<VCam>;

/* A virtual camera plugin manages a single virtual camera.
 *
 * The frames given to write() can be in any format and resolution, the base
 * class converts them to the format defined in outputCaps, so the plugins
 * only need to negotiate the format with the driver (openDevice()) and copy
 * the already converted frames to it (writeFrame()).
 */
class VCam: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString error
               READ error
               NOTIFY errorChanged)
    Q_PROPERTY(bool isInstalled
               READ isInstalled
               CONSTANT)
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
    Q_PROPERTY(QString picture
               READ picture
               WRITE setPicture
               RESET resetPicture
               NOTIFY pictureChanged)
    Q_PROPERTY(bool canEditVCamDescription
               READ canEditVCamDescription
               CONSTANT)

    public:
        VCam(QObject *parent=nullptr);
        virtual ~VCam();

        Q_INVOKABLE virtual QString error() const;
        Q_INVOKABLE virtual bool isInstalled() const;
        Q_INVOKABLE virtual QString installedVersion() const;
        Q_INVOKABLE virtual QString device() const;
        Q_INVOKABLE QString description() const;
        Q_INVOKABLE AkVideoCaps outputCaps() const;
        Q_INVOKABLE AkVideoConverter::AspectRatioMode aspectRatioMode() const;
        Q_INVOKABLE virtual QList<quint64> clientsPids() const;
        Q_INVOKABLE virtual QString clientExe(quint64 pid) const;
        Q_INVOKABLE virtual QString picture() const;
        Q_INVOKABLE virtual bool canEditVCamDescription() const;
        Q_INVOKABLE virtual QString deviceCreate(const QString &description);
        Q_INVOKABLE virtual bool changeDescription(const QString &description);
        Q_INVOKABLE virtual bool deviceDestroy();

    signals:
        void errorChanged(const QString &error);
        void deviceChanged(const QString &device);
        void descriptionChanged(const QString &description);
        void outputCapsChanged(const AkVideoCaps &outputCaps);
        void aspectRatioModeChanged(AkVideoConverter::AspectRatioMode aspectRatioMode);
        void pictureChanged(const QString &picture);

    public slots:
        // Starts the output using the current outputCaps.
        bool init();
        void uninit();
        // Converts the packet to the active output format and sends it.
        bool write(const AkVideoPacket &packet);
        virtual bool applyPicture();

        /* Changing the outputCaps while the camera is running does not have
         * any effect until the next call to init().
         */
        void setDescription(const QString &description);
        void setOutputCaps(const AkVideoCaps &outputCaps);

        // It is applied immediately, even if the camera is running.
        void setAspectRatioMode(AkVideoConverter::AspectRatioMode aspectRatioMode);
        virtual void setPicture(const QString &picture);
        void resetDescription();
        void resetOutputCaps();
        void resetAspectRatioMode();
        void resetPicture();

    protected:
        /* Negotiates the format with the driver and starts the streaming.
         * 'caps' is the requested format, the plugin must update it with the
         * format that will be really used if the driver changes it.
         */
        virtual bool openDevice(AkVideoCaps &caps);
        virtual void closeDevice();

        // Receives frames already converted to the active format.
        virtual bool writeFrame(const AkVideoPacket &packet);

    private:
        VCamPrivate *d;
};

#endif // VCAM_H

/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#ifndef VIRTUALCAMERAS_H
#define VIRTUALCAMERAS_H

#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <iak/akelement.h>

#include "downloadmanager.h"

class VirtualCamerasPrivate;
class VirtualCameras;
class QQmlApplicationEngine;

using VirtualCamerasPtr = QSharedPointer<VirtualCameras>;

class VirtualCameras: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString outputError
               READ outputError
               NOTIFY outputErrorChanged)
    Q_PROPERTY(QString output
               READ output
               NOTIFY outputChanged)
    Q_PROPERTY(QString outputDescription
               READ outputDescription
               NOTIFY outputDescriptionChanged)
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
    Q_PROPERTY(AkVideoCaps::PixelFormatList supportedOutputPixelFormats
               READ supportedOutputPixelFormats
               CONSTANT)
    Q_PROPERTY(AkVideoCaps::PixelFormat defaultOutputPixelFormat
               READ defaultOutputPixelFormat
               CONSTANT)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)
    Q_PROPERTY(QList<quint64> clientsPids
               READ clientsPids
               CONSTANT)
    Q_PROPERTY(bool driverInstalled
               READ driverInstalled
               CONSTANT)
    Q_PROPERTY(QString picture
               READ picture
               WRITE setPicture
               RESET resetPicture
               NOTIFY pictureChanged)
    Q_PROPERTY(bool isVCamSupported
               READ isVCamSupported
               CONSTANT)
    Q_PROPERTY(VCamStatus vcamInstallStatus
               READ vcamInstallStatus
               CONSTANT)
    Q_PROPERTY(QString vcamDriver
               READ vcamDriver
               NOTIFY vcamDriverChanged)
    Q_PROPERTY(QString currentVCamVersion
               READ currentVCamVersion
               NOTIFY currentVCamVersionChanged)
    Q_PROPERTY(bool isCurrentVCamInstalled
               READ isCurrentVCamInstalled
               NOTIFY currentVCamInstalledChanged)
    Q_PROPERTY(bool canEditVCamDescription
               READ canEditVCamDescription
               CONSTANT)
    Q_PROPERTY(QString vcamUpdateUrl
               READ vcamUpdateUrl
               CONSTANT)
    Q_PROPERTY(QString vcamDownloadUrl
               READ vcamDownloadUrl
               CONSTANT)
    Q_PROPERTY(QString defaultVCamDriver
               READ defaultVCamDriver
               CONSTANT)

    public:
        enum VCamStatus
        {
            VCamNotInstalled,
            VCamInstalled,
            VCamInstalledOther
        };
        Q_ENUM(VCamStatus)

        VirtualCameras(QQmlApplicationEngine *engine=nullptr,
                   QObject *parent=nullptr);
        ~VirtualCameras();

        Q_INVOKABLE QString output() const;
        Q_INVOKABLE QString outputDescription() const;
        Q_INVOKABLE AkVideoCaps outputCaps() const;
        Q_INVOKABLE AkVideoConverter::AspectRatioMode aspectRatioMode() const;
        Q_INVOKABLE AkVideoCaps::PixelFormatList supportedOutputPixelFormats() const;
        Q_INVOKABLE AkVideoCaps::PixelFormat defaultOutputPixelFormat() const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE bool createOutput(const QString &description);
        Q_INVOKABLE bool editOutput(const QString &description);
        Q_INVOKABLE bool removeOutput();
        Q_INVOKABLE void setOutputFormat(quint32 format,
                                         int width,
                                         int height,
                                         int fps);
        Q_INVOKABLE QString outputError() const;
        Q_INVOKABLE QList<quint64> clientsPids() const;
        Q_INVOKABLE QString clientExe(quint64 pid) const;
        Q_INVOKABLE bool driverInstalled() const;
        Q_INVOKABLE QString picture() const;
        Q_INVOKABLE bool isVCamSupported() const;
        Q_INVOKABLE VCamStatus vcamInstallStatus() const;
        Q_INVOKABLE QString vcamDriver() const;
        Q_INVOKABLE QString currentVCamVersion() const;
        Q_INVOKABLE bool isCurrentVCamInstalled() const;
        Q_INVOKABLE bool canEditVCamDescription() const;
        Q_INVOKABLE QString vcamUpdateUrl() const;
        Q_INVOKABLE QString vcamDownloadUrl() const;
        Q_INVOKABLE QString defaultVCamDriver() const;

    private:
        VirtualCamerasPrivate *d;

    signals:
        void outputChanged(const QString &output);
        void outputDescriptionChanged(const QString &outputDescription);
        void outputCapsChanged(const AkVideoCaps &outputCaps);
        void aspectRatioModeChanged(AkVideoConverter::AspectRatioMode aspectRatioMode);
        void stateChanged(AkElement::ElementState state);
        void oStream(const AkPacket &packet);
        void outputErrorChanged(const QString &outputError);
        void pictureChanged(const QString &picture);
        void vcamDriverChanged(const QString &vcamDriver);
        void currentVCamVersionChanged(const QString &currentVCamVersion);
        void currentVCamInstalledChanged(bool installed);
        void startVCamDownload(const QString &title,
                               const QString &fromUrl,
                               const QString &toFile);
        void vcamDownloadReady(const QString &filePath);
        void vcamDownloadFailed(const QString &error);
        void vcamInstallFinished(int exitCode, const QString &error);
        void vcamCliInstallStarted();
        void vcamCliInstallLineReady(const QString &line);
        void vcamCliInstallFinished();
        void outputCreated(bool ok, const QString &msg);
        void outputEdited(bool ok, const QString &msg);
        void outputRemoved(bool ok, const QString &msg);
        void creatingOutput(const QString &outputDescription);
        void editingOutput(const QString &outputDescription);
        void removingOutput(const QString &outputDescription);

    public slots:
        AkPacket iStream(const AkPacket &packet);
        bool applyPicture();
        void setLatestVCamVersion(const QString &version);
        bool downloadVCam();
        bool executeVCamInstaller(const QString &installer);
        void checkVCamDownloadReady(const QString &url,
                                    const QString &filePath,
                                    DownloadManager::DownloadStatus status,
                                    const QString &error);
        void setOutputCaps(const AkVideoCaps &outputCaps);
        void setAspectRatioMode(AkVideoConverter::AspectRatioMode aspectRatioMode);
        void setState(AkElement::ElementState state);
        void setPicture(const QString &picture);
        void resetOutputCaps();
        void resetAspectRatioMode();
        void resetState();
        void resetPicture();
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);

    private slots:
        void onOutputChanged(const QString &output);
        void onOutputDescriptionChanged(const QString &description);
        void onOutputCapsChanged(const AkVideoCaps &outputCaps);
        void onAspectRatioModeChanged(AkVideoConverter::AspectRatioMode aspectRatioMode);
        void onOutputCreated(bool ok, const QString &msg);
        void onOutputEdited(bool ok, const QString &msg);
        void onOutputRemoved(bool ok, const QString &msg);

        friend class VirtualCamerasPrivate;
};

Q_DECLARE_METATYPE(VirtualCameras::VCamStatus)

#endif // VIRTUALCAMERAS_H

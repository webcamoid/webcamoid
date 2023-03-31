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

#ifndef VIRTUALCAMERAELEMENT_H
#define VIRTUALCAMERAELEMENT_H

#include <akcaps.h>
#include <QVariantMap>
#include <akelement.h>
#include <akvideocaps.h>

class VirtualCameraElementPrivate;

class VirtualCameraElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QString error
               READ error
               NOTIFY errorChanged)
    Q_PROPERTY(QStringList medias
               READ medias
               NOTIFY mediasChanged)
    Q_PROPERTY(QString media
               READ media
               WRITE setMedia
               RESET resetMedia
               NOTIFY mediaChanged)
    Q_PROPERTY(QList<int> streams
               READ streams
               NOTIFY streamsChanged)
    Q_PROPERTY(int maxCameras
               READ maxCameras
               NOTIFY maxCamerasChanged)
    Q_PROPERTY(AkVideoCaps::PixelFormatList supportedOutputPixelFormats
               READ supportedOutputPixelFormats
               NOTIFY supportedOutputPixelFormatsChanged)
    Q_PROPERTY(AkVideoCaps::PixelFormat defaultOutputPixelFormat
               READ defaultOutputPixelFormat
               NOTIFY defaultOutputPixelFormatChanged)
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
    Q_PROPERTY(QString rootMethod
               READ rootMethod
               WRITE setRootMethod
               RESET resetRootMethod
               NOTIFY rootMethodChanged)
    Q_PROPERTY(QStringList availableRootMethods
               READ availableRootMethods
               CONSTANT)

    public:
        VirtualCameraElement();
        ~VirtualCameraElement();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE QStringList medias() const;
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;
        Q_INVOKABLE int maxCameras() const;
        Q_INVOKABLE AkVideoCaps::PixelFormatList supportedOutputPixelFormats() const;
        Q_INVOKABLE AkVideoCaps::PixelFormat defaultOutputPixelFormat() const;
        Q_INVOKABLE int defaultStream(AkCaps::CapsType type) const;
        Q_INVOKABLE QString description(const QString &media) const;
        Q_INVOKABLE AkCaps caps(int stream) const;
        Q_INVOKABLE AkVideoCapsList outputCaps(const QString &webcam) const;
        Q_INVOKABLE QVariantMap addStream(int streamIndex,
                                          const AkCaps &streamCaps,
                                          const QVariantMap &streamParams={});
        Q_INVOKABLE QVariantMap updateStream(int streamIndex,
                                             const QVariantMap &streamParams={});
        Q_INVOKABLE QString createWebcam(const QString &description,
                                         const AkVideoCapsList &formats);
        Q_INVOKABLE bool editWebcam(const QString &webcam,
                                    const QString &description,
                                    const AkVideoCapsList &formats);
        Q_INVOKABLE bool changeDescription(const QString &webcam,
                                           const QString &description={});
        Q_INVOKABLE bool removeWebcam(const QString &webcam);
        Q_INVOKABLE bool removeAllWebcams();
        Q_INVOKABLE QVariantList controls() const;
        Q_INVOKABLE bool setControls(const QVariantMap &controls);
        Q_INVOKABLE bool resetControls();
        Q_INVOKABLE QList<quint64> clientsPids() const;
        Q_INVOKABLE QString clientExe(quint64 pid) const;
        Q_INVOKABLE bool driverInstalled() const;
        Q_INVOKABLE QString driverVersion() const;
        Q_INVOKABLE QString picture() const;
        Q_INVOKABLE QString rootMethod() const;
        Q_INVOKABLE QStringList availableRootMethods() const;

    private:
        VirtualCameraElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void errorChanged(const QString &error);
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void maxCamerasChanged(int maxCameras);
        void supportedOutputPixelFormatsChanged(const AkVideoCaps::PixelFormatList &supportedOutputPixelFormats);
        void defaultOutputPixelFormatChanged(const AkVideoCaps::PixelFormat &defaultOutputPixelFormat);
        void pictureChanged(const QString &picture);
        void rootMethodChanged(const QString &rootMethod);

    public slots:
        bool applyPicture();
        void setMedia(const QString &media);
        void setPicture(const QString &picture);
        void setRootMethod(const QString &rootMethod);
        void resetMedia();
        void resetPicture();
        void resetRootMethod();
        void clearStreams();
        bool setState(AkElement::ElementState state) override;
};

#endif // VIRTUALCAMERAELEMENT_H

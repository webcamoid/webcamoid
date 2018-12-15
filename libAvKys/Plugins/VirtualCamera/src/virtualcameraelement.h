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

#include <QVariantMap>
#include <akelement.h>

class VirtualCameraElementPrivate;
class AkCaps;

class VirtualCameraElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QStringList driverPaths
               READ driverPaths
               WRITE setDriverPaths
               RESET resetDriverPaths
               NOTIFY driverPathsChanged)
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
    Q_PROPERTY(QString driver
               READ driver
               WRITE setDriver
               RESET resetDriver
               NOTIFY driverChanged)
    Q_PROPERTY(QStringList availableDrivers
               READ availableDrivers
               NOTIFY availableDriversChanged)
    Q_PROPERTY(QString rootMethod
               READ rootMethod
               WRITE setRootMethod
               RESET resetRootMethod
               NOTIFY rootMethodChanged)
    Q_PROPERTY(QStringList availableMethods
               READ availableMethods
               NOTIFY availableMethodsChanged)

    public:
        explicit VirtualCameraElement();
        ~VirtualCameraElement();

        Q_INVOKABLE QStringList driverPaths() const;
        Q_INVOKABLE QStringList medias() const;
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;
        Q_INVOKABLE int maxCameras() const;
        Q_INVOKABLE QString driver() const;
        Q_INVOKABLE QStringList availableDrivers() const;
        Q_INVOKABLE QString rootMethod() const;
        Q_INVOKABLE QStringList availableMethods() const;

        Q_INVOKABLE int defaultStream(const QString &mimeType) const;
        Q_INVOKABLE QString description(const QString &media) const;
        Q_INVOKABLE AkCaps caps(int stream) const;
        Q_INVOKABLE QVariantMap addStream(int streamIndex,
                                          const AkCaps &streamCaps,
                                          const QVariantMap &streamParams={});
        Q_INVOKABLE QVariantMap updateStream(int streamIndex,
                                             const QVariantMap &streamParams={});
        Q_INVOKABLE QString createWebcam(const QString &description={});
        Q_INVOKABLE bool changeDescription(const QString &webcam,
                                           const QString &description={}) const;
        Q_INVOKABLE bool removeWebcam(const QString &webcam);
        Q_INVOKABLE bool removeAllWebcams();

    private:
        VirtualCameraElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;

    signals:
        void driverPathsChanged(const QStringList &driverPaths);
        void mediasChanged(const QStringList &medias) const;
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void maxCamerasChanged(int maxCameras);
        void driverChanged(const QString &driver);
        void availableDriversChanged(const QStringList &availableDrivers);
        void rootMethodChanged(const QString &rootMethod);
        void availableMethodsChanged(const QStringList &availableMethods);
        void error(const QString &message);

    public slots:
        void setDriverPaths(const QStringList &driverPaths);
        void addDriverPath(const QString &driverPath);
        void addDriverPaths(const QStringList &driverPaths);
        void removeDriverPath(const QString &driverPath);
        void removeDriverPaths(const QStringList &driverPaths);
        void setMedia(const QString &media);
        void setDriver(const QString &driver);
        void setRootMethod(const QString &rootMethod);
        void resetDriverPaths();
        void resetMedia();
        void resetDriver();
        void resetRootMethod();
        void clearStreams();

        bool setState(AkElement::ElementState state);
        AkPacket iStream(const AkPacket &packet);

    private slots:
        void rootMethodUpdated(const QString &rootMethod);
};

#endif // VIRTUALCAMERAELEMENT_H

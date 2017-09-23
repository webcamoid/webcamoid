/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include <QMutex>

#include <akmultimediasourceelement.h>

#include "convertvideo.h"
#include "cameraout.h"

typedef QSharedPointer<ConvertVideo> ConvertVideoPtr;
typedef QSharedPointer<CameraOut> CameraOutPtr;

class VirtualCameraElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QString driverPath
               READ driverPath
               WRITE setDriverPath
               RESET resetDriverPath
               NOTIFY driverPathChanged)
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
    Q_PROPERTY(QStringList availableMethods
               READ availableMethods
               NOTIFY availableMethodsChanged)
    Q_PROPERTY(QString convertLib
               READ convertLib
               WRITE setConvertLib
               RESET resetConvertLib
               NOTIFY convertLibChanged)
    Q_PROPERTY(QString outputLib
               READ outputLib
               WRITE setOutputLib
               RESET resetOutputLib
               NOTIFY outputLibChanged)

    public:
        explicit VirtualCameraElement();
        ~VirtualCameraElement();

        Q_INVOKABLE QString driverPath() const;
        Q_INVOKABLE QStringList medias() const;
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;
        Q_INVOKABLE int maxCameras() const;
        Q_INVOKABLE bool needRoot() const;
        Q_INVOKABLE int passwordTimeout() const;
        Q_INVOKABLE QString rootMethod() const;
        Q_INVOKABLE QStringList availableMethods() const;
        Q_INVOKABLE QString convertLib() const;
        Q_INVOKABLE QString outputLib() const;

        Q_INVOKABLE int defaultStream(const QString &mimeType) const;
        Q_INVOKABLE QString description(const QString &media) const;
        Q_INVOKABLE AkCaps caps(int stream) const;
        Q_INVOKABLE QVariantMap addStream(int streamIndex,
                                          const AkCaps &streamCaps,
                                          const QVariantMap &streamParams=QVariantMap());
        Q_INVOKABLE QVariantMap updateStream(int streamIndex,
                                             const QVariantMap &streamParams=QVariantMap());
        Q_INVOKABLE QString createWebcam(const QString &description="",
                                         const QString &password="");
        Q_INVOKABLE bool changeDescription(const QString &webcam,
                                           const QString &description="",
                                           const QString &password="") const;
        Q_INVOKABLE bool removeWebcam(const QString &webcam,
                                      const QString &password="");
        Q_INVOKABLE bool removeAllWebcams(const QString &password="");

    private:
        ConvertVideoPtr m_convertVideo;
        CameraOutPtr m_cameraOut;
        int m_streamIndex;
        AkCaps m_streamCaps;
        QMutex m_mutex;
        QMutex m_mutexLib;

        QImage swapChannels(const QImage &image) const;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;

    signals:
        void driverPathChanged(const QString &driverPath);
        void mediasChanged(const QStringList &medias) const;
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void maxCamerasChanged(int maxCameras);
        void needRootChanged(bool needRoot);
        void passwordTimeoutChanged(int passwordTimeout);
        void rootMethodChanged(const QString &rootMethod);
        void availableMethodsChanged(const QStringList &availableMethods);
        void convertLibChanged(const QString &convertLib);
        void outputLibChanged(const QString &outputLib);
        void error(const QString &message);

    public slots:
        void setDriverPath(const QString &driverPath);
        void setMedia(const QString &media);
        void setPasswordTimeout(int passwordTimeout);
        void setRootMethod(const QString &rootMethod);
        void setConvertLib(const QString &convertLib);
        void setOutputLib(const QString &outputLib);
        void resetDriverPath();
        void resetMedia();
        void resetPasswordTimeout();
        void resetRootMethod();
        void resetConvertLib();
        void resetOutputLib();
        void clearStreams();

        bool setState(AkElement::ElementState state);
        AkPacket iStream(const AkPacket &packet);

    private slots:
        void convertLibUpdated(const QString &convertLib);
        void outputLibUpdated(const QString &outputLib);
        void rootMethodUpdated(const QString &rootMethod);
};

#endif // VIRTUALCAMERAELEMENT_H

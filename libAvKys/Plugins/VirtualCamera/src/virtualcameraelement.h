/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef VIRTUALCAMERAELEMENT_H
#define VIRTUALCAMERAELEMENT_H

#include <QQmlComponent>
#include <QQmlContext>
#include <QMutex>

#include <akmultimediasourceelement.h>

#ifdef Q_OS_LINUX
#include "v4l2/cameraout.h"
#elif defined(Q_OS_WIN32)
#include "dshow/cameraout.h"
#endif

#ifdef USE_GSTREAMER
#include "gstreamer/convertvideo.h"
#else
#include "ffmpeg/convertvideo.h"
#endif

class VirtualCameraElement: public AkElement
{
    Q_OBJECT
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

    public:
        explicit VirtualCameraElement();
        ~VirtualCameraElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QStringList medias() const;
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;

        Q_INVOKABLE int defaultStream(const QString &mimeType) const;
        Q_INVOKABLE QString description(const QString &media) const;
        Q_INVOKABLE AkCaps caps(int stream) const;
        Q_INVOKABLE QVariantMap addStream(int streamIndex,
                                          const AkCaps &streamCaps,
                                          const QVariantMap &streamParams=QVariantMap());
        Q_INVOKABLE QVariantMap updateStream(int streamIndex,
                                             const QVariantMap &streamParams=QVariantMap());

    protected:
        void stateChange(AkElement::ElementState from,
                         AkElement::ElementState to);

    private:
        CameraOut m_cameraOut;
        ConvertVideo m_convertVideo;
        int m_streamIndex;
        AkCaps m_streamCaps;
        QMutex m_mutex;
        bool m_isRunning;

    signals:
        void mediasChanged(const QStringList &medias) const;
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void error(const QString &message);

    public slots:
        void setMedia(const QString &media);
        void resetMedia();
        void clearStreams();

        AkPacket iStream(const AkPacket &packet);
};

#endif // VIRTUALCAMERAELEMENT_H

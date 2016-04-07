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
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef VIDEOCAPTUREELEMENT_H
#define VIDEOCAPTUREELEMENT_H

#include <QTimer>
#include <QThreadPool>
#include <QtConcurrent>
#include <QQmlComponent>
#include <QQmlContext>

#include <akmultimediasourceelement.h>

#ifdef Q_OS_LINUX
#include "v4l2/capture.h"
#elif defined(Q_OS_WIN32)
#include "dshow/capture.h"
#endif

#ifdef USE_GSTREAMER
#include "gstreamer/convertvideo.h"
#else
#include "ffmpeg/convertvideo.h"
#endif

class VideoCaptureElement: public AkMultimediaSourceElement
{
    Q_OBJECT
    Q_PROPERTY(QList<int> streams
               READ streams
               WRITE setStreams
               RESET resetStreams
               NOTIFY streamsChanged)
    Q_PROPERTY(QString ioMethod
               READ ioMethod
               WRITE setIoMethod
               RESET resetIoMethod)
    Q_PROPERTY(int nBuffers
               READ nBuffers
               WRITE setNBuffers
               RESET resetNBuffers)

    public:
        explicit VideoCaptureElement();
        ~VideoCaptureElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QStringList medias() const;
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;
        Q_INVOKABLE QList<int> listTracks(const QString &mimeType="");

        Q_INVOKABLE int defaultStream(const QString &mimeType) const;
        Q_INVOKABLE QString description(const QString &media) const;
        Q_INVOKABLE AkCaps caps(int stream) const;
        Q_INVOKABLE QStringList listCapsDescription() const;

        Q_INVOKABLE QString ioMethod() const;
        Q_INVOKABLE int nBuffers() const;

        Q_INVOKABLE QVariantList imageControls() const;
        Q_INVOKABLE bool setImageControls(const QVariantMap &imageControls) const;
        Q_INVOKABLE bool resetImageControls() const;
        Q_INVOKABLE QVariantList cameraControls() const;
        Q_INVOKABLE bool setCameraControls(const QVariantMap &cameraControls) const;
        Q_INVOKABLE bool resetCameraControls() const;

    private:
        Capture m_capture;
        QTimer m_timer;
        ConvertVideo m_convertVideo;

    signals:
        void error(const QString &message);
        void sizeChanged(const QString &webcam, const QSize &size) const;
        void imageControlsChanged(const QVariantMap &imageControls) const;
        void cameraControlsChanged(const QVariantMap &cameraControls) const;
        void streamsChanged(const QList<int> &streams);

    public slots:
        void setMedia(const QString &media);
        void setStreams(const QList<int> &streams);
        void setIoMethod(const QString &ioMethod);
        void setNBuffers(int nBuffers);
        void resetMedia();
        void resetStreams();
        void resetIoMethod();
        void resetNBuffers();
        void reset();
        bool setState(AkElement::ElementState state);
        void frameReady(const AkPacket &packet);

    private slots:
        void readFrame();
};

#endif // VIDEOCAPTUREELEMENT_H

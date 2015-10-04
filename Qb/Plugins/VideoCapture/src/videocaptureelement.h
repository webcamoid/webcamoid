/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#ifndef VIDEOCAPTUREELEMENT_H
#define VIDEOCAPTUREELEMENT_H

#include <QTimer>
#include <QThreadPool>
#include <QtConcurrent>
#include <QQmlComponent>
#include <QQmlContext>

#include <qbmultimediasourceelement.h>

#ifdef Q_OS_LINUX
#include "v4l2/capture.h"
#endif

#ifdef Q_OS_WIN32
#include "dshow/capture.h"
#endif

#include "ffmpeg/convertvideo.h"

class VideoCaptureElement: public QbMultimediaSourceElement
{
    Q_OBJECT
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

        Q_INVOKABLE int defaultStream(const QString &mimeType) const;
        Q_INVOKABLE QString description(const QString &media) const;
        Q_INVOKABLE QbCaps caps(int stream) const;
        Q_INVOKABLE bool isCompressed(int stream) const;

        Q_INVOKABLE QString ioMethod() const;
        Q_INVOKABLE int nBuffers() const;
        Q_INVOKABLE QVariantList availableSizes(const QString &webcam) const;
        Q_INVOKABLE QSize size(const QString &webcam) const;
        Q_INVOKABLE bool setSize(const QString &webcam, const QSize &size);
        Q_INVOKABLE bool resetSize(const QString &webcam);
        Q_INVOKABLE QVariantList imageControls(const QString &webcam) const;
        Q_INVOKABLE bool setImageControls(const QString &webcam, const QVariantMap &imageControls) const;
        Q_INVOKABLE bool resetImageControls(const QString &webcam) const;
        Q_INVOKABLE QVariantList cameraControls(const QString &webcam) const;
        Q_INVOKABLE bool setCameraControls(const QString &webcam, const QVariantMap &cameraControls) const;
        Q_INVOKABLE bool resetCameraControls(const QString &webcam) const;

    private:
        Capture m_capture;
        bool m_threadedRead;
        QTimer m_timer;
        QThreadPool m_threadPool;
        QFuture<void> m_threadStatus;
        QbPacket m_curPacket;
        ConvertVideo m_convertVideo;

        static void sendPacket(VideoCaptureElement *element, const QbPacket &packet);

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    signals:
        void error(const QString &message);
        void sizeChanged(const QString &webcam, const QSize &size) const;
        void imageControlsChanged(const QString &webcam, const QVariantMap &imageControls) const;
        void cameraControlsChanged(const QString &webcam, const QVariantMap &cameraControls) const;

    public slots:
        void setMedia(const QString &media);
        void setIoMethod(const QString &ioMethod);
        void setNBuffers(int nBuffers);
        void resetMedia();
        void resetIoMethod();
        void resetNBuffers();
        void reset(const QString &webcam="");

    private slots:
        bool init();
        void uninit();
        void readFrame();
};

#endif // VIDEOCAPTUREELEMENT_H

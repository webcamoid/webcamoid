/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef VIDEOCAPTUREELEMENT_H
#define VIDEOCAPTUREELEMENT_H

#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QQmlComponent>
#include <QQmlContext>

#include <qbmultimediasourceelement.h>

#ifdef Q_OS_LINUX
#include "platform/capturelinux.h"
#endif

#ifdef Q_OS_WIN32
#include "platform/capturewin.h"
#endif

#include "outputthread.h"

typedef QSharedPointer<QThread> ThreadPtr;

class VideoCaptureElement: public QbMultimediaSourceElement
{
    Q_OBJECT
    Q_PROPERTY(QStringList webcams
               READ webcams
               NOTIFY webcamsChanged)
    Q_PROPERTY(QString device
               READ device
               WRITE setDevice
               RESET resetDevice)
    Q_PROPERTY(QString ioMethod
               READ ioMethod
               WRITE setIoMethod
               RESET resetIoMethod)
    Q_PROPERTY(int nBuffers
               READ nBuffers
               WRITE setNBuffers
               RESET resetNBuffers)
    Q_PROPERTY(bool isCompressed
               READ isCompressed)
    Q_PROPERTY(QString caps
               READ caps)

    public:
        explicit VideoCaptureElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QStringList webcams() const;
        Q_INVOKABLE QString device() const;
        Q_INVOKABLE QString ioMethod() const;
        Q_INVOKABLE int nBuffers() const;
        Q_INVOKABLE bool isCompressed() const;
        Q_INVOKABLE QString caps() const;
        Q_INVOKABLE QString description(const QString &webcam) const;
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
        bool m_threadedRead;
        ThreadPtr m_thread;
        QTimer m_timer;
        QMutex m_mutex;
        Capture m_capture;

        static void deleteThread(QThread *thread);

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    signals:
        void error(const QString &message);
        void webcamsChanged(const QStringList &webcams) const;
        void sizeChanged(const QString &webcam, const QSize &size) const;
        void imageControlsChanged(const QString &webcam, const QVariantMap &imageControls) const;
        void cameraControlsChanged(const QString &webcam, const QVariantMap &cameraControls) const;

    public slots:
        void setDevice(const QString &device);
        void setIoMethod(const QString &ioMethod);
        void setNBuffers(int nBuffers);
        void resetDevice();
        void resetIoMethod();
        void resetNBuffers();
        void reset(const QString &webcam="");

    private slots:
        void readFrame();
};

#endif // VIDEOCAPTUREELEMENT_H

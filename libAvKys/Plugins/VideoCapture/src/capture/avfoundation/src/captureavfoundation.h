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

#ifndef CAPTUREAVFOUNDATION_H
#define CAPTUREAVFOUNDATION_H

#include "capture.h"

class CaptureAvFoundationPrivate;
class QWaitCondition;
class QMutex;

class CaptureAvFoundation: public Capture
{
    Q_OBJECT

    public:
        CaptureAvFoundation(QObject *parent=nullptr);
        ~CaptureAvFoundation();

        Q_INVOKABLE QStringList webcams() const;
        Q_INVOKABLE QString device() const;
        Q_INVOKABLE QList<int> streams();
        Q_INVOKABLE QList<int> listTracks(const QString &mimeType);
        Q_INVOKABLE QString ioMethod() const;
        Q_INVOKABLE int nBuffers() const;
        Q_INVOKABLE QString description(const QString &webcam) const;
        Q_INVOKABLE QVariantList caps(const QString &webcam) const;
        Q_INVOKABLE QString capsDescription(const AkCaps &caps) const;
        Q_INVOKABLE QVariantList imageControls() const;
        Q_INVOKABLE bool setImageControls(const QVariantMap &imageControls);
        Q_INVOKABLE bool resetImageControls();
        Q_INVOKABLE AkPacket readFrame();
        Q_INVOKABLE quint32 modelId(const QString &webcam) const;

        QMutex &mutex();
        QWaitCondition &frameReady();
        void *curFrame();

    private:
        CaptureAvFoundationPrivate *d;

    public slots:
        bool init();
        void uninit();
        void setDevice(const QString &device);
        void setStreams(const QList<int> &streams);
        void setIoMethod(const QString &ioMethod);
        void setNBuffers(int nBuffers);
        void resetDevice();
        void resetStreams();
        void resetIoMethod();
        void resetNBuffers();
        void reset();

        void cameraConnected();
        void cameraDisconnected();

    private slots:
        void updateDevices();
};

#endif // CAPTUREAVFOUNDATION_H

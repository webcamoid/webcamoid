/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#ifndef CAMERAOUTV4L2_H
#define CAMERAOUTV4L2_H

#include "cameraout.h"

class CameraOutV4L2Private;

class CameraOutV4L2: public CameraOut
{
    Q_OBJECT

    public:
        explicit CameraOutV4L2(QObject *parent=nullptr);
        ~CameraOutV4L2();

        Q_INVOKABLE QStringList webcams() const;
        Q_INVOKABLE int streamIndex() const;
        Q_INVOKABLE QString description(const QString &webcam) const;
        Q_INVOKABLE void writeFrame(const AkPacket &frame);
        Q_INVOKABLE int maxCameras() const;
        Q_INVOKABLE bool needRoot() const;
        Q_INVOKABLE QString createWebcam(const QString &description,
                                         const QString &password);
        Q_INVOKABLE bool changeDescription(const QString &webcam,
                                           const QString &description,
                                           const QString &password);
        Q_INVOKABLE bool removeWebcam(const QString &webcam,
                                      const QString &password);
        Q_INVOKABLE bool removeAllWebcams(const QString &password);

    private:
        CameraOutV4L2Private *d;

    public slots:
        bool init(int streamIndex);
        void uninit();
        void resetRootMethod();

    private slots:
        void onDirectoryChanged(const QString &path);
};

#endif // CAMERAOUTV4L2_H

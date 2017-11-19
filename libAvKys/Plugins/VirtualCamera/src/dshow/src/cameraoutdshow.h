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

#ifndef CAMERAOUTDSHOW_H
#define CAMERAOUTDSHOW_H

#include <strmif.h>
#include <initguid.h>
#include <uuids.h>
#include <vfwmsgs.h>
#include <ipcbridge.h>

#include "cameraout.h"

class CameraOutDShow: public CameraOut
{
    Q_OBJECT

    public:
        explicit CameraOutDShow(QObject *parent=nullptr);
        ~CameraOutDShow();

        Q_INVOKABLE QStringList webcams() const;
        Q_INVOKABLE int streamIndex() const;
        Q_INVOKABLE QString description(const QString &webcam) const;
        Q_INVOKABLE void writeFrame(const AkPacket &frame);
        Q_INVOKABLE int maxCameras() const;
        Q_INVOKABLE QString createWebcam(const QString &description,
                                         const QString &password);
        Q_INVOKABLE bool changeDescription(const QString &webcam,
                                           const QString &description,
                                           const QString &password);
        Q_INVOKABLE bool removeWebcam(const QString &webcam,
                                      const QString &password);
        Q_INVOKABLE bool removeAllWebcams(const QString &password);

    private:
        QStringList m_webcams;
        int m_streamIndex;
        IpcBridge m_ipcBridge;

        HRESULT enumerateCameras(IEnumMoniker **ppEnum) const;
        QString iidToString(const IID &iid) const;
        bool sudo(const QString &command,
                  const QString &params,
                  const QString &dir=QString(),
                  bool hide=false) const;

    public slots:
        bool init(int streamIndex);
        void uninit();
        void resetDriverPath();
};

#endif // CAMERAOUTDSHOW_H

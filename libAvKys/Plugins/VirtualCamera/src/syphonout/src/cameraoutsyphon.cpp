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

#include <akvideopacket.h>

#include "cameraoutsyphon.h"

#define MAX_CAMERAS 1

CameraOutSyphon::CameraOutSyphon(QObject *parent):
    CameraOut(parent)
{
    this->m_streamIndex = -1;
}

CameraOutSyphon::~CameraOutSyphon()
{
}

QString CameraOutSyphon::driverPath() const
{
    return "";
}

QStringList CameraOutSyphon::webcams() const
{
    QStringList webcams;

    for (auto &webcam: this->m_webcams)
        webcams << webcam->property("media").toString();

    return webcams;
}

QString CameraOutSyphon::device() const
{
    return this->m_device;
}

int CameraOutSyphon::streamIndex() const
{
    return this->m_streamIndex;
}

AkCaps CameraOutSyphon::caps() const
{
    return this->m_caps;
}

QString CameraOutSyphon::description(const QString &webcam) const
{
    if (webcam.isEmpty())
        return QString();

    for (auto &camera: this->m_webcams) {
        auto webcamId = camera->property("media").toString();

        if (webcamId == webcam)
            return camera->property("description").toString();
    }

    return QString();
}

void CameraOutSyphon::writeFrame(const AkPacket &frame)
{
    if (!this->m_webcam) {
        qDebug() << "Error writing frame";

        return;
    }

    this->m_webcam->iStream(frame);
}

int CameraOutSyphon::maxCameras() const
{
    return MAX_CAMERAS;
}

bool CameraOutSyphon::needRoot() const
{
    return false;
}

int CameraOutSyphon::passwordTimeout() const
{
    return 0;
}

QString CameraOutSyphon::rootMethod() const
{
    return QString();
}

QString CameraOutSyphon::createWebcam(const QString &description,
                                      const QString &password)
{
    Q_UNUSED(password)

    if (this->m_webcams.size() >= MAX_CAMERAS)
        return QString();

    auto webcam = AkElement::create("SyphonIO");

    if (!webcam)
        return QString();

    webcam->setProperty("isOutput", true);
    webcam->setProperty("description",
                        description.isEmpty()?
                            QString("Syphon Virtual Webcam"): description);
    this->m_webcams << webcam;

    auto webcams = this->webcams();
    emit this->webcamsChanged(webcams);

    return webcam->property("media").toString();
}

bool CameraOutSyphon::changeDescription(const QString &webcam,
                                        const QString &description,
                                        const QString &password) const
{
    Q_UNUSED(password)

    if (webcam.isEmpty() || description.isEmpty())
        return false;

    for (auto &camera: this->m_webcams) {
        auto webcamId = camera->property("media").toString();

        if (webcamId == webcam) {
            auto curDescription = camera->property("description").toString();

            if (curDescription != description) {
                camera->setProperty("description", description);
                QStringList webcams = this->webcams();
                emit this->webcamsChanged(webcams);

                return true;
            } else {
                break;
            }
        }
    }

    return false;
}

bool CameraOutSyphon::removeWebcam(const QString &webcam,
                                   const QString &password)
{
    Q_UNUSED(password)

    if (webcam.isEmpty())
        return false;

    for (int i = 0; i < this->m_webcams.size(); i++) {
        auto media = this->m_webcams[i]->property("media").toString();

        if (media == webcam) {
            this->m_webcams.removeAt(i);
            QStringList webcams = this->webcams();
            emit this->webcamsChanged(webcams);

            return true;
        }
    }

    return false;
}

bool CameraOutSyphon::removeAllWebcams(const QString &password)
{
    Q_UNUSED(password)

    if (this->m_webcams.isEmpty())
        return false;

    this->m_webcams.clear();
    this->webcamsChanged({});

    return true;
}

bool CameraOutSyphon::init(int streamIndex, const AkCaps &caps)
{
    if (!caps)
        return false;


    for (auto &camera: this->m_webcams) {
        auto webcamId = camera->property("media").toString();

        if (webcamId == this->m_device) {
            this->m_webcam = camera;
            camera->setState(AkElement::ElementStatePlaying);
            this->m_streamIndex = streamIndex;
            this->m_caps = caps;

            return true;
        }
    }

    return false;
}

void CameraOutSyphon::uninit()
{
    if (!this->m_webcam)
        return;

    this->m_webcam->setState(AkElement::ElementStateNull);
    this->m_webcam.clear();
}

void CameraOutSyphon::setDriverPath(const QString &driverPath)
{
    Q_UNUSED(driverPath)
}

void CameraOutSyphon::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(device);
}

void CameraOutSyphon::setPasswordTimeout(int passwordTimeout)
{
    Q_UNUSED(passwordTimeout)
}

void CameraOutSyphon::setRootMethod(const QString &rootMethod)
{
    Q_UNUSED(rootMethod)
}

void CameraOutSyphon::resetDriverPath()
{
}

void CameraOutSyphon::resetDevice()
{
    this->setDevice("");
}

void CameraOutSyphon::resetPasswordTimeout()
{
}

void CameraOutSyphon::resetRootMethod()
{
}

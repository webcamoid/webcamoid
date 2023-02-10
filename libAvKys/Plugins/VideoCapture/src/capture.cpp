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

#include <QVariant>
#include <QtConcurrent>
#include <akcaps.h>
#include <akpacket.h>

#include "capture.h"

class CapturePrivate
{
    public:
        QThreadPool m_threadPool;
};

Capture::Capture(QObject *parent):
    QObject(parent)
{
    this->d = new CapturePrivate;
    this->d->m_threadPool.setMaxThreadCount(16);
}

Capture::~Capture()
{
    delete this->d;
}

QString Capture::error() const
{
    return {};
}

QStringList Capture::webcams() const
{
    return QStringList();
}

QString Capture::device() const
{
    return QString();
}

QList<int> Capture::streams()
{
    return QList<int>();
}

QList<int> Capture::listTracks(AkCaps::CapsType type)
{
    Q_UNUSED(type)

    return {};
}

QString Capture::ioMethod() const
{
    return QString();
}

int Capture::nBuffers() const
{
    return 0;
}

QString Capture::description(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return QString();
}

CaptureVideoCaps Capture::caps(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return {};
}

QVariantList Capture::imageControls() const
{
    return QVariantList();
}

bool Capture::setImageControls(const QVariantMap &imageControls)
{
    Q_UNUSED(imageControls)

    return false;
}

bool Capture::resetImageControls()
{
    return false;
}

QVariantList Capture::cameraControls() const
{
    return QVariantList();
}

bool Capture::setCameraControls(const QVariantMap &cameraControls)
{
    Q_UNUSED(cameraControls)

    return false;
}

bool Capture::resetCameraControls()
{
    return false;
}

Capture::FlashModeList Capture::supportedFlashModes(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return {};
}

Capture::FlashMode Capture::flashMode() const
{
    return FlashMode_Off;
}

AkPacket Capture::readFrame()
{
    return AkPacket();
}

bool Capture::init()
{
    return false;
}

void Capture::uninit()
{
}

void Capture::setDevice(const QString &device)
{
    Q_UNUSED(device)
}

void Capture::setStreams(const QList<int> &streams)
{
    Q_UNUSED(streams)
}

void Capture::setIoMethod(const QString &ioMethod)
{
    Q_UNUSED(ioMethod)
}

void Capture::setNBuffers(int nBuffers)
{
    Q_UNUSED(nBuffers)
}

void Capture::setFlashMode(FlashMode mode)
{
    Q_UNUSED(mode)
}

void Capture::resetDevice()
{
}

void Capture::resetStreams()
{
}

void Capture::resetIoMethod()
{
}

void Capture::resetNBuffers()
{
}

void Capture::resetFlashMode()
{

}

void Capture::reset()
{
}

void Capture::takePictures(int count, int delayMsecs)
{
    QtConcurrent::run(&this->d->m_threadPool,
                      [this, count, delayMsecs] () {
        for (int i = 0; i < count; i++) {
            auto frame = this->readFrame();
            emit this->pictureTaken(i, frame);
            QThread::msleep(delayMsecs);
        }
    });
}

#include "moc_capture.cpp"

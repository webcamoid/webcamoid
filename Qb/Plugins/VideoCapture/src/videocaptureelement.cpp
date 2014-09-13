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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "videocaptureelement.h"

VideoCaptureElement::VideoCaptureElement(): QbElement()
{
    QObject::connect(&this->m_capture,
                     SIGNAL(error(const QString &)),
                     this,
                     SIGNAL(error(const QString &)));

    QObject::connect(&this->m_capture,
                     SIGNAL(webcamsChanged(const QStringList &)),
                     this,
                     SIGNAL(webcamsChanged(const QStringList &)));

    QObject::connect(&this->m_capture,
                     SIGNAL(sizeChanged(const QString &, const QSize &)),
                     this,
                     SIGNAL(sizeChanged(const QString &, const QSize &)));

    QObject::connect(&this->m_capture,
                     SIGNAL(controlsChanged(const QString &, const QVariantMap &)),
                     this,
                     SIGNAL(controlsChanged(const QString &, const QVariantMap &)));

    this->m_thread = ThreadPtr(new QThread, this->deleteThread);
    this->m_thread->start();
    this->m_timer.moveToThread(this->m_thread.data());

    QObject::connect(&this->m_timer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(readFrame()),
                     Qt::DirectConnection);
}

QStringList VideoCaptureElement::webcams() const
{
    return this->m_capture.webcams();
}

QString VideoCaptureElement::device() const
{
    return this->m_capture.device();
}

QString VideoCaptureElement::ioMethod() const
{
    return this->m_capture.ioMethod();
}

int VideoCaptureElement::nBuffers() const
{
    return this->m_capture.nBuffers();
}

bool VideoCaptureElement::isCompressed() const
{
    return this->m_capture.isCompressed();
}

QString VideoCaptureElement::caps() const
{
    return this->m_capture.caps();
}

QString VideoCaptureElement::description(const QString &webcam) const
{
    return this->m_capture.description(webcam);
}

QVariantList VideoCaptureElement::availableSizes(const QString &webcam) const
{
    return this->m_capture.availableSizes(webcam);
}

QSize VideoCaptureElement::size(const QString &webcam) const
{
    return this->m_capture.size(webcam);
}

bool VideoCaptureElement::setSize(const QString &webcam, const QSize &size) const
{
    return this->m_capture.setSize(webcam, size);
}

bool VideoCaptureElement::resetSize(const QString &webcam) const
{
    return this->m_capture.resetSize(webcam);
}

QVariantList VideoCaptureElement::controls(const QString &webcam) const
{
    return this->m_capture.controls(webcam);
}

bool VideoCaptureElement::setControls(const QString &webcam, const QVariantMap &controls) const
{
    return this->m_capture.setControls(webcam, controls);
}

bool VideoCaptureElement::resetControls(const QString &webcam) const
{
    return this->m_capture.resetControls(webcam);
}

void VideoCaptureElement::deleteThread(QThread *thread)
{
    thread->quit();
    thread->wait();
    delete thread;
}

void VideoCaptureElement::stateChange(QbElement::ElementState from, QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused) {
        if (this->m_capture.init())
            QMetaObject::invokeMethod(&this->m_timer, "start");
    }
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull) {
        QMetaObject::invokeMethod(&this->m_timer, "stop");
        this->m_capture.uninit();
    }
}

void VideoCaptureElement::setDevice(const QString &device)
{
    this->m_capture.setDevice(device);
}

void VideoCaptureElement::setIoMethod(const QString &ioMethod)
{
    this->m_capture.setIoMethod(ioMethod);
}

void VideoCaptureElement::setNBuffers(int nBuffers)
{
    this->m_capture.setNBuffers(nBuffers);
}

void VideoCaptureElement::resetDevice()
{
    this->m_capture.resetDevice();
}

void VideoCaptureElement::resetIoMethod()
{
    this->m_capture.resetIoMethod();
}

void VideoCaptureElement::resetNBuffers()
{
    this->m_capture.resetNBuffers();
}

void VideoCaptureElement::reset() const
{
    this->m_capture.reset();
}

void VideoCaptureElement::readFrame()
{
    QbPacket packet = this->m_capture.readFrame();

    if (packet)
        emit this->oStream(packet);
}

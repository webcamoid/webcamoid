/* Webcamoid, camera capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#include <QMutex>
#include <QMutexLocker>
#include <akfrac.h>
#include <akvideoconverter.h>

#include "vcam.h"

class VCamPrivate
{
    public:
        static AkVideoCaps defaultCaps()
        {
            return {AkVideoCaps::Format_yuyv422, 640, 480, AkFrac(30, 1)};
        }

        QString m_description;
        AkVideoCaps m_outputCaps {VCamPrivate::defaultCaps()};
        AkVideoCaps m_activeCaps;
        AkVideoConverter::AspectRatioMode m_aspectRatioMode {AkVideoConverter::AspectRatioMode_Expanding};
        AkVideoConverter m_videoConverter;
        QMutex m_mutex;
        bool m_running {false};
};

VCam::VCam(QObject *parent):
    QObject(parent)
{
    this->d = new VCamPrivate;
}

VCam::~VCam()
{
    delete this->d;
}

QString VCam::error() const
{
    return {};
}

bool VCam::isInstalled() const
{
    return false;
}

QString VCam::installedVersion() const
{
    return {};
}

QString VCam::device() const
{
    return {};
}

QString VCam::description() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_description;
}

AkVideoCaps VCam::outputCaps() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_outputCaps;
}

AkVideoConverter::AspectRatioMode VCam::aspectRatioMode() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_aspectRatioMode;
}

QList<quint64> VCam::clientsPids() const
{
    return {};
}

QString VCam::clientExe(quint64 pid) const
{
    Q_UNUSED(pid)

    return {};
}

QString VCam::picture() const
{
    return {};
}

bool VCam::canEditVCamDescription() const
{
    return true;
}

QString VCam::deviceCreate(const QString &description)
{
    Q_UNUSED(description)

    return {};
}

bool VCam::changeDescription(const QString &description)
{
    Q_UNUSED(description)

    return false;
}

bool VCam::deviceDestroy()
{
    return false;
}

bool VCam::init()
{
    if (this->d->m_running)
        return true;

    this->d->m_mutex.lock();
    auto caps = this->d->m_outputCaps;
    this->d->m_mutex.unlock();

    if (!caps)
        return false;

    // The plugin can fix the format to what the driver really supports.
    if (!this->openDevice(caps))
        return false;

    this->d->m_mutex.lock();
    this->d->m_activeCaps = caps;
    this->d->m_videoConverter.setOutputCaps(caps);
    this->d->m_running = true;
    this->d->m_mutex.unlock();

    return true;
}

void VCam::uninit()
{
    if (!this->d->m_running)
        return;

    this->d->m_mutex.lock();
    this->d->m_running = false;
    this->d->m_mutex.unlock();
    this->closeDevice();
}

bool VCam::write(const AkVideoPacket &packet)
{
    if (!this->d->m_running)
        return false;

    this->d->m_mutex.lock();
    auto aspectRatioMode = this->d->m_aspectRatioMode;
    this->d->m_mutex.unlock();

    /* The converter's 'Keep' mode shrinks the output frame to the aspect ratio
     * of the input, but the driver needs frames with the size of the format
     * that was negotiated. 'Fit' keeps the aspect ratio too, but centers the
     * image in a frame of the requested size and fills the rest with black.
     */
    if (aspectRatioMode == AkVideoConverter::AspectRatioMode_Keep)
        aspectRatioMode = AkVideoConverter::AspectRatioMode_Fit;

    this->d->m_videoConverter.setAspectRatioMode(aspectRatioMode);
    this->d->m_videoConverter.begin();
    auto videoPacket = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!videoPacket)
        return false;

    // Never write a frame with a different size than the one negotiated.
    auto caps = videoPacket.caps();

    if (caps.width() != this->d->m_activeCaps.width()
        || caps.height() != this->d->m_activeCaps.height())
        return false;

    return this->writeFrame(videoPacket);
}

bool VCam::applyPicture()
{
    return false;
}

void VCam::setDescription(const QString &description)
{
    this->d->m_mutex.lock();

    if (this->d->m_description == description) {
        this->d->m_mutex.unlock();

        return;
    }

    this->d->m_description = description;
    this->d->m_mutex.unlock();
    emit this->descriptionChanged(description);
}

void VCam::setOutputCaps(const AkVideoCaps &outputCaps)
{
    this->d->m_mutex.lock();

    if (this->d->m_outputCaps == outputCaps) {
        this->d->m_mutex.unlock();

        return;
    }

    // Only stored, the active format is not changed until the next init().
    this->d->m_outputCaps = outputCaps;
    this->d->m_mutex.unlock();
    emit this->outputCapsChanged(outputCaps);
}

void VCam::setAspectRatioMode(AkVideoConverter::AspectRatioMode aspectRatioMode)
{
    this->d->m_mutex.lock();

    if (this->d->m_aspectRatioMode == aspectRatioMode) {
        this->d->m_mutex.unlock();

        return;
    }

    this->d->m_aspectRatioMode = aspectRatioMode;
    this->d->m_mutex.unlock();
    emit this->aspectRatioModeChanged(aspectRatioMode);
}

void VCam::setPicture(const QString &picture)
{
    Q_UNUSED(picture)
}

void VCam::resetDescription()
{
    this->setDescription({});
}

void VCam::resetOutputCaps()
{
    this->setOutputCaps(VCamPrivate::defaultCaps());
}

void VCam::resetAspectRatioMode()
{
    this->setAspectRatioMode(AkVideoConverter::AspectRatioMode_Expanding);
}

void VCam::resetPicture()
{
    this->setPicture({});
}

bool VCam::openDevice(AkVideoCaps &caps)
{
    Q_UNUSED(caps)

    return false;
}

void VCam::closeDevice()
{
}

bool VCam::writeFrame(const AkVideoPacket &packet)
{
    Q_UNUSED(packet)

    return false;
}

#include "moc_vcam.cpp"

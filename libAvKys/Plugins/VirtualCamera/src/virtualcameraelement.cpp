/* Webcamoid, camera capture application.
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

#include <QFuture>
#include <QMutex>
#include <QQueue>
#include <QSharedPointer>
#include <QThread>
#include <QThreadPool>
#include <QWaitCondition>
#include <QtConcurrent>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideopacket.h>

#include "virtualcameraelement.h"
#include "vcam.h"

#define THREAD_WAIT_LIMIT 500

#define DEFAULT_PICTURE ":/VirtualCamera/share/TestFrame/TestFrame.bmp"

enum VCamOperationType {
    VCamOperationType_Unknown = -1,
    VCamOperationType_Create,
    VCamOperationType_ChangeDescription,
    VCamOperationType_DeviceDestroy
};

struct VirtualCameraOperation
{
    VCamOperationType type;
    QString description;
    VCamPtr vcam;
};

class VirtualCameraElementPrivate
{
    public:
        VirtualCameraElement *self;
        VCamPtr m_vcam;
        QString m_vcamImpl;
        mutable QMutex m_mutex;
        QThreadPool m_threadPool;
        QFuture<void> m_operationsLoopResult;
        QWaitCondition m_operationsQueueNotEmpty;
        QQueue<VirtualCameraOperation> m_operations;
        bool m_run {true};

        explicit VirtualCameraElementPrivate(VirtualCameraElement *self);
        VCamPtr vcam() const;
        void connectVCam();
        void enqueue(const VirtualCameraOperation &operation);
        void linksChanged(const AkPluginLinks &links);
        void operationsLoop();
};

VirtualCameraElement::VirtualCameraElement():
    AkElement()
{
    this->d = new VirtualCameraElementPrivate(this);
    QObject::connect(akPluginManager,
                     &AkPluginManager::linksChanged,
                     this,
                     [this] (const AkPluginLinks &links) {
                        this->d->linksChanged(links);
                     });

    if (this->d->m_vcam) {
        this->d->connectVCam();
        this->d->m_vcam->setPicture(DEFAULT_PICTURE);
    }

    this->d->m_operationsLoopResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              &VirtualCameraElementPrivate::operationsLoop,
                              this->d);
}

VirtualCameraElement::~VirtualCameraElement()
{
    this->setState(AkElement::ElementStateNull);

    this->d->m_run = false;
    this->d->m_operationsLoopResult.waitForFinished();

    delete this->d;
}

QString VirtualCameraElement::error() const
{
    auto vcam = this->d->vcam();

    return vcam? vcam->error(): QString();
}

QString VirtualCameraElement::device() const
{
    auto vcam = this->d->vcam();

    return vcam? vcam->device(): QString();
}

QString VirtualCameraElement::description() const
{
    auto vcam = this->d->vcam();

    return vcam? vcam->description(): QString();
}

AkVideoCaps VirtualCameraElement::outputCaps() const
{
    auto vcam = this->d->vcam();

    return vcam? vcam->outputCaps(): AkVideoCaps();
}

AkVideoConverter::AspectRatioMode VirtualCameraElement::aspectRatioMode() const
{
    auto vcam = this->d->vcam();

    return vcam?
               vcam->aspectRatioMode():
               AkVideoConverter::AspectRatioMode_Expanding;
}

void VirtualCameraElement::createWebcam(const QString &description)
{
    this->d->enqueue({VCamOperationType_Create,
                      description,
                      this->d->vcam()});
}

void VirtualCameraElement::changeDescription(const QString &description)
{
    this->d->enqueue({VCamOperationType_ChangeDescription,
                      description,
                      this->d->vcam()});
}

void VirtualCameraElement::removeWebcam()
{
    this->d->enqueue({VCamOperationType_DeviceDestroy,
                      {},
                      this->d->vcam()});
}

QList<quint64> VirtualCameraElement::clientsPids() const
{
    auto vcam = this->d->vcam();

    return vcam? vcam->clientsPids(): QList<quint64>();
}

QString VirtualCameraElement::clientExe(quint64 pid) const
{
    auto vcam = this->d->vcam();

    return vcam? vcam->clientExe(pid): QString();
}

bool VirtualCameraElement::driverInstalled() const
{
    auto vcam = this->d->vcam();

    return vcam && vcam->isInstalled();
}

QString VirtualCameraElement::driverVersion() const
{
    auto vcam = this->d->vcam();

    return vcam? vcam->installedVersion(): QString();
}

QString VirtualCameraElement::picture() const
{
    auto vcam = this->d->vcam();

    return vcam? vcam->picture(): QString();
}

bool VirtualCameraElement::canEditVCamDescription() const
{
    auto vcam = this->d->vcam();

    return vcam && vcam->canEditVCamDescription();
}

AkPacket VirtualCameraElement::iVideoStream(const AkVideoPacket &packet)
{
    if (this->state() == AkElement::ElementStatePlaying) {
        auto vcam = this->d->vcam();

        if (vcam)
            vcam->write(packet);
    }

    if (packet)
        emit this->oStream(packet);

    return packet;
}

bool VirtualCameraElement::applyPicture()
{
    auto vcam = this->d->vcam();

    return vcam && vcam->applyPicture();
}

void VirtualCameraElement::setDescription(const QString &description)
{
    auto vcam = this->d->vcam();

    if (vcam)
        vcam->setDescription(description);
}

void VirtualCameraElement::setOutputCaps(const AkVideoCaps &outputCaps)
{
    auto vcam = this->d->vcam();

    if (!vcam || vcam->outputCaps() == outputCaps)
        return;

    /* The new format is only taken by the virtual camera when it starts, so
     * if it is running we must restart it.
     */
    auto state = this->state();

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStateNull);

    vcam->setOutputCaps(outputCaps);

    if (state != AkElement::ElementStateNull)
        this->setState(state);
}

void VirtualCameraElement::setAspectRatioMode(AkVideoConverter::AspectRatioMode aspectRatioMode)
{
    auto vcam = this->d->vcam();

    if (vcam)
        vcam->setAspectRatioMode(aspectRatioMode);
}

void VirtualCameraElement::setPicture(const QString &picture)
{
    auto vcam = this->d->vcam();

    if (vcam)
        vcam->setPicture(picture);
}

void VirtualCameraElement::resetDescription()
{
    this->setDescription({});
}

void VirtualCameraElement::resetOutputCaps()
{
    auto vcam = this->d->vcam();

    if (vcam)
        vcam->resetOutputCaps();
}

void VirtualCameraElement::resetAspectRatioMode()
{
    auto vcam = this->d->vcam();

    if (vcam)
        vcam->resetAspectRatioMode();
}

void VirtualCameraElement::resetPicture()
{
    auto vcam = this->d->vcam();

    if (vcam)
        vcam->setPicture(DEFAULT_PICTURE);
}

bool VirtualCameraElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
        case AkElement::ElementStatePlaying: {
            auto vcam = this->d->vcam();

            if (!vcam)
                return false;

            if (!vcam->init())
                return false;

            return AkElement::setState(state);
        }
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull: {
            auto vcam = this->d->vcam();

            if (vcam)
                vcam->uninit();

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePlaying:
            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull: {
            auto vcam = this->d->vcam();

            if (vcam)
                vcam->uninit();

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

VirtualCameraElementPrivate::VirtualCameraElementPrivate(VirtualCameraElement *self):
    self(self)
{
    this->m_vcam =
            akPluginManager->create<VCam>("VideoSink/VirtualCamera/Impl/*");
    this->m_vcamImpl =
            akPluginManager->defaultPlugin("VideoSink/VirtualCamera/Impl/*",
                                           {"VirtualCameraImpl"}).id();
}

VCamPtr VirtualCameraElementPrivate::vcam() const
{
    QMutexLocker mutexLocker(&this->m_mutex);

    return this->m_vcam;
}

void VirtualCameraElementPrivate::connectVCam()
{
    QObject::connect(this->m_vcam.data(),
                     &VCam::errorChanged,
                     self,
                     &VirtualCameraElement::errorChanged);
    QObject::connect(this->m_vcam.data(),
                     &VCam::deviceChanged,
                     self,
                     &VirtualCameraElement::deviceChanged);
    QObject::connect(this->m_vcam.data(),
                     &VCam::descriptionChanged,
                     self,
                     &VirtualCameraElement::descriptionChanged);
    QObject::connect(this->m_vcam.data(),
                     &VCam::outputCapsChanged,
                     self,
                     &VirtualCameraElement::outputCapsChanged);
    QObject::connect(this->m_vcam.data(),
                     &VCam::aspectRatioModeChanged,
                     self,
                     &VirtualCameraElement::aspectRatioModeChanged);
    QObject::connect(this->m_vcam.data(),
                     &VCam::pictureChanged,
                     self,
                     &VirtualCameraElement::pictureChanged);
}

void VirtualCameraElementPrivate::enqueue(const VirtualCameraOperation &operation)
{
    QMutexLocker mutexLocker(&this->m_mutex);
    this->m_operations << operation;
    this->m_operationsQueueNotEmpty.wakeOne();
}

void VirtualCameraElementPrivate::linksChanged(const AkPluginLinks &links)
{
    if (!links.contains("VideoSink/VirtualCamera/Impl/*")
        || links["VideoSink/VirtualCamera/Impl/*"] == this->m_vcamImpl)
        return;

    auto state = self->state();
    self->setState(AkElement::ElementStateNull);

    AkVideoCaps outputCaps;
    auto aspectRatioMode = AkVideoConverter::AspectRatioMode_Expanding;
    QString description;
    QString picture;

    if (this->m_vcam) {
        outputCaps = this->m_vcam->outputCaps();
        aspectRatioMode = this->m_vcam->aspectRatioMode();
        description = this->m_vcam->description();
        picture = this->m_vcam->picture();
    }

    this->m_mutex.lock();
    this->m_vcam = akPluginManager->create<VCam>("VideoSink/VirtualCamera/Impl/*");
    this->m_mutex.unlock();
    this->m_vcamImpl = links["VideoSink/VirtualCamera/Impl/*"];

    if (!this->m_vcam)
        return;

    this->connectVCam();
    this->m_vcam->setOutputCaps(outputCaps);
    this->m_vcam->setAspectRatioMode(aspectRatioMode);
    this->m_vcam->setDescription(description);

    if (this->m_vcam->picture().isEmpty()) {
        if (picture.isEmpty())
            this->m_vcam->setPicture(DEFAULT_PICTURE);
        else
            this->m_vcam->setPicture(picture);
    }

    emit self->deviceChanged(this->m_vcam->device());

    self->setState(state);
}

void VirtualCameraElementPrivate::operationsLoop()
{
    while (this->m_run) {
        bool gotOperation = true;
        VirtualCameraOperation operation;

        {
            QMutexLocker mutexLocker(&this->m_mutex);

            if (this->m_operations.isEmpty())
                gotOperation =
                        this->m_operationsQueueNotEmpty.wait(&this->m_mutex,
                                                             THREAD_WAIT_LIMIT);

            if (gotOperation && !this->m_operations.isEmpty())
                operation = this->m_operations.dequeue();
            else
                gotOperation = false;
        }

        if (!gotOperation)
            continue;

        auto vcam = operation.vcam;
        bool ok = false;
        QString result;

        if (vcam) {
            switch (operation.type) {
            case VCamOperationType_Create:
                result = vcam->deviceCreate(operation.description);
                ok = !result.isEmpty();

                break;

            case VCamOperationType_ChangeDescription:
                ok = vcam->changeDescription(operation.description);
                result = vcam->description();

                break;

            case VCamOperationType_DeviceDestroy:
                ok = vcam->deviceDestroy();
                result = {};

                break;

            default:
                break;
            }

            if (!ok)
                result = vcam->error();
        } else {
            result = "Invalid submodule";
        }

        if (!ok)
            emit self->errorChanged(result);

        switch (operation.type) {
        case VCamOperationType_Create:
            emit self->webcamCreated(ok, result);

            break;

        case VCamOperationType_ChangeDescription:
            emit self->descriptionEdited(ok, result);

            break;

        case VCamOperationType_DeviceDestroy:
            emit self->webcamRemoved(ok, result);

            break;

        default:
            break;
        }
    }
}

#include "moc_virtualcameraelement.cpp"

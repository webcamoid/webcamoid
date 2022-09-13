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

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QReadWriteLock>
#include <QSharedPointer>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "virtualcameraelement.h"
#include "vcam.h"

#define MAX_CAMERAS 64
#define PREFERRED_ROUNDING 32

class VirtualCameraElementPrivate
{
    public:
        VirtualCameraElement *self;
        VCamPtr m_vcam;
        QString m_vcamImpl;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_rgb24, 0, 0, {}}};
        QReadWriteLock m_mutex;
        int m_streamIndex {-1};
        bool m_playing {false};

        explicit VirtualCameraElementPrivate(VirtualCameraElement *self);
        static inline int roundTo(int value, int n);
        void linksChanged(const AkPluginLinks &links);
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
        QObject::connect(this->d->m_vcam.data(),
                         &VCam::errorChanged,
                         this,
                         &VirtualCameraElement::errorChanged);
        QObject::connect(this->d->m_vcam.data(),
                         &VCam::webcamsChanged,
                         this,
                         &VirtualCameraElement::mediasChanged);
        QObject::connect(this->d->m_vcam.data(),
                         &VCam::deviceChanged,
                         this,
                         &VirtualCameraElement::mediaChanged);
        QObject::connect(this->d->m_vcam.data(),
                         &VCam::pictureChanged,
                         this,
                         &VirtualCameraElement::pictureChanged);
        QObject::connect(this->d->m_vcam.data(),
                         &VCam::rootMethodChanged,
                         this,
                         &VirtualCameraElement::rootMethodChanged);

        this->d->m_vcam->setPicture(":/VirtualCamera/share/TestFrame/TestFrame.bmp");
        auto medias = this->d->m_vcam->webcams();

        if (!medias.isEmpty())
            this->d->m_vcam->setDevice(medias.first());
    }
}

VirtualCameraElement::~VirtualCameraElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QString VirtualCameraElement::error() const
{
    this->d->m_mutex.lockForRead();
    QString error;

    if (this->d->m_vcam)
        error = this->d->m_vcam->error();

    this->d->m_mutex.unlock();

    return error;
}

QStringList VirtualCameraElement::medias() const
{
    this->d->m_mutex.lockForRead();
    QStringList medias;

    if (this->d->m_vcam)
        medias = this->d->m_vcam->webcams();

    this->d->m_mutex.unlock();

    return medias;
}

QString VirtualCameraElement::media() const
{
    this->d->m_mutex.lockForRead();
    QString media;

    if (this->d->m_vcam)
        media = this->d->m_vcam->device();

    this->d->m_mutex.unlock();

    return media;
}

QList<int> VirtualCameraElement::streams() const
{
    return QList<int> {0};
}

int VirtualCameraElement::maxCameras() const
{
    return MAX_CAMERAS;
}

AkVideoCaps::PixelFormatList VirtualCameraElement::supportedOutputPixelFormats() const
{
    this->d->m_mutex.lockForRead();
    AkVideoCaps::PixelFormatList formats;

    if (this->d->m_vcam)
        formats = this->d->m_vcam->supportedOutputPixelFormats();

    this->d->m_mutex.unlock();

    return formats;
}

AkVideoCaps::PixelFormat VirtualCameraElement::defaultOutputPixelFormat() const
{
    this->d->m_mutex.lockForRead();
    AkVideoCaps::PixelFormat format = AkVideoCaps::Format_none;

    if (this->d->m_vcam)
        format = this->d->m_vcam->defaultOutputPixelFormat();

    this->d->m_mutex.unlock();

    return format;
}

int VirtualCameraElement::defaultStream(AkCaps::CapsType type) const
{
    if (type == AkCaps::CapsVideo)
        return 0;

    return -1;
}

QString VirtualCameraElement::description(const QString &media) const
{
    this->d->m_mutex.lockForRead();
    QString description;

    if (this->d->m_vcam)
        description = this->d->m_vcam->description(media);

    this->d->m_mutex.unlock();

    return description;
}

AkCaps VirtualCameraElement::caps(int stream) const
{
    if (stream != 0)
        return {};

    this->d->m_mutex.lockForRead();
    AkCaps caps;

    if (this->d->m_vcam)
        caps = this->d->m_vcam->currentCaps();

    this->d->m_mutex.unlock();

    return caps;
}

AkVideoCapsList VirtualCameraElement::outputCaps(const QString &webcam) const
{
    this->d->m_mutex.lockForRead();
    AkVideoCapsList caps;

    if (this->d->m_vcam)
        caps = this->d->m_vcam->caps(webcam);

    this->d->m_mutex.unlock();

    return caps;
}

QVariantMap VirtualCameraElement::addStream(int streamIndex,
                                            const AkCaps &streamCaps,
                                            const QVariantMap &streamParams)
{
    Q_UNUSED(streamParams)

    if (streamIndex != 0)
        return {};

    AkVideoCaps videoCaps(streamCaps);
    videoCaps.setFormat(AkVideoCaps::Format_rgb24);
    videoCaps.setWidth(VirtualCameraElementPrivate::roundTo(videoCaps.width(),
                                                            PREFERRED_ROUNDING));
    videoCaps.setHeight(VirtualCameraElementPrivate::roundTo(videoCaps.height(),
                                                             PREFERRED_ROUNDING));

    this->d->m_streamIndex = streamIndex;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        this->d->m_vcam->setCurrentCaps(videoCaps);

    this->d->m_mutex.unlock();
    QVariantMap outputParams = {
        {"caps", QVariant::fromValue(streamCaps)}
    };

    return outputParams;
}

QVariantMap VirtualCameraElement::updateStream(int streamIndex,
                                               const QVariantMap &streamParams)
{
    if (streamIndex != 0)
        return {};

    auto streamCaps = streamParams.value("caps").value<AkCaps>();

    if (!streamCaps)
        return {};

    AkVideoCaps videoCaps(streamCaps);
    videoCaps.setFormat(AkVideoCaps::Format_rgb24);
    videoCaps.setWidth(VirtualCameraElementPrivate::roundTo(videoCaps.width(),
                                                            PREFERRED_ROUNDING));
    videoCaps.setHeight(VirtualCameraElementPrivate::roundTo(videoCaps.height(),
                                                             PREFERRED_ROUNDING));

    this->d->m_streamIndex = streamIndex;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        this->d->m_vcam->setCurrentCaps(videoCaps);

    this->d->m_mutex.unlock();

    QVariantMap outputParams {
        {"caps", QVariant::fromValue(streamCaps)}
    };

    return outputParams;
}

QString VirtualCameraElement::createWebcam(const QString &description,
                                           const AkVideoCapsList &formats)
{
    QString webcam;
    QString error;

    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam) {
        webcam = this->d->m_vcam->deviceCreate(description, formats);

        if (webcam.isEmpty())
            error = this->d->m_vcam->error();
    } else {
        error = "Invalid submodule";
    }

    this->d->m_mutex.unlock();

    if (error.isEmpty())
        emit this->mediasChanged(this->medias());
    else
        emit this->errorChanged(error);

    return webcam;
}

bool VirtualCameraElement::editWebcam(const QString &webcam,
                                      const QString &description,
                                      const AkVideoCapsList &formats)
{
    bool ok = false;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        ok = this->d->m_vcam->deviceEdit(webcam, description, formats);

    this->d->m_mutex.unlock();

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

bool VirtualCameraElement::changeDescription(const QString &webcam,
                                             const QString &description)
{
    bool ok = false;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        ok = this->d->m_vcam->changeDescription(webcam, description);

    this->d->m_mutex.unlock();

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

bool VirtualCameraElement::removeWebcam(const QString &webcam)
{
    bool ok = false;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        ok = this->d->m_vcam->deviceDestroy(webcam);

    this->d->m_mutex.unlock();

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

bool VirtualCameraElement::removeAllWebcams()
{
    bool ok = false;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        ok = this->d->m_vcam->destroyAllDevices();

    this->d->m_mutex.unlock();

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

QVariantList VirtualCameraElement::controls() const
{
    this->d->m_mutex.lockForRead();
    QVariantList controls;

    if (this->d->m_vcam)
        controls = this->d->m_vcam->controls();

    this->d->m_mutex.unlock();

    return controls;
}

bool VirtualCameraElement::setControls(const QVariantMap &controls)
{
    this->d->m_mutex.lockForWrite();
    bool result = false;

    if (this->d->m_vcam)
        result = this->d->m_vcam->setControls(controls);

    this->d->m_mutex.unlock();

    return result;
}

bool VirtualCameraElement::resetControls()
{
    this->d->m_mutex.lockForWrite();
    bool result = this->d->m_vcam != nullptr;
    this->d->m_mutex.unlock();

    return result;
}

QList<quint64> VirtualCameraElement::clientsPids() const
{
    this->d->m_mutex.lockForRead();
    QList<quint64> pids;

    if (this->d->m_vcam)
        pids = this->d->m_vcam->clientsPids();

    this->d->m_mutex.unlock();

    return pids;
}

QString VirtualCameraElement::clientExe(quint64 pid) const
{
    this->d->m_mutex.lockForRead();
    QString exe;

    if (this->d->m_vcam)
        exe = this->d->m_vcam->clientExe(pid);

    this->d->m_mutex.unlock();

    return exe;
}

bool VirtualCameraElement::driverInstalled() const
{
    this->d->m_mutex.lockForRead();
    bool installed = false;

    if (this->d->m_vcam)
        installed = this->d->m_vcam->isInstalled();

    this->d->m_mutex.unlock();

    return installed;
}

QString VirtualCameraElement::driverVersion() const
{
    this->d->m_mutex.lockForRead();
    QString version;

    if (this->d->m_vcam)
        version = this->d->m_vcam->installedVersion();

    this->d->m_mutex.unlock();

    return version;
}

QString VirtualCameraElement::picture() const
{
    this->d->m_mutex.lockForRead();
    QString picture;

    if (this->d->m_vcam)
        picture = this->d->m_vcam->picture();

    this->d->m_mutex.unlock();

    return picture;
}

QString VirtualCameraElement::rootMethod() const
{
    this->d->m_mutex.lockForRead();
    QString rootMethod;

    if (this->d->m_vcam)
        rootMethod = this->d->m_vcam->rootMethod();

    this->d->m_mutex.unlock();

    return rootMethod;
}

QStringList VirtualCameraElement::availableRootMethods() const
{
    this->d->m_mutex.lockForRead();
    QStringList methods;

    if (this->d->m_vcam)
        methods = this->d->m_vcam->availableRootMethods();

    this->d->m_mutex.unlock();

    return methods;
}

QString VirtualCameraElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VirtualCamera/share/qml/main.qml");
}

void VirtualCameraElement::controlInterfaceConfigure(QQmlContext *context,
                                                     const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("virtualCamera", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", controlId);
}

AkPacket VirtualCameraElement::iVideoStream(const AkVideoPacket &packet)
{
    if (this->state() == AkElement::ElementStatePlaying) {
        auto videoPacket = this->d->m_videoConverter.convert(packet);
        this->d->m_mutex.lockForWrite();

        if (this->d->m_vcam)
            this->d->m_vcam->write(videoPacket);

        this->d->m_mutex.unlock();
    }

    if (packet)
        emit this->oStream(packet);

    return packet;
}

bool VirtualCameraElement::applyPicture()
{
    bool result = false;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        result = this->d->m_vcam->applyPicture();

    this->d->m_mutex.unlock();

    return result;
}

void VirtualCameraElement::setMedia(const QString &media)
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        this->d->m_vcam->setDevice(media);

    this->d->m_mutex.unlock();
}

void VirtualCameraElement::setPicture(const QString &picture)
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        this->d->m_vcam->setPicture(picture);

    this->d->m_mutex.unlock();
}

void VirtualCameraElement::setRootMethod(const QString &rootMethod)
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        this->d->m_vcam->setRootMethod(rootMethod);

    this->d->m_mutex.unlock();
}

void VirtualCameraElement::resetMedia()
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        this->d->m_vcam->resetPicture();

    this->d->m_mutex.unlock();
}

void VirtualCameraElement::resetPicture()
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        this->d->m_vcam->setPicture(":/VirtualCamera/share/TestFrame/TestFrame.bmp");

    this->d->m_mutex.unlock();
}

void VirtualCameraElement::resetRootMethod()
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        this->d->m_vcam->resetRootMethod();

    this->d->m_mutex.unlock();
}

void VirtualCameraElement::clearStreams()
{
    this->d->m_streamIndex = -1;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_vcam)
        this->d->m_vcam->resetCurrentCaps();

    this->d->m_mutex.unlock();
}

bool VirtualCameraElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
        case AkElement::ElementStatePlaying: {
            this->d->m_mutex.lockForWrite();

            if (!this->d->m_vcam) {
                this->d->m_mutex.unlock();

                return false;
            }

            if (!this->d->m_vcam->init()) {
                this->d->m_mutex.unlock();

                return false;
            }

            this->d->m_mutex.unlock();
            this->d->m_playing = true;

            return AkElement::setState(state);
        }
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->m_playing = false;

            this->d->m_mutex.lockForWrite();

            if (this->d->m_vcam)
                this->d->m_vcam->uninit();

            this->d->m_mutex.unlock();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->m_playing = false;

            this->d->m_mutex.lockForWrite();

            if (this->d->m_vcam)
                this->d->m_vcam->uninit();

            this->d->m_mutex.unlock();

            return AkElement::setState(state);
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

int VirtualCameraElementPrivate::roundTo(int value, int n)
{
    return n * qRound(value / qreal(n));
}

void VirtualCameraElementPrivate::linksChanged(const AkPluginLinks &links)
{
    if (!links.contains("VideoSink/VirtualCamera/Impl/*")
        || links["VideoSink/VirtualCamera/Impl/*"] == this->m_vcamImpl)
        return;

    auto state = self->state();
    self->setState(AkElement::ElementStateNull);
    this->m_mutex.lockForWrite();

    AkVideoCaps videoCaps;
    QString rootMethod;
    QString picture;

    if (this->m_vcam) {
        videoCaps = this->m_vcam->currentCaps();
        picture = this->m_vcam->picture();
        rootMethod = this->m_vcam->rootMethod();
    }

    this->m_vcam = akPluginManager->create<VCam>("VideoSink/VirtualCamera/Impl/*");
    this->m_mutex.unlock();
    this->m_vcamImpl = links["VideoSink/VirtualCamera/Impl/*"];

    if (!this->m_vcam)
        return;

    QObject::connect(this->m_vcam.data(),
                     &VCam::errorChanged,
                     self,
                     &VirtualCameraElement::errorChanged);
    QObject::connect(this->m_vcam.data(),
                     &VCam::webcamsChanged,
                     self,
                     &VirtualCameraElement::mediasChanged);
    QObject::connect(this->m_vcam.data(),
                     &VCam::deviceChanged,
                     self,
                     &VirtualCameraElement::mediaChanged);
    QObject::connect(this->m_vcam.data(),
                     &VCam::pictureChanged,
                     self,
                     &VirtualCameraElement::pictureChanged);
    QObject::connect(this->m_vcam.data(),
                     &VCam::rootMethodChanged,
                     self,
                     &VirtualCameraElement::rootMethodChanged);

    this->m_vcam->setCurrentCaps(videoCaps);
    this->m_vcam->setRootMethod(rootMethod);
    auto medias = this->m_vcam->webcams();

    if (this->m_vcam->picture().isEmpty()) {
        if (picture.isEmpty())
            this->m_vcam->setPicture(":/VirtualCamera/share/TestFrame/TestFrame.bmp");
        else
            this->m_vcam->setPicture(picture);
    }

    emit self->mediasChanged(medias);
    emit self->streamsChanged(self->streams());
    emit self->supportedOutputPixelFormatsChanged(this->m_vcam->supportedOutputPixelFormats());
    emit self->defaultOutputPixelFormatChanged(this->m_vcam->defaultOutputPixelFormat());

    if (!medias.isEmpty())
        self->setMedia(medias.first());

    self->setState(state);
}

#include "moc_virtualcameraelement.cpp"

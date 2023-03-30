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
#include <QMutex>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
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

class VirtualCameraElementPrivate
{
    public:
        VirtualCameraElement *self;
        VCamPtr m_vcam;
        QString m_vcamImpl;
        QMutex m_mutex;
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
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    QString error;

    if (vcam)
        error = vcam->error();

    return error;
}

QStringList VirtualCameraElement::medias() const
{
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    QStringList medias;

    if (vcam)
        medias = vcam->webcams();

    return medias;
}

QString VirtualCameraElement::media() const
{
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    QString media;

    if (vcam)
        media = vcam->device();

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
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    AkVideoCaps::PixelFormatList formats;

    if (vcam)
        formats = vcam->supportedOutputPixelFormats();

    return formats;
}

AkVideoCaps::PixelFormat VirtualCameraElement::defaultOutputPixelFormat() const
{
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    AkVideoCaps::PixelFormat format = AkVideoCaps::Format_none;

    if (vcam)
        format = vcam->defaultOutputPixelFormat();

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
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    QString description;

    if (vcam)
        description = vcam->description(media);

    return description;
}

AkCaps VirtualCameraElement::caps(int stream) const
{
    if (stream != 0)
        return {};

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    AkCaps caps;

    if (vcam)
        caps = vcam->currentCaps();

    return caps;
}

AkVideoCapsList VirtualCameraElement::outputCaps(const QString &webcam) const
{
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    AkVideoCapsList caps;

    if (vcam)
        caps = vcam->caps(webcam);

    return caps;
}

QVariantMap VirtualCameraElement::addStream(int streamIndex,
                                            const AkCaps &streamCaps,
                                            const QVariantMap &streamParams)
{
    Q_UNUSED(streamParams)

    if (streamIndex != 0)
        return {};

    this->d->m_streamIndex = streamIndex;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        vcam->setCurrentCaps(streamCaps);

    QVariantMap outputParams {
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

    this->d->m_streamIndex = streamIndex;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        vcam->setCurrentCaps(streamCaps);

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

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam) {
        webcam = vcam->deviceCreate(description, formats);

        if (webcam.isEmpty())
            error = vcam->error();
    } else {
        error = "Invalid submodule";
    }

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

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        ok = vcam->deviceEdit(webcam, description, formats);

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

bool VirtualCameraElement::changeDescription(const QString &webcam,
                                             const QString &description)
{
    bool ok = false;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        ok = vcam->changeDescription(webcam, description);

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

bool VirtualCameraElement::removeWebcam(const QString &webcam)
{
    bool ok = false;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        ok = vcam->deviceDestroy(webcam);

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

bool VirtualCameraElement::removeAllWebcams()
{
    bool ok = false;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        ok = vcam->destroyAllDevices();

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

QVariantList VirtualCameraElement::controls() const
{
    QVariantList controls;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        controls = vcam->controls();

    return controls;
}

bool VirtualCameraElement::setControls(const QVariantMap &controls)
{
    bool result = false;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        result = vcam->setControls(controls);

    return result;
}

bool VirtualCameraElement::resetControls()
{
    this->d->m_mutex.lock();
    bool result = this->d->m_vcam != nullptr;
    this->d->m_mutex.unlock();

    return result;
}

QList<quint64> VirtualCameraElement::clientsPids() const
{
    QList<quint64> pids;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        pids = vcam->clientsPids();

    return pids;
}

QString VirtualCameraElement::clientExe(quint64 pid) const
{
    QString exe;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        exe = vcam->clientExe(pid);

    return exe;
}

bool VirtualCameraElement::driverInstalled() const
{
    bool installed = false;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        installed = vcam->isInstalled();

    return installed;
}

QString VirtualCameraElement::driverVersion() const
{
    QString version;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        version = vcam->installedVersion();

    return version;
}

QString VirtualCameraElement::picture() const
{
    QString picture;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        picture = vcam->picture();

    return picture;
}

QString VirtualCameraElement::rootMethod() const
{
    QString rootMethod;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        rootMethod = vcam->rootMethod();

    return rootMethod;
}

QStringList VirtualCameraElement::availableRootMethods() const
{
    QStringList methods;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        methods = vcam->availableRootMethods();

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
        this->d->m_mutex.lock();
        auto vcam = this->d->m_vcam;
        this->d->m_mutex.unlock();

        if (vcam)
            vcam->write(packet);
    }

    if (packet)
        emit this->oStream(packet);

    return packet;
}

bool VirtualCameraElement::applyPicture()
{
    bool result = false;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        result = vcam->applyPicture();

    return result;
}

void VirtualCameraElement::setMedia(const QString &media)
{
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        vcam->setDevice(media);
}

void VirtualCameraElement::setPicture(const QString &picture)
{
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        vcam->setPicture(picture);
}

void VirtualCameraElement::setRootMethod(const QString &rootMethod)
{
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        vcam->setRootMethod(rootMethod);
}

void VirtualCameraElement::resetMedia()
{
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        vcam->resetPicture();
}

void VirtualCameraElement::resetPicture()
{
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        vcam->setPicture(":/VirtualCamera/share/TestFrame/TestFrame.bmp");
}

void VirtualCameraElement::resetRootMethod()
{
    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        vcam->resetRootMethod();
}

void VirtualCameraElement::clearStreams()
{
    this->d->m_streamIndex = -1;

    this->d->m_mutex.lock();
    auto vcam = this->d->m_vcam;
    this->d->m_mutex.unlock();

    if (vcam)
        vcam->resetCurrentCaps();
}

bool VirtualCameraElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
        case AkElement::ElementStatePlaying: {
            this->d->m_mutex.lock();
            auto vcam = this->d->m_vcam;
            this->d->m_mutex.unlock();

            if (!vcam) {

                return false;
            }

            if (!vcam->init())
                return false;

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
        case AkElement::ElementStateNull: {
            this->d->m_playing = false;

            this->d->m_mutex.lock();
            auto vcam = this->d->m_vcam;
            this->d->m_mutex.unlock();

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
            this->d->m_playing = false;

            this->d->m_mutex.lock();
            auto vcam = this->d->m_vcam;
            this->d->m_mutex.unlock();

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

    AkVideoCaps videoCaps;
    QString rootMethod;
    QString picture;

    if (this->m_vcam) {
        videoCaps = this->m_vcam->currentCaps();
        picture = this->m_vcam->picture();
        rootMethod = this->m_vcam->rootMethod();
    }

    this->m_mutex.lock();
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

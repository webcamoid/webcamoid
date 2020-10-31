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
#include <QSharedPointer>
#include <QMutex>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideopacket.h>

#include "virtualcameraelement.h"
#include "virtualcameraelementsettings.h"
#include "vcam.h"

#define MAX_CAMERAS 64
#define PREFERRED_ROUNDING 32

template<typename T>
inline QSharedPointer<T> ptr_cast(QObject *obj=nullptr)
{
    return QSharedPointer<T>(static_cast<T *>(obj));
}

class VirtualCameraElementPrivate
{
    public:
        VirtualCameraElement *self;
        VirtualCameraElementSettings m_settings;
        VCamPtr m_vcam;
        QMutex m_mutex;
        int m_streamIndex {-1};
        bool m_playing {false};

        explicit VirtualCameraElementPrivate(VirtualCameraElement *self);
        static inline int roundTo(int value, int n);
        void outputLibUpdated(const QString &outputLib);
        void rootMethodUpdated(const QString &rootMethod);
};

VirtualCameraElement::VirtualCameraElement():
    AkElement()
{
    this->d = new VirtualCameraElementPrivate(this);
    QObject::connect(&this->d->m_settings,
                     &VirtualCameraElementSettings::outputLibChanged,
                     [this] (const QString &outputLib) {
                        this->d->outputLibUpdated(outputLib);
                     });
    QObject::connect(&this->d->m_settings,
                     &VirtualCameraElementSettings::rootMethodChanged,
                     [this] (const QString &rootMethod) {
                        this->d->rootMethodUpdated(rootMethod);
                     });
    this->d->outputLibUpdated(this->d->m_settings.outputLib());
    this->d->rootMethodUpdated(this->d->m_settings.rootMethod());
}

VirtualCameraElement::~VirtualCameraElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QString VirtualCameraElement::error() const
{
    if (!this->d->m_vcam)
        return {};

    return this->d->m_vcam->error();
}

QStringList VirtualCameraElement::medias() const
{
    if (!this->d->m_vcam)
        return {};

    return this->d->m_vcam->webcams();
}

QString VirtualCameraElement::media() const
{
    if (!this->d->m_vcam)
        return {};

    return this->d->m_vcam->device();
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
    if (!this->d->m_vcam)
        return {};

    return this->d->m_vcam->supportedOutputPixelFormats();
}

AkVideoCaps::PixelFormat VirtualCameraElement::defaultOutputPixelFormat() const
{
    if (!this->d->m_vcam)
        return AkVideoCaps::Format_none;

    return this->d->m_vcam->defaultOutputPixelFormat();
}

int VirtualCameraElement::defaultStream(const QString &mimeType) const
{
    if (mimeType == "video/x-raw")
        return 0;

    return -1;
}

QString VirtualCameraElement::description(const QString &media) const
{
    if (!this->d->m_vcam)
        return {};

    return this->d->m_vcam->description(media);
}

AkCaps VirtualCameraElement::caps(int stream) const
{
    if (stream != 0 || !this->d->m_vcam)
        return {};

    return this->d->m_vcam->currentCaps();
}

AkVideoCapsList VirtualCameraElement::outputCaps(const QString &webcam) const
{
    if (!this->d->m_vcam)
        return {};

    return this->d->m_vcam->caps(webcam);
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

    if (this->d->m_vcam)
        this->d->m_vcam->setCurrentCaps(videoCaps);

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

    if (this->d->m_vcam)
        this->d->m_vcam->setCurrentCaps(videoCaps);

    QVariantMap outputParams {
        {"caps", QVariant::fromValue(streamCaps)}
    };

    return outputParams;
}

QString VirtualCameraElement::createWebcam(const QString &description,
                                           const AkVideoCapsList &formats)
{
    if (!this->d->m_vcam)
        emit this->errorChanged("Invalid submodule");

    auto webcam = this->d->m_vcam->deviceCreate(description, formats);

    if (webcam.isEmpty()) {
        auto error = this->d->m_vcam->error();
        emit this->errorChanged(error);

        return {};
    }

    emit this->mediasChanged(this->medias());

    return webcam;
}

bool VirtualCameraElement::editWebcam(const QString &webcam,
                                      const QString &description,
                                      const AkVideoCapsList &formats)
{
    if (!this->d->m_vcam)
        return {};

    bool ok = this->d->m_vcam->deviceEdit(webcam, description, formats);

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

bool VirtualCameraElement::changeDescription(const QString &webcam,
                                             const QString &description)
{
    if (!this->d->m_vcam)
        return {};

    bool ok = this->d->m_vcam->changeDescription(webcam, description);

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

bool VirtualCameraElement::removeWebcam(const QString &webcam)
{
    if (!this->d->m_vcam)
        return {};

    bool ok = this->d->m_vcam->deviceDestroy(webcam);

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

bool VirtualCameraElement::removeAllWebcams()
{
    if (!this->d->m_vcam)
        return {};

    bool ok = this->d->m_vcam->destroyAllDevices();

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

QVariantList VirtualCameraElement::controls() const
{
    if (!this->d->m_vcam)
        return {};

    return this->d->m_vcam->controls();
}

bool VirtualCameraElement::setControls(const QVariantMap &controls)
{
    if (!this->d->m_vcam)
        return false;

    return this->d->m_vcam->setControls(controls);
}

bool VirtualCameraElement::resetControls()
{
    if (!this->d->m_vcam)
        return false;

    return true;
}

QList<quint64> VirtualCameraElement::clientsPids() const
{
    if (!this->d->m_vcam)
        return {};

    return this->d->m_vcam->clientsPids();
}

QString VirtualCameraElement::clientExe(quint64 pid) const
{
    if (!this->d->m_vcam)
        return {};

    return this->d->m_vcam->clientExe(pid);
}

bool VirtualCameraElement::driverInstalled() const
{
    if (!this->d->m_vcam)
        return false;

    return this->d->m_vcam->isInstalled();
}

QString VirtualCameraElement::picture() const
{
    if (!this->d->m_vcam)
        return {};

    return this->d->m_vcam->picture();
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
    this->d->m_mutex.lock();

    if (this->state() == AkElement::ElementStatePlaying) {
        auto videoPacket = packet.convert(AkVideoCaps::Format_rgb24, 32);

        if (this->d->m_vcam)
            this->d->m_vcam->write(videoPacket);
    }

    this->d->m_mutex.unlock();

    akSend(packet)
}

bool VirtualCameraElement::applyPicture()
{
    if (!this->d->m_vcam)
        return false;

    return this->d->m_vcam->applyPicture();
}

void VirtualCameraElement::setMedia(const QString &media)
{
    if (this->d->m_vcam)
        this->d->m_vcam->setDevice(media);
}

void VirtualCameraElement::setPicture(const QString &picture)
{
    if (this->d->m_vcam)
        this->d->m_vcam->setPicture(picture);
}

void VirtualCameraElement::resetMedia()
{
    if (this->d->m_vcam)
        this->d->m_vcam->resetPicture();
}

void VirtualCameraElement::resetPicture()
{
    if (this->d->m_vcam)
        this->d->m_vcam->setPicture(":/VirtualCamera/share/TestFrame/TestFrame.bmp");
}

void VirtualCameraElement::clearStreams()
{
    this->d->m_streamIndex = -1;

    if (this->d->m_vcam)
        this->d->m_vcam->resetCurrentCaps();
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

            this->d->m_mutex.lock();

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

            this->d->m_mutex.lock();

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
}

int VirtualCameraElementPrivate::roundTo(int value, int n)
{
    return n * qRound(value / qreal(n));
}

void VirtualCameraElementPrivate::outputLibUpdated(const QString &outputLib)
{
    auto state = self->state();
    self->setState(AkElement::ElementStateNull);
    QString picture;

    if (this->m_vcam)
        picture = this->m_vcam->picture();

    this->m_mutex.lock();
    this->m_vcam =
            ptr_cast<VCam>(VirtualCameraElement::loadSubModule("VirtualCamera",
                                                               outputLib));
    this->m_mutex.unlock();

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

    this->m_vcam->setRootMethod(this->m_settings.rootMethod());
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

void VirtualCameraElementPrivate::rootMethodUpdated(const QString &rootMethod)
{
    if (this->m_vcam)
        this->m_vcam->setRootMethod(rootMethod);
}

#include "moc_virtualcameraelement.cpp"

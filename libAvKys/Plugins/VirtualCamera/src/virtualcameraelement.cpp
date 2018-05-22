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

#include <QImage>
#include <QQmlContext>
#include <QSharedPointer>
#include <QMutex>
#include <akutils.h>
#include <akcaps.h>
#include <akpacket.h>

#include "virtualcameraelement.h"
#include "virtualcameraglobals.h"
#include "convertvideo.h"
#include "cameraout.h"

#if defined(Q_OS_OSX) || defined(Q_OS_WIN32)
    #define PREFERRED_FORMAT AkVideoCaps::Format_rgb24
#else
    #define PREFERRED_FORMAT AkVideoCaps::Format_yuv420p
#endif

Q_GLOBAL_STATIC(VirtualCameraGlobals, globalVirtualCamera)

template<typename T>
inline QSharedPointer<T> ptr_cast(QObject *obj=nullptr)
{
    return QSharedPointer<T>(static_cast<T *>(obj));
}

#define PREFERRED_ROUNDING 32

inline int roundTo(int value, int n)
{
    return n * qRound(value / qreal(n));
}

struct XRGB
{
    quint8 x;
    quint8 r;
    quint8 g;
    quint8 b;
};

struct BGRX
{
    quint8 b;
    quint8 g;
    quint8 r;
    quint8 x;
};

class VirtualCameraElementPrivate
{
    public:
        ConvertVideoPtr m_convertVideo;
        CameraOutPtr m_cameraOut;
        int m_streamIndex;
        AkCaps m_streamCaps;
        QMutex m_mutex;
        QMutex m_mutexLib;

        VirtualCameraElementPrivate():
            m_streamIndex(-1)
        {
        }
};

VirtualCameraElement::VirtualCameraElement():
    AkElement()
{
    this->d = new VirtualCameraElementPrivate;

    QObject::connect(globalVirtualCamera,
                     SIGNAL(convertLibChanged(const QString &)),
                     this,
                     SIGNAL(convertLibChanged(const QString &)));
    QObject::connect(globalVirtualCamera,
                     SIGNAL(convertLibChanged(const QString &)),
                     this,
                     SLOT(convertLibUpdated(const QString &)));
    QObject::connect(globalVirtualCamera,
                     SIGNAL(outputLibChanged(const QString &)),
                     this,
                     SIGNAL(outputLibChanged(const QString &)));
    QObject::connect(globalVirtualCamera,
                     SIGNAL(outputLibChanged(const QString &)),
                     this,
                     SLOT(outputLibUpdated(const QString &)));
    QObject::connect(globalVirtualCamera,
                     SIGNAL(rootMethodChanged(const QString &)),
                     this,
                     SLOT(rootMethodUpdated(const QString &)));

    this->convertLibUpdated(globalVirtualCamera->convertLib());
    this->outputLibUpdated(globalVirtualCamera->outputLib());
    this->rootMethodUpdated(globalVirtualCamera->rootMethod());
}

VirtualCameraElement::~VirtualCameraElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QString VirtualCameraElement::driverPath() const
{
    if (!this->d->m_cameraOut)
        return {};

    return this->d->m_cameraOut->driverPath();
}

QStringList VirtualCameraElement::medias() const
{
    if (!this->d->m_cameraOut)
        return {};

    return this->d->m_cameraOut->webcams();
}

QString VirtualCameraElement::media() const
{
    if (!this->d->m_cameraOut)
        return {};

    return this->d->m_cameraOut->device();
}

QList<int> VirtualCameraElement::streams() const
{
    QList<int> streams;
    streams << 0;

    return streams;
}

int VirtualCameraElement::maxCameras() const
{
    if (!this->d->m_cameraOut)
        return 0;

    return this->d->m_cameraOut->maxCameras();
}

bool VirtualCameraElement::needRoot() const
{
    if (!this->d->m_cameraOut)
        return false;

    return this->d->m_cameraOut->needRoot();
}

int VirtualCameraElement::passwordTimeout() const
{
    if (!this->d->m_cameraOut)
        return 0;

    return this->d->m_cameraOut->passwordTimeout();
}

QString VirtualCameraElement::rootMethod() const
{
    return globalVirtualCamera->rootMethod();
}

QStringList VirtualCameraElement::availableMethods() const
{
    return globalVirtualCamera->availableMethods();
}

QString VirtualCameraElement::convertLib() const
{
    return globalVirtualCamera->convertLib();
}

QString VirtualCameraElement::outputLib() const
{
    return globalVirtualCamera->outputLib();
}

int VirtualCameraElement::defaultStream(const QString &mimeType) const
{
    if (mimeType == "video/x-raw")
        return 0;

    return -1;
}

QString VirtualCameraElement::description(const QString &media) const
{
    if (!this->d->m_cameraOut)
        return {};

    return this->d->m_cameraOut->description(media);
}

AkCaps VirtualCameraElement::caps(int stream) const
{
    if (stream != 0)
        return AkCaps();

    return this->d->m_streamCaps;
}

QVariantMap VirtualCameraElement::addStream(int streamIndex,
                                            const AkCaps &streamCaps,
                                            const QVariantMap &streamParams)
{
    Q_UNUSED(streamParams)

    if (!this->d->m_cameraOut)
        return {};

    AkVideoCaps videoCaps(streamCaps);
    videoCaps.format() = PREFERRED_FORMAT;
    videoCaps.bpp() = AkVideoCaps::bitsPerPixel(PREFERRED_FORMAT);
    videoCaps.width() = roundTo(videoCaps.width(), PREFERRED_ROUNDING);
    videoCaps.height() = roundTo(videoCaps.height(), PREFERRED_ROUNDING);

    this->d->m_streamIndex = streamIndex;
    this->d->m_streamCaps = videoCaps.toCaps();
    this->d->m_cameraOut->setCaps(this->d->m_streamCaps);

    return QVariantMap();
}

QVariantMap VirtualCameraElement::updateStream(int streamIndex,
                                               const QVariantMap &streamParams)
{
    Q_UNUSED(streamParams)
    this->d->m_streamIndex = streamIndex;

    return QVariantMap();
}

QString VirtualCameraElement::createWebcam(const QString &description,
                                           const QString &password)
{
    if (!this->d->m_cameraOut)
        return {};

    return this->d->m_cameraOut->createWebcam(description, password);
}

bool VirtualCameraElement::changeDescription(const QString &webcam,
                                             const QString &description,
                                             const QString &password) const
{
    if (!this->d->m_cameraOut)
        return false;

    return this->d->m_cameraOut->changeDescription(webcam, description, password);
}

bool VirtualCameraElement::removeWebcam(const QString &webcam,
                                        const QString &password)
{
    if (!this->d->m_cameraOut)
        return false;

    return this->d->m_cameraOut->removeWebcam(webcam, password);
}

bool VirtualCameraElement::removeAllWebcams(const QString &password)
{
    if (!this->d->m_cameraOut)
        return false;

    return this->d->m_cameraOut->removeAllWebcams(password);
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

    context->setContextProperty("VirtualCamera", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", controlId);

#ifdef Q_OS_LINUX
    context->setContextProperty("OsName", "linux");
#elif defined(Q_OS_OSX)
    context->setContextProperty("OsName", "mac");
#elif defined(Q_OS_WIN32)
    context->setContextProperty("OsName", "windows");
#else
    context->setContextProperty("OsName", "");
#endif
}

void VirtualCameraElement::setDriverPath(const QString &driverPath)
{
    if (!this->d->m_cameraOut
        || this->d->m_cameraOut->driverPath() == driverPath)
        return;

    this->d->m_cameraOut->setDriverPath(driverPath);
    emit this->driverPathChanged(driverPath);
}

void VirtualCameraElement::setMedia(const QString &media)
{
    if (!this->d->m_cameraOut
        || this->d->m_cameraOut->device() == media)
        return;

    this->d->m_cameraOut->setDevice(media);
    emit this->mediaChanged(media);
}

void VirtualCameraElement::setPasswordTimeout(int passwordTimeout)
{
    if (this->d->m_cameraOut)
        this->d->m_cameraOut->setPasswordTimeout(passwordTimeout);
}

void VirtualCameraElement::setRootMethod(const QString &rootMethod)
{
    globalVirtualCamera->setRootMethod(rootMethod);
}

void VirtualCameraElement::setConvertLib(const QString &convertLib)
{
    globalVirtualCamera->setConvertLib(convertLib);
}

void VirtualCameraElement::setOutputLib(const QString &outputLib)
{
    globalVirtualCamera->setOutputLib(outputLib);
}

void VirtualCameraElement::resetDriverPath()
{
    if (this->d->m_cameraOut)
        this->d->m_cameraOut->resetDriverPath();
}

void VirtualCameraElement::resetMedia()
{
    if (!this->d->m_cameraOut)
        return;

    QString media = this->d->m_cameraOut->device();
    this->d->m_cameraOut->resetDevice();

    if (media != this->d->m_cameraOut->device())
        emit this->mediaChanged(this->d->m_cameraOut->device());
}

void VirtualCameraElement::resetPasswordTimeout()
{
    if (this->d->m_cameraOut)
        this->d->m_cameraOut->resetPasswordTimeout();
}

void VirtualCameraElement::resetRootMethod()
{
    globalVirtualCamera->resetRootMethod();
}

void VirtualCameraElement::resetConvertLib()
{
    globalVirtualCamera->resetConvertLib();
}

void VirtualCameraElement::resetOutputLib()
{
    globalVirtualCamera->resetOutputLib();
}

void VirtualCameraElement::clearStreams()
{
    this->d->m_streamIndex = -1;
    this->d->m_streamCaps = AkCaps();
}

bool VirtualCameraElement::setState(AkElement::ElementState state)
{
    if (!this->d->m_convertVideo || !this->d->m_cameraOut)
        return false;

    AkElement::ElementState curState = this->state();
    QMutexLocker locker(&this->d->m_mutexLib);

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
        case AkElement::ElementStatePlaying: {
            this->d->m_mutex.lock();
            QString device = this->d->m_cameraOut->device();

            if (device.isEmpty()) {
                QStringList webcams = this->d->m_cameraOut->webcams();

                if (webcams.isEmpty()) {
                    this->d->m_mutex.unlock();

                    return false;
                }

                this->d->m_cameraOut->setDevice(webcams.at(0));
            }

            if (!this->d->m_cameraOut->init(this->d->m_streamIndex)) {
                this->d->m_mutex.unlock();

                return false;
            }

            this->d->m_mutex.unlock();

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
            this->d->m_mutex.lock();
            this->d->m_cameraOut->uninit();
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
            this->d->m_mutex.lock();
            this->d->m_cameraOut->uninit();
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

AkPacket VirtualCameraElement::iStream(const AkPacket &packet)
{
    if (!this->d->m_convertVideo || !this->d->m_cameraOut)
        return AkPacket();

    this->d->m_mutex.lock();

    if (this->state() == AkElement::ElementStatePlaying) {
        QImage image = AkUtils::packetToImage(packet);
        image = image.convertToFormat(QImage::Format_RGB32);
        AkPacket oPacket;

#if defined(Q_OS_OSX) || defined(Q_OS_WIN32)
        image = image.convertToFormat(QImage::Format_RGB888);
        oPacket = AkUtils::roundSizeTo(AkUtils::imageToPacket(image, packet),
                                       PREFERRED_ROUNDING);
#else
        image = this->d->swapChannels(image);

        this->d->m_mutexLib.lock();
        oPacket = this->d->m_convertVideo->convert(AkUtils::imageToPacket(image, packet),
                                                   this->d->m_cameraOut->caps());
        this->d->m_mutexLib.unlock();
#endif

        this->d->m_mutexLib.lock();
        this->d->m_cameraOut->writeFrame(oPacket);
        this->d->m_mutexLib.unlock();
    }

    this->d->m_mutex.unlock();

    akSend(packet)
}

void VirtualCameraElement::convertLibUpdated(const QString &convertLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    this->d->m_mutexLib.lock();

    this->d->m_convertVideo =
            ptr_cast<ConvertVideo>(this->loadSubModule("VirtualCamera", convertLib));

    this->d->m_mutexLib.unlock();

    this->setState(state);
}

void VirtualCameraElement::outputLibUpdated(const QString &outputLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    this->d->m_mutexLib.lock();

    this->d->m_cameraOut =
            ptr_cast<CameraOut>(this->loadSubModule("VirtualCamera", outputLib));

    if (!this->d->m_cameraOut) {
        this->d->m_mutexLib.unlock();

        return;
    }

    QObject::connect(this->d->m_cameraOut.data(),
                     &CameraOut::driverPathChanged,
                     this,
                     &VirtualCameraElement::driverPathChanged);
    QObject::connect(this->d->m_cameraOut.data(),
                     &CameraOut::error,
                     this,
                     &VirtualCameraElement::error);
    QObject::connect(this->d->m_cameraOut.data(),
                     &CameraOut::webcamsChanged,
                     this,
                     &VirtualCameraElement::mediasChanged);
    QObject::connect(this->d->m_cameraOut.data(),
                     &CameraOut::passwordTimeoutChanged,
                     this,
                     &VirtualCameraElement::passwordTimeoutChanged);

    this->d->m_mutexLib.unlock();

    emit this->driverPathChanged(this->driverPath());
    emit this->mediasChanged(this->medias());
    emit this->mediaChanged(this->media());
    emit this->streamsChanged(this->streams());
    emit this->needRootChanged(this->needRoot());
    emit this->passwordTimeoutChanged(this->passwordTimeout());
    emit this->rootMethodChanged(this->rootMethod());

    this->setState(state);
}

void VirtualCameraElement::rootMethodUpdated(const QString &rootMethod)
{
    if (this->d->m_cameraOut)
        this->d->m_cameraOut->setRootMethod(rootMethod);
}

#include "moc_virtualcameraelement.cpp"

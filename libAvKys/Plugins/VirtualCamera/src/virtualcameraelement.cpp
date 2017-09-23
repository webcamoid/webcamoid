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

#include <akutils.h>

#include "virtualcameraelement.h"
#include "virtualcameraglobals.h"

#ifdef Q_OS_WIN32
    #define PREFERRED_FORMAT AkVideoCaps::Format_0rgb
#elif defined(Q_OS_OSX)
    #define PREFERRED_FORMAT AkVideoCaps::Format_argb
#else
    #define PREFERRED_FORMAT AkVideoCaps::Format_yuv420p
#endif

Q_GLOBAL_STATIC(VirtualCameraGlobals, globalVirtualCamera)

template<typename T>
inline QSharedPointer<T> ptr_init(QObject *obj=nullptr)
{
    if (!obj)
        return QSharedPointer<T>(new T());

    return QSharedPointer<T>(static_cast<T *>(obj));
}

#define PREFERRED_ROUNDING 4

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

VirtualCameraElement::VirtualCameraElement():
    AkElement(),
    m_convertVideo(ptr_init<ConvertVideo>()),
    m_cameraOut(ptr_init<CameraOut>())
{
    this->m_streamIndex = -1;

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
}

QString VirtualCameraElement::driverPath() const
{
    return this->m_cameraOut->driverPath();
}

QStringList VirtualCameraElement::medias() const
{
    return this->m_cameraOut->webcams();
}

QString VirtualCameraElement::media() const
{
    return this->m_cameraOut->device();
}

QList<int> VirtualCameraElement::streams() const
{
    QList<int> streams;
    streams << 0;

    return streams;
}

int VirtualCameraElement::maxCameras() const
{
    return this->m_cameraOut->maxCameras();
}

bool VirtualCameraElement::needRoot() const
{
    return this->m_cameraOut->needRoot();
}

int VirtualCameraElement::passwordTimeout() const
{
    return this->m_cameraOut->passwordTimeout();
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
    return this->m_cameraOut->description(media);
}

AkCaps VirtualCameraElement::caps(int stream) const
{
    if (stream != 0)
        return AkCaps();

    return this->m_streamCaps;
}

QVariantMap VirtualCameraElement::addStream(int streamIndex,
                                            const AkCaps &streamCaps,
                                            const QVariantMap &streamParams)
{
    Q_UNUSED(streamParams)

    AkVideoCaps videoCaps(streamCaps);
    videoCaps.format() = PREFERRED_FORMAT;
    videoCaps.bpp() = AkVideoCaps::bitsPerPixel(PREFERRED_FORMAT);
    videoCaps.width() = roundTo(videoCaps.width(), PREFERRED_ROUNDING);
    videoCaps.height() = roundTo(videoCaps.height(), PREFERRED_ROUNDING);

    this->m_streamIndex = streamIndex;
    this->m_streamCaps = videoCaps.toCaps();

    return QVariantMap();
}

QVariantMap VirtualCameraElement::updateStream(int streamIndex,
                                               const QVariantMap &streamParams)
{
    Q_UNUSED(streamParams)
    this->m_streamIndex = streamIndex;

    return QVariantMap();
}

QString VirtualCameraElement::createWebcam(const QString &description,
                                           const QString &password)
{
    return this->m_cameraOut->createWebcam(description, password);
}

bool VirtualCameraElement::changeDescription(const QString &webcam,
                                             const QString &description,
                                             const QString &password) const
{
    return this->m_cameraOut->changeDescription(webcam, description, password);
}

bool VirtualCameraElement::removeWebcam(const QString &webcam,
                                        const QString &password)
{
    return this->m_cameraOut->removeWebcam(webcam, password);
}

bool VirtualCameraElement::removeAllWebcams(const QString &password)
{
    return this->m_cameraOut->removeAllWebcams(password);
}

QImage VirtualCameraElement::swapChannels(const QImage &image) const
{
    QImage swapped(image.size(), image.format());

    for (int y = 0; y < image.height(); y++) {
        const XRGB *src = reinterpret_cast<const XRGB *>(image.constScanLine(y));
        BGRX *dst = reinterpret_cast<BGRX *>(swapped.scanLine(y));

        for (int x = 0; x < image.width(); x++) {
            dst[x].x = src[x].x;
            dst[x].r = src[x].r;
            dst[x].g = src[x].g;
            dst[x].b = src[x].b;
        }
    }

    return swapped;
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
    if (this->m_cameraOut->driverPath() == driverPath)
        return;

    this->m_cameraOut->setDriverPath(driverPath);
    emit this->driverPathChanged(driverPath);
}

void VirtualCameraElement::setMedia(const QString &media)
{
    if (this->m_cameraOut->device() == media)
        return;

    this->m_cameraOut->setDevice(media);
    emit this->mediaChanged(media);
}

void VirtualCameraElement::setPasswordTimeout(int passwordTimeout)
{
    this->m_cameraOut->setPasswordTimeout(passwordTimeout);
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
    this->m_cameraOut->resetDriverPath();
}

void VirtualCameraElement::resetMedia()
{
    QString media = this->m_cameraOut->device();
    this->m_cameraOut->resetDevice();

    if (media != this->m_cameraOut->device())
        emit this->mediaChanged(this->m_cameraOut->device());
}

void VirtualCameraElement::resetPasswordTimeout()
{
    this->m_cameraOut->resetPasswordTimeout();
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
    this->m_streamIndex = -1;
    this->m_streamCaps = AkCaps();
}

bool VirtualCameraElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();
    QMutexLocker locker(&this->m_mutexLib);

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
        case AkElement::ElementStatePlaying: {
            this->m_mutex.lock();
            QString device = this->m_cameraOut->device();

            if (device.isEmpty()) {
                QStringList webcams = this->m_cameraOut->webcams();

                if (webcams.isEmpty()) {
                    this->m_mutex.unlock();

                    return false;
                }

                this->m_cameraOut->setDevice(webcams.at(0));
            }

            if (!this->m_cameraOut->init(this->m_streamIndex,
                                         this->m_streamCaps)) {
                this->m_mutex.unlock();

                return false;
            }

            this->m_mutex.unlock();

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
            this->m_mutex.lock();
            this->m_cameraOut->uninit();
            this->m_mutex.unlock();

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
            this->m_mutex.lock();
            this->m_cameraOut->uninit();
            this->m_mutex.unlock();

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
    this->m_mutex.lock();

    if (this->state() == AkElement::ElementStatePlaying) {
        QImage image = AkUtils::packetToImage(packet);
        image = image.convertToFormat(QImage::Format_RGB32);
        AkPacket oPacket;

#ifdef Q_OS_WIN32
        oPacket = AkUtils::roundSizeTo(AkUtils::imageToPacket(image, packet),
                                       PREFERRED_ROUNDING);
#elif defined(Q_OS_OSX)
        oPacket = packet;
#else
        image = this->swapChannels(image);

        this->m_mutexLib.lock();
        oPacket = this->m_convertVideo->convert(AkUtils::imageToPacket(image, packet),
                                                this->m_cameraOut->caps());
        this->m_mutexLib.unlock();
#endif

        this->m_mutexLib.lock();
        this->m_cameraOut->writeFrame(oPacket);
        this->m_mutexLib.unlock();
    }

    this->m_mutex.unlock();

    akSend(packet)
}

void VirtualCameraElement::convertLibUpdated(const QString &convertLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    this->m_mutexLib.lock();

    this->m_convertVideo =
            ptr_init<ConvertVideo>(this->loadSubModule("VirtualCamera", convertLib));

    this->m_mutexLib.unlock();

    this->setState(state);
}

void VirtualCameraElement::outputLibUpdated(const QString &outputLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    this->m_mutexLib.lock();

    this->m_cameraOut =
            ptr_init<CameraOut>(this->loadSubModule("VirtualCamera", outputLib));

    QObject::connect(this->m_cameraOut.data(),
                     &CameraOut::driverPathChanged,
                     this,
                     &VirtualCameraElement::driverPathChanged);
    QObject::connect(this->m_cameraOut.data(),
                     &CameraOut::error,
                     this,
                     &VirtualCameraElement::error);
    QObject::connect(this->m_cameraOut.data(),
                     &CameraOut::webcamsChanged,
                     this,
                     &VirtualCameraElement::mediasChanged);
    QObject::connect(this->m_cameraOut.data(),
                     &CameraOut::passwordTimeoutChanged,
                     this,
                     &VirtualCameraElement::passwordTimeoutChanged);

    this->m_mutexLib.unlock();

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
    this->m_cameraOut->setRootMethod(rootMethod);
}

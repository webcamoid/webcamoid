/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#ifdef Q_OS_WIN32
    #define PREFERRED_FORMAT AkVideoCaps::Format_0rgb
#else
    #define PREFERRED_FORMAT AkVideoCaps::Format_yuv420p
#endif

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
    AkElement()
{
    this->m_streamIndex = -1;
    this->m_isRunning = false;

    QObject::connect(&this->m_cameraOut,
                     &CameraOut::driverPathChanged,
                     this,
                     &VirtualCameraElement::driverPathChanged);
    QObject::connect(&this->m_cameraOut,
                     &CameraOut::error,
                     this,
                     &VirtualCameraElement::error);
    QObject::connect(&this->m_cameraOut,
                     &CameraOut::webcamsChanged,
                     this,
                     &VirtualCameraElement::mediasChanged);
    QObject::connect(&this->m_cameraOut,
                     &CameraOut::passwordTimeoutChanged,
                     this,
                     &VirtualCameraElement::passwordTimeoutChanged);
}

VirtualCameraElement::~VirtualCameraElement()
{
    this->setState(AkElement::ElementStateNull);
}

QObject *VirtualCameraElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/VirtualCamera/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("VirtualCamera", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", controlId);

#ifdef Q_OS_LINUX
    context->setContextProperty("OsName", "linux");
#elif defined(Q_OS_WIN32)
    context->setContextProperty("OsName", "windows");
#else
    context->setContextProperty("OsName", "");
#endif

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

QString VirtualCameraElement::driverPath() const
{
    return this->m_cameraOut.driverPath();
}

QStringList VirtualCameraElement::medias() const
{
    return this->m_cameraOut.webcams();
}

QString VirtualCameraElement::media() const
{
    return this->m_cameraOut.device();
}

QList<int> VirtualCameraElement::streams() const
{
    QList<int> streams;
    streams << 0;

    return streams;
}

int VirtualCameraElement::maxCameras() const
{
    return this->m_cameraOut.maxCameras();
}

bool VirtualCameraElement::needRoot() const
{
    return this->m_cameraOut.needRoot();
}

int VirtualCameraElement::passwordTimeout() const
{
    return this->m_cameraOut.passwordTimeout();
}

int VirtualCameraElement::defaultStream(const QString &mimeType) const
{
    if (mimeType == "video/x-raw")
        return 0;

    return -1;
}

QString VirtualCameraElement::description(const QString &media) const
{
    return this->m_cameraOut.description(media);
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
    videoCaps.width() = roundTo(videoCaps.width(), PREFERRED_ROUNDING);
    videoCaps.height() = roundTo(videoCaps.height(), PREFERRED_ROUNDING);

    this->m_streamIndex = streamIndex;
    this->m_streamCaps = streamCaps;

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
                                           const QString &password) const
{
    return this->m_cameraOut.createWebcam(description, password);
}

bool VirtualCameraElement::changeDescription(const QString &webcam,
                                             const QString &description,
                                             const QString &password) const
{
    return this->m_cameraOut.changeDescription(webcam, description, password);
}

bool VirtualCameraElement::removeWebcam(const QString &webcam,
                                        const QString &password) const
{
    return this->m_cameraOut.removeWebcam(webcam, password);
}

bool VirtualCameraElement::removeAllWebcams(const QString &password) const
{
    return this->m_cameraOut.removeAllWebcams(password);
}

void VirtualCameraElement::stateChange(AkElement::ElementState from,
                                       AkElement::ElementState to)
{
    this->m_mutex.lock();

    if (from == AkElement::ElementStateNull
        && to == AkElement::ElementStatePaused) {
        QString device = this->m_cameraOut.device();

        if (device.isEmpty()) {
            QStringList webcams = this->m_cameraOut.webcams();

            if (webcams.isEmpty()) {
                this->m_mutex.unlock();

                return;
            }

            this->m_cameraOut.setDevice(webcams.at(0));
        }

        this->m_isRunning = this->m_cameraOut.init(this->m_streamIndex, this->m_streamCaps);
    } else if (from == AkElement::ElementStatePaused
               && to == AkElement::ElementStateNull) {
        this->m_isRunning = false;
        this->m_cameraOut.uninit();
    }

    this->m_mutex.unlock();
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

void VirtualCameraElement::setDriverPath(const QString &driverPath)
{
    if (this->m_cameraOut.driverPath() == driverPath)
        return;

    this->m_cameraOut.setDriverPath(driverPath);
    emit this->driverPathChanged(driverPath);
}

void VirtualCameraElement::setMedia(const QString &media)
{
    if (this->m_cameraOut.device() == media)
        return;

    this->m_cameraOut.setDevice(media);
    emit this->mediaChanged(media);
}

void VirtualCameraElement::setPasswordTimeout(int passwordTimeout)
{
    this->m_cameraOut.setPasswordTimeout(passwordTimeout);
}

void VirtualCameraElement::resetDriverPath()
{
    this->m_cameraOut.resetDriverPath();
}

void VirtualCameraElement::resetMedia()
{
    QString media = this->m_cameraOut.device();
    this->m_cameraOut.resetDevice();

    if (media != this->m_cameraOut.device())
        emit this->mediaChanged(this->m_cameraOut.device());
}

void VirtualCameraElement::resetPasswordTimeout()
{
    this->m_cameraOut.resetPasswordTimeout();
}

void VirtualCameraElement::clearStreams()
{
    this->m_streamIndex = -1;
    this->m_streamCaps = AkCaps();
}

AkPacket VirtualCameraElement::iStream(const AkPacket &packet)
{
    this->m_mutex.lock();

    if (this->m_isRunning) {
        AkPacket videoPacket(packet);
        QImage image = AkUtils::packetToImage(videoPacket);
        image = image.convertToFormat(QImage::Format_RGB32);
        AkPacket oPacket;

#ifdef Q_OS_WIN32
        oPacket = AkUtils::roundSizeTo(AkUtils::imageToPacket(image, packet),
                                       PREFERRED_ROUNDING);
#else
        image = this->swapChannels(image);
        videoPacket = AkUtils::imageToPacket(image, packet);

        oPacket = this->m_convertVideo.convert(videoPacket,
                                               this->m_cameraOut.caps());
#endif

        this->m_cameraOut.writeFrame(oPacket);
    }

    this->m_mutex.unlock();

    akSend(packet)
}

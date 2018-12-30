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
#include <QAbstractEventDispatcher>
#include <QtConcurrent>
#include <QThreadPool>
#include <QFuture>
#include <QMutex>
#include <akcaps.h>
#include <akvideopacket.h>

#include "videocaptureelement.h"
#include "videocaptureglobals.h"
#include "convertvideo.h"
#include "capture.h"

#define PAUSE_TIMEOUT 500

#ifdef Q_OS_WIN32
#include <combaseapi.h>

inline const QStringList *mirrorFormats()
{
    static const QStringList mirrorFormats = {
        "RGB3",
        "RGB4",
        "RGBP",
        "RGBO",
        "BGR0"
    };

    return &mirrorFormats;
}

inline const QStringList *swapRgbFormats()
{
    static const QStringList swapRgbFormats = {
        "RGB3",
        "YV12"
    };

    return &swapRgbFormats;
}
#endif

Q_GLOBAL_STATIC(VideoCaptureGlobals, globalVideoCapture)

template<typename T>
inline QSharedPointer<T> ptr_cast(QObject *obj=nullptr)
{
    return QSharedPointer<T>(static_cast<T *>(obj));
}

template <typename T>
inline void waitLoop(const QFuture<T> &loop)
{
    while (!loop.isFinished()) {
        auto eventDispatcher = QThread::currentThread()->eventDispatcher();

        if (eventDispatcher)
            eventDispatcher->processEvents(QEventLoop::AllEvents);
    }
}

class VideoCaptureElementPrivate
{
    public:
        ConvertVideoPtr m_convertVideo;
        CapturePtr m_capture;
        QThreadPool m_threadPool;
        QFuture<void> m_cameraLoopResult;
        QMutex m_mutexLib;
        bool m_runCameraLoop {false};
        bool m_pause {false};
        bool m_mirror {false};
        bool m_swapRgb {false};

        void cameraLoop();
};

VideoCaptureElement::VideoCaptureElement():
    AkMultimediaSourceElement()
{
    this->d = new VideoCaptureElementPrivate;

    QObject::connect(globalVideoCapture,
                     SIGNAL(codecLibChanged(const QString &)),
                     this,
                     SIGNAL(codecLibChanged(const QString &)));
    QObject::connect(globalVideoCapture,
                     SIGNAL(codecLibChanged(const QString &)),
                     this,
                     SLOT(codecLibUpdated(const QString &)));
    QObject::connect(globalVideoCapture,
                     SIGNAL(captureLibChanged(const QString &)),
                     this,
                     SIGNAL(captureLibChanged(const QString &)));
    QObject::connect(globalVideoCapture,
                     SIGNAL(captureLibChanged(const QString &)),
                     this,
                     SLOT(captureLibUpdated(const QString &)));

    this->codecLibUpdated(globalVideoCapture->codecLib());
    this->captureLibUpdated(globalVideoCapture->captureLib());
}

VideoCaptureElement::~VideoCaptureElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QStringList VideoCaptureElement::medias()
{
    if (!this->d->m_capture)
        return {};

    return this->d->m_capture->webcams();
}

QString VideoCaptureElement::media() const
{
    if (!this->d->m_capture)
        return {};

    return this->d->m_capture->device();
}

QList<int> VideoCaptureElement::streams()
{
    if (!this->d->m_capture)
        return {};

    return this->d->m_capture->streams();
}

QList<int> VideoCaptureElement::listTracks(const QString &mimeType)
{
    if (!this->d->m_capture)
        return {};

    return this->d->m_capture->listTracks(mimeType);
}

int VideoCaptureElement::defaultStream(const QString &mimeType)
{
    if (mimeType == "video/x-raw")
        return 0;

    return -1;
}

QString VideoCaptureElement::description(const QString &media)
{
    if (!this->d->m_capture)
        return {};

    return this->d->m_capture->description(media);
}

AkCaps VideoCaptureElement::caps(int stream)
{
    if (!this->d->m_capture)
        return AkCaps();

    auto streams = this->d->m_capture->caps(this->d->m_capture->device());
    auto caps = streams.value(stream).value<AkCaps>();

    if (!caps)
        return AkCaps();

    AkVideoCaps videoCaps;
    videoCaps.isValid() = true;
    videoCaps.format() = AkVideoCaps::Format_rgb24;
    videoCaps.bpp() = AkVideoCaps::bitsPerPixel(videoCaps.format());
    videoCaps.width() = caps.property("width").toInt();
    videoCaps.height() = caps.property("height").toInt();
    videoCaps.fps() = caps.property("fps").toString();

    return videoCaps;
}

AkCaps VideoCaptureElement::rawCaps(int stream) const
{
    if (!this->d->m_capture)
        return AkCaps();

    auto streams = this->d->m_capture->caps(this->d->m_capture->device());

    return streams.value(stream).value<AkCaps>();
}

QStringList VideoCaptureElement::listCapsDescription() const
{
    if (!this->d->m_capture)
        return {};

    QStringList capsDescriptions;
    auto streams = this->d->m_capture->caps(this->d->m_capture->device());

    for (const QVariant &caps: streams)
        capsDescriptions << this->d->m_capture->capsDescription(caps.value<AkCaps>());

    return capsDescriptions;
}

QString VideoCaptureElement::ioMethod() const
{
    if (!this->d->m_capture)
        return {};

    return this->d->m_capture->ioMethod();
}

int VideoCaptureElement::nBuffers() const
{
    if (!this->d->m_capture)
        return {};

    return this->d->m_capture->nBuffers();
}

QString VideoCaptureElement::codecLib() const
{
    return globalVideoCapture->codecLib();
}

QString VideoCaptureElement::captureLib() const
{
    return globalVideoCapture->captureLib();
}

QVariantList VideoCaptureElement::imageControls() const
{
    if (!this->d->m_capture)
        return {};

    return this->d->m_capture->imageControls();
}

bool VideoCaptureElement::setImageControls(const QVariantMap &imageControls)
{
    if (!this->d->m_capture)
        return false;

    return this->d->m_capture->setImageControls(imageControls);
}

bool VideoCaptureElement::resetImageControls()
{
    if (!this->d->m_capture)
        return false;

    return this->d->m_capture->resetImageControls();
}

QVariantList VideoCaptureElement::cameraControls() const
{
    if (!this->d->m_capture)
        return {};

    return this->d->m_capture->cameraControls();
}

bool VideoCaptureElement::setCameraControls(const QVariantMap &cameraControls)
{
    if (!this->d->m_capture)
        return false;

    return this->d->m_capture->setCameraControls(cameraControls);
}

bool VideoCaptureElement::resetCameraControls()
{
    if (!this->d->m_capture)
        return false;

    return this->d->m_capture->resetCameraControls();
}

void VideoCaptureElementPrivate::cameraLoop()
{
    if (!this->m_convertVideo || !this->m_capture)
        return;

#ifdef Q_OS_WIN32
    // Initialize the COM library in multithread mode.
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    bool initConvert = true;

    if (this->m_capture->init()) {
        while (this->m_runCameraLoop) {
            if (this->m_pause) {
                QThread::msleep(PAUSE_TIMEOUT);

                continue;
            }

            AkPacket packet = this->m_capture->readFrame();

            if (!packet)
                continue;

            if (initConvert) {
                AkCaps caps = packet.caps();

#ifdef Q_OS_WIN32
                QString fourcc = caps.property("fourcc").toString();
                this->m_mirror = mirrorFormats()->contains(fourcc);
                this->m_swapRgb = swapRgbFormats()->contains(fourcc);
#endif

                if (!this->m_convertVideo->init(caps))
                    break;

                initConvert = false;
            }

            this->m_convertVideo->packetEnqueue(packet);
        }

        this->m_convertVideo->uninit();
        this->m_capture->uninit();
    }

#ifdef Q_OS_WIN32
    // Close COM library.
    CoUninitialize();
#endif
}

QString VideoCaptureElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoCapture/share/qml/main.qml");
}

void VideoCaptureElement::controlInterfaceConfigure(QQmlContext *context,
                                                    const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoCapture", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", controlId);
}

void VideoCaptureElement::setMedia(const QString &media)
{
    if (this->d->m_capture)
        this->d->m_capture->setDevice(media);
}

void VideoCaptureElement::setStreams(const QList<int> &streams)
{
    if (!this->d->m_capture)
        return;

    bool running = this->d->m_runCameraLoop;
    this->setState(AkElement::ElementStateNull);

    this->d->m_capture->setStreams(streams);

    if (running)
        this->setState(AkElement::ElementStatePlaying);
}

void VideoCaptureElement::setIoMethod(const QString &ioMethod)
{
    if (this->d->m_capture)
        this->d->m_capture->setIoMethod(ioMethod);
}

void VideoCaptureElement::setNBuffers(int nBuffers)
{
    if (this->d->m_capture)
        this->d->m_capture->setNBuffers(nBuffers);
}

void VideoCaptureElement::setCodecLib(const QString &codecLib)
{
    globalVideoCapture->setCodecLib(codecLib);
}

void VideoCaptureElement::setCaptureLib(const QString &captureLib)
{
    globalVideoCapture->setCaptureLib(captureLib);
}

void VideoCaptureElement::resetMedia()
{
    if (this->d->m_capture)
        this->d->m_capture->resetDevice();
}

void VideoCaptureElement::resetStreams()
{
    if (this->d->m_capture)
        this->d->m_capture->resetStreams();
}

void VideoCaptureElement::resetIoMethod()
{
    if (this->d->m_capture)
        this->d->m_capture->resetIoMethod();
}

void VideoCaptureElement::resetNBuffers()
{
    if (this->d->m_capture)
        this->d->m_capture->resetNBuffers();
}

void VideoCaptureElement::resetCodecLib()
{
    globalVideoCapture->resetCodecLib();
}

void VideoCaptureElement::resetCaptureLib()
{
    globalVideoCapture->resetCaptureLib();
}

void VideoCaptureElement::reset()
{
    if (this->d->m_capture)
        this->d->m_capture->reset();
}

bool VideoCaptureElement::setState(AkElement::ElementState state)
{
    if (!this->d->m_convertVideo || !this->d->m_capture)
        return false;

    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused: {
            this->d->m_pause = true;
            this->d->m_runCameraLoop = true;
            this->d->m_cameraLoopResult =
                    QtConcurrent::run(&this->d->m_threadPool,
                                      this->d,
                                      &VideoCaptureElementPrivate::cameraLoop);

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePlaying: {
            this->d->m_pause = false;
            this->d->m_runCameraLoop = true;
            this->d->m_cameraLoopResult =
                    QtConcurrent::run(&this->d->m_threadPool,
                                      this->d,
                                      &VideoCaptureElementPrivate::cameraLoop);

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
            this->d->m_pause = false;
            this->d->m_runCameraLoop = false;
            waitLoop(this->d->m_cameraLoopResult);

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->d->m_pause = false;

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull: {
            this->d->m_runCameraLoop = false;
            waitLoop(this->d->m_cameraLoopResult);

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePaused:
            this->d->m_pause = true;

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

void VideoCaptureElement::frameReady(const AkPacket &packet)
{
#ifdef Q_OS_WIN32
    if (this->d->m_mirror || this->d->m_swapRgb) {
        AkVideoPacket videoPacket(packet);
        QImage oImage = videoPacket.toImage();

        if (this->d->m_mirror)
            oImage = oImage.mirrored();

        if (this->d->m_swapRgb)
            oImage = oImage.rgbSwapped();

        emit this->oStream(AkVideoPacket::fromImage(oImage,
                                                    videoPacket).toPacket());
    } else
#endif
        emit this->oStream(packet);
}

void VideoCaptureElement::codecLibUpdated(const QString &codecLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    this->d->m_mutexLib.lock();

    this->d->m_convertVideo =
            ptr_cast<ConvertVideo>(VideoCaptureElement::loadSubModule("VideoCapture",
                                                                      codecLib));

    if (this->d->m_convertVideo)
        QObject::connect(this->d->m_convertVideo.data(),
                         &ConvertVideo::frameReady,
                         this,
                         &VideoCaptureElement::frameReady,
                         Qt::DirectConnection);

    this->d->m_mutexLib.unlock();

    this->setState(state);
}

void VideoCaptureElement::captureLibUpdated(const QString &captureLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    this->d->m_mutexLib.lock();

    this->d->m_capture =
            ptr_cast<Capture>(VideoCaptureElement::loadSubModule("VideoCapture",
                                                                 captureLib));

    this->d->m_mutexLib.unlock();

    if (!this->d->m_capture)
        return;

    QObject::connect(this->d->m_capture.data(),
                     &Capture::error,
                     this,
                     &VideoCaptureElement::error);
    QObject::connect(this->d->m_capture.data(),
                     &Capture::webcamsChanged,
                     this,
                     &VideoCaptureElement::mediasChanged);
    QObject::connect(this->d->m_capture.data(),
                     &Capture::deviceChanged,
                     this,
                     &VideoCaptureElement::mediaChanged);
    QObject::connect(this->d->m_capture.data(),
                     &Capture::imageControlsChanged,
                     this,
                     &VideoCaptureElement::imageControlsChanged);
    QObject::connect(this->d->m_capture.data(),
                     &Capture::cameraControlsChanged,
                     this,
                     &VideoCaptureElement::cameraControlsChanged);
    QObject::connect(this->d->m_capture.data(),
                     &Capture::streamsChanged,
                     this,
                     &VideoCaptureElement::streamsChanged);

    emit this->mediasChanged(this->medias());
    emit this->streamsChanged(this->streams());

    auto medias = this->medias();

    if (!medias.isEmpty())
        this->setMedia(medias.first());

    this->setState(state);
}

#include "moc_videocaptureelement.cpp"

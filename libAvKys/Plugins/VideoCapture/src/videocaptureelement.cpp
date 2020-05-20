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
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akvideopacket.h>

#include "videocaptureelement.h"
#include "videocaptureelementsettings.h"
#include "convertvideo.h"
#include "capture.h"

#define PAUSE_TIMEOUT 500

#ifdef Q_OS_WIN32
#include <combaseapi.h>

inline const QStringList *mirrorFormats()
{
    static const QStringList mirrorFormats {
        "RGB",
        "RGB565",
        "RGB555",
        "BGRX",
    };

    return &mirrorFormats;
}
#endif

#if !defined(Q_OS_OSX)
inline const QStringList *swapRgbFormats()
{
    static const QStringList swapRgbFormats {
#ifdef Q_OS_WIN32
        "RGB",
        "BGRX",
#endif
        "YV12"
    };

    return &swapRgbFormats;
}
#endif

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
        VideoCaptureElement *self;
        VideoCaptureElementSettings settings;
        ConvertVideoPtr m_convertVideo;
        CapturePtr m_capture;
        QThreadPool m_threadPool;
        QFuture<void> m_cameraLoopResult;
        QMutex m_mutexLib;
        bool m_runCameraLoop {false};
        bool m_pause {false};
        bool m_mirror {false};
        bool m_swapRgb {false};

        explicit VideoCaptureElementPrivate(VideoCaptureElement *self);
        void cameraLoop();
        void codecLibUpdated(const QString &codecLib);
        void captureLibUpdated(const QString &captureLib);
        void frameReady(const AkPacket &packet) const;
};

VideoCaptureElement::VideoCaptureElement():
    AkMultimediaSourceElement()
{
    this->d = new VideoCaptureElementPrivate(this);
    QObject::connect(&this->d->settings,
                     &VideoCaptureElementSettings::codecLibChanged,
                     [this] (const QString &codecLib) {
                        this->d->codecLibUpdated(codecLib);
                     });
    QObject::connect(&this->d->settings,
                     &VideoCaptureElementSettings::captureLibChanged,
                     [this] (const QString &captureLib) {
                        this->d->captureLibUpdated(captureLib);
                     });

    this->d->codecLibUpdated(this->d->settings.codecLib());
    this->d->captureLibUpdated(this->d->settings.captureLib());
}

VideoCaptureElement::~VideoCaptureElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QString VideoCaptureElement::error() const
{
    if (!this->d->m_capture)
        return {};

    return this->d->m_capture->error();
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

    return AkVideoCaps(AkVideoCaps::Format_rgb24,
                       caps.property("width").toInt(),
                       caps.property("height").toInt(),
                       caps.property("fps").toString());
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

VideoCaptureElementPrivate::VideoCaptureElementPrivate(VideoCaptureElement *self):
    self(self)
{

}

void VideoCaptureElementPrivate::cameraLoop()
{
    if (!this->m_convertVideo || !this->m_capture)
        return;

#ifdef Q_OS_WIN32
    // Initialize the COM library in multithread mode.
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    if (this->m_capture->init()) {
        bool initConvert = true;

        while (this->m_runCameraLoop) {
            if (this->m_pause) {
                QThread::msleep(PAUSE_TIMEOUT);

                continue;
            }

            auto packet = this->m_capture->readFrame();

            if (!packet)
                continue;

            if (initConvert) {
                AkCaps caps = packet.caps();

#if !defined(Q_OS_OSX)
                QString fourcc = caps.property("fourcc").toString();
#ifdef Q_OS_WIN32
                this->m_mirror = mirrorFormats()->contains(fourcc);
#endif
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

void VideoCaptureElementPrivate::codecLibUpdated(const QString &codecLib)
{
    auto state = self->state();
    self->setState(AkElement::ElementStateNull);

    this->m_mutexLib.lock();

    this->m_convertVideo =
            ptr_cast<ConvertVideo>(VideoCaptureElement::loadSubModule("VideoCapture",
                                                                      codecLib));

    if (this->m_convertVideo)
        QObject::connect(this->m_convertVideo.data(),
                         &ConvertVideo::frameReady,
                         [this] (const AkPacket &packet) {
                            this->frameReady(packet);
                         });

    this->m_mutexLib.unlock();

    self->setState(state);
}

void VideoCaptureElementPrivate::captureLibUpdated(const QString &captureLib)
{
    auto state = self->state();
    self->setState(AkElement::ElementStateNull);

    this->m_mutexLib.lock();

    this->m_capture =
            ptr_cast<Capture>(VideoCaptureElement::loadSubModule("VideoCapture",
                                                                 captureLib));

    this->m_mutexLib.unlock();

    if (!this->m_capture)
        return;

    QObject::connect(this->m_capture.data(),
                     &Capture::errorChanged,
                     self,
                     &VideoCaptureElement::errorChanged);
    QObject::connect(this->m_capture.data(),
                     &Capture::webcamsChanged,
                     self,
                     &VideoCaptureElement::mediasChanged);
    QObject::connect(this->m_capture.data(),
                     &Capture::deviceChanged,
                     self,
                     &VideoCaptureElement::mediaChanged);
    QObject::connect(this->m_capture.data(),
                     &Capture::imageControlsChanged,
                     self,
                     &VideoCaptureElement::imageControlsChanged);
    QObject::connect(this->m_capture.data(),
                     &Capture::cameraControlsChanged,
                     self,
                     &VideoCaptureElement::cameraControlsChanged);

    emit self->mediasChanged(self->medias());
    emit self->streamsChanged(self->streams());

    auto medias = self->medias();

    if (!medias.isEmpty())
        self->setMedia(medias.first());

    self->setState(state);
}

void VideoCaptureElementPrivate::frameReady(const AkPacket &packet) const
{
    if (this->m_mirror || this->m_swapRgb) {
        AkVideoPacket videoPacket(packet);
        QImage oImage = videoPacket.toImage();

        if (this->m_mirror)
            oImage = oImage.mirrored();

        if (this->m_swapRgb)
            oImage = oImage.rgbSwapped();

        emit self->oStream(AkVideoPacket::fromImage(oImage, videoPacket));
    } else {
        emit self->oStream(packet);
    }
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
    if (!this->d->m_capture)
        return;

    this->d->m_capture->setDevice(media);
    QSettings settings;

    settings.beginGroup("VideoCapture");
    auto ndevices = settings.beginReadArray("devices");
    auto deviceDescription = this->d->m_capture->description(media);
    int streamIndex = 0;

    for (decltype(ndevices) i = 0; i < ndevices; i++) {
        settings.setArrayIndex(i);
        auto deviceId = settings.value("id").toString();
        auto description = settings.value("description").toString();

        if (deviceId == media && description == deviceDescription) {
            streamIndex = settings.value("stream", 0).toInt();
            streamIndex = qBound(0,
                                 streamIndex,
                                 this->d->m_capture->listTracks({}).size() - 1);

            break;
        }
    }

    settings.endArray();
    settings.endGroup();

    this->d->m_capture->setStreams({streamIndex});
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

    QSettings settings;

    settings.beginGroup("VideoCapture");
    auto ndevices = settings.beginReadArray("devices");
    auto media = this->d->m_capture->device();
    auto deviceDescription = this->d->m_capture->description(media);
    decltype(ndevices) i = 0;

    for (; i < ndevices; i++) {
        settings.setArrayIndex(i);
        auto deviceId = settings.value("id").toString();
        auto description = settings.value("description").toString();

        if (deviceId == media && description == deviceDescription)
            break;
    }

    settings.endArray();

    settings.beginWriteArray("devices");
    settings.setArrayIndex(i);
    settings.setValue("id", media);
    settings.setValue("description", deviceDescription);
    settings.setValue("stream", streams.isEmpty()? 0: streams.first());

    settings.endArray();
    settings.endGroup();
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

void VideoCaptureElement::reset()
{
    bool running = this->d->m_runCameraLoop;
    this->setState(AkElement::ElementStateNull);

    if (this->d->m_capture)
        this->d->m_capture->reset();

    if (running)
        this->setState(AkElement::ElementStatePlaying);

    QSettings settings;

    settings.beginGroup("VideoCapture");
    auto ndevices = settings.beginReadArray("devices");
    auto media = this->d->m_capture->device();
    auto deviceDescription = this->d->m_capture->description(media);
    decltype(ndevices) i = 0;

    for (; i < ndevices; i++) {
        settings.setArrayIndex(i);
        auto deviceId = settings.value("id").toString();
        auto description = settings.value("description").toString();

        if (deviceId == media && description == deviceDescription)
            break;
    }

    auto streams = this->d->m_capture->streams();

    settings.endArray();

    settings.beginWriteArray("devices");
    settings.setArrayIndex(i);
    settings.setValue("id", media);
    settings.setValue("description", deviceDescription);
    settings.setValue("stream", streams.isEmpty()? 0: streams.first());

    settings.endArray();
    settings.endGroup();
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

#include "moc_videocaptureelement.cpp"

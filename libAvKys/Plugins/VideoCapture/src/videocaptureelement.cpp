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
#include <QReadWriteLock>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideopacket.h>

#include "videocaptureelement.h"
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
        CapturePtr m_capture;
        QString m_captureImpl;
        QThreadPool m_threadPool;
        QFuture<void> m_cameraLoopResult;
        QReadWriteLock m_mutex;
        bool m_runCameraLoop {false};
        bool m_pause {false};
        bool m_mirror {false};
        bool m_swapRgb {false};

        explicit VideoCaptureElementPrivate(VideoCaptureElement *self);
        void cameraLoop();
        void linksChanged(const AkPluginLinks &links);
        void frameReady(const AkPacket &packet) const;
};

VideoCaptureElement::VideoCaptureElement():
    AkMultimediaSourceElement()
{
    this->d = new VideoCaptureElementPrivate(this);
    QObject::connect(akPluginManager,
                     &AkPluginManager::linksChanged,
                     this,
                     [this] (const AkPluginLinks &links) {
                        this->d->linksChanged(links);
                     });

    if (this->d->m_capture) {
        QObject::connect(this->d->m_capture.data(),
                         &Capture::errorChanged,
                         this,
                         &VideoCaptureElement::errorChanged);
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

        auto medias = this->d->m_capture->webcams();

        if (!medias.isEmpty()) {
            auto media = medias.first();
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
    }
}

VideoCaptureElement::~VideoCaptureElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QString VideoCaptureElement::error() const
{
    QString error;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        error = this->d->m_capture->error();

    this->d->m_mutex.unlock();

    return error;
}

QStringList VideoCaptureElement::medias()
{
    QStringList medias;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        medias = this->d->m_capture->webcams();

    this->d->m_mutex.unlock();

    return medias;
}

QString VideoCaptureElement::media() const
{
    QString media;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        media = this->d->m_capture->device();

    this->d->m_mutex.unlock();

    return media;
}

QList<int> VideoCaptureElement::streams()
{
    QList<int> streams;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        streams = this->d->m_capture->streams();

    this->d->m_mutex.unlock();

    return streams;
}

QList<int> VideoCaptureElement::listTracks(const QString &mimeType)
{
    QList<int> tracks;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        tracks = this->d->m_capture->listTracks(mimeType);

    this->d->m_mutex.unlock();

    return tracks;
}

int VideoCaptureElement::defaultStream(const QString &mimeType)
{
    if (mimeType == "video/x-raw")
        return 0;

    return -1;
}

QString VideoCaptureElement::description(const QString &media)
{
    QString description;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        description = this->d->m_capture->description(media);

    this->d->m_mutex.unlock();

    return description;
}

AkCaps VideoCaptureElement::caps(int stream)
{
    AkCaps caps;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture) {
        auto streams = this->d->m_capture->caps(this->d->m_capture->device());
        auto deviceCaps = streams.value(stream).value<AkCaps>();

        if (deviceCaps) {
            caps = AkVideoCaps(AkVideoCaps::Format_rgb24,
                               deviceCaps.property("width").toInt(),
                               deviceCaps.property("height").toInt(),
                               deviceCaps.property("fps").toString());
        }
    }

    this->d->m_mutex.unlock();

    return caps;
}

AkCaps VideoCaptureElement::rawCaps(int stream) const
{
    AkCaps caps;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture) {
        auto streams = this->d->m_capture->caps(this->d->m_capture->device());
        caps = streams.value(stream).value<AkCaps>();
    }

    this->d->m_mutex.unlock();

    return caps;
}

QString VideoCaptureElement::streamDescription(int stream) const
{
    AkCaps caps;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture) {
        auto streams = this->d->m_capture->caps(this->d->m_capture->device());
        caps = streams.value(stream).value<AkCaps>();
    }

    this->d->m_mutex.unlock();

    if (!caps)
        return {};

    auto fourcc = caps.property("fourcc").toString();
    auto width = caps.property("width").toInt();
    auto height = caps.property("height").toInt();
    auto fps = AkFrac(caps.property("fps").toString()).value();

    return QString("%1 %2x%3 %4 FPS")
            .arg(fourcc)
            .arg(width)
            .arg(height)
            .arg(fps);
}

QStringList VideoCaptureElement::listCapsDescription() const
{
    QStringList capsDescriptions;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture) {
        auto streams = this->d->m_capture->caps(this->d->m_capture->device());

        for (auto &caps: streams)
            capsDescriptions << this->d->m_capture->capsDescription(caps.value<AkCaps>());
    }

    this->d->m_mutex.unlock();

    return capsDescriptions;
}

QString VideoCaptureElement::ioMethod() const
{
    QString ioMethod;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        ioMethod = this->d->m_capture->ioMethod();

    this->d->m_mutex.unlock();

    return ioMethod;
}

int VideoCaptureElement::nBuffers() const
{
    int nBuffers = 0;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        nBuffers = this->d->m_capture->nBuffers();

    this->d->m_mutex.unlock();

    return nBuffers;
}

QVariantList VideoCaptureElement::imageControls() const
{
    QVariantList imageControls;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        imageControls = this->d->m_capture->imageControls();

    this->d->m_mutex.unlock();

    return imageControls;
}

bool VideoCaptureElement::setImageControls(const QVariantMap &imageControls)
{
    bool result = false;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture)
        result = this->d->m_capture->setImageControls(imageControls);

    this->d->m_mutex.unlock();

    return result;
}

bool VideoCaptureElement::resetImageControls()
{
    bool result = false;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture)
        result = this->d->m_capture->resetImageControls();

    this->d->m_mutex.unlock();

    return result;
}

QVariantList VideoCaptureElement::cameraControls() const
{
    QVariantList cameraControls;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        cameraControls = this->d->m_capture->cameraControls();

    this->d->m_mutex.unlock();

    return cameraControls;
}

bool VideoCaptureElement::setCameraControls(const QVariantMap &cameraControls)
{
    bool result = false;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture)
        result = this->d->m_capture->setCameraControls(cameraControls);

    this->d->m_mutex.unlock();

    return result;
}

bool VideoCaptureElement::resetCameraControls()
{
    bool result = false;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture)
        result = this->d->m_capture->resetCameraControls();

    this->d->m_mutex.unlock();

    return result;
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
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture) {
        this->d->m_capture->setDevice(media);
        QSettings settings;

        settings.beginGroup("VideoCapture");
        auto ndevices = settings.beginReadArray("devices");
        auto deviceDescription = this->d->m_capture->description(media);
        int streamIndex = 0;

        for (int i = 0; i < ndevices; i++) {
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

    this->d->m_mutex.unlock();
}

void VideoCaptureElement::setStreams(const QList<int> &streams)
{
    bool running = this->d->m_runCameraLoop;
    this->setState(AkElement::ElementStateNull);
    QString media;
    QString deviceDescription;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture) {
        this->d->m_capture->setStreams(streams);
        media = this->d->m_capture->device();
        deviceDescription = this->d->m_capture->description(media);
    }

    this->d->m_mutex.unlock();

    if (running)
        this->setState(AkElement::ElementStatePlaying);

    QSettings settings;

    settings.beginGroup("VideoCapture");
    auto ndevices = settings.beginReadArray("devices");
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
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture)
        this->d->m_capture->setIoMethod(ioMethod);

    this->d->m_mutex.unlock();
}

void VideoCaptureElement::setNBuffers(int nBuffers)
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture)
        this->d->m_capture->setNBuffers(nBuffers);

    this->d->m_mutex.unlock();
}

void VideoCaptureElement::resetMedia()
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture)
        this->d->m_capture->resetDevice();

    this->d->m_mutex.unlock();
}

void VideoCaptureElement::resetStreams()
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture)
        this->d->m_capture->resetStreams();

    this->d->m_mutex.unlock();
}

void VideoCaptureElement::resetIoMethod()
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture)
        this->d->m_capture->resetIoMethod();

    this->d->m_mutex.unlock();
}

void VideoCaptureElement::resetNBuffers()
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture)
        this->d->m_capture->resetNBuffers();

    this->d->m_mutex.unlock();
}

void VideoCaptureElement::reset()
{
    bool running = this->d->m_runCameraLoop;
    this->setState(AkElement::ElementStateNull);
    QString media;
    QString deviceDescription;
    QList<int> streams;
    this->d->m_mutex.lockForWrite();

    if (this->d->m_capture) {
        this->d->m_capture->reset();
        media = this->d->m_capture->device();
        deviceDescription = this->d->m_capture->description(media);
        streams = this->d->m_capture->streams();
    }

    this->d->m_mutex.unlock();

    if (running)
        this->setState(AkElement::ElementStatePlaying);

    QSettings settings;

    settings.beginGroup("VideoCapture");
    auto ndevices = settings.beginReadArray("devices");
    int i = 0;

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

bool VideoCaptureElement::setState(AkElement::ElementState state)
{
    if (!this->d->m_capture)
        return false;

    auto curState = this->state();

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

VideoCaptureElementPrivate::VideoCaptureElementPrivate(VideoCaptureElement *self):
    self(self)
{
    this->m_capture =
            akPluginManager->create<Capture>("VideoSource/CameraCapture/Impl/*");
    this->m_captureImpl =
            akPluginManager->defaultPlugin("VideoSource/CameraCapture/Impl/*",
                                           {"CameraCaptureImpl"}).id();
}

void VideoCaptureElementPrivate::cameraLoop()
{
    auto convertVideo =
            akPluginManager->create<ConvertVideo>("VideoSource/CameraCapture/Convert/*");

    if (!convertVideo)
        return;

    QObject::connect(convertVideo.data(),
                     &ConvertVideo::frameReady,
                     self,
                     [this] (const AkPacket &packet) {
                        this->frameReady(packet);
                     });

#ifdef Q_OS_WIN32
    // Initialize the COM library in multithread mode.
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    if (this->m_capture && this->m_capture->init()) {
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

                if (!convertVideo->init(caps))
                    break;

                initConvert = false;
            }

            convertVideo->packetEnqueue(packet);
        }

        convertVideo->uninit();
        this->m_capture->uninit();
    }

#ifdef Q_OS_WIN32
    // Close COM library.
    CoUninitialize();
#endif
}

void VideoCaptureElementPrivate::linksChanged(const AkPluginLinks &links)
{
    if (!links.contains("VideoSource/CameraCapture/Impl/*")
        || links["VideoSource/CameraCapture/Impl/*"] == this->m_captureImpl)
        return;

    auto state = self->state();
    self->setState(AkElement::ElementStateNull);

    this->m_mutex.lockForWrite();
    this->m_capture =
            akPluginManager->create<Capture>("VideoSource/CameraCapture/Impl/*");
    this->m_mutex.unlock();
    this->m_captureImpl = links["VideoSource/CameraCapture/Impl/*"];

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
        auto oImage = videoPacket.toImage();

        if (this->m_mirror)
            oImage = oImage.mirrored();

        if (this->m_swapRgb)
            oImage = oImage.rgbSwapped();

        emit self->oStream(AkVideoPacket::fromImage(oImage, videoPacket));
    } else {
        emit self->oStream(packet);
    }
}

#include "moc_videocaptureelement.cpp"

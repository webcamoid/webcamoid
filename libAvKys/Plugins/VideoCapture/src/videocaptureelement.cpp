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

#include <QAbstractEventDispatcher>
#include <QFuture>
#include <QQmlContext>
#include <QReadWriteLock>
#include <QSharedPointer>
#include <QThreadPool>
#include <QtConcurrent>
#include <akcaps.h>
#include <akcompressedvideocaps.h>
#include <akcompressedvideopacket.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "videocaptureelement.h"
#include "convertvideo.h"
#include "capture.h"

#define PAUSE_TIMEOUT 500

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
        AkVideoConverter m_videoConverter;
        CapturePtr m_capture;
        QString m_captureImpl;
        QThreadPool m_threadPool;
        QFuture<void> m_cameraLoopResult;
        QReadWriteLock m_mutex;
        bool m_runCameraLoop {false};
        bool m_pause {false};

        explicit VideoCaptureElementPrivate(VideoCaptureElement *self);
        QString capsDescription(const AkCaps &caps) const;
        void cameraLoop();
        void linksChanged(const AkPluginLinks &links);
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
        QObject::connect(this->d->m_capture.data(),
                         &Capture::pictureTaken,
                         this,
                         &VideoCaptureElement::pictureTaken);
        QObject::connect(this->d->m_capture.data(),
                         &Capture::flashModeChanged,
                         this,
                         [this] (Capture::FlashMode mode) {
                             emit this->flashModeChanged(FlashMode(mode));
                         });

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
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    QString error;

    if (capture)
        error = capture->error();

    return error;
}

QStringList VideoCaptureElement::medias()
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    QStringList medias;

    if (capture)
        medias = capture->webcams();

    return medias;
}

QString VideoCaptureElement::media() const
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    QString media;

    if (capture)
        media = capture->device();

    return media;
}

QList<int> VideoCaptureElement::streams()
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    QList<int> streams;

    if (capture)
        streams = capture->streams();

    return streams;
}

QList<int> VideoCaptureElement::listTracks(AkCaps::CapsType type)
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    QList<int> tracks;

    if (capture)
        tracks = capture->listTracks(type);

    return tracks;
}

int VideoCaptureElement::defaultStream(AkCaps::CapsType type)
{
    if (type == AkCaps::CapsVideo)
        return 0;

    return -1;
}

QString VideoCaptureElement::description(const QString &media)
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    QString description;

    if (capture)
        description = capture->description(media);

    return description;
}

AkCaps VideoCaptureElement::caps(int stream)
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (!capture)
        return {};

    auto streams = capture->caps(capture->device());
    auto deviceCaps = streams.value(stream);

    if (!deviceCaps)
        return {};

    AkCaps caps;

    if (deviceCaps.type() == AkCaps::CapsVideoCompressed) {
        AkVideoCaps videoCaps(deviceCaps);
        caps = AkVideoCaps(AkVideoCaps::Format_argb,
                           videoCaps.width(),
                           videoCaps.height(),
                           videoCaps.fps());
    } else {
        caps = deviceCaps;
    }

    return caps;
}

AkCaps VideoCaptureElement::rawCaps(int stream) const
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    AkCaps caps;

    if (capture) {
        auto streams = capture->caps(capture->device());
        caps = streams.value(stream);
    }

    return caps;
}

QString VideoCaptureElement::streamDescription(int stream) const
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    AkCaps caps;

    if (capture) {
        auto streams = capture->caps(capture->device());
        caps = streams.value(stream);
    }

    if (!caps)
        return {};

    return this->d->capsDescription(caps);
}

QStringList VideoCaptureElement::listCapsDescription() const
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    QStringList capsDescriptions;

    if (capture) {
        auto streams = capture->caps(capture->device());

        for (auto &caps: streams)
            capsDescriptions << this->d->capsDescription(caps);
    }

    return capsDescriptions;
}

QString VideoCaptureElement::ioMethod() const
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    QString ioMethod;

    if (capture)
        ioMethod = capture->ioMethod();

    return ioMethod;
}

int VideoCaptureElement::nBuffers() const
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    int nBuffers = 0;

    if (capture)
        nBuffers = capture->nBuffers();

    return nBuffers;
}

QVariantList VideoCaptureElement::imageControls() const
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    QVariantList imageControls;

    if (capture)
        imageControls = capture->imageControls();

    return imageControls;
}

bool VideoCaptureElement::setImageControls(const QVariantMap &imageControls)
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    bool result = false;

    if (capture)
        result = capture->setImageControls(imageControls);

    return result;
}

bool VideoCaptureElement::resetImageControls()
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    bool result = false;

    if (capture)
        result = capture->resetImageControls();

    return result;
}

QVariantList VideoCaptureElement::cameraControls() const
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    QVariantList cameraControls;

    if (capture)
        cameraControls = capture->cameraControls();

    return cameraControls;
}

bool VideoCaptureElement::setCameraControls(const QVariantMap &cameraControls)
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    bool result = false;

    if (capture)
        result = capture->setCameraControls(cameraControls);

    return result;
}

bool VideoCaptureElement::resetCameraControls()
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    bool result = false;

    if (capture)
        result = capture->resetCameraControls();

    return result;
}

VideoCaptureElement::FlashModeList VideoCaptureElement::supportedFlashModes(const QString &webcam) const
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    FlashModeList result;

    if (capture) {
        auto modes = capture->supportedFlashModes(webcam);

        for (auto &mode: modes)
            result << FlashMode(mode);
    }

    return result;
}

VideoCaptureElement::FlashMode VideoCaptureElement::flashMode() const
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    FlashMode result = FlashMode_Off;

    if (capture)
        result = FlashMode(capture->flashMode());

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
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (!capture)
        return;

    capture->setDevice(media);
    QSettings settings;

    settings.beginGroup("VideoCapture");
    auto ndevices = settings.beginReadArray("devices");
    auto deviceDescription = capture->description(media);
    int streamIndex = 0;

    for (int i = 0; i < ndevices; i++) {
        settings.setArrayIndex(i);
        auto deviceId = settings.value("id").toString();
        auto description = settings.value("description").toString();

        if (deviceId == media && description == deviceDescription) {
            streamIndex = settings.value("stream", 0).toInt();
            streamIndex = qBound(0,
                                 streamIndex,
                                 capture->listTracks({}).size() - 1);

            break;
        }
    }

    settings.endArray();
    settings.endGroup();

    capture->setStreams({streamIndex});
}

void VideoCaptureElement::setStreams(const QList<int> &streams)
{
    bool running = this->d->m_runCameraLoop;
    this->setState(AkElement::ElementStateNull);
    QString media;
    QString deviceDescription;

    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (capture) {
        capture->setStreams(streams);
        media = capture->device();
        deviceDescription = capture->description(media);
    }

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
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (capture)
        capture->setIoMethod(ioMethod);
}

void VideoCaptureElement::setNBuffers(int nBuffers)
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (capture)
        capture->setNBuffers(nBuffers);
}

void VideoCaptureElement::setFlashMode(FlashMode mode)
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (capture)
        capture->setFlashMode(Capture::FlashMode(mode));
}

void VideoCaptureElement::resetMedia()
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (capture)
        capture->resetDevice();
}

void VideoCaptureElement::resetStreams()
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (capture)
        capture->resetStreams();
}

void VideoCaptureElement::resetIoMethod()
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (capture)
        capture->resetIoMethod();
}

void VideoCaptureElement::resetNBuffers()
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (capture)
        capture->resetNBuffers();
}

void VideoCaptureElement::resetFlashMode()
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (capture)
        capture->resetFlashMode();
}

void VideoCaptureElement::reset()
{
    bool running = this->d->m_runCameraLoop;
    this->setState(AkElement::ElementStateNull);
    QString media;
    QString deviceDescription;
    QList<int> streams;

    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (capture) {
        capture->reset();
        media = capture->device();
        deviceDescription = capture->description(media);
        streams = capture->streams();
    }

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

void VideoCaptureElement::takePictures(int count, int delayMsecs)
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (capture)
        capture->takePictures(count, delayMsecs);
}

bool VideoCaptureElement::setState(AkElement::ElementState state)
{
    this->d->m_mutex.lockForRead();
    auto capture = this->d->m_capture;
    this->d->m_mutex.unlock();

    if (!capture)
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

QString VideoCaptureElementPrivate::capsDescription(const AkCaps &caps) const
{
    switch (caps.type()) {
    case AkCaps::CapsVideo: {
        AkVideoCaps videoCaps(caps);
        auto format = AkVideoCaps::pixelFormatToString(videoCaps.format());

        return QString("%1, %2x%3, %4 FPS")
                .arg(format.toUpper(),
                     videoCaps.width(),
                     videoCaps.height())
                .arg(qRound(videoCaps.fps().value()));
    }

    case AkCaps::CapsVideoCompressed: {
        AkCompressedVideoCaps videoCaps(caps);

        return QString("%1, %2x%3, %4 FPS")
                .arg(videoCaps.format().toUpper(),
                     videoCaps.width(),
                     videoCaps.height())
                .arg(qRound(videoCaps.fps().value()));
    }

    default:
        break;
    }

    return {};
}

void VideoCaptureElementPrivate::cameraLoop()
{
    this->m_mutex.lockForRead();
    auto capture = this->m_capture;
    this->m_mutex.unlock();

    if (capture && capture->init()) {
        QSharedPointer<ConvertVideo> convertVideo;
        bool initConvert = true;

        while (this->m_runCameraLoop) {
            if (this->m_pause) {
                QThread::msleep(PAUSE_TIMEOUT);

                continue;
            }

            auto packet = capture->readFrame();

            if (!packet)
                continue;

            auto caps = packet.caps();

            if (caps.type() == AkCaps::CapsVideoCompressed) {
                if (initConvert) {
                    convertVideo =
                            akPluginManager->create<ConvertVideo>("VideoSource/CameraCapture/Convert/*");

                    if (!convertVideo)
                        break;

                    QObject::connect(convertVideo.data(),
                                     &ConvertVideo::frameReady,
                                     self,
                                     &VideoCaptureElement::oStream,
                                     Qt::DirectConnection);

                    if (!convertVideo->init(caps))
                        break;

                    initConvert = false;
                }

                convertVideo->packetEnqueue(packet);
            } else {
                emit self->oStream(packet);
            }
        }

        if (convertVideo)
            convertVideo->uninit();

        capture->uninit();
    }
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
    QObject::connect(this->m_capture.data(),
                     &Capture::pictureTaken,
                     self,
                     &VideoCaptureElement::pictureTaken);
    QObject::connect(this->m_capture.data(),
                     &Capture::flashModeChanged,
                     self,
                     [=] (Capture::FlashMode mode) {
                         emit self->flashModeChanged(VideoCaptureElement::FlashMode(mode));
                     });

    emit self->mediasChanged(self->medias());
    emit self->streamsChanged(self->streams());

    auto medias = self->medias();

    if (!medias.isEmpty())
        self->setMedia(medias.first());

    self->setState(state);
}

#include "moc_videocaptureelement.cpp"

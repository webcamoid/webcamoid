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

QList<int> VideoCaptureElement::listTracks(AkCaps::CapsType type)
{
    QList<int> tracks;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        tracks = this->d->m_capture->listTracks(type);

    this->d->m_mutex.unlock();

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
        auto deviceCaps = streams.value(stream);

        if (deviceCaps) {
            if (deviceCaps.type() == AkCaps::CapsVideoCompressed) {
                AkVideoCaps videoCaps(deviceCaps);
                caps = AkVideoCaps(AkVideoCaps::Format_argb,
                                   videoCaps.width(),
                                   videoCaps.height(),
                                   videoCaps.fps());
            } else {
                caps = deviceCaps;
            }
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
        caps = streams.value(stream);
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
        caps = streams.value(stream);
    }

    this->d->m_mutex.unlock();

    if (!caps)
        return {};

    return this->d->capsDescription(caps);
}

QStringList VideoCaptureElement::listCapsDescription() const
{
    QStringList capsDescriptions;
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture) {
        auto streams = this->d->m_capture->caps(this->d->m_capture->device());

        for (auto &caps: streams)
            capsDescriptions << this->d->capsDescription(caps);
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
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        result = this->d->m_capture->setImageControls(imageControls);

    this->d->m_mutex.unlock();

    return result;
}

bool VideoCaptureElement::resetImageControls()
{
    bool result = false;
    this->d->m_mutex.lockForRead();

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
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        result = this->d->m_capture->setCameraControls(cameraControls);

    this->d->m_mutex.unlock();

    return result;
}

bool VideoCaptureElement::resetCameraControls()
{
    bool result = false;
    this->d->m_mutex.lockForRead();

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
    this->d->m_mutex.lockForRead();

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
    this->d->m_mutex.lockForRead();

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
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        this->d->m_capture->setIoMethod(ioMethod);

    this->d->m_mutex.unlock();
}

void VideoCaptureElement::setNBuffers(int nBuffers)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        this->d->m_capture->setNBuffers(nBuffers);

    this->d->m_mutex.unlock();
}

void VideoCaptureElement::resetMedia()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        this->d->m_capture->resetDevice();

    this->d->m_mutex.unlock();
}

void VideoCaptureElement::resetStreams()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        this->d->m_capture->resetStreams();

    this->d->m_mutex.unlock();
}

void VideoCaptureElement::resetIoMethod()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_capture)
        this->d->m_capture->resetIoMethod();

    this->d->m_mutex.unlock();
}

void VideoCaptureElement::resetNBuffers()
{
    this->d->m_mutex.lockForRead();

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
    this->d->m_mutex.lockForRead();

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
    if (this->m_capture && this->m_capture->init()) {
        QSharedPointer<ConvertVideo> convertVideo;
        bool initConvert = true;

        while (this->m_runCameraLoop) {
            if (this->m_pause) {
                QThread::msleep(PAUSE_TIMEOUT);

                continue;
            }

            auto packet = this->m_capture->readFrame();

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

        this->m_capture->uninit();
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

    emit self->mediasChanged(self->medias());
    emit self->streamsChanged(self->streams());

    auto medias = self->medias();

    if (!medias.isEmpty())
        self->setMedia(medias.first());

    self->setState(state);
}

#include "moc_videocaptureelement.cpp"

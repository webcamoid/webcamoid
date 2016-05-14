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

#include "videocaptureelement.h"

#define PAUSE_TIMEOUT 500

VideoCaptureElement::VideoCaptureElement():
    AkMultimediaSourceElement()
{
    this->m_runCameraLoop = false;
    this->m_pause = false;

    QObject::connect(&this->m_capture,
                     &Capture::error,
                     this,
                     &VideoCaptureElement::error);
    QObject::connect(&this->m_capture,
                     &Capture::webcamsChanged,
                     this,
                     &VideoCaptureElement::mediasChanged);
    QObject::connect(&this->m_capture,
                     &Capture::deviceChanged,
                     this,
                     &VideoCaptureElement::mediaChanged);
    QObject::connect(&this->m_capture,
                     &Capture::imageControlsChanged,
                     this,
                     &VideoCaptureElement::imageControlsChanged);
    QObject::connect(&this->m_capture,
                     &Capture::cameraControlsChanged,
                     this,
                     &VideoCaptureElement::cameraControlsChanged);
    QObject::connect(&this->m_capture,
                     &Capture::streamsChanged,
                     this,
                     &VideoCaptureElement::streamsChanged);
    QObject::connect(&this->m_convertVideo,
                     &ConvertVideo::frameReady,
                     this,
                     &VideoCaptureElement::frameReady,
                     Qt::DirectConnection);
}

VideoCaptureElement::~VideoCaptureElement()
{
    this->setState(AkElement::ElementStateNull);
}

QObject *VideoCaptureElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/VideoCapture/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("VideoCapture", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", controlId);

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

QStringList VideoCaptureElement::medias() const
{
    return this->m_capture.webcams();
}

QString VideoCaptureElement::media() const
{
    return this->m_capture.device();
}

QList<int> VideoCaptureElement::streams() const
{
    return this->m_capture.streams();
}

QList<int> VideoCaptureElement::listTracks(const QString &mimeType)
{
    return this->m_capture.listTracks(mimeType);
}

int VideoCaptureElement::defaultStream(const QString &mimeType) const
{
    if (mimeType == "video/x-raw")
        return 0;

    return -1;
}

QString VideoCaptureElement::description(const QString &media) const
{
    return this->m_capture.description(media);
}

AkCaps VideoCaptureElement::caps(int stream) const
{
    QVariantList streams = this->m_capture.caps(this->m_capture.device());
    AkCaps caps = streams.value(stream).value<AkCaps>();

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

QStringList VideoCaptureElement::listCapsDescription() const
{
    QStringList capsDescriptions;
    QVariantList streams = this->m_capture.caps(this->m_capture.device());

    foreach (QVariant caps, streams)
        capsDescriptions << this->m_capture.capsDescription(caps.value<AkCaps>());

    return capsDescriptions;
}

QString VideoCaptureElement::ioMethod() const
{
    return this->m_capture.ioMethod();
}

int VideoCaptureElement::nBuffers() const
{
    return this->m_capture.nBuffers();
}

QVariantList VideoCaptureElement::imageControls() const
{
    return this->m_capture.imageControls();
}

bool VideoCaptureElement::setImageControls(const QVariantMap &imageControls) const
{
    return this->m_capture.setImageControls(imageControls);
}

bool VideoCaptureElement::resetImageControls() const
{
    return this->m_capture.resetImageControls();
}

QVariantList VideoCaptureElement::cameraControls() const
{
    return this->m_capture.cameraControls();
}

bool VideoCaptureElement::setCameraControls(const QVariantMap &cameraControls) const
{
    return this->m_capture.setCameraControls(cameraControls);
}

bool VideoCaptureElement::resetCameraControls() const
{
    return this->m_capture.resetCameraControls();
}

void VideoCaptureElement::cameraLoop(VideoCaptureElement *captureElement)
{
#ifdef Q_OS_WIN32
    // Initialize the COM library in multithread mode.
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif

    if (captureElement->m_capture.init()) {
        while (captureElement->m_runCameraLoop) {
            if (captureElement->m_pause) {
                QThread::msleep(PAUSE_TIMEOUT);

                continue;
            }

            AkPacket packet = captureElement->m_capture.readFrame();

            if (!packet)
                continue;

            captureElement->m_convertVideo.packetEnqueue(packet);
        }

        captureElement->m_capture.uninit();
    }

#ifdef Q_OS_WIN32
    // Close COM library.
    CoUninitialize();
#endif
}

void VideoCaptureElement::setMedia(const QString &media)
{
    this->m_capture.setDevice(media);
}

void VideoCaptureElement::setStreams(const QList<int> &streams)
{
    bool running = this->m_runCameraLoop;
    this->setState(AkElement::ElementStateNull);

    this->m_capture.setStreams(streams);

    if (running)
        this->setState(AkElement::ElementStatePlaying);
}

void VideoCaptureElement::setIoMethod(const QString &ioMethod)
{
    this->m_capture.setIoMethod(ioMethod);
}

void VideoCaptureElement::setNBuffers(int nBuffers)
{
    this->m_capture.setNBuffers(nBuffers);
}

void VideoCaptureElement::resetMedia()
{
    this->m_capture.resetDevice();
}

void VideoCaptureElement::resetStreams()
{
    this->m_capture.resetStreams();
}

void VideoCaptureElement::resetIoMethod()
{
    this->m_capture.resetIoMethod();
}

void VideoCaptureElement::resetNBuffers()
{
    this->m_capture.resetNBuffers();
}

void VideoCaptureElement::reset()
{
    this->m_capture.reset();
}

bool VideoCaptureElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused: {
            QList<int> streams = this->m_capture.streams();

            if (streams.isEmpty())
                return false;

            QVariantList supportedCaps = this->m_capture.caps(this->m_capture.device());
            AkCaps caps = supportedCaps.value(streams[0]).value<AkCaps>();

            if (!this->m_convertVideo.init(caps))
                return false;

            this->m_pause = true;
            this->m_runCameraLoop = true;
            this->m_cameraLoopResult = QtConcurrent::run(&this->m_threadPool, this->cameraLoop, this);

            return AkElement::setState(state);
        }
        case AkElement::ElementStatePlaying: {
            QList<int> streams = this->m_capture.streams();

            if (streams.isEmpty())
                return false;

            QVariantList supportedCaps = this->m_capture.caps(this->m_capture.device());
            AkCaps caps = supportedCaps.value(streams[0]).value<AkCaps>();

            if (!this->m_convertVideo.init(caps))
                return false;

            this->m_pause = false;
            this->m_runCameraLoop = true;
            this->m_cameraLoopResult = QtConcurrent::run(&this->m_threadPool, this->cameraLoop, this);

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
            this->m_pause = false;
            this->m_runCameraLoop = false;
            this->m_cameraLoopResult.waitForFinished();
            this->m_convertVideo.uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->m_pause = false;

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->m_runCameraLoop = false;
            this->m_cameraLoopResult.waitForFinished();
            this->m_convertVideo.uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            this->m_pause = true;

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
#if defined(Q_OS_WIN32)
    QImage oImage = AkUtils::packetToImage(packet).mirrored().rgbSwapped();

    emit this->oStream(AkUtils::imageToPacket(oImage, packet));
#else
    emit this->oStream(packet);
#endif
}

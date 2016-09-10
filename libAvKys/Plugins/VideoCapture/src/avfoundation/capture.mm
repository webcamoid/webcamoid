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

#import <AVFoundation/AVFoundation.h>

#include "capture.h"
#include "devicewatcher.h"

typedef QMap<FourCharCode, QString> FourCharCodeToStrMap;

inline FourCharCodeToStrMap initFourCharCodeToStrMap()
{
    FourCharCodeToStrMap fourccToStrMap = {
        // Raw formats
        {kCMPixelFormat_32ARGB         , "BGRA"},
        {kCMPixelFormat_24RGB          , "RGB3"},
        {kCMPixelFormat_16BE555        , "RGBQ"},
        {kCMPixelFormat_16BE565        , "RGBR"},
        {kCMPixelFormat_16LE555        , "RGBO"},
        {kCMPixelFormat_16LE565        , "RGBP"},
        {kCMPixelFormat_16LE5551       , "AR15"},
        {kCMPixelFormat_422YpCbCr8     , "UYVY"},
        {kCMPixelFormat_422YpCbCr8_yuvs, "YUY2"},

        // Compressed formats
        {kCMVideoCodecType_422YpCbCr8  , "UYVY"},
        {kCMVideoCodecType_JPEG        , "JPEG"},
        {kCMVideoCodecType_JPEG_OpenDML, "MJPG"},
        {kCMVideoCodecType_H263        , "H263"},
        {kCMVideoCodecType_H264        , "H264"},
        {kCMVideoCodecType_HEVC        , "HEVC"},
        {kCMVideoCodecType_MPEG4Video  , "MPG4"},
        {kCMVideoCodecType_MPEG2Video  , "MPG2"},
        {kCMVideoCodecType_MPEG1Video  , "MPG1"}
    };

    return fourccToStrMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(FourCharCodeToStrMap, fourccToStrMap, (initFourCharCodeToStrMap()))

class CapturePrivate
{
    public:
        DeviceWatcher *m_deviceWatcher;

        static inline QString fourccToStr(FourCharCode format)
        {
            char fourcc[5];
            memcpy(fourcc, &format, sizeof(FourCharCode));
            fourcc[4] = 0;

            return QString(fourcc);
        }

        static inline FourCharCode strToFourCC(const QString &format)
        {
            FourCharCode fourcc;
            memcpy(&fourcc, format.toStdString().c_str(), sizeof(FourCharCode));

            return fourcc;
        }

        static inline QVariantList imageControls(AVCaptureDevice *camera)
        {
            // Exposure
            // Low Light
            // Image Exposure
            // White Balance

            return QVariantList();
        }

        static inline QVariantList cameraControls(AVCaptureDevice *camera)
        {
            // Focus
            // Zoom
            // Flash
            // Torch

            return QVariantList();
        }
};

Capture::Capture(): QObject()
{
    this->m_id = -1;
    this->m_ioMethod = IoMethodUnknown;
    this->m_nBuffers = 32;
    this->m_webcams = this->webcams();
    this->m_device = this->m_webcams.value(0, "");
    this->d = new CapturePrivate();
    this->d->m_deviceWatcher = [[DeviceWatcher alloc]
                               initWithCaptureObject: this];

    [[NSNotificationCenter defaultCenter]
     addObserver: this->d->m_deviceWatcher
     selector: @selector(cameraConnected:)
     name: AVCaptureDeviceWasConnectedNotification
     object: nil];

    [[NSNotificationCenter defaultCenter]
     addObserver: this->d->m_deviceWatcher
     selector: @selector(cameraDisconnected:)
     name: AVCaptureDeviceWasDisconnectedNotification
     object: nil];

    // I've added this code is case AVCaptureDeviceWasDisconnectedNotification
    // signal doesn't works. Need more tests here.
    this->m_timer.setInterval(1000);

    QObject::connect(&this->m_timer,
                     &QTimer::timeout,
                     this,
                     &Capture::updateWebcams);

    //this->m_timer.start();
}

Capture::~Capture()
{
    //this->m_timer.stop();

    [[NSNotificationCenter defaultCenter]
     removeObserver: this->d->m_deviceWatcher];

    [this->d->m_deviceWatcher disconnect];
    [this->d->m_deviceWatcher release];

    delete this->d;
}

QStringList Capture::webcams() const
{
    QStringList webcams;
    NSArray *cameras = [AVCaptureDevice devicesWithMediaType: AVMediaTypeVideo];

    for (AVCaptureDevice *camera in cameras)
        webcams << camera.uniqueID.UTF8String;

    return webcams;
}

QString Capture::device() const
{
    return this->m_device;
}

QList<int> Capture::streams() const
{
    if (!this->m_streams.isEmpty())
        return this->m_streams;

    QVariantList caps = this->caps(this->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int>() << 0;
}

QList<int> Capture::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return QList<int>();

    QVariantList caps = this->caps(this->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString Capture::ioMethod() const
{
    return QString();
}

int Capture::nBuffers() const
{
    return this->m_nBuffers;
}

QString Capture::description(const QString &webcam) const
{
    NSString *uniqueID = [[NSString alloc]
                          initWithUTF8String: webcam.toStdString().c_str()];
    AVCaptureDevice *camera = [AVCaptureDevice deviceWithUniqueID: uniqueID];
    [uniqueID release];

    if (!camera)
        return QString();

    return QString(camera.localizedName.UTF8String);
}

QVariantList Capture::caps(const QString &webcam) const
{
    QVariantList caps;
    NSString *uniqueID = [[NSString alloc]
                          initWithUTF8String: webcam.toStdString().c_str()];
    AVCaptureDevice *camera = [AVCaptureDevice deviceWithUniqueID: uniqueID];
    [uniqueID release];

    if (!camera)
        return caps;

    for (AVCaptureDeviceFormat *format in camera.formats) {
        if ([format.mediaType isEqualToString: AVMediaTypeVideo] == NO)
            continue;

        FourCharCode fourCC = CMFormatDescriptionGetMediaSubType(format.formatDescription);
        CMVideoDimensions size =
                CMVideoFormatDescriptionGetDimensions(format.formatDescription);

        QString fourccStr =
                fourccToStrMap->value(fourCC,
                                      CapturePrivate::fourccToStr(fourCC));

        AkCaps videoCaps;
        videoCaps.setMimeType("video/unknown");
        videoCaps.setProperty("fourcc", fourccStr);
        videoCaps.setProperty("width", size.width);
        videoCaps.setProperty("height", size.height);

        for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges) {
            videoCaps.setProperty("fps", AkFrac(1e3 * fpsRange.maxFrameRate, 1e3).toString());
            caps << QVariant::fromValue(videoCaps);
        }
    }

    return caps;
}

QString Capture::capsDescription(const AkCaps &caps) const
{
    if (caps.mimeType() != "video/unknown")
        return QString();

    AkFrac fps = caps.property("fps").toString();

    return QString("%1, %2x%3, %4 FPS")
                .arg(caps.property("fourcc").toString())
                .arg(caps.property("width").toString())
                .arg(caps.property("height").toString())
                .arg(qRound(fps.value()));
}

QVariantList Capture::imageControls() const
{
    return this->m_globalImageControls;
}

bool Capture::setImageControls(const QVariantMap &imageControls)
{
    this->m_controlsMutex.lock();
    QVariantList globalImageControls = this->m_globalImageControls;
    this->m_controlsMutex.unlock();

    for (int i = 0; i < globalImageControls.count(); i++) {
        QVariantList control = globalImageControls[i].toList();
        QString controlName = control[0].toString();

        if (imageControls.contains(controlName)) {
            control[6] = imageControls[controlName];
            globalImageControls[i] = control;
        }
    }

    this->m_controlsMutex.lock();

    if (this->m_globalImageControls == globalImageControls) {
        this->m_controlsMutex.unlock();

        return false;
    }

    this->m_globalImageControls = globalImageControls;
    this->m_controlsMutex.unlock();

    emit this->imageControlsChanged(imageControls);

    return true;
}

bool Capture::resetImageControls()
{
    QVariantMap controls;

    foreach (QVariant control, this->imageControls()) {
        QVariantList params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList Capture::cameraControls() const
{
    return this->m_globalCameraControls;
}

bool Capture::setCameraControls(const QVariantMap &cameraControls)
{
    this->m_controlsMutex.lock();
    QVariantList globalCameraControls = this->m_globalCameraControls;
    this->m_controlsMutex.unlock();

    for (int i = 0; i < globalCameraControls.count(); i++) {
        QVariantList control = globalCameraControls[i].toList();
        QString controlName = control[0].toString();

        if (cameraControls.contains(controlName)) {
            control[6] = cameraControls[controlName];
            globalCameraControls[i] = control;
        }
    }

    this->m_controlsMutex.lock();

    if (this->m_globalCameraControls == globalCameraControls) {
        this->m_controlsMutex.unlock();

        return false;
    }

    this->m_globalCameraControls = globalCameraControls;
    this->m_controlsMutex.unlock();

    emit this->cameraControlsChanged(cameraControls);

    return true;
}

bool Capture::resetCameraControls()
{
    QVariantMap controls;

    foreach (QVariant control, this->cameraControls()) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket Capture::readFrame()
{
    return AkPacket();
}

QVariantMap Capture::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    foreach (QVariant control, controls) {
        QVariantList params = control.toList();
        QString controlName = params[0].toString();
        controlStatus[controlName] = params[0];
    }

    return controlStatus;
}

bool Capture::init()
{
    return false;
}

void Capture::uninit()
{
}

void Capture::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;

    if (device.isEmpty()) {
        this->m_controlsMutex.lock();
        this->m_globalImageControls.clear();
        this->m_globalCameraControls.clear();
        this->m_controlsMutex.unlock();
    } else {
        NSString *uniqueID = [[NSString alloc]
                              initWithUTF8String: device.toStdString().c_str()];
        AVCaptureDevice *camera = [AVCaptureDevice deviceWithUniqueID: uniqueID];
        [uniqueID release];
        this->m_controlsMutex.lock();
        this->m_globalImageControls = this->d->imageControls(camera);
        this->m_globalCameraControls = this->d->cameraControls(camera);
        this->m_controlsMutex.unlock();
    }

    this->m_controlsMutex.lock();
    QVariantMap imageStatus = this->controlStatus(this->m_globalImageControls);
    QVariantMap cameraStatus = this->controlStatus(this->m_globalCameraControls);
    this->m_controlsMutex.unlock();

    emit this->deviceChanged(device);
    emit this->imageControlsChanged(imageStatus);
    emit this->cameraControlsChanged(cameraStatus);
}

void Capture::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    QVariantList supportedCaps = this->caps(this->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams;
    inputStreams << stream;

    if (this->streams() == inputStreams)
        return;

    this->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void Capture::setIoMethod(const QString &ioMethod)
{
    Q_UNUSED(ioMethod)
}

void Capture::setNBuffers(int nBuffers)
{
    if (this->m_nBuffers == nBuffers)
        return;

    this->m_nBuffers = nBuffers;
    emit this->nBuffersChanged(nBuffers);
}

void Capture::resetDevice()
{
    this->setDevice(this->m_webcams.value(0, ""));
}

void Capture::resetStreams()
{
    QVariantList supportedCaps = this->caps(this->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void Capture::resetIoMethod()
{
    this->setIoMethod("any");
}

void Capture::resetNBuffers()
{
    this->setNBuffers(32);
}

void Capture::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

void Capture::cameraConnected()
{
    this->updateWebcams();
}

void Capture::cameraDisconnected()
{
    this->updateWebcams();
}

void Capture::updateWebcams()
{
    QStringList webcams = this->webcams();

    if (this->m_webcams != webcams) {
        this->m_webcams = webcams;
        emit this->webcamsChanged(webcams);
    }
}

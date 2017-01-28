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

#include <sys/time.h>
#import <AVFoundation/AVFoundation.h>

#include "captureavfoundation.h"
#include "deviceobserver.h"

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

typedef QMap<OSType, QString> PixelFormatToStrMap;

inline PixelFormatToStrMap initPixelFormatToStrMap()
{
    FourCharCodeToStrMap pixelFormatToStrMap = {
        {kCVPixelFormatType_1Monochrome                 , "B0W1"},
        {kCVPixelFormatType_16BE555                     , "RGBQ"},
        {kCVPixelFormatType_16LE555                     , "RGBO"},
        {kCVPixelFormatType_16LE5551                    , "AR15"},
        {kCVPixelFormatType_16BE565                     , "RGBR"},
        {kCVPixelFormatType_16LE565                     , "RGBP"},
        {kCVPixelFormatType_24RGB                       , "RGB3"},
        {kCVPixelFormatType_24BGR                       , "BGR3"},
        {kCVPixelFormatType_32ARGB                      , "BGRA"},
        {kCVPixelFormatType_32BGRA                      , "RGB4"},
        {kCVPixelFormatType_32RGBA                      , "BGR4"},
        {kCVPixelFormatType_422YpCbCr8                  , "UYVY"},
        {kCVPixelFormatType_444YpCbCr8                  , "Y444"},
        {kCVPixelFormatType_420YpCbCr8Planar            , "YV12"},
        {kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange, "NV12"},
        {kCVPixelFormatType_422YpCbCr8_yuvs             , "YUY2"},
        {kCVPixelFormatType_OneComponent8               , "Y800"}
    };

    return pixelFormatToStrMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(PixelFormatToStrMap, pixelFormatToStrMap, (initPixelFormatToStrMap()))

class CaptureAvFoundationPrivate
{
    public:
        id m_deviceObserver;
        AVCaptureDeviceInput *m_deviceInput;
        AVCaptureVideoDataOutput *m_dataOutput;
        AVCaptureSession *m_session;
        CMSampleBufferRef m_curFrame;

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
            QVariantList controls;

            if ([camera lockForConfiguration: nil] == NO)
                return controls;

            // This controls will be not implemented since Apple doesn't
            // provides an interface for controlling camera controls
            // (ie. UVC controls).

            [camera unlockForConfiguration];

            return controls;
        }

        static inline QVariantList cameraControls(AVCaptureDevice *camera)
        {
            QVariantList controls;

            if ([camera lockForConfiguration: nil] == NO)
                return controls;

            // Same as above.

            [camera unlockForConfiguration];

            return controls;
        }

        static inline AVCaptureDeviceFormat *formatFromCaps(AVCaptureDevice *camera,
                                                            const AkCaps &caps)
        {
            for (AVCaptureDeviceFormat *format in camera.formats) {
                if ([format.mediaType isEqualToString: AVMediaTypeVideo] == NO)
                    continue;

                FourCharCode fourCC = CMFormatDescriptionGetMediaSubType(format.formatDescription);
                CMVideoDimensions size =
                        CMVideoFormatDescriptionGetDimensions(format.formatDescription);

                QString fourccStr =
                        fourccToStrMap->value(fourCC,
                                              CaptureAvFoundationPrivate::fourccToStr(fourCC));

                AkCaps videoCaps;
                videoCaps.setMimeType("video/unknown");
                videoCaps.setProperty("fourcc", fourccStr);
                videoCaps.setProperty("width", size.width);
                videoCaps.setProperty("height", size.height);

                for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges) {
                    videoCaps.setProperty("fps", AkFrac(qRound(1e3 * fpsRange.maxFrameRate), 1e3).toString());

                    if (videoCaps == caps)
                        return format;
                }
            }

            return nil;
        }

        static inline AVFrameRateRange *frameRateRangeFromFps(AVCaptureDeviceFormat *format,
                                                              const AkFrac &fps)
        {
            for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges)
                if (AkFrac(qRound(1e3 * fpsRange.maxFrameRate), 1e3) == fps)
                    return fpsRange;

            return nil;
        }
};

CaptureAvFoundation::CaptureAvFoundation(QObject *parent):
    Capture(parent)
{
    this->m_id = -1;
    this->m_ioMethod = IoMethodUnknown;
    this->m_nBuffers = 32;
    this->d = new CaptureAvFoundationPrivate();
    this->d->m_deviceInput = nil;
    this->d->m_dataOutput = nil;
    this->d->m_session = nil;
    this->d->m_curFrame = nil;
    this->d->m_deviceObserver = [[DeviceObserver alloc]
                                 initWithCaptureObject: this];

    [[NSNotificationCenter defaultCenter]
     addObserver: this->d->m_deviceObserver
     selector: @selector(cameraConnected:)
     name: AVCaptureDeviceWasConnectedNotification
     object: nil];

    [[NSNotificationCenter defaultCenter]
     addObserver: this->d->m_deviceObserver
     selector: @selector(cameraDisconnected:)
     name: AVCaptureDeviceWasDisconnectedNotification
     object: nil];

    this->updateDevices();
}

CaptureAvFoundation::~CaptureAvFoundation()
{
    this->uninit();

    [[NSNotificationCenter defaultCenter]
     removeObserver: this->d->m_deviceObserver];

    [this->d->m_deviceObserver disconnect];
    [this->d->m_deviceObserver release];

    delete this->d;
}

QStringList CaptureAvFoundation::webcams() const
{
    return this->m_devices;
}

QString CaptureAvFoundation::device() const
{
    return this->m_device;
}

QList<int> CaptureAvFoundation::streams() const
{
    if (!this->m_streams.isEmpty())
        return this->m_streams;

    auto caps = this->caps(this->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int> {0};
}

QList<int> CaptureAvFoundation::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return QList<int>();

    auto caps = this->caps(this->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CaptureAvFoundation::ioMethod() const
{
    return QString();
}

int CaptureAvFoundation::nBuffers() const
{
    return this->m_nBuffers;
}

QString CaptureAvFoundation::description(const QString &webcam) const
{
    return this->m_descriptions.value(webcam);
}

QVariantList CaptureAvFoundation::caps(const QString &webcam) const
{
    return this->m_devicesCaps.value(webcam);
}

QString CaptureAvFoundation::capsDescription(const AkCaps &caps) const
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

QVariantList CaptureAvFoundation::imageControls() const
{
    return this->m_globalImageControls;
}

bool CaptureAvFoundation::setImageControls(const QVariantMap &imageControls)
{
    this->m_controlsMutex.lock();
    auto globalImageControls = this->m_globalImageControls;
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

bool CaptureAvFoundation::resetImageControls()
{
    QVariantMap controls;

    for (const QVariant &control: this->imageControls()) {
        QVariantList params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList CaptureAvFoundation::cameraControls() const
{
    return this->m_globalCameraControls;
}

bool CaptureAvFoundation::setCameraControls(const QVariantMap &cameraControls)
{
    this->m_controlsMutex.lock();
    auto globalCameraControls = this->m_globalCameraControls;
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

bool CaptureAvFoundation::resetCameraControls()
{
    QVariantMap controls;

    for (const QVariant &control: this->cameraControls()) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket CaptureAvFoundation::readFrame()
{
    this->m_mutex.lock();

    if (!this->d->m_curFrame)
        if (!this->m_frameReady.wait(&this->m_mutex, 1000)) {
            this->m_mutex.unlock();

            return AkPacket();
        }

    // Read frame data.
    QByteArray oBuffer;

    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(this->d->m_curFrame);
    CMBlockBufferRef dataBuffer = CMSampleBufferGetDataBuffer(this->d->m_curFrame);
    QString fourcc;

    if (imageBuffer) {
        size_t dataSize = CVPixelBufferGetDataSize(imageBuffer);
        oBuffer.resize(int(dataSize));
        CVPixelBufferLockBaseAddress(imageBuffer, 0);
        void *data = CVPixelBufferGetBaseAddress(imageBuffer);
        memcpy(oBuffer.data(), data, dataSize);
        CVPixelBufferUnlockBaseAddress(imageBuffer, 0);

        OSType format = CVPixelBufferGetPixelFormatType(imageBuffer);
        fourcc = pixelFormatToStrMap->value(format,
                                            CaptureAvFoundationPrivate::fourccToStr(format));
    } else if (dataBuffer) {
        size_t dataSize = 0;
        char *data = NULL;
        CMBlockBufferGetDataPointer(dataBuffer,
                                    0,
                                    NULL,
                                    &dataSize,
                                    &data);
        oBuffer.resize(int(dataSize));
        memcpy(oBuffer.data(), data, dataSize);
    }

    // Read pts.
    CMItemCount count = 0;
    CMSampleTimingInfo timingInfo;
    qint64 pts;
    AkFrac timeBase;

    if (CMSampleBufferGetOutputSampleTimingInfoArray(this->d->m_curFrame,
                                                     1,
                                                     &timingInfo,
                                                     &count) == noErr) {
        pts = timingInfo.presentationTimeStamp.value;
        timeBase = AkFrac(1, timingInfo.presentationTimeStamp.timescale);
    } else {
        timeval timestamp;
        gettimeofday(&timestamp, NULL);
        pts = qint64((timestamp.tv_sec
                      + 1e-6 * timestamp.tv_usec)
                     * this->m_timeBase.invert().value());
        timeBase = this->m_timeBase;
    }

    AkCaps caps(this->m_caps);

    if (!fourcc.isEmpty())
        caps.setProperty("fourcc", fourcc);

    // Create package.
    AkPacket packet(caps, oBuffer);
    packet.setPts(pts);
    packet.setTimeBase(this->m_timeBase);
    packet.setIndex(0);
    packet.setId(this->m_id);

    CFRelease(this->d->m_curFrame);
    this->d->m_curFrame = nil;

    this->m_mutex.unlock();

    return packet;
}

quint32 CaptureAvFoundation::modelId(const QString &webcam) const
{
    return this->m_modelId.value(webcam);
}

QMutex &CaptureAvFoundation::mutex()
{
    return this->m_mutex;
}

QWaitCondition &CaptureAvFoundation::frameReady()
{
    return this->m_frameReady;
}

void *CaptureAvFoundation::curFrame()
{
    return &this->d->m_curFrame;
}

QVariantMap CaptureAvFoundation::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (const QVariant &control: controls) {
        QVariantList params = control.toList();
        QString controlName = params[0].toString();
        controlStatus[controlName] = params[0];
    }

    return controlStatus;
}

bool CaptureAvFoundation::init()
{
    QString webcam = this->m_device;

    if (webcam.isEmpty())
        return false;

    // Read selected caps.
    auto streams = this->streams();

    if (streams.isEmpty())
        return false;

    auto supportedCaps = this->caps(webcam);

    if (supportedCaps.isEmpty())
        return false;

    AkCaps caps = streams[0] < supportedCaps.size()?
                    supportedCaps[streams[0]].value<AkCaps>():
                    supportedCaps.first().value<AkCaps>();

    // Get camera input.
    NSString *uniqueID = [[NSString alloc]
                          initWithUTF8String: webcam.toStdString().c_str()];
    AVCaptureDevice *camera = [AVCaptureDevice deviceWithUniqueID: uniqueID];
    [uniqueID release];

    if (!camera)
        return false;

    // Add camera input unit.
    this->d->m_deviceInput = [AVCaptureDeviceInput
                              deviceInputWithDevice: camera
                              error: nil];

    if (!this->d->m_deviceInput)
        return false;

    // Create capture session.
    this->d->m_session = [AVCaptureSession new];
    [this->d->m_session beginConfiguration];

    if ([this->d->m_session canAddInput: this->d->m_deviceInput] == NO) {
        [this->d->m_session release];

        return false;
    }

    [this->d->m_session addInput: this->d->m_deviceInput];

    // Add data output unit.
    this->d->m_dataOutput = [AVCaptureVideoDataOutput new];
    this->d->m_dataOutput.videoSettings = nil;
    this->d->m_dataOutput.alwaysDiscardsLateVideoFrames = YES;

    dispatch_queue_t queue = dispatch_queue_create("frameQueue", NULL);
    [this->d->m_dataOutput
     setSampleBufferDelegate: this->d->m_deviceObserver
     queue: queue];
    dispatch_release(queue);

    if ([this->d->m_session canAddOutput: this->d->m_dataOutput] == NO) {
        [this->d->m_dataOutput release];
        [this->d->m_session release];

        return false;
    }

    [this->d->m_session addOutput: this->d->m_dataOutput];
    [this->d->m_session commitConfiguration];

    if ([camera lockForConfiguration: nil] == NO) {
        [this->d->m_session release];

        return false;
    }

    // Configure camera format.
    auto format = CaptureAvFoundationPrivate::formatFromCaps(camera, caps);

    if (!format) {
        [camera unlockForConfiguration];
        [this->d->m_session release];

        return false;
    }

    AkFrac fps = caps.property("fps").toString();
    auto fpsRange = CaptureAvFoundationPrivate::frameRateRangeFromFps(format,
                                                                      fps);

    camera.activeFormat = format;
    camera.activeVideoMinFrameDuration = fpsRange.minFrameDuration;
    camera.activeVideoMaxFrameDuration = fpsRange.maxFrameDuration;

    // Start capturing from the camera.
    [this->d->m_session startRunning];
    [camera unlockForConfiguration];
    [this->d->m_deviceInput retain];

    this->m_caps = caps;
    this->m_timeBase = fps.invert();
    this->m_id = Ak::id();

    return true;
}

void CaptureAvFoundation::uninit()
{
    if (this->d->m_session) {
        [this->d->m_session stopRunning];
        [this->d->m_session beginConfiguration];

        if (this->d->m_deviceInput)
            [this->d->m_session removeInput: this->d->m_deviceInput];

        if (this->d->m_dataOutput)
            [this->d->m_session removeOutput: this->d->m_dataOutput];

        [this->d->m_session commitConfiguration];
        [this->d->m_session release];
        this->d->m_session = nil;
    }

    if (this->d->m_deviceInput) {
        [this->d->m_deviceInput release];
        this->d->m_deviceInput = nil;
    }

    if (this->d->m_dataOutput) {
        [this->d->m_dataOutput release];
        this->d->m_dataOutput = nil;
    }

    this->m_mutex.lock();

    if (this->d->m_curFrame) {
        CFRelease(this->d->m_curFrame);
        this->d->m_curFrame = nil;
    }

    this->m_mutex.unlock();
}

void CaptureAvFoundation::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(device);
}

void CaptureAvFoundation::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    auto supportedCaps = this->caps(this->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams;
    inputStreams << stream;

    if (this->streams() == inputStreams)
        return;

    this->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void CaptureAvFoundation::setIoMethod(const QString &ioMethod)
{
    Q_UNUSED(ioMethod)
}

void CaptureAvFoundation::setNBuffers(int nBuffers)
{
    if (this->m_nBuffers == nBuffers)
        return;

    this->m_nBuffers = nBuffers;
    emit this->nBuffersChanged(nBuffers);
}

void CaptureAvFoundation::resetDevice()
{
    this->setDevice(this->m_devices.value(0, ""));
}

void CaptureAvFoundation::resetStreams()
{
    QVariantList supportedCaps = this->caps(this->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void CaptureAvFoundation::resetIoMethod()
{
    this->setIoMethod("any");
}

void CaptureAvFoundation::resetNBuffers()
{
    this->setNBuffers(32);
}

void CaptureAvFoundation::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

void CaptureAvFoundation::cameraConnected()
{
    this->updateDevices();
}

void CaptureAvFoundation::cameraDisconnected()
{
    this->updateDevices();
}

void CaptureAvFoundation::updateDevices()
{
    decltype(this->m_devices) devices;
    decltype(this->m_modelId) modelId;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;

    NSArray *cameras = [AVCaptureDevice devicesWithMediaType: AVMediaTypeVideo];

    for (AVCaptureDevice *camera in cameras) {
        QString deviceId = camera.uniqueID.UTF8String;
        devices << deviceId;
        descriptions[deviceId] = camera.localizedName.UTF8String;
        QString modelIdStr = camera.modelID.UTF8String;
        QRegExp vpMatch("VendorID_(\\d+) ProductID_(\\d+)");
        quint16 vendorId = 0;
        quint16 productId = 0;
        int pos = 0;

        forever {
            pos = vpMatch.indexIn(modelIdStr, pos);

            if (pos < 0)
                break;

            vendorId = vpMatch.cap(1).toUShort();
            productId = vpMatch.cap(2).toUShort();
            pos += vpMatch.matchedLength();
        }

        modelId[deviceId] = quint32(vendorId << 16) | productId;

        // List supported frame formats.
        for (AVCaptureDeviceFormat *format in camera.formats) {
            FourCharCode fourCC = CMFormatDescriptionGetMediaSubType(format.formatDescription);
            CMVideoDimensions size =
                    CMVideoFormatDescriptionGetDimensions(format.formatDescription);

            QString fourccStr =
                    fourccToStrMap->value(fourCC,
                                          CaptureAvFoundationPrivate::fourccToStr(fourCC));

            AkCaps videoCaps;
            videoCaps.setMimeType("video/unknown");
            videoCaps.setProperty("fourcc", fourccStr);
            videoCaps.setProperty("width", size.width);
            videoCaps.setProperty("height", size.height);

            // List all supported frame rates for the format.
            for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges) {
                videoCaps.setProperty("fps", AkFrac(qRound(1e3 * fpsRange.maxFrameRate), 1e3).toString());
                devicesCaps[deviceId] << QVariant::fromValue(videoCaps);
            }
        }
    }

    if (this->m_devices != devices) {
        this->m_devices = devices;
        emit this->webcamsChanged(devices);
    }

    this->m_devices = devices;
    this->m_modelId = modelId;
    this->m_descriptions = descriptions;
    this->m_devicesCaps = devicesCaps;
}

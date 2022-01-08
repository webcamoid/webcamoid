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

#include <QtDebug>
#include <QCoreApplication>
#include <QMap>
#include <QVariant>
#include <QWaitCondition>
#include <QMutex>
#include <ak.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <sys/time.h>
#import <AVFoundation/AVFoundation.h>

#include "captureavfoundation.h"
#include "deviceobserver.h"

using FourCharCodeToStrMap = QMap<FourCharCode, QString>;

class CaptureAvFoundationPrivate
{
    public:
        id m_deviceObserver {nil};
        AVCaptureDeviceInput *m_deviceInput {nil};
        AVCaptureVideoDataOutput *m_dataOutput {nil};
        AVCaptureSession *m_session {nil};
        CMSampleBufferRef m_curFrame {nil};
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, quint32> m_modelId;
        QMap<QString, QString> m_descriptions;
        QMap<QString, QVariantList> m_devicesCaps;
        int m_nBuffers {32};
        QMutex m_mutex;
        QWaitCondition m_frameReady;
        AkFrac m_fps;
        AkFrac m_timeBase;
        AkCaps m_caps;
        qint64 m_id {-1};

        CaptureAvFoundationPrivate();
        static bool canUseCamera();
        static inline QString fourccToStr(FourCharCode format);
        static inline FourCharCode strToFourCC(const QString &format);
        static inline AVCaptureDeviceFormat *formatFromCaps(AVCaptureDevice *camera,
                                                            const AkCaps &caps);
        static inline AVFrameRateRange *frameRateRangeFromFps(AVCaptureDeviceFormat *format,
                                                              const AkFrac &fps);
        static inline const FourCharCodeToStrMap &fourccToStrMap();
        AkCaps capsFromFrameSampleBuffer(const CMSampleBufferRef sampleBuffer) const;
};

CaptureAvFoundation::CaptureAvFoundation(QObject *parent):
    Capture(parent)
{
    this->d = new CaptureAvFoundationPrivate();
    this->d->m_deviceObserver = [[DeviceObserverAVFoundation alloc]
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
    return this->d->m_devices;
}

QString CaptureAvFoundation::device() const
{
    return this->d->m_device;
}

QList<int> CaptureAvFoundation::streams()
{
    if (!this->d->m_streams.isEmpty())
        return this->d->m_streams;

    auto caps = this->caps(this->d->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int> {0};
}

QList<int> CaptureAvFoundation::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return {};

    auto caps = this->caps(this->d->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CaptureAvFoundation::ioMethod() const
{
    return {};
}

int CaptureAvFoundation::nBuffers() const
{
    return this->d->m_nBuffers;
}

QString CaptureAvFoundation::description(const QString &webcam) const
{
    return this->d->m_descriptions.value(webcam);
}

QVariantList CaptureAvFoundation::caps(const QString &webcam) const
{
    return this->d->m_devicesCaps.value(webcam);
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

AkPacket CaptureAvFoundation::readFrame()
{
    this->d->m_mutex.lock();

    if (!this->d->m_curFrame)
        if (!this->d->m_frameReady.wait(&this->d->m_mutex, 1000)) {
            this->d->m_mutex.unlock();

            return {};
        }

    // Read frame data.
    QByteArray oBuffer;
    auto imageBuffer = CMSampleBufferGetImageBuffer(this->d->m_curFrame);
    auto dataBuffer = CMSampleBufferGetDataBuffer(this->d->m_curFrame);
    auto caps = this->d->capsFromFrameSampleBuffer(this->d->m_curFrame);
    caps.setProperty("fps", this->d->m_timeBase.invert().toString());

    if (imageBuffer) {
        size_t dataSize = CVPixelBufferGetDataSize(imageBuffer);
        oBuffer.resize(int(dataSize));
        CVPixelBufferLockBaseAddress(imageBuffer, 0);
        void *data = CVPixelBufferGetBaseAddress(imageBuffer);
        memcpy(oBuffer.data(), data, dataSize);
        CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
    } else if (dataBuffer) {
        size_t dataSize = 0;
        char *data = nullptr;
        CMBlockBufferGetDataPointer(dataBuffer,
                                    0,
                                    nullptr,
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
        gettimeofday(&timestamp, nullptr);
        pts = qint64((timestamp.tv_sec
                      + 1e-6 * timestamp.tv_usec)
                     * this->d->m_timeBase.invert().value());
        timeBase = this->d->m_timeBase;
    }

    // Create package.
    AkPacket packet(caps);
    packet.setBuffer(oBuffer);
    packet.setPts(pts);
    packet.setTimeBase(this->d->m_timeBase);
    packet.setIndex(0);
    packet.setId(this->d->m_id);

    CFRelease(this->d->m_curFrame);
    this->d->m_curFrame = nil;

    this->d->m_mutex.unlock();

    return packet;
}

quint32 CaptureAvFoundation::modelId(const QString &webcam) const
{
    return this->d->m_modelId.value(webcam);
}

QMutex &CaptureAvFoundation::mutex()
{
    return this->d->m_mutex;
}

QWaitCondition &CaptureAvFoundation::frameReady()
{
    return this->d->m_frameReady;
}

void *CaptureAvFoundation::curFrame()
{
    return &this->d->m_curFrame;
}

bool CaptureAvFoundation::init()
{
    auto webcam = this->d->m_device;

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
    auto uniqueID = [[NSString alloc]
                     initWithUTF8String: webcam.toStdString().c_str()];
    auto camera = [AVCaptureDevice deviceWithUniqueID: uniqueID];
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

    this->d->m_dataOutput.videoSettings = @{
        (NSString *) kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_24RGB)
    };
    this->d->m_dataOutput.alwaysDiscardsLateVideoFrames = YES;

    dispatch_queue_t queue = dispatch_queue_create("frameQueue", nullptr);
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

    this->d->m_caps = caps;
    this->d->m_timeBase = fps.invert();
    this->d->m_id = Ak::id();

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

    this->d->m_mutex.lock();

    if (this->d->m_curFrame) {
        CFRelease(this->d->m_curFrame);
        this->d->m_curFrame = nil;
    }

    this->d->m_mutex.unlock();
}

void CaptureAvFoundation::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;
    emit this->deviceChanged(device);
}

void CaptureAvFoundation::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    auto supportedCaps = this->caps(this->d->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams;
    inputStreams << stream;

    if (this->streams() == inputStreams)
        return;

    this->d->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void CaptureAvFoundation::setIoMethod(const QString &ioMethod)
{
    Q_UNUSED(ioMethod)
}

void CaptureAvFoundation::setNBuffers(int nBuffers)
{
    if (this->d->m_nBuffers == nBuffers)
        return;

    this->d->m_nBuffers = nBuffers;
    emit this->nBuffersChanged(nBuffers);
}

void CaptureAvFoundation::resetDevice()
{
    this->setDevice(this->d->m_devices.value(0, ""));
}

void CaptureAvFoundation::resetStreams()
{
    QVariantList supportedCaps = this->caps(this->d->m_device);
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
    if (!CaptureAvFoundationPrivate::canUseCamera())
        return;

    decltype(this->d->m_devices) devices;
    decltype(this->d->m_modelId) modelId;
    decltype(this->d->m_descriptions) descriptions;
    decltype(this->d->m_devicesCaps) devicesCaps;

    auto devicesDiscovery =
            [AVCaptureDeviceDiscoverySession
             discoverySessionWithDeviceTypes: @[
                AVCaptureDeviceTypeExternalUnknown,
                AVCaptureDeviceTypeBuiltInWideAngleCamera
             ]
             mediaType: AVMediaTypeVideo
             position: AVCaptureDevicePositionUnspecified];

    for (AVCaptureDevice *camera in [devicesDiscovery devices]) {
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
            auto fourCC = CMFormatDescriptionGetMediaSubType(format.formatDescription);
            CMVideoDimensions size =
                    CMVideoFormatDescriptionGetDimensions(format.formatDescription);
            auto &map = CaptureAvFoundationPrivate::fourccToStrMap();
            QString fourccStr =
                    map.value(fourCC,
                              CaptureAvFoundationPrivate::fourccToStr(fourCC));

            // Ignore formats that can cause alignment problems when capturing.
            if ((size.width % 32) != 0)
                continue;

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

    if (devicesCaps.isEmpty()) {
        devices.clear();
        modelId.clear();
        descriptions.clear();
    }

    this->d->m_modelId = modelId;
    this->d->m_descriptions = descriptions;
    this->d->m_devicesCaps = devicesCaps;

    if (this->d->m_devices != devices) {
        this->d->m_devices = devices;
        emit this->webcamsChanged(devices);
    }
}

CaptureAvFoundationPrivate::CaptureAvFoundationPrivate()
{

}

bool CaptureAvFoundationPrivate::canUseCamera()
{
    if (@available(macOS 10.14, *)) {
        auto status = [AVCaptureDevice authorizationStatusForMediaType: AVMediaTypeVideo];

        if (status == AVAuthorizationStatusAuthorized)
            return true;

        static bool done;
        static bool result = false;
        done = false;

        [AVCaptureDevice
         requestAccessForMediaType: AVMediaTypeVideo
         completionHandler: ^(BOOL granted) {
            done = true;
            result = granted;
        }];

        while (!done)
            qApp->processEvents();

        return result;
    }

    return true;
}

QString CaptureAvFoundationPrivate::fourccToStr(FourCharCode format)
{
    char fourcc[5];
    memcpy(fourcc, &format, sizeof(FourCharCode));
    fourcc[4] = 0;

    return QString(fourcc);
}

FourCharCode CaptureAvFoundationPrivate::strToFourCC(const QString &format)
{
    FourCharCode fourcc;
    memcpy(&fourcc, format.toStdString().c_str(), sizeof(FourCharCode));

    return fourcc;
}

AVCaptureDeviceFormat *CaptureAvFoundationPrivate::formatFromCaps(AVCaptureDevice *camera,
                                                                  const AkCaps &caps)
{
    for (AVCaptureDeviceFormat *format in camera.formats) {
        if ([format.mediaType isEqualToString: AVMediaTypeVideo] == NO)
            continue;

        auto fourCC = CMFormatDescriptionGetMediaSubType(format.formatDescription);
        CMVideoDimensions size =
                CMVideoFormatDescriptionGetDimensions(format.formatDescription);
        auto &map = CaptureAvFoundationPrivate::fourccToStrMap();
        QString fourccStr =
                map.value(fourCC,
                           CaptureAvFoundationPrivate::fourccToStr(fourCC));

        AkCaps videoCaps;
        videoCaps.setMimeType("video/unknown");
        videoCaps.setProperty("fourcc", fourccStr);
        videoCaps.setProperty("width", size.width);
        videoCaps.setProperty("height", size.height);

        for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges) {
            videoCaps.setProperty("fps",
                                  AkFrac(qRound(1e3 * fpsRange.maxFrameRate),
                                         1e3).toString());

            if (videoCaps == caps)
                return format;
        }
    }

    return nil;
}

AVFrameRateRange *CaptureAvFoundationPrivate::frameRateRangeFromFps(AVCaptureDeviceFormat *format,
                                                                    const AkFrac &fps)
{
    for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges)
        if (AkFrac(qRound(1e3 * fpsRange.maxFrameRate), 1e3) == fps)
            return fpsRange;

    return nil;
}

const FourCharCodeToStrMap &CaptureAvFoundationPrivate::fourccToStrMap()
{
    static const FourCharCodeToStrMap fourccToStrMap {
        // Raw formats
        {kCMPixelFormat_32ARGB         , "ARGB"    },
        {kCMPixelFormat_32BGRA         , "BGRA"    },
        {kCMPixelFormat_24RGB          , "RGB"     },
        {kCMPixelFormat_16BE555        , "RGB555BE"},
        {kCMPixelFormat_16BE565        , "RGB565BE"},
        {kCMPixelFormat_16LE555        , "RGB555"  },
        {kCMPixelFormat_16LE565        , "RGB565"  },
        {kCMPixelFormat_16LE5551       , "ARGB555" },
        {kCMPixelFormat_422YpCbCr8     , "UYVY"    },
        {kCMPixelFormat_422YpCbCr8_yuvs, "YUY2"    },

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

AkCaps CaptureAvFoundationPrivate::capsFromFrameSampleBuffer(const CMSampleBufferRef sampleBuffer) const
{
    auto formatDesc = CMSampleBufferGetFormatDescription(sampleBuffer);

    AkCaps videoCaps;
    videoCaps.setMimeType("video/unknown");

    auto fourCC = CMFormatDescriptionGetMediaSubType(formatDesc);
    auto &map = CaptureAvFoundationPrivate::fourccToStrMap();
    videoCaps.setProperty("fourcc",
                          map.value(fourCC,
                                    CaptureAvFoundationPrivate::fourccToStr(fourCC)));

    auto size = CMVideoFormatDescriptionGetDimensions(formatDesc);
    videoCaps.setProperty("width", size.width);
    videoCaps.setProperty("height", size.height);

    auto time = CMSampleBufferGetDuration(sampleBuffer);
    videoCaps.setProperty("fps", AkFrac(time.timescale, time.value).toString());

    return videoCaps;
}

#include "moc_captureavfoundation.cpp"

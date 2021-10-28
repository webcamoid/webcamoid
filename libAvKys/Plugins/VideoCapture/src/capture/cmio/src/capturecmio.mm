/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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
#import <CoreMediaIO/CMIOHardwarePlugIn.h>

#include "capturecmio.h"
#include "deviceobserver.h"

#define ENABLE_CONTROLS 0

using FourCharCodeToStrMap = QMap<FourCharCode, QString>;

enum StreamDirection
{
    StreamDirectionOutput,
    StreamDirectionInput
};

enum ControlType
{
    ControlTypeAutomatic,
    ControlTypeManual
};

enum ControlStatus
{
    ControlStatusOn,
    ControlStatusOff
};

enum ControlValueType
{
    ControlValueTypeAbsolute,
    ControlValueTypeNative,
};

class CaptureCMIOPrivate
{
    public:
        id m_deviceObserver {nil};
        CMSimpleQueueRef m_queue {nullptr};
        CMSampleBufferRef m_curFrame {nil};
        CMIODeviceID m_deviceID {kCMIODeviceUnknown};
        CMIOStreamID m_streamID {kCMIOStreamUnknown};
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, QVariantList> m_devicesCaps;
        QMap<QString, CMIODeviceID> m_devicesID;
        int m_nBuffers {32};
        QMutex m_mutex;
        QMutex m_controlsMutex;
        QWaitCondition m_frameReady;
        AkFrac m_fps;
        AkFrac m_timeBase;
        AkCaps m_caps;
        qint64 m_id {-1};
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;

        CaptureCMIOPrivate();
        static bool canUseCamera();
        static inline QString fourccToStr(FourCharCode format);
        static inline FourCharCode strToFourCC(const QString &format);
        CMVideoFormatDescriptionRef formatFromCaps(CMIODeviceID deviceID,
                                                   const AkCaps &caps,
                                                   CMIOStreamID *streamID);
        static inline const FourCharCodeToStrMap &fourccToStrMap();
        AkCaps capsFromFrameSampleBuffer(const CMSampleBufferRef sampleBuffer) const;
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        QVector<CMIODeviceID> devices() const;
        QString deviceUID(CMIODeviceID deviceID) const;
        QString objectName(CMIOObjectID objectID) const;
        QVector<CMIOStreamID> deviceStreams(CMIODeviceID deviceID) const;
        QVector<Float64> streamFrameRates(CMIOStreamID streamID,
                                          CMVideoFormatDescriptionRef formatDescription=nullptr) const;
        AkCaps capsFromDescription(CMVideoFormatDescriptionRef formatDescription) const;
        StreamDirection streamDirection(CMIOStreamID streamID) const;
        CFArrayRef formatDescriptions(CMIOStreamID streamID) const;
        CMIOClassID objectClassID(CMIOObjectID objectID) const;
        QVector<CMIOControlID> deviceControls(CMIODeviceID deviceID) const;
        ControlType controlType(CMIOControlID controlID, Boolean *isSettable) const;
        void setControlType(CMIOControlID controlID, ControlType type) const;
        ControlStatus controlStatus(CMIOControlID controlID, Boolean *isSettable) const;
        void setControlStatus(CMIOControlID controlID, ControlStatus status) const;
        ControlValueType controlValueType(CMIOControlID controlID) const;
        void controlRange(CMIOControlID controlID,
                          Float64 *min,
                          Float64 *max) const;
        Float64 controlValue(CMIOControlID controlID) const;
        void setControlValue(CMIOControlID controlID, Float64 value) const;
        void setFormatDescription(CMIOStreamID streamID,
                                  CMVideoFormatDescriptionRef formatDescription);
        void setFrameRate(CMIOStreamID streamID, Float64 frameRate);
        static void streamQueueAltered(CMIOStreamID streamID,
                                       void *token,
                                       void *refCon);
        QVariantList controls(CMIODeviceID deviceID) const;
        bool setControls(CMIODeviceID deviceID, const QVariantMap &controls) const;
};

CaptureCMIO::CaptureCMIO(QObject *parent):
    Capture(parent)
{
    this->d = new CaptureCMIOPrivate();
    this->d->m_deviceObserver = [[DeviceObserverCMIO alloc]
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

CaptureCMIO::~CaptureCMIO()
{
    this->uninit();

    [[NSNotificationCenter defaultCenter]
     removeObserver: this->d->m_deviceObserver];

    [this->d->m_deviceObserver disconnect];
    [this->d->m_deviceObserver release];

    delete this->d;
}

QStringList CaptureCMIO::webcams() const
{
    return this->d->m_devices;
}

QString CaptureCMIO::device() const
{
    return this->d->m_device;
}

QList<int> CaptureCMIO::streams()
{
    if (!this->d->m_streams.isEmpty())
        return this->d->m_streams;

    auto caps = this->caps(this->d->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int> {0};
}

QList<int> CaptureCMIO::listTracks(const QString &mimeType)
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

QString CaptureCMIO::ioMethod() const
{
    return {};
}

int CaptureCMIO::nBuffers() const
{
    return this->d->m_nBuffers;
}

QString CaptureCMIO::description(const QString &webcam) const
{
    return this->d->m_descriptions.value(webcam);
}

QVariantList CaptureCMIO::caps(const QString &webcam) const
{
    return this->d->m_devicesCaps.value(webcam);
}

QString CaptureCMIO::capsDescription(const AkCaps &caps) const
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

QVariantList CaptureCMIO::imageControls() const
{
    return this->d->m_globalImageControls;
}

bool CaptureCMIO::setImageControls(const QVariantMap &imageControls)
{
    this->d->m_controlsMutex.lock();
    auto globalImageControls = this->d->m_globalImageControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalImageControls.count(); i++) {
        auto control = globalImageControls[i].toList();
        auto controlName = control[0].toString();

        if (imageControls.contains(controlName)) {
            control[6] = imageControls[controlName];
            globalImageControls[i] = control;
        }
    }

    this->d->m_controlsMutex.lock();

    if (this->d->m_globalImageControls == globalImageControls) {
        this->d->m_controlsMutex.unlock();

        return false;
    }

    this->d->m_globalImageControls = globalImageControls;
    this->d->m_controlsMutex.unlock();

    emit this->imageControlsChanged(imageControls);

    return true;
}

bool CaptureCMIO::resetImageControls()
{
    QVariantMap controls;

    for (auto &control: this->imageControls()) {
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList CaptureCMIO::cameraControls() const
{
    return this->d->m_globalCameraControls;
}

bool CaptureCMIO::setCameraControls(const QVariantMap &cameraControls)
{
    this->d->m_controlsMutex.lock();
    auto globalCameraControls = this->d->m_globalCameraControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalCameraControls.count(); i++) {
        auto control = globalCameraControls[i].toList();
        auto controlName = control[0].toString();

        if (cameraControls.contains(controlName)) {
            control[6] = cameraControls[controlName];
            globalCameraControls[i] = control;
        }
    }

    this->d->m_controlsMutex.lock();

    if (this->d->m_globalCameraControls == globalCameraControls) {
        this->d->m_controlsMutex.unlock();

        return false;
    }

    this->d->m_globalCameraControls = globalCameraControls;
    this->d->m_controlsMutex.unlock();

    emit this->cameraControlsChanged(cameraControls);

    return true;
}

bool CaptureCMIO::resetCameraControls()
{
    QVariantMap controls;

    for (auto &control: this->cameraControls()) {
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket CaptureCMIO::readFrame()
{
    this->d->m_mutex.lock();

#if defined(ENABLE_CONTROLS) && ENABLE_CONTROLS != 0
    this->d->m_controlsMutex.lock();
    auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localImageControls != imageControls) {
        auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                         imageControls);
        this->d->setControls(this->d->m_deviceID, controls);
        this->d->m_localImageControls = imageControls;
    }
#endif

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
        size_t lengthAtOffset = 0;
        size_t dataSize = 0;
        char *data = nullptr;
        CMBlockBufferGetDataPointer(dataBuffer,
                                    0,
                                    &lengthAtOffset,
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

QVariantMap CaptureCMIO::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlName = params[0].toString();
        controlStatus[controlName] = params[0];
    }

    return controlStatus;
}

bool CaptureCMIO::init()
{
    this->d->m_localImageControls.clear();
    this->d->m_localCameraControls.clear();

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

    auto deviceID = this->d->m_devicesID.value(webcam, kCMIODeviceUnknown);

    if (deviceID == kCMIODeviceUnknown)
        return false;

    AkCaps caps = streams[0] < supportedCaps.size()?
                    supportedCaps[streams[0]].value<AkCaps>():
                    supportedCaps.first().value<AkCaps>();

    // Configure camera format.
    CMIOStreamID streamID = kCMIOStreamUnknown;
    auto format = this->d->formatFromCaps(deviceID, caps, &streamID);

    if (!format)
        return false;

    this->d->setFormatDescription(streamID, format);
    CFRelease(format);
    AkFrac fps = caps.property("fps").toString();
    this->d->setFrameRate(streamID, fps.value());

    // Start capturing from the camera.
    this->d->m_queue = nullptr;
    CMIOStreamCopyBufferQueue(streamID,
                              CaptureCMIOPrivate::streamQueueAltered,
                              this->d,
                              &this->d->m_queue);
    auto status = CMIODeviceStartStream(deviceID, streamID);

    if (status != kCMIOHardwareNoError)
        return false;

    this->d->m_deviceID = deviceID;
    this->d->m_streamID = streamID;
    this->d->m_caps = caps;
    this->d->m_timeBase = fps.invert();
    this->d->m_id = Ak::id();

    return true;
}

void CaptureCMIO::uninit()
{
    if (this->d->m_deviceID != kCMIODeviceUnknown
        && this->d->m_streamID != kCMIOStreamUnknown) {
        CMIODeviceStopStream(this->d->m_deviceID, this->d->m_streamID);
    }

    this->d->m_deviceID = kCMIODeviceUnknown;
    this->d->m_streamID = kCMIOStreamUnknown;

    if (this->d->m_queue) {
        CFRelease(this->d->m_queue);
        this->d->m_queue = nullptr;
    }
}

void CaptureCMIO::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;

    if (device.isEmpty()) {
        this->d->m_controlsMutex.lock();
        this->d->m_globalImageControls.clear();
        this->d->m_globalCameraControls.clear();
        this->d->m_controlsMutex.unlock();
    } else {
        this->d->m_controlsMutex.lock();

#if defined(ENABLE_CONTROLS) && ENABLE_CONTROLS != 0
        auto deviceID = this->d->m_devicesID.value(device, kCMIODeviceUnknown);

        if (deviceID != kCMIODeviceUnknown)
            this->d->m_globalImageControls = this->d->controls(deviceID);
#endif

        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lock();
    auto imageStatus = this->d->controlStatus(this->d->m_globalImageControls);
    auto cameraStatus = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    emit this->deviceChanged(device);
    emit this->imageControlsChanged(imageStatus);
    emit this->cameraControlsChanged(cameraStatus);
}

void CaptureCMIO::setStreams(const QList<int> &streams)
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

void CaptureCMIO::setIoMethod(const QString &ioMethod)
{
    Q_UNUSED(ioMethod)
}

void CaptureCMIO::setNBuffers(int nBuffers)
{
    if (this->d->m_nBuffers == nBuffers)
        return;

    this->d->m_nBuffers = nBuffers;
    emit this->nBuffersChanged(nBuffers);
}

void CaptureCMIO::resetDevice()
{
    this->setDevice(this->d->m_devices.value(0, ""));
}

void CaptureCMIO::resetStreams()
{
    QVariantList supportedCaps = this->caps(this->d->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void CaptureCMIO::resetIoMethod()
{
    this->setIoMethod("any");
}

void CaptureCMIO::resetNBuffers()
{
    this->setNBuffers(32);
}

void CaptureCMIO::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

void CaptureCMIO::cameraConnected()
{
    this->updateDevices();
}

void CaptureCMIO::cameraDisconnected()
{
    this->updateDevices();
}

void CaptureCMIO::updateDevices()
{
    if (!CaptureCMIOPrivate::canUseCamera())
        return;

    decltype(this->d->m_devices) devices;
    decltype(this->d->m_descriptions) descriptions;
    decltype(this->d->m_devicesCaps) devicesCaps;
    decltype(this->d->m_devicesID) devicesID;

    if (this->d->m_devices != devices) {
        this->d->m_devices = devices;
        emit this->webcamsChanged(devices);
    }

    this->d->m_devices = devices;
    this->d->m_descriptions = descriptions;
    this->d->m_devicesCaps = devicesCaps;
    this->d->m_devicesID = devicesID;

    for (auto &id: this->d->devices()) {
        auto deviceUID = this->d->deviceUID(id);
        QVariantList caps;

        for (auto &stream: this->d->deviceStreams(id)) {
            if (this->d->streamDirection(stream) != StreamDirectionOutput)
                continue;

            auto formats = this->d->formatDescriptions(stream);

            if (!formats)
                continue;

            for (CFIndex i = 0; i < CFArrayGetCount(formats); i++) {
                auto format =
                        reinterpret_cast<CMVideoFormatDescriptionRef>(CFArrayGetValueAtIndex(formats,
                                                                                             i));
                auto videoCaps = this->d->capsFromDescription(format);
                auto frameRates = this->d->streamFrameRates(stream, format);

                if (!videoCaps)
                    continue;

                for (auto &fpsRange: frameRates) {
                    videoCaps.setProperty("fps",
                                          AkFrac(qRound(1e3 * fpsRange),
                                                 1e3).toString());
                    caps << QVariant::fromValue(videoCaps);
                }
            }

            CFRelease(formats);
        }

        if (!caps.isEmpty()) {
            devices << deviceUID;
            descriptions[deviceUID] = this->d->objectName(id);
            devicesID[deviceUID] = id;
            devicesCaps[deviceUID] = caps;
        }
    }

    if (devicesCaps.isEmpty()) {
        devices.clear();
        descriptions.clear();
        devicesID.clear();
    }

    this->d->m_descriptions = descriptions;
    this->d->m_devicesCaps = devicesCaps;
    this->d->m_devicesID = devicesID;

    if (this->d->m_devices != devices) {
        this->d->m_devices = devices;
        emit this->webcamsChanged(devices);
    }
}

CaptureCMIOPrivate::CaptureCMIOPrivate()
{

}

bool CaptureCMIOPrivate::canUseCamera()
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

QString CaptureCMIOPrivate::fourccToStr(FourCharCode format)
{
    char fourcc[5];
    memcpy(fourcc, &format, sizeof(FourCharCode));
    fourcc[4] = 0;

    return QString(fourcc);
}

FourCharCode CaptureCMIOPrivate::strToFourCC(const QString &format)
{
    FourCharCode fourcc;
    memcpy(&fourcc, format.toStdString().c_str(), sizeof(FourCharCode));

    return fourcc;
}

CMVideoFormatDescriptionRef CaptureCMIOPrivate::formatFromCaps(CMIODeviceID deviceID,
                                                               const AkCaps &caps,
                                                               CMIOStreamID *streamID)
{
    if (streamID)
        *streamID = kCMIOStreamUnknown;

    CMVideoFormatDescriptionRef formatDescription = nullptr;
    bool exitLoop = false;
    auto streams = this->deviceStreams(deviceID);

    for (auto &stream: streams) {
        if (this->streamDirection(stream) != StreamDirectionOutput)
            continue;

        auto formats = this->formatDescriptions(stream);

        for (CFIndex i = 0; i < CFArrayGetCount(formats); i++) {
            auto format =
                    reinterpret_cast<CMVideoFormatDescriptionRef>(CFArrayGetValueAtIndex(formats,
                                                                                         i));
            auto videoCaps = this->capsFromDescription(format);
            auto frameRates = this->streamFrameRates(stream);

            if (!videoCaps)
                continue;

            for (auto &fpsRange: frameRates) {
                videoCaps.setProperty("fps",
                                      AkFrac(qRound(1e3 * fpsRange),
                                             1e3).toString());

                if (videoCaps == caps) {
                    CFRetain(format);
                    formatDescription = format;

                    if (streamID)
                        *streamID = stream;

                    exitLoop = true;

                    break;
                }
            }
        }

        CFRelease(formats);

        if (exitLoop)
            break;
    }

    return formatDescription;
}

const FourCharCodeToStrMap &CaptureCMIOPrivate::fourccToStrMap()
{
    static const FourCharCodeToStrMap fourccToStrMap {
        // Raw formats
        {kCMPixelFormat_32ARGB         , "ARGB"    },
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

AkCaps CaptureCMIOPrivate::capsFromFrameSampleBuffer(const CMSampleBufferRef sampleBuffer) const
{
    auto formatDesc = CMSampleBufferGetFormatDescription(sampleBuffer);

    AkCaps videoCaps;
    videoCaps.setMimeType("video/unknown");

    auto fourCC = CMFormatDescriptionGetMediaSubType(formatDesc);
    auto &map = CaptureCMIOPrivate::fourccToStrMap();
    videoCaps.setProperty("fourcc",
                          map.value(fourCC,
                                    CaptureCMIOPrivate::fourccToStr(fourCC)));

    auto size = CMVideoFormatDescriptionGetDimensions(formatDesc);
    videoCaps.setProperty("width", size.width);
    videoCaps.setProperty("height", size.height);

    return videoCaps;
}

QVariantMap CaptureCMIOPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlName = params[0].toString();
        controlStatus[controlName] = params[6];
    }

    return controlStatus;
}

QVariantMap CaptureCMIOPrivate::mapDiff(const QVariantMap &map1,
                                                const QVariantMap &map2) const
{
    QVariantMap map;

    for (auto it = map2.cbegin(); it != map2.cend(); it++)
        if (!map1.contains(it.key())
            || map1[it.key()] != it.value()) {
            map[it.key()] = it.value();
        }

    return map;
}

QVector<CMIODeviceID> CaptureCMIOPrivate::devices() const
{
    CMIOObjectPropertyAddress devicesProperty {
        kCMIOHardwarePropertyDevices,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 devicesSize = 0;
    auto status =
        CMIOObjectGetPropertyDataSize(kCMIOObjectSystemObject,
                                      &devicesProperty,
                                      0,
                                      nullptr,
                                      &devicesSize);

    if (status != kCMIOHardwareNoError)
        return {};

    QVector<CMIODeviceID> devices(devicesSize / sizeof(CMIODeviceID));
    status =
        CMIOObjectGetPropertyData(kCMIOObjectSystemObject,
                                  &devicesProperty,
                                  0,
                                  nullptr,
                                  devicesSize,
                                  &devicesSize,
                                  devices.data());

    return status == kCMIOHardwareNoError? devices: QVector<CMIODeviceID>();
}

QString CaptureCMIOPrivate::deviceUID(CMIODeviceID deviceID) const
{
    CMIOObjectPropertyAddress deviceUIDProperty {
        kCMIODevicePropertyDeviceUID,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 deviceUIDSize = sizeof(CFStringRef);
    CFStringRef deviceUID = nullptr;
    auto status =
        CMIOObjectGetPropertyData(deviceID,
                                  &deviceUIDProperty,
                                  0,
                                  nullptr,
                                  deviceUIDSize,
                                  &deviceUIDSize,
                                  &deviceUID);

    if (status != kCMIOHardwareNoError)
        return {};

    auto uid = QString::fromCFString(deviceUID);
    CFRelease(deviceUID);

    return uid;
}

QString CaptureCMIOPrivate::objectName(CMIOObjectID objectID) const
{
    CMIOObjectPropertyAddress objectNameProperty {
        kCMIOObjectPropertyName,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 objectNameSize = sizeof(CFStringRef);
    CFStringRef objectName = nullptr;
    auto status =
        CMIOObjectGetPropertyData(objectID,
                                  &objectNameProperty,
                                  0,
                                  nullptr,
                                  objectNameSize,
                                  &objectNameSize,
                                  &objectName);

    if (status != kCMIOHardwareNoError)
        return {};

    auto name = QString::fromCFString(objectName);
    CFRelease(objectName);

    return name;
}

QVector<CMIOStreamID> CaptureCMIOPrivate::deviceStreams(CMIODeviceID deviceID) const
{
    CMIOObjectPropertyAddress streamsProperty {
        kCMIODevicePropertyStreams,
        kCMIODevicePropertyScopeInput,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 deviceStreamsSize = 0;
    auto status =
        CMIOObjectGetPropertyDataSize(deviceID,
                                      &streamsProperty,
                                      0,
                                      nullptr,
                                      &deviceStreamsSize);

    if (status != kCMIOHardwareNoError)
        return {};

    QVector<CMIOStreamID> streams(deviceStreamsSize / sizeof(CMIOStreamID));
    status =
        CMIOObjectGetPropertyData(deviceID,
                                  &streamsProperty,
                                  0,
                                  nullptr,
                                  deviceStreamsSize,
                                  &deviceStreamsSize,
                                  streams.data());

    return status == kCMIOHardwareNoError? streams: QVector<CMIOStreamID>();
}

QVector<Float64> CaptureCMIOPrivate::streamFrameRates(CMIOStreamID streamID,
                                                      CMVideoFormatDescriptionRef formatDescription) const
{
    UInt32 formatDescriptionSize =
            formatDescription?
                sizeof(CMVideoFormatDescriptionRef):
                0;
    CMIOObjectPropertyAddress frameRatesProperty {
        kCMIOStreamPropertyFrameRates,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 frameRatesSize = 0;
    auto status =
        CMIOObjectGetPropertyDataSize(streamID,
                                      &frameRatesProperty,
                                      formatDescriptionSize,
                                      formatDescription?
                                          &formatDescription:
                                          nullptr,
                                      &frameRatesSize);

    if (status != kCMIOHardwareNoError)
        return {};

    QVector<Float64> frameRates(frameRatesSize / sizeof(Float64));
    status =
        CMIOObjectGetPropertyData(streamID,
                                  &frameRatesProperty,
                                  formatDescriptionSize,
                                  formatDescription?
                                      &formatDescription:
                                      nullptr,
                                  frameRatesSize,
                                  &frameRatesSize,
                                  frameRates.data());

    return status == kCMIOHardwareNoError? frameRates: QVector<Float64>();
}

AkCaps CaptureCMIOPrivate::capsFromDescription(CMVideoFormatDescriptionRef formatDescription) const
{
    auto mediaType = CMFormatDescriptionGetMediaType(formatDescription);

    if (mediaType != kCMMediaType_Video)
        return {};

    auto fourCC = CMFormatDescriptionGetMediaSubType(formatDescription);
    auto size = CMVideoFormatDescriptionGetDimensions(formatDescription);
    auto &map = CaptureCMIOPrivate::fourccToStrMap();
    auto fourccStr = map.value(fourCC, CaptureCMIOPrivate::fourccToStr(fourCC));

    // Ignore formats that can cause alignment problems when capturing.
    if ((size.width % 32) != 0)
        return {};

    AkCaps videoCaps;
    videoCaps.setMimeType("video/unknown");
    videoCaps.setProperty("fourcc", fourccStr);
    videoCaps.setProperty("width", size.width);
    videoCaps.setProperty("height", size.height);

    return videoCaps;
}

StreamDirection CaptureCMIOPrivate::streamDirection(CMIOStreamID streamID) const
{
    CMIOObjectPropertyAddress directionProperty {
        kCMIOStreamPropertyDirection,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 directionSize = sizeof(UInt32);
    UInt32 direction = 0;
    CMIOObjectGetPropertyData(streamID,
                              &directionProperty,
                              0,
                              nullptr,
                              directionSize,
                              &directionSize,
                              &direction);

    return direction? StreamDirectionInput: StreamDirectionOutput;
}

CFArrayRef CaptureCMIOPrivate::formatDescriptions(CMIOStreamID streamID) const
{
    CMIOObjectPropertyAddress formatDescriptionsProperty {
        kCMIOStreamPropertyFormatDescriptions,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 formatDescriptionsSize = sizeof(CFArrayRef);
    CFArrayRef formats = nullptr;
    CMIOObjectGetPropertyData(streamID,
                              &formatDescriptionsProperty,
                              0,
                              nullptr,
                              formatDescriptionsSize,
                              &formatDescriptionsSize,
                              &formats);

    return formats;
}

CMIOClassID CaptureCMIOPrivate::objectClassID(CMIOObjectID objectID) const
{
    CMIOObjectPropertyAddress classIDProperty {
        kCMIOObjectPropertyClass,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 classIDSize = sizeof(CMIOClassID);
    CMIOClassID classID = 0;
    CMIOObjectGetPropertyData(objectID,
                              &classIDProperty,
                              0,
                              nullptr,
                              classIDSize,
                              &classIDSize,
                              &classID);

    return classID;
}

QVector<CMIOControlID> CaptureCMIOPrivate::deviceControls(CMIODeviceID deviceID) const
{
    CMIOObjectPropertyAddress ownedObjectsProperty {
        kCMIOObjectPropertyOwnedObjects,
        0,
        kCMIOObjectPropertyElementMaster
    };
    QVector<CMIOClassID> controlClasses {
        kCMIOFeatureControlClassID
    };
    UInt32 ownedObjectsSize = 0;
    auto status =
        CMIOObjectGetPropertyDataSize(deviceID,
                                      &ownedObjectsProperty,
                                      controlClasses.size() * sizeof(CMIOClassID),
                                      controlClasses.data(),
                                      &ownedObjectsSize);

    if (status != kCMIOHardwareNoError)
        return {};

    QVector<CMIOControlID> deviceControls(ownedObjectsSize / sizeof(CMIOControlID));
    status =
        CMIOObjectGetPropertyData(deviceID,
                                  &ownedObjectsProperty,
                                  controlClasses.size() * sizeof(CMIOClassID),
                                  controlClasses.data(),
                                  ownedObjectsSize,
                                  &ownedObjectsSize,
                                  deviceControls.data());

    return status == kCMIOHardwareNoError?
                deviceControls:
                QVector<CMIOControlID>();
}

ControlType CaptureCMIOPrivate::controlType(CMIOControlID controlID, Boolean *isSettable) const
{
    CMIOObjectPropertyAddress automaticManualProperty {
        kCMIOFeatureControlPropertyAutomaticManual,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 automaticManualSize = sizeof(UInt32);
    UInt32 automaticManual = 0;
    CMIOObjectGetPropertyData(controlID,
                              &automaticManualProperty,
                              0,
                              nullptr,
                              automaticManualSize,
                              &automaticManualSize,
                              &automaticManual);

    if (isSettable) {
        *isSettable = false;
        CMIOObjectIsPropertySettable(controlID,
                                     &automaticManualProperty,
                                     isSettable);
    }

    return automaticManual? ControlTypeAutomatic: ControlTypeManual;
}

void CaptureCMIOPrivate::setControlType(CMIOControlID controlID, ControlType type) const
{
    CMIOObjectPropertyAddress automaticManualProperty {
        kCMIOFeatureControlPropertyAutomaticManual,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 automaticManualSize = sizeof(UInt32);
    UInt32 automaticManual = type == ControlTypeAutomatic? 1: 0;
    CMIOObjectSetPropertyData(controlID,
                              &automaticManualProperty,
                              0,
                              nullptr,
                              automaticManualSize,
                              &automaticManual);
}

ControlStatus CaptureCMIOPrivate::controlStatus(CMIOControlID controlID, Boolean *isSettable) const
{
    CMIOObjectPropertyAddress onOffProperty {
        kCMIOFeatureControlPropertyOnOff,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 onOffSize = sizeof(UInt32);
    UInt32 onOff = 0;
    CMIOObjectGetPropertyData(controlID,
                              &onOffProperty,
                              0,
                              nullptr,
                              onOffSize,
                              &onOffSize,
                              &onOff);

    if (isSettable) {
        *isSettable = false;
        CMIOObjectIsPropertySettable(controlID,
                                     &onOffProperty,
                                     isSettable);
    }

    return onOff? ControlStatusOn: ControlStatusOff;
}

void CaptureCMIOPrivate::setControlStatus(CMIOControlID controlID, ControlStatus status) const
{
    CMIOObjectPropertyAddress onOffProperty {
        kCMIOFeatureControlPropertyOnOff,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 onOffSize = sizeof(UInt32);
    UInt32 onOff = status == ControlStatusOn? 1: 0;
    CMIOObjectSetPropertyData(controlID,
                              &onOffProperty,
                              0,
                              nullptr,
                              onOffSize,
                              &onOff);
}

ControlValueType CaptureCMIOPrivate::controlValueType(CMIOControlID controlID) const
{
    CMIOObjectPropertyAddress absoluteNativeProperty {
        kCMIOFeatureControlPropertyAbsoluteNative,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 absoluteNativeSize = sizeof(UInt32);
    UInt32 absoluteNative = 0;
    CMIOObjectGetPropertyData(controlID,
                              &absoluteNativeProperty,
                              0,
                              nullptr,
                              absoluteNativeSize,
                              &absoluteNativeSize,
                              &absoluteNative);

    return absoluteNative? ControlValueTypeAbsolute: ControlValueTypeNative;
}

void CaptureCMIOPrivate::controlRange(CMIOControlID controlID,
                                      Float64 *min,
                                      Float64 *max) const
{
    if (!min && !max)
        return;

    auto valueType = this->controlValueType(controlID);
    CMIOObjectPropertyAddress rangeProperty {
        valueType == ControlValueTypeAbsolute?
            kCMIOFeatureControlPropertyAbsoluteRange:
            kCMIOFeatureControlPropertyNativeRange,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 rangeSize = sizeof(AudioValueRange);
    AudioValueRange range;
    auto status =
        CMIOObjectGetPropertyData(controlID,
                                  &rangeProperty,
                                  0,
                                  nullptr,
                                  rangeSize,
                                  &rangeSize,
                                  &range);

    if (status == kCMIOHardwareNoError) {
        if (min)
            *min = range.mMinimum;

        if (max)
            *max = range.mMaximum;
    } else {
        if (min)
            *min = 0;

        if (max)
            *max = 0;
    }
}

Float64 CaptureCMIOPrivate::controlValue(CMIOControlID controlID) const
{
    auto valueType = this->controlValueType(controlID);
    CMIOObjectPropertyAddress valueProperty {
        valueType == ControlValueTypeAbsolute?
            kCMIOFeatureControlPropertyAbsoluteValue:
            kCMIOFeatureControlPropertyNativeValue,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 valueSize = sizeof(Float32);
    Float32 value;
    CMIOObjectGetPropertyData(controlID,
                              &valueProperty,
                              0,
                              nullptr,
                              valueSize,
                              &valueSize,
                              &value);

    return value;
}

void CaptureCMIOPrivate::setControlValue(CMIOControlID controlID,
                                         Float64 value) const
{
    auto valueType = this->controlValueType(controlID);
    CMIOObjectPropertyAddress valueProperty {
        valueType == ControlValueTypeAbsolute?
            kCMIOFeatureControlPropertyAbsoluteValue:
            kCMIOFeatureControlPropertyNativeValue,
        0,
        kCMIOObjectPropertyElementMaster
    };
    UInt32 valueSize = sizeof(Float32);
    Float32 val = value;
    CMIOObjectSetPropertyData(controlID,
                              &valueProperty,
                              0,
                              nullptr,
                              valueSize,
                              &val);
}

void CaptureCMIOPrivate::setFormatDescription(CMIOStreamID streamID,
                                              CMVideoFormatDescriptionRef formatDescription)
{
    CMIOObjectPropertyAddress formatDescriptionProperty {
        kCMIOStreamPropertyFormatDescription,
        0,
        kCMIOObjectPropertyElementMaster
    };
    CMIOObjectSetPropertyData(streamID,
                              &formatDescriptionProperty,
                              0,
                              nullptr,
                              sizeof(CMVideoFormatDescriptionRef),
                              &formatDescription);
}

void CaptureCMIOPrivate::setFrameRate(CMIOStreamID streamID, Float64 frameRate)
{
    CMIOObjectPropertyAddress frameRateProperty {
        kCMIOStreamPropertyFrameRate,
        0,
        kCMIOObjectPropertyElementMaster
    };
    CMIOObjectSetPropertyData(streamID,
                              &frameRateProperty,
                              0,
                              nullptr,
                              sizeof(Float64),
                              &frameRate);
}

void CaptureCMIOPrivate::streamQueueAltered(CMIOStreamID streamID,
                                            void *token,
                                            void *refCon)
{
    Q_UNUSED(streamID)
    Q_UNUSED(token)
    auto self = reinterpret_cast<CaptureCMIOPrivate *>(refCon);

    forever {
        auto videoFrame = CMSampleBufferRef(CMSimpleQueueDequeue(self->m_queue));

        if (!videoFrame)
            break;

        self->m_mutex.lock();

        if (self->m_curFrame)
            CFRelease(self->m_curFrame);

        self->m_curFrame = videoFrame;
        CFRetain(videoFrame);

        self->m_mutex.unlock();
    }
}

QVariantList CaptureCMIOPrivate::controls(CMIODeviceID deviceID) const
{
    QVariantList controls;

    auto deviceControls = this->deviceControls(deviceID);

    for (auto control: deviceControls) {
        Boolean isTypeSettable = false;
        auto type = this->controlType(control, &isTypeSettable);
        Boolean isStatusSettable = false;
        auto status = this->controlStatus(control, &isStatusSettable);
        Float64 min = 0;
        Float64 max = 0;
        this->controlRange(control, &min, &max);

        if (!qIsFinite(min) || !qIsFinite(min))
            continue;

        auto value = this->controlValue(control);

        if (isTypeSettable && !isStatusSettable) {
            controls << QVariant(QVariantList {
                this->objectName(control) + " (Auto)",
                "boolean",
                0,
                1,
                1,
                1,
                type == ControlTypeAutomatic,
                {}
            });
        }

        if (isStatusSettable) {
            controls << QVariant(QVariantList {
                (min != max? "Enable ": "")
                + this->objectName(control),
                "boolean",
                0,
                1,
                1,
                1,
                status == ControlStatusOn,
                {}
            });

            if (min != max)
                controls << QVariant(QVariantList {
                    this->objectName(control),
                    "integer",
                    qRound(min),
                    qRound(max),
                    1,             // step
                    qRound(value), // default
                    qRound(value),
                    {}
                });
        } else {
            controls << QVariant(QVariantList {
                this->objectName(control),
                "integer",
                qRound(min),
                qRound(max),
                1,             // step
                qRound(value), // default
                qRound(value),
                {}
            });
        }
    }

    return controls;
}

bool CaptureCMIOPrivate::setControls(CMIODeviceID deviceID,
                                     const QVariantMap &controls) const
{
    bool set = false;
    auto deviceControls = this->deviceControls(deviceID);

    for (auto control: deviceControls) {
        Boolean isTypeSettable = false;
        this->controlType(control, &isTypeSettable);
        Boolean isStatusSettable = false;
        this->controlStatus(control, &isStatusSettable);
        Float64 min = 0;
        Float64 max = 0;
        this->controlRange(control, &min, &max);

        if (!qIsFinite(min) || !qIsFinite(min))
            continue;

        if (isTypeSettable && !isStatusSettable) {
            auto controlName = this->objectName(control) + " (Auto)";

            if (controls.contains(controlName)) {
                auto value = controls.value(controlName).toBool();
                this->setControlType(control,
                                     value?
                                         ControlTypeAutomatic:
                                         ControlTypeManual);
                set |= true;
            }
        }

        if (isStatusSettable) {
            auto controlName =
                    (min != max? "Enable ": "")
                        + this->objectName(control);

            if (controls.contains(controlName)) {
                auto value = controls.value(controlName).toBool();
                this->setControlStatus(control,
                                       value?
                                           ControlStatusOn:
                                           ControlStatusOff);
                set |= true;
            }

            if (min != max) {
                auto controlName = this->objectName(control);

                if (controls.contains(controlName)) {
                    auto value = controls.value(controlName).toDouble();
                    this->setControlValue(control, value);
                    set |= true;
                }
            }
        } else {
            auto controlName = this->objectName(control);

            if (controls.contains(controlName)) {
                auto value = controls.value(controlName).toDouble();
                this->setControlValue(control, value);
                set |= true;
            }
        }
    }

    return set;
}

#include "moc_capturecmio.cpp"

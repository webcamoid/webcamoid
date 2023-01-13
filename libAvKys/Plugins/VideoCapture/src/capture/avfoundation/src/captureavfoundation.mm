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

#include <QCoreApplication>
#include <QMap>
#include <QMutex>
#include <QReadWriteLock>
#include <QVariant>
#include <QWaitCondition>
#include <QtDebug>
#include <ak.h>
#include <akcaps.h>
#include <akcompressedvideocaps.h>
#include <akcompressedvideopacket.h>
#include <akelement.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideopacket.h>
#include <sys/time.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMediaIO/CMIOHardwarePlugIn.h>

#include "captureavfoundation.h"
#include "deviceobserver.h"

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

using RawFmtToAkFmtMap = QMap<FourCharCode, AkVideoCaps::PixelFormat>;

inline RawFmtToAkFmtMap initRawFmtToAkFmt()
{
    RawFmtToAkFmtMap rawFmtToAkFmt {
        {kCVPixelFormatType_16BE555                          , AkVideoCaps::Format_rgb555be     },
        {kCVPixelFormatType_16LE555                          , AkVideoCaps::Format_rgb555be     },
        {kCVPixelFormatType_16LE5551                         , AkVideoCaps::Format_rgb5550le    },
        {kCVPixelFormatType_16BE565                          , AkVideoCaps::Format_rgb565be     },
        {kCVPixelFormatType_16LE565                          , AkVideoCaps::Format_rgb565le     },
        {kCVPixelFormatType_24RGB                            , AkVideoCaps::Format_rgb24        },
        {kCVPixelFormatType_24BGR                            , AkVideoCaps::Format_bgr24        },
        {kCVPixelFormatType_32ARGB                           , AkVideoCaps::Format_argb         },
        {kCVPixelFormatType_32BGRA                           , AkVideoCaps::Format_bgra         },
        {kCVPixelFormatType_32ABGR                           , AkVideoCaps::Format_abgr         },
        {kCVPixelFormatType_32RGBA                           , AkVideoCaps::Format_rgba         },
        {kCVPixelFormatType_64ARGB                           , AkVideoCaps::Format_argb64be     },
        {kCVPixelFormatType_48RGB                            , AkVideoCaps::Format_rgb48be      },
        {kCVPixelFormatType_32AlphaGray                      , AkVideoCaps::Format_graya16be    },
        {kCVPixelFormatType_16Gray                           , AkVideoCaps::Format_gray16be     },
        {kCVPixelFormatType_30RGB                            , AkVideoCaps::Format_rgb30be      },
        {kCVPixelFormatType_422YpCbCr8                       , AkVideoCaps::Format_uyvy422      },
        {kCVPixelFormatType_4444YpCbCrA8                     , AkVideoCaps::Format_uyva         },
        {kCVPixelFormatType_4444YpCbCrA8R                    , AkVideoCaps::Format_ayuv         },
        {kCVPixelFormatType_4444AYpCbCr8                     , AkVideoCaps::Format_ayuv         },
        {kCVPixelFormatType_4444AYpCbCr16                    , AkVideoCaps::Format_ayuv64le     },
        {kCVPixelFormatType_444YpCbCr8                       , AkVideoCaps::Format_yuv24        },
        {kCVPixelFormatType_422YpCbCr16                      , AkVideoCaps::Format_yuyv422_32   },
        {kCVPixelFormatType_422YpCbCr10                      , AkVideoCaps::Format_yuyv422_32_10},
        {kCVPixelFormatType_444YpCbCr10                      , AkVideoCaps::Format_yuv30        },
        {kCVPixelFormatType_422YpCbCr_4A_8BiPlanar           , AkVideoCaps::Format_uyvy422a     },
        {kCVPixelFormatType_422YpCbCr8_yuvs                  , AkVideoCaps::Format_yuyv422      },
        {kCVPixelFormatType_422YpCbCr8FullRange              , AkVideoCaps::Format_yuyv422      },
        {kCVPixelFormatType_OneComponent8                    , AkVideoCaps::Format_gray8        },
        {kCVPixelFormatType_ARGB2101010LEPacked              , AkVideoCaps::Format_argb2101010le},
        {kCVPixelFormatType_OneComponent10                   , AkVideoCaps::Format_gray10       },
        {kCVPixelFormatType_OneComponent12                   , AkVideoCaps::Format_gray12       },
        {kCVPixelFormatType_OneComponent16                   , AkVideoCaps::Format_gray16       },
        {kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange    , AkVideoCaps::Format_p010         },
        {kCVPixelFormatType_422YpCbCr10BiPlanarVideoRange    , AkVideoCaps::Format_p210         },
        {kCVPixelFormatType_444YpCbCr10BiPlanarVideoRange    , AkVideoCaps::Format_p410         },
        {kCVPixelFormatType_420YpCbCr10BiPlanarFullRange     , AkVideoCaps::Format_p010         },
        {kCVPixelFormatType_422YpCbCr10BiPlanarFullRange     , AkVideoCaps::Format_p210         },
        {kCVPixelFormatType_444YpCbCr10BiPlanarFullRange     , AkVideoCaps::Format_p410         },
        {kCVPixelFormatType_420YpCbCr8VideoRange_8A_TriPlanar, AkVideoCaps::Format_nv12a        }, // first and second planes as per 420YpCbCr8BiPlanarVideoRange (420v), alpha 8 bits in third plane full-range.  No CVPlanarPixelBufferInfo struct.
    };

    return rawFmtToAkFmt;
}

Q_GLOBAL_STATIC_WITH_ARGS(RawFmtToAkFmtMap,
                          rawFmtToAkFmt,
                          (initRawFmtToAkFmt()))

using CompressedFormatToStrMap = QMap<FourCharCode, QString>;

inline CompressedFormatToStrMap initCompressedFormatToStr()
{
    CompressedFormatToStrMap compressedFormatToStr {
        {kCMVideoCodecType_Animation          , "qtrle"   },
        {kCMVideoCodecType_Cinepak            , "cinepack"},
        {kCMVideoCodecType_JPEG               , "jpeg"    },
        {kCMVideoCodecType_JPEG_OpenDML       , "mjpg"    },
        {kCMVideoCodecType_SorensonVideo      , "svq1"    },
        {kCMVideoCodecType_SorensonVideo3     , "svq3"    },
        {kCMVideoCodecType_H263               , "h263"    },
        {kCMVideoCodecType_H264               , "h264"    },
        {kCMVideoCodecType_HEVC               , "hevc"    },
        {kCMVideoCodecType_HEVCWithAlpha      , "hevc"    },
        {kCMVideoCodecType_MPEG4Video         , "mpg4"    },
        {kCMVideoCodecType_MPEG2Video         , "mpg2"    },
        {kCMVideoCodecType_MPEG1Video         , "mpg1"    },
        {kCMVideoCodecType_VP9                , "vp9"     },
        {kCMVideoCodecType_DVCNTSC            , "dvvideo" },
        {kCMVideoCodecType_DVCPAL             , "dvvideo" },
        {kCMVideoCodecType_DVCProPAL          , "dvvideo" },
        {kCMVideoCodecType_DVCPro50NTSC       , "dvvideo" },
        {kCMVideoCodecType_DVCPro50PAL        , "dvvideo" },
        {kCMVideoCodecType_DVCPROHD720p60     , "dvvideo" },
        {kCMVideoCodecType_DVCPROHD720p50     , "dvvideo" },
        {kCMVideoCodecType_DVCPROHD1080i60    , "dvvideo" },
        {kCMVideoCodecType_DVCPROHD1080i50    , "dvvideo" },
        {kCMVideoCodecType_DVCPROHD1080p30    , "dvvideo" },
        {kCMVideoCodecType_DVCPROHD1080p25    , "dvvideo" },
        {kCMVideoCodecType_AppleProRes4444XQ  , "prores"  },
        {kCMVideoCodecType_AppleProRes4444    , "prores"  },
        {kCMVideoCodecType_AppleProRes422HQ   , "prores"  },
        {kCMVideoCodecType_AppleProRes422     , "prores"  },
        {kCMVideoCodecType_AppleProRes422LT   , "prores"  },
        {kCMVideoCodecType_AppleProRes422Proxy, "prores"  },
        {kCMVideoCodecType_AppleProResRAW     , "prores"  },
        {kCMVideoCodecType_AppleProResRAWHQ   , "prores"  },
    };

    return compressedFormatToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(CompressedFormatToStrMap,
                          compressedFormatToStr,
                          (initCompressedFormatToStr()))

class CaptureAvFoundationPrivate
{
    public:
        CaptureAvFoundation *self;
        bool m_enableHwControls {false};
        id m_deviceObserver {nil};
        AVCaptureDeviceInput *m_deviceInput {nil};
        AVCaptureVideoDataOutput *m_dataOutput {nil};
        AVCaptureSession *m_session {nil};
        CMSampleBufferRef m_curFrame {nil};
        CMIODeviceID m_deviceID {kCMIODeviceUnknown};
        AkElementPtr m_hslFilter {akPluginManager->create<AkElement>("VideoFilter/AdjustHSL")};
        AkElementPtr m_contrastFilter {akPluginManager->create<AkElement>("VideoFilter/Contrast")};
        AkElementPtr m_gammaFilter {akPluginManager->create<AkElement>("VideoFilter/Gamma")};
        QString m_device;
        QList<int> m_streams;
        QMap<QString, CMIODeviceID> m_cmioIDs;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, CaptureVideoCaps> m_devicesCaps;
        int m_nBuffers {32};
        QMutex m_mutex;
        QReadWriteLock m_controlsMutex;
        QWaitCondition m_frameReady;
        AkFrac m_fps;
        AkFrac m_timeBase;
        AkCaps m_caps;
        qint64 m_id {-1};
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;

        CaptureAvFoundationPrivate(CaptureAvFoundation *self);
        static bool canUseCamera();
        QVector<CMIODeviceID> cmioDevices() const;
        QString deviceUID(CMIODeviceID deviceID) const;
        QString objectName(CMIOObjectID objectID) const;
        static inline AVCaptureDeviceFormat *formatFromCaps(AVCaptureDevice *camera,
                                                            const AkCaps &caps);
        static inline AVFrameRateRange *frameRateRangeFromFps(AVCaptureDeviceFormat *format,
                                                              const AkFrac &fps);
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
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
        QVariantList controls(CMIODeviceID deviceID) const;
        bool setControls(CMIODeviceID deviceID, const QVariantMap &controls) const;
};

CaptureAvFoundation::CaptureAvFoundation(QObject *parent):
    Capture(parent)
{
    this->d = new CaptureAvFoundationPrivate(this);
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

QList<int> CaptureAvFoundation::listTracks(AkCaps::CapsType type)
{
    if (type != AkCaps::CapsVideo && type != AkCaps::CapsUnknown)
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

CaptureVideoCaps CaptureAvFoundation::caps(const QString &webcam) const
{
    return this->d->m_devicesCaps.value(webcam);
}

QVariantList CaptureAvFoundation::imageControls() const
{
    if (this->d->m_enableHwControls)
        return this->d->m_globalImageControls;

    return {
        QVariant(QVariantList {
                     "Brightness",
                     "integer",
                     -255,
                     255,
                     1,
                     0,
                     this->d->m_hslFilter->property("luminance").toInt(),
                     QStringList()}),
        QVariant(QVariantList {
                     "Contrast",
                     "integer",
                     -255,
                     255,
                     1,
                     0,
                     this->d->m_contrastFilter->property("contrast").toInt(),
                     QStringList()}),
        QVariant(QVariantList {
                     "Saturation",
                     "integer",
                     -255,
                     255,
                     1,
                     0,
                     this->d->m_hslFilter->property("saturation").toInt(),
                     QStringList()}),
        QVariant(QVariantList {
                     "Hue",
                     "integer",
                     -359,
                     359,
                     1,
                     0,
                     this->d->m_hslFilter->property("hue").toInt(),
                     QStringList()}),
        QVariant(QVariantList {
                     "Gamma",
                     "integer",
                     -255,
                     255,
                     1,
                     0,
                     this->d->m_gammaFilter->property("gamma").toInt(),
                     QStringList()}),
    };
}

bool CaptureAvFoundation::setImageControls(const QVariantMap &imageControls)
{
    if (this->d->m_enableHwControls) {
        this->d->m_controlsMutex.lockForRead();
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

        this->d->m_controlsMutex.lockForWrite();

        if (this->d->m_globalImageControls == globalImageControls) {
            this->d->m_controlsMutex.unlock();

            return false;
        }

        this->d->m_globalImageControls = globalImageControls;
        this->d->m_controlsMutex.unlock();

        emit this->imageControlsChanged(imageControls);

        return true;
    }

    bool ok = true;

    for (auto it = imageControls.cbegin(); it != imageControls.cend(); it++) {
        if (it.key() == "Brightness")
            this->d->m_hslFilter->setProperty("luminance", it.value());
        else if (it.key() == "Contrast")
            this->d->m_contrastFilter->setProperty("contrast", it.value());
        else if (it.key() == "Saturation")
            this->d->m_hslFilter->setProperty("saturation", it.value());
        else if (it.key() == "Hue")
            this->d->m_hslFilter->setProperty("hue", it.value());
        else if (it.key() == "Gamma")
            this->d->m_gammaFilter->setProperty("gamma", it.value());
        else
            ok = false;
    }

    return ok;
}

bool CaptureAvFoundation::resetImageControls()
{
    if (this->d->m_enableHwControls) {
        QVariantMap controls;

        for (auto &control: this->imageControls()) {
            auto params = control.toList();
            controls[params[0].toString()] = params[5].toInt();
        }

        return this->setImageControls(controls);
    }

    this->d->m_hslFilter->setProperty("luminance", 0);
    this->d->m_contrastFilter->setProperty("contrast", 0);
    this->d->m_hslFilter->setProperty("saturation", 0);
    this->d->m_hslFilter->setProperty("hue", 0);
    this->d->m_gammaFilter->setProperty("gamma", 0);

    return true;
}

QVariantList CaptureAvFoundation::cameraControls() const
{
    if (this->d->m_enableHwControls)
        return this->d->m_globalCameraControls;

    return {};
}

bool CaptureAvFoundation::setCameraControls(const QVariantMap &cameraControls)
{
    if (!this->d->m_enableHwControls)
        return false;

    this->d->m_controlsMutex.lockForRead();
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

    this->d->m_controlsMutex.lockForWrite();

    if (this->d->m_globalCameraControls == globalCameraControls) {
        this->d->m_controlsMutex.unlock();

        return false;
    }

    this->d->m_globalCameraControls = globalCameraControls;
    this->d->m_controlsMutex.unlock();

    emit this->cameraControlsChanged(cameraControls);

    return true;
}

bool CaptureAvFoundation::resetCameraControls()
{
    if (!this->d->m_enableHwControls)
        return false;

    QVariantMap controls;

    for (auto &control: this->cameraControls()) {
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket CaptureAvFoundation::readFrame()
{
    this->d->m_mutex.lock();

    if (this->d->m_enableHwControls) {
        this->d->m_controlsMutex.lockForRead();
        auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
        this->d->m_controlsMutex.unlock();

        if (this->d->m_localImageControls != imageControls) {
            auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                             imageControls);
            this->d->setControls(this->d->m_deviceID, controls);
            this->d->m_localImageControls = imageControls;
        }
    }

    if (!this->d->m_curFrame)
        if (!this->d->m_frameReady.wait(&this->d->m_mutex, 1000)) {
            this->d->m_mutex.unlock();

            return {};
        }

    auto formatDesc = CMSampleBufferGetFormatDescription(this->d->m_curFrame);
    int width = 0;
    int height = 0;

    if (formatDesc) {
        auto size = CMVideoFormatDescriptionGetDimensions(formatDesc);
        width = size.width;
        height = size.height;
    }

    if (width < 1 || height < 1) {
        CFRelease(this->d->m_curFrame);
        this->d->m_curFrame = nil;
        this->d->m_mutex.unlock();

        return {};
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
    auto fourCC = CMFormatDescriptionGetMediaSubType(formatDesc);
    AkPacket packet;

    // Read frame data.
    auto imageBuffer = CMSampleBufferGetImageBuffer(this->d->m_curFrame);

    if (imageBuffer) {
        AkVideoPacket videoPacket({rawFmtToAkFmt->value(fourCC),
                                   width,
                                   height,
                                   this->d->m_timeBase.invert()});
        CVPixelBufferLockBaseAddress(imageBuffer, 0);
        auto iData = reinterpret_cast<const quint8 *>(CVPixelBufferGetBaseAddress(imageBuffer));
        auto iLineSize = CVPixelBufferGetBytesPerRow(imageBuffer);
        auto lineSize = qMin<size_t>(videoPacket.lineSize(0),
                                     iLineSize);

        for (int y = 0; y < videoPacket.caps().height(); ++y) {
            auto srcLine = iData + y * iLineSize;
            auto dstLine = videoPacket.line(0, y);
            memcpy(dstLine, srcLine, lineSize);
        }

        CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
        packet = videoPacket;
    } else {
        auto dataBuffer = CMSampleBufferGetDataBuffer(this->d->m_curFrame);

        if (dataBuffer) {
            size_t dataSize = 0;
            char *data = nullptr;
            CMBlockBufferGetDataPointer(dataBuffer,
                                        0,
                                        nullptr,
                                        &dataSize,
                                        &data);
            AkCompressedVideoPacket videoPacket({compressedFormatToStr->value(fourCC),
                                                 width,
                                                 height,
                                                 this->d->m_timeBase.invert()},
                                                dataSize);
            memcpy(videoPacket.data(), data, dataSize);
            packet = videoPacket;
        }
    }

    packet.setPts(pts);
    packet.setTimeBase(timeBase);
    packet.setIndex(0);
    packet.setId(this->d->m_id);

    if (!this->d->m_enableHwControls) {
        packet = this->d->m_hslFilter->iStream(packet);
        packet = this->d->m_gammaFilter->iStream(packet);
        packet = this->d->m_contrastFilter->iStream(packet);
    }

    CFRelease(this->d->m_curFrame);
    this->d->m_curFrame = nil;
    this->d->m_mutex.unlock();

    return packet;
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
    this->d->m_localImageControls.clear();
    this->d->m_localCameraControls.clear();

    auto webcam = this->d->m_device;

    if (webcam.isEmpty()) {
        qFatal("Device is empty");

        return false;
    }

    // Read selected caps.
    auto streams = this->streams();

    if (streams.isEmpty()) {
        qFatal("Streams are empty");

        return false;
    }

    auto supportedCaps = this->caps(webcam);

    if (supportedCaps.isEmpty()) {
        qFatal("Caps are empty");

        return false;
    }

    auto deviceID = this->d->m_cmioIDs.value(webcam, kCMIODeviceUnknown);

    if (deviceID == kCMIODeviceUnknown)
        return false;

    AkCaps caps = streams[0] < supportedCaps.size()?
                    supportedCaps[streams[0]]:
                    supportedCaps.first();

    // Get camera input.
    auto uniqueID = [[NSString alloc]
                     initWithUTF8String: webcam.toStdString().c_str()];

    if (!uniqueID) {
        qFatal("Camera ID not found");

        return false;
    }

    auto camera = [AVCaptureDevice deviceWithUniqueID: uniqueID];
    [uniqueID release];

    if (!camera) {
        qFatal("Can't get AVCaptureDevice");

        return false;
    }

    // Add camera input unit.
    this->d->m_deviceInput = [AVCaptureDeviceInput
                              deviceInputWithDevice: camera
                              error: nil];

    if (!this->d->m_deviceInput) {
        qFatal("Can't get AVCaptureDeviceInput");

        return false;
    }

    // Create capture session.
    this->d->m_session = [AVCaptureSession new];
    [this->d->m_session beginConfiguration];

    if ([this->d->m_session canAddInput: this->d->m_deviceInput] == NO) {
        [this->d->m_session release];
        qFatal("Can't get AVCaptureSession");

        return false;
    }

    [this->d->m_session addInput: this->d->m_deviceInput];

    // Add data output unit.
    this->d->m_dataOutput = [AVCaptureVideoDataOutput new];

    this->d->m_dataOutput.videoSettings = @{
        (NSString *) kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32ARGB)
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
        qFatal("Can't add AVCaptureVideoDataOutput to AVCaptureSession");

        return false;
    }

    [this->d->m_session addOutput: this->d->m_dataOutput];
    [this->d->m_session commitConfiguration];

    if ([camera lockForConfiguration: nil] == NO) {
        [this->d->m_session release];
        qFatal("Can't lock camera configuration");

        return false;
    }

    // Configure camera format.
    auto format = CaptureAvFoundationPrivate::formatFromCaps(camera, caps);

    if (!format) {
        [camera unlockForConfiguration];
        [this->d->m_session release];
        qFatal("No matching camera format");

        return false;
    }

    AkFrac fps = caps.type() == AkCaps::CapsVideo?
                    AkVideoCaps(caps).fps():
                    AkCompressedVideoCaps(caps).fps();
    auto fpsRange = CaptureAvFoundationPrivate::frameRateRangeFromFps(format,
                                                                      fps);

    camera.activeFormat = format;
    camera.activeVideoMinFrameDuration = fpsRange.minFrameDuration;
    camera.activeVideoMaxFrameDuration = fpsRange.maxFrameDuration;

    // Start capturing from the camera.
    [this->d->m_session startRunning];
    [camera unlockForConfiguration];
    [this->d->m_deviceInput retain];

    this->d->m_deviceID = deviceID;
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

    if (device.isEmpty()) {
        this->d->m_controlsMutex.lockForWrite();
        this->d->m_globalImageControls.clear();
        this->d->m_globalCameraControls.clear();
        this->d->m_controlsMutex.unlock();
    } else {
        this->d->m_controlsMutex.lockForWrite();

        if (this->d->m_enableHwControls) {
            auto deviceID = this->d->m_cmioIDs.value(device, kCMIODeviceUnknown);

            if (deviceID != kCMIODeviceUnknown)
                this->d->m_globalImageControls = this->d->controls(deviceID);
        }

        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lockForRead();
    auto imageStatus = this->d->controlStatus(this->d->m_globalImageControls);
    auto cameraStatus = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    emit this->deviceChanged(device);
    emit this->imageControlsChanged(imageStatus);
    emit this->cameraControlsChanged(cameraStatus);
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
    auto supportedCaps = this->caps(this->d->m_device);
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
    if (!CaptureAvFoundationPrivate::canUseCamera())
        return;

    decltype(this->d->m_cmioIDs) cmioIDs;
    decltype(this->d->m_devices) devices;
    decltype(this->d->m_descriptions) descriptions;
    decltype(this->d->m_devicesCaps) devicesCaps;

    for (auto &id: this->d->cmioDevices())
        cmioIDs[this->d->deviceUID(id)] = id;

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

        // List supported frame formats.
        for (AVCaptureDeviceFormat *format in camera.formats) {
            auto fourCC = CMFormatDescriptionGetMediaSubType(format.formatDescription);
            CMVideoDimensions size =
                    CMVideoFormatDescriptionGetDimensions(format.formatDescription);

            if (rawFmtToAkFmt->contains(fourCC)) {
                AkVideoCaps videoCaps({rawFmtToAkFmt->value(fourCC),
                                       size.width,
                                       size.height,
                                       {}});

                // List all supported frame rates for the format.
                for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges) {
                    videoCaps.setFps(AkFrac(qRound(1e3 * fpsRange.maxFrameRate), 1e3));
                    devicesCaps[deviceId] << videoCaps;
                }
            } else if (compressedFormatToStr->contains(fourCC)) {
                AkCompressedVideoCaps videoCaps({compressedFormatToStr->value(fourCC),
                                                 size.width,
                                                 size.height,
                                                 {}});

                // List all supported frame rates for the format.
                for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges) {
                    videoCaps.setFps(AkFrac(qRound(1e3 * fpsRange.maxFrameRate), 1e3));
                    devicesCaps[deviceId] << videoCaps;
                }
            }
        }
    }

    if (devicesCaps.isEmpty()) {
        cmioIDs.clear();
        devices.clear();
        descriptions.clear();
    }

    this->d->m_descriptions = descriptions;
    this->d->m_devicesCaps = devicesCaps;

    if (this->d->m_devices != devices) {
        this->d->m_cmioIDs = cmioIDs;
        this->d->m_devices = devices;
        emit this->webcamsChanged(devices);
    }
}

CaptureAvFoundationPrivate::CaptureAvFoundationPrivate(CaptureAvFoundation *self):
    self(self)
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
QVector<CMIODeviceID> CaptureAvFoundationPrivate::cmioDevices() const
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

QString CaptureAvFoundationPrivate::deviceUID(CMIODeviceID deviceID) const
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

QString CaptureAvFoundationPrivate::objectName(CMIOObjectID objectID) const
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

AVCaptureDeviceFormat *CaptureAvFoundationPrivate::formatFromCaps(AVCaptureDevice *camera,
                                                                  const AkCaps &caps)
{
    for (AVCaptureDeviceFormat *format in camera.formats) {
        if ([format.mediaType isEqualToString: AVMediaTypeVideo] == NO)
            continue;

        auto fourCC = CMFormatDescriptionGetMediaSubType(format.formatDescription);
        CMVideoDimensions size =
                CMVideoFormatDescriptionGetDimensions(format.formatDescription);

        if (rawFmtToAkFmt->contains(fourCC)) {
            AkVideoCaps videoCaps({rawFmtToAkFmt->value(fourCC),
                                   size.width,
                                   size.height,
                                   {}});

            // List all supported frame rates for the format.
            for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges) {
                videoCaps.setFps(AkFrac(qRound(1e3 * fpsRange.maxFrameRate), 1e3));

                if (videoCaps == caps)
                    return format;
            }
        } else if (compressedFormatToStr->contains(fourCC)) {
            AkCompressedVideoCaps videoCaps({compressedFormatToStr->value(fourCC),
                                             size.width,
                                             size.height,
                                             {}});

            // List all supported frame rates for the format.
            for (AVFrameRateRange *fpsRange in format.videoSupportedFrameRateRanges) {
                videoCaps.setFps(AkFrac(qRound(1e3 * fpsRange.maxFrameRate), 1e3));

                if (videoCaps == caps)
                    return format;
            }
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

QVariantMap CaptureAvFoundationPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlName = params[0].toString();
        controlStatus[controlName] = params[6];
    }

    return controlStatus;
}

QVariantMap CaptureAvFoundationPrivate::mapDiff(const QVariantMap &map1,
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

QVector<CMIOControlID> CaptureAvFoundationPrivate::deviceControls(CMIODeviceID deviceID) const
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

ControlType CaptureAvFoundationPrivate::controlType(CMIOControlID controlID,
                                                    Boolean *isSettable) const
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

void CaptureAvFoundationPrivate::setControlType(CMIOControlID controlID,
                                                ControlType type) const
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

ControlStatus CaptureAvFoundationPrivate::controlStatus(CMIOControlID controlID,
                                                        Boolean *isSettable) const
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

void CaptureAvFoundationPrivate::setControlStatus(CMIOControlID controlID,
                                                  ControlStatus status) const
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

ControlValueType CaptureAvFoundationPrivate::controlValueType(CMIOControlID controlID) const
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

void CaptureAvFoundationPrivate::controlRange(CMIOControlID controlID,
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

Float64 CaptureAvFoundationPrivate::controlValue(CMIOControlID controlID) const
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

void CaptureAvFoundationPrivate::setControlValue(CMIOControlID controlID,
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

QVariantList CaptureAvFoundationPrivate::controls(CMIODeviceID deviceID) const
{
    QVariantList controls;

    auto deviceControls = this->deviceControls(deviceID);

    for (auto &control: deviceControls) {
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

bool CaptureAvFoundationPrivate::setControls(CMIODeviceID deviceID,
                                             const QVariantMap &controls) const
{
    bool set = false;
    auto deviceControls = this->deviceControls(deviceID);

    for (auto &control: deviceControls) {
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

#include "moc_captureavfoundation.cpp"

/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

#include <QApplication>
#include <QDateTime>
#include <QReadWriteLock>
#include <QScreen>
#include <QThread>
#include <QVariant>
#include <QVector>
#include <QWaitCondition>
#include <QtAndroid>
#include <ak.h>
#include <akcaps.h>
#include <akcompressedvideocaps.h>
#include <akelement.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideopacket.h>
#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>

#include "capturendkcamera.h"

#define SURFACE_ROTATION_0   0
#define SURFACE_ROTATION_90  1
#define SURFACE_ROTATION_180 2
#define SURFACE_ROTATION_270 3

using RawFmtToAkMap = QMap<AIMAGE_FORMATS, AkVideoCaps::PixelFormat>;

inline const RawFmtToAkMap initRawFmtToAkMap()
{
    const RawFmtToAkMap rawFmtToAkMap {
        {AIMAGE_FORMAT_RGBA_8888  , AkVideoCaps::Format_rgba   },
        {AIMAGE_FORMAT_RGBX_8888  , AkVideoCaps::Format_rgb0   },
        {AIMAGE_FORMAT_RGB_888    , AkVideoCaps::Format_rgb24  },
        {AIMAGE_FORMAT_RGB_565    , AkVideoCaps::Format_rgb565 },
        {AIMAGE_FORMAT_YUV_420_888, AkVideoCaps::Format_yuv420p},
        {AIMAGE_FORMAT_Y8         , AkVideoCaps::Format_gray8  },
    };

    return rawFmtToAkMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(RawFmtToAkMap,
                          rawFmtToAkMap,
                          (initRawFmtToAkMap()))

using CompressedFmtToAkMap = QMap<AIMAGE_FORMATS, QString>;

inline const CompressedFmtToAkMap initCompressedFmtToAkMap()
{
    const CompressedFmtToAkMap compressedFmtToAkMap {
        {AIMAGE_FORMAT_JPEG, "jpeg"},
        {AIMAGE_FORMAT_HEIC, "heic"},
    };

    return compressedFmtToAkMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(CompressedFmtToAkMap,
                          compressedFmtToAkMap,
                          (initCompressedFmtToAkMap()))

enum ControlType
{
    Integer,
    Boolean,
    Menu,
    Float,
    Frac,
};

using MenuOptions = QMap<int, QString>;

inline const MenuOptions &initAntibandingOptions()
{
    static const MenuOptions options {
        {ACAMERA_CONTROL_AE_ANTIBANDING_MODE_OFF , "Off"  },
        {ACAMERA_CONTROL_AE_ANTIBANDING_MODE_50HZ, "50 Hz"},
        {ACAMERA_CONTROL_AE_ANTIBANDING_MODE_60HZ, "60 Hz"},
        {ACAMERA_CONTROL_AE_ANTIBANDING_MODE_AUTO, "Auto" },
    };

    return options;
}

inline const MenuOptions &initAutoExposureOptions()
{
    static const MenuOptions options {
        {ACAMERA_CONTROL_AE_MODE_OFF                 , "Off"              },
        {ACAMERA_CONTROL_AE_MODE_ON                  , "On"               },
        {ACAMERA_CONTROL_AE_MODE_ON_AUTO_FLASH       , "Auto Flash"       },
        {ACAMERA_CONTROL_AE_MODE_ON_ALWAYS_FLASH     , "Always Flash"     },
        {ACAMERA_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE, "Auto Flash Redeye"},
        {ACAMERA_CONTROL_AE_MODE_ON_EXTERNAL_FLASH   , "External Flash"   },
    };

    return options;
}

inline const MenuOptions &initAutoFocusOptions()
{
    static const MenuOptions options {
        {ACAMERA_CONTROL_AF_MODE_OFF               , "Off"                    },
        {ACAMERA_CONTROL_AF_MODE_AUTO              , "Auto"                   },
        {ACAMERA_CONTROL_AF_MODE_MACRO             , "Close-up"               },
        {ACAMERA_CONTROL_AF_MODE_CONTINUOUS_VIDEO  , "Continuous Video"       },
        {ACAMERA_CONTROL_AF_MODE_CONTINUOUS_PICTURE, "Continuous Picture"     },
        {ACAMERA_CONTROL_AF_MODE_EDOF              , "Extended Depth of Field"},
    };

    return options;
}

inline const MenuOptions &initAwbOptions()
{
    static const MenuOptions options {
        {ACAMERA_CONTROL_AWB_MODE_OFF             , "Off"             },
        {ACAMERA_CONTROL_AWB_MODE_AUTO            , "Auto"            },
        {ACAMERA_CONTROL_AWB_MODE_INCANDESCENT    , "Incandescent"    },
        {ACAMERA_CONTROL_AWB_MODE_FLUORESCENT     , "Fluorescent"     },
        {ACAMERA_CONTROL_AWB_MODE_WARM_FLUORESCENT, "Warm Fluorescent"},
        {ACAMERA_CONTROL_AWB_MODE_DAYLIGHT        , "Daylight"        },
        {ACAMERA_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT , "Cloudy Daylight" },
        {ACAMERA_CONTROL_AWB_MODE_TWILIGHT        , "Twilight"        },
        {ACAMERA_CONTROL_AWB_MODE_SHADE           , "Shade"           },
    };

    return options;
}

inline const MenuOptions &initEffectOptions()
{
    static const MenuOptions options {
        {ACAMERA_CONTROL_EFFECT_MODE_OFF       , "Off"       },
        {ACAMERA_CONTROL_EFFECT_MODE_MONO      , "Mono"      },
        {ACAMERA_CONTROL_EFFECT_MODE_NEGATIVE  , "Negative"  },
        {ACAMERA_CONTROL_EFFECT_MODE_SOLARIZE  , "Solarize"  },
        {ACAMERA_CONTROL_EFFECT_MODE_SEPIA     , "Sepia"     },
        {ACAMERA_CONTROL_EFFECT_MODE_POSTERIZE , "Posterize" },
        {ACAMERA_CONTROL_EFFECT_MODE_WHITEBOARD, "Whiteboard"},
        {ACAMERA_CONTROL_EFFECT_MODE_BLACKBOARD, "Blackboard"},
        {ACAMERA_CONTROL_EFFECT_MODE_AQUA      , "Aqua"      },
    };

    return options;
}

inline const MenuOptions &initControlModeOptions()
{
    static const MenuOptions options {
        {ACAMERA_CONTROL_MODE_OFF                    , "Off"                },
        {ACAMERA_CONTROL_MODE_AUTO                   , "Auto"               },
        {ACAMERA_CONTROL_MODE_USE_SCENE_MODE         , "Scene Mode"         },
        {ACAMERA_CONTROL_MODE_OFF_KEEP_STATE         , "Keep State"         },
        {ACAMERA_CONTROL_MODE_USE_EXTENDED_SCENE_MODE, "Extended Scene Mode"},
    };

    return options;
}

inline const MenuOptions &initSceneModeOptions()
{
    static const MenuOptions options {
        {ACAMERA_CONTROL_SCENE_MODE_DISABLED      , "Disabled"      },
        {ACAMERA_CONTROL_SCENE_MODE_FACE_PRIORITY , "Face Priority" },
        {ACAMERA_CONTROL_SCENE_MODE_ACTION        , "Action"        },
        {ACAMERA_CONTROL_SCENE_MODE_PORTRAIT      , "Portrait"      },
        {ACAMERA_CONTROL_SCENE_MODE_LANDSCAPE     , "Landscape"     },
        {ACAMERA_CONTROL_SCENE_MODE_NIGHT         , "Night"         },
        {ACAMERA_CONTROL_SCENE_MODE_NIGHT_PORTRAIT, "Night Portrait"},
        {ACAMERA_CONTROL_SCENE_MODE_THEATRE       , "Theatre"       },
        {ACAMERA_CONTROL_SCENE_MODE_BEACH         , "Beach"         },
        {ACAMERA_CONTROL_SCENE_MODE_SNOW          , "Snow"          },
        {ACAMERA_CONTROL_SCENE_MODE_SUNSET        , "Sunset"        },
        {ACAMERA_CONTROL_SCENE_MODE_STEADYPHOTO   , "Steadyphoto"   },
        {ACAMERA_CONTROL_SCENE_MODE_FIREWORKS     , "Fireworks"     },
        {ACAMERA_CONTROL_SCENE_MODE_SPORTS        , "Sports"        },
        {ACAMERA_CONTROL_SCENE_MODE_PARTY         , "Party"         },
        {ACAMERA_CONTROL_SCENE_MODE_CANDLELIGHT   , "Candlelight"   },
        {ACAMERA_CONTROL_SCENE_MODE_BARCODE       , "Barcode"       },
        {ACAMERA_CONTROL_SCENE_MODE_HDR           , "Hdr"           },
    };

    return options;
}

inline const MenuOptions &initVideoStabilizationOptions()
{
    static const MenuOptions options {
        {ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE_OFF, "Off"},
        {ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE_ON , "On" },
    };

    return options;
}

inline const MenuOptions &initNoiseReductionOptions()
{
    static const MenuOptions options {
        {ACAMERA_NOISE_REDUCTION_MODE_OFF             , "Off"             },
        {ACAMERA_NOISE_REDUCTION_MODE_FAST            , "Fast"            },
        {ACAMERA_NOISE_REDUCTION_MODE_HIGH_QUALITY    , "High Quality"    },
        {ACAMERA_NOISE_REDUCTION_MODE_MINIMAL         , "Minimal"         },
        {ACAMERA_NOISE_REDUCTION_MODE_ZERO_SHUTTER_LAG, "Zero Shutter Lag"},
    };

    return options;
}

inline const MenuOptions &initEdgeModeOptions()
{
    static const MenuOptions options {
        {ACAMERA_EDGE_MODE_OFF             , "Off"             },
        {ACAMERA_EDGE_MODE_FAST            , "Fast"            },
        {ACAMERA_EDGE_MODE_HIGH_QUALITY    , "High Quality"    },
        {ACAMERA_EDGE_MODE_ZERO_SHUTTER_LAG, "Zero Shutter Lag"},
    };

    return options;
}

inline const MenuOptions &initColorCorrectionOptions()
{
    static const MenuOptions options {
        {ACAMERA_COLOR_CORRECTION_MODE_TRANSFORM_MATRIX, "Transform Matrix"},
        {ACAMERA_COLOR_CORRECTION_MODE_FAST            , "Fast"            },
        {ACAMERA_COLOR_CORRECTION_MODE_HIGH_QUALITY    , "High Quality"    },
    };

    return options;
}

inline const MenuOptions &initTonemapOptions()
{
    static const MenuOptions options {
        {ACAMERA_TONEMAP_MODE_CONTRAST_CURVE, "Contrast Curve"},
        {ACAMERA_TONEMAP_MODE_FAST          , "Fast"          },
        {ACAMERA_TONEMAP_MODE_HIGH_QUALITY  , "High Quality"  },
        {ACAMERA_TONEMAP_MODE_GAMMA_VALUE   , "Gamma Value"   },
        {ACAMERA_TONEMAP_MODE_PRESET_CURVE  , "Preset Curve"  },
    };

    return options;
}

struct Control
{
    ControlType type;
    acamera_metadata_tag tag;
    QVector<acamera_metadata_tag> relTags;
    QString description;
    QVariant defaultValue;
    MenuOptions menuOptions;
};

using ControlVector = QVector<Control>;

inline const ControlVector &initImageControls()
{
    static const ControlVector controls {
        {ControlType::Menu, ACAMERA_NOISE_REDUCTION_MODE            , {ACAMERA_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES}, "Noise Reduction" , "Off" , initNoiseReductionOptions() },
        {ControlType::Menu, ACAMERA_EDGE_MODE                       , {ACAMERA_EDGE_AVAILABLE_EDGE_MODES}                      , "Edge Mode"       , "Off" , initEdgeModeOptions()       },
        {ControlType::Menu, ACAMERA_COLOR_CORRECTION_ABERRATION_MODE, {ACAMERA_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES}    , "Color Correction", "Off" , initColorCorrectionOptions()},
        {ControlType::Menu, ACAMERA_TONEMAP_MODE                    , {ACAMERA_TONEMAP_AVAILABLE_TONE_MAP_MODES}               , "Tonemap"         , "Fast", initTonemapOptions()        },
    };

    return controls;
}

Q_GLOBAL_STATIC_WITH_ARGS(ControlVector,
                          globalImageControls,
                          (initImageControls()))

inline const ControlVector &initCameraControls()
{
    static const ControlVector controls {
        {ControlType::Menu   , ACAMERA_CONTROL_AE_ANTIBANDING_MODE       , {ACAMERA_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES}     , "Auto Exposure Antibanding" , "Auto"    , initAntibandingOptions()       },
        {ControlType::Menu   , ACAMERA_CONTROL_AE_MODE                   , {ACAMERA_CONTROL_AE_AVAILABLE_MODES}                 , "Auto Exposure"             , "On"      , initAutoExposureOptions()      },
        {ControlType::Integer, ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION  , {ACAMERA_CONTROL_AE_COMPENSATION_RANGE,
                                                                            ACAMERA_CONTROL_AE_COMPENSATION_STEP}               , "Auto Exposure Compensation", 0         , {}                             },
        {ControlType::Boolean, ACAMERA_CONTROL_AE_LOCK                   , {ACAMERA_CONTROL_AE_LOCK_AVAILABLE}                  , "Auto Exposure Lock"        , true      , {}                             },
        {ControlType::Menu   , ACAMERA_CONTROL_AF_MODE                   , {ACAMERA_CONTROL_AF_AVAILABLE_MODES}                 , "Auto Focus"                , "Auto"    , initAutoFocusOptions()         },
        {ControlType::Boolean, ACAMERA_CONTROL_AWB_LOCK                  , {ACAMERA_CONTROL_AWB_LOCK_AVAILABLE}                 , "Auto White Balance Lock"   , true      , {}                             },
        {ControlType::Menu   , ACAMERA_CONTROL_AWB_MODE                  , {ACAMERA_CONTROL_AWB_AVAILABLE_MODES}                , "Auto White Balance"        , "Auto"    , initAwbOptions()               },
        {ControlType::Menu   , ACAMERA_CONTROL_EFFECT_MODE               , {ACAMERA_CONTROL_AVAILABLE_EFFECTS}                  , "Effect"                    , "Off"     , initEffectOptions()            },
        {ControlType::Boolean, ACAMERA_CONTROL_ENABLE_ZSL                , {}                                                   , "Zero Shutter Lag"          , true      , {}                             },
        {ControlType::Menu   , ACAMERA_CONTROL_MODE                      , {ACAMERA_CONTROL_AVAILABLE_MODES}                    , "Control Mode"              , "Auto"    , initControlModeOptions()       },
        {ControlType::Integer, ACAMERA_CONTROL_POST_RAW_SENSITIVITY_BOOST, {ACAMERA_CONTROL_POST_RAW_SENSITIVITY_BOOST_RANGE}   , "Post Raw Sensitivity Boost", 0         , {}                             },
        {ControlType::Menu   , ACAMERA_CONTROL_SCENE_MODE                , {ACAMERA_CONTROL_AVAILABLE_SCENE_MODES}              , "Scene Mode"                , "Disabled", initSceneModeOptions()         },
        {ControlType::Menu   , ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE  , {ACAMERA_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES}, "Video Stabilization"       , "On"      , initVideoStabilizationOptions()},
    };

    return controls;
}

Q_GLOBAL_STATIC_WITH_ARGS(ControlVector,
                          globalCameraControls,
                          (initCameraControls()))

using AeFlashModeMap = QMap<acamera_metadata_enum_android_control_ae_mode_t, Capture::FlashMode>;

inline const AeFlashModeMap &initAeFlashModeMap()
{
    static const AeFlashModeMap aeFlashModeMap {
        {ACAMERA_CONTROL_AE_MODE_OFF                 , Capture::FlashMode_Off     },
        {ACAMERA_CONTROL_AE_MODE_ON                  , Capture::FlashMode_On      },
        {ACAMERA_CONTROL_AE_MODE_ON_AUTO_FLASH       , Capture::FlashMode_Auto    },
        {ACAMERA_CONTROL_AE_MODE_ON_ALWAYS_FLASH     , Capture::FlashMode_Torch   },
        {ACAMERA_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE, Capture::FlashMode_RedEye  },
        {ACAMERA_CONTROL_AE_MODE_ON_EXTERNAL_FLASH   , Capture::FlashMode_External},
    };

    return aeFlashModeMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(AeFlashModeMap, aeFlashModeMap, (initAeFlashModeMap()))

using FlashModeMap = QMap<Capture::FlashMode, acamera_metadata_enum_acamera_flash_mode>;

inline const FlashModeMap &initFlashModeMap()
{
    static const FlashModeMap flashModeMap {
        {Capture::FlashMode_Off     , ACAMERA_FLASH_MODE_OFF   },
        {Capture::FlashMode_On      , ACAMERA_FLASH_MODE_SINGLE},
        {Capture::FlashMode_Auto    , ACAMERA_FLASH_MODE_SINGLE},
        {Capture::FlashMode_RedEye  , ACAMERA_FLASH_MODE_SINGLE},
        {Capture::FlashMode_External, ACAMERA_FLASH_MODE_SINGLE},
        {Capture::FlashMode_Torch   , ACAMERA_FLASH_MODE_TORCH },
    };

    return flashModeMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(FlashModeMap, flashModeMap, (initFlashModeMap()))

using CameraManagerPtr = QSharedPointer<ACameraManager>;

class CaptureNdkCameraPrivate
{
    public:
        CaptureNdkCamera *self;
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, CaptureVideoCaps> m_devicesCaps;
        QMap<QString, Capture::FlashModeList> m_supportedFlashModes;
        QReadWriteLock m_controlsMutex;
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;
        QReadWriteLock m_mutex;
        AkPacket m_curPacket;
        QWaitCondition m_waitCondition;
        AkFrac m_fps;
        AkCaps m_caps;
        QString m_curDeviceId;
        qint64 m_id {-1};
        CameraManagerPtr m_manager;
        ACameraDevice *m_camera {nullptr};
        AImageReader *m_imageReader {nullptr};
        ANativeWindow *m_imageReaderWindow {nullptr};
        ACaptureSessionOutputContainer *m_outputContainer {nullptr};
        ACaptureSessionOutput *m_sessionOutput {nullptr};
        ACameraOutputTarget *m_outputTarget {nullptr};
        ACaptureRequest *m_captureRequest {nullptr};
        ACameraCaptureSession *m_captureSession {nullptr};
        AkElementPtr m_rotate {akPluginManager->create<AkElement>("VideoFilter/Rotate")};
        int m_nBuffers {4};

        explicit CaptureNdkCameraPrivate(CaptureNdkCamera *self);
        bool nearestFpsRangue(const QString &cameraId,
                              const AkFrac &fps,
                              int32_t &min,
                              int32_t &max) const;
        static void onCamerasChanged(void *context, const char *cameraId);
        static void deviceDisconnected(void *context, ACameraDevice *device);
        static void deviceError(void *context,
                                ACameraDevice *device,
                                int error);
        static void imageAvailable(void *context, AImageReader *reader);
        qreal cameraRotation(const QString &deviceId) const;
        static void sessionClosed(void *context,
                                  ACameraCaptureSession *session);
        static void sessionReady(void *context,
                                 ACameraCaptureSession *session);
        static void sessionActive(void *context,
                                  ACameraCaptureSession *session);
        QVariantList controls(const ControlVector &controlsTable,
                              const QString &device) const;
        bool setControls(const ControlVector &controlsTable,
                         const QVariantMap &controls) const;
        bool readControlRange(const ACameraMetadata_const_entry &entry,
                              int *min,
                              int *max) const;
        int readControlInteger(const ACameraMetadata_const_entry &entry) const;
        bool readControlBool(const ACameraMetadata_const_entry &entry) const;
        QStringList readControlMenuOptions(const ACameraMetadata_const_entry &entry,
                                           const MenuOptions &menuOptions) const;
        QVariantList controlBoolean(const ACameraMetadata_const_entry &entry,
                                    const QString &description,
                                    bool defaultValue=false) const;
        QVariantList controlMenu(const ACameraMetadata_const_entry &entry,
                                 const QString &description,
                                 const QString &defaultValue={},
                                 const QStringList &menuOptions={}) const;
        QVariantList controlInteger(const ACameraMetadata_const_entry &entry,
                                    int min,
                                    int max,
                                    int step,
                                    const QString &description,
                                    int defaultValue=0) const;
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        static bool canUseCamera();
        bool isFlashSupported() const;
        void updateDevices();
};

CaptureNdkCamera::CaptureNdkCamera(QObject *parent):
    Capture(parent)
{
    this->d = new CaptureNdkCameraPrivate(this);
    ACameraManager_AvailabilityCallbacks availabilityCb = {
        this->d,
        CaptureNdkCameraPrivate::onCamerasChanged,
        CaptureNdkCameraPrivate::onCamerasChanged,
    };
    ACameraManager_registerAvailabilityCallback(this->d->m_manager.data(),
                                                &availabilityCb);

    auto rotateFunc = [this] () {
        if (!this->d->m_curDeviceId.isEmpty()) {
            auto angle = -this->d->cameraRotation(this->d->m_curDeviceId);
            this->d->m_rotate->setProperty("angle", angle);
        }
    };

    for (auto &screen: QApplication::screens()) {
        QObject::connect(screen,
                         &QScreen::geometryChanged,
                         this,
                         rotateFunc);
        QObject::connect(screen,
                         &QScreen::primaryOrientationChanged,
                         this,
                         rotateFunc);
    }

    this->d->updateDevices();
}

CaptureNdkCamera::~CaptureNdkCamera()
{
    this->uninit();
    delete this->d;
}

QStringList CaptureNdkCamera::webcams() const
{
    return this->d->m_devices;
}

QString CaptureNdkCamera::device() const
{
    return this->d->m_device;
}

QList<int> CaptureNdkCamera::streams()
{
    if (!this->d->m_streams.isEmpty())
        return this->d->m_streams;

    auto caps = this->caps(this->d->m_device);

    if (caps.isEmpty())
        return {};

    return QList<int> {0};
}

QList<int> CaptureNdkCamera::listTracks(AkCaps::CapsType type)
{
    if (type != AkCaps::CapsVideo && type != AkCaps::CapsUnknown)
        return {};

    auto caps = this->caps(this->d->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CaptureNdkCamera::ioMethod() const
{
    return {};
}

int CaptureNdkCamera::nBuffers() const
{
    return this->d->m_nBuffers;
}

QString CaptureNdkCamera::description(const QString &webcam) const
{
    return this->d->m_descriptions.value(webcam);
}

CaptureVideoCaps CaptureNdkCamera::caps(const QString &webcam) const
{
    return this->d->m_devicesCaps.value(webcam);
}

QVariantList CaptureNdkCamera::imageControls() const
{
    return this->d->m_globalImageControls;
}

bool CaptureNdkCamera::setImageControls(const QVariantMap &imageControls)
{
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

bool CaptureNdkCamera::resetImageControls()
{
    QVariantMap controls;

    for (auto &control: this->imageControls()) {
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList CaptureNdkCamera::cameraControls() const
{
    return this->d->m_globalCameraControls;
}

bool CaptureNdkCamera::setCameraControls(const QVariantMap &cameraControls)
{
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

bool CaptureNdkCamera::resetCameraControls()
{
    QVariantMap controls;

    for (auto &control: this->cameraControls()) {
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

Capture::FlashModeList CaptureNdkCamera::supportedFlashModes(const QString &webcam) const
{
    return this->d->m_supportedFlashModes.value(webcam);
}

Capture::FlashMode CaptureNdkCamera::flashMode() const
{
    acamera_metadata_enum_android_control_ae_mode_t mode =
            ACAMERA_CONTROL_AE_MODE_OFF;

    this->d->m_mutex.lockForWrite();

    if (this->d->m_captureRequest) {
        ACameraMetadata_const_entry aeModeEntry;

        if (ACaptureRequest_getConstEntry(this->d->m_captureRequest,
                                          ACAMERA_CONTROL_AE_MODE,
                                          &aeModeEntry) == ACAMERA_OK) {
            mode = acamera_metadata_enum_android_control_ae_mode_t(aeModeEntry.data.u8[0]);
        }
    }

    this->d->m_mutex.unlock();

    return aeFlashModeMap->value(mode, FlashMode_Off);
}

AkPacket CaptureNdkCamera::readFrame()
{
    this->d->m_controlsMutex.lockForRead();
    auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localImageControls != imageControls) {
        auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                         imageControls);
        this->d->setControls(*globalImageControls, controls);
        this->d->m_localImageControls = imageControls;
    }

    this->d->m_controlsMutex.lockForRead();
    auto cameraControls = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localCameraControls != cameraControls) {
        auto controls = this->d->mapDiff(this->d->m_localCameraControls,
                                         cameraControls);
        this->d->setControls(*globalCameraControls, controls);
        this->d->m_localCameraControls = cameraControls;
    }

    AkPacket packet;

    this->d->m_mutex.lockForWrite();

    if (!this->d->m_curPacket)
        this->d->m_waitCondition.wait(&this->d->m_mutex, 1000);

    if (this->d->m_curPacket) {
        packet = this->d->m_curPacket;
        this->d->m_curPacket = {};
    }

    this->d->m_mutex.unlock();

    return this->d->m_rotate->iStream(packet);
}

CaptureNdkCameraPrivate::CaptureNdkCameraPrivate(CaptureNdkCamera *self):
    self(self)
{
    this->m_manager = CameraManagerPtr(ACameraManager_create(),
                                       [] (ACameraManager *manager) {
        ACameraManager_delete(manager);
    });
}

bool CaptureNdkCameraPrivate::nearestFpsRangue(const QString &cameraId,
                                               const AkFrac &fps,
                                               int32_t &min,
                                               int32_t &max) const
{
    ACameraMetadata *metaData {nullptr};

    if (ACameraManager_getCameraCharacteristics(this->m_manager.data(),
                                                cameraId.toStdString().c_str(),
                                                &metaData) != ACAMERA_OK) {
        return false;
    }

    ACameraMetadata_const_entry frameRates;

    if (ACameraMetadata_getConstEntry(metaData,
                                      ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
                                      &frameRates) != ACAMERA_OK) {
        ACameraMetadata_free(metaData);

        return false;
    }

    auto fpsValue = fps.value();
    bool ok = false;
    min = 0;
    max = 0;

    for (uint32_t i = 0; i < frameRates.count; i += 2) {
        auto fpsMin = frameRates.data.i32[i + 0];
        auto fpsMax = frameRates.data.i32[i + 1];

        if (fpsValue >= qreal(fpsMin)
            && fpsValue <= qreal(fpsMax)) {
            min = fpsMin;
            max = fpsMax;
            ok = true;

            break;
        }
    }

    ACameraMetadata_free(metaData);

    return ok;
}

void CaptureNdkCameraPrivate::onCamerasChanged(void *context,
                                               const char *cameraId)
{
    Q_UNUSED(cameraId)
    auto self = reinterpret_cast<CaptureNdkCameraPrivate *>(context);
    self->updateDevices();
}

void CaptureNdkCameraPrivate::deviceDisconnected(void *context,
                                                 ACameraDevice *device)
{
    Q_UNUSED(context)
    Q_UNUSED(device)
}

void CaptureNdkCameraPrivate::deviceError(void *context,
                                          ACameraDevice *device,
                                          int error)
{
    Q_UNUSED(context)
    Q_UNUSED(device)
    Q_UNUSED(error)
}

void CaptureNdkCameraPrivate::imageAvailable(void *context,
                                             AImageReader *reader)
{
    auto self = reinterpret_cast<CaptureNdkCameraPrivate *>(context);
    AImage *image {nullptr};

    if (AImageReader_acquireLatestImage(reader, &image) != AMEDIA_OK)
        return;

    int32_t format = 0;

    if (AImage_getFormat(image, &format) != AMEDIA_OK) {
        AImage_delete(image);

        return;
    }

    int32_t width = 0;

    if (AImage_getWidth(image, &width) != AMEDIA_OK) {
        AImage_delete(image);

        return;
    }

    int32_t height = 0;

    if (AImage_getHeight(image, &height) != AMEDIA_OK) {
        AImage_delete(image);

        return;
    }

    int32_t numPlanes = 0;

    if (AImage_getNumberOfPlanes(image, &numPlanes) != AMEDIA_OK) {
        AImage_delete(image);

        return;
    }

    int64_t timestampNs = 0;

    if (AImage_getTimestamp(image, &timestampNs) != AMEDIA_OK) {
        AImage_delete(image);

        return;
    }

    AkVideoPacket packet({rawFmtToAkMap->value(AIMAGE_FORMATS(format)),
                          width,
                          height,
                          self->m_fps});

    for (int32_t plane = 0; plane < numPlanes; plane++) {
        uint8_t *data = nullptr;
        int dataLength = 0;

        if (AImage_getPlaneData(image, plane, &data, &dataLength) != AMEDIA_OK)
            continue;

        int32_t iLineSize = 0;
        AImage_getPlaneRowStride(image, plane, &iLineSize);
        auto oLineSize = packet.lineSize(plane);
        auto lineSize = qMin<size_t>(iLineSize, oLineSize);
        auto heightDiv = packet.heightDiv(plane);

        for (int y = 0; y < height; y++) {
            auto ys = y >> heightDiv;
            auto srcLine = data + ys * iLineSize;
            auto dstLine = packet.line(plane, y);
            memcpy(dstLine, srcLine, lineSize);
        }

        data += iLineSize * (height >> heightDiv);
    }

    packet.setPts(timestampNs);
    packet.setTimeBase({1, qint64(1e9)});
    packet.setIndex(0);
    packet.setId(self->m_id);

    self->m_mutex.lockForWrite();
    self->m_curPacket = packet;
    self->m_waitCondition.wakeAll();
    self->m_mutex.unlock();

    AImage_delete(image);
}

qreal CaptureNdkCameraPrivate::cameraRotation(const QString &deviceId) const
{
    if (!this->m_manager)
        return 0.0;

    ACameraMetadata *characteristics = nullptr;
    ACameraManager_getCameraCharacteristics(this->m_manager.data(),
                                            deviceId.toStdString().c_str(),
                                            &characteristics);

    if (!characteristics)
        return 0.0;

    auto activity = QtAndroid::androidActivity();
    auto windowManager =
        activity.callObjectMethod("getWindowManager",
                                  "()Landroid/view/WindowManager;");
    auto display =
            windowManager.callObjectMethod("getDefaultDisplay",
                                           "()Landroid/view/Display;");
    int degrees = 0;

    switch (display.callMethod<jint>("getRotation")) {
    case SURFACE_ROTATION_0:
        degrees = 0;

        break;
    case SURFACE_ROTATION_90:
        degrees = 90;

        break;
    case SURFACE_ROTATION_180:
        degrees = 180;

        break;
    case SURFACE_ROTATION_270:
        degrees = 270;

        break;
    default:
        break;
    }

    ACameraMetadata_const_entry entry;
    memset(&entry, 0, sizeof(ACameraMetadata_const_entry));
    ACameraMetadata_getConstEntry(characteristics,
                                  ACAMERA_LENS_FACING,
                                  &entry);
    auto facing =
            acamera_metadata_enum_android_lens_facing_t(entry.data.u8[0]);

    memset(&entry, 0, sizeof(ACameraMetadata_const_entry));
    ACameraMetadata_getConstEntry(characteristics,
                                  ACAMERA_SENSOR_ORIENTATION,
                                  &entry);
    auto orientation = entry.data.i32[0];
    int rotation = 0;

    switch (facing) {
    case ACAMERA_LENS_FACING_FRONT:
        rotation = (orientation + degrees) % 360;
        rotation = (360 - rotation) % 360;

        break;

    case ACAMERA_LENS_FACING_BACK:
        rotation = (orientation - degrees + 360) % 360;
        break;

    default:
        break;
    }

    return rotation;
}

void CaptureNdkCameraPrivate::sessionClosed(void *context,
                                            ACameraCaptureSession *session)
{
    Q_UNUSED(context)
    Q_UNUSED(session)
}

void CaptureNdkCameraPrivate::sessionReady(void *context,
                                           ACameraCaptureSession *session)
{
    Q_UNUSED(context)
    Q_UNUSED(session)
}

void CaptureNdkCameraPrivate::sessionActive(void *context,
                                            ACameraCaptureSession *session)
{
    Q_UNUSED(context)
    Q_UNUSED(session)
}

QVariantList CaptureNdkCameraPrivate::controls(const ControlVector &controlsTable,
                                               const QString &device) const
{
    ACameraMetadata *metaData {nullptr};
    auto cameraId = device;
    cameraId.remove(QRegExp("^NdkCamera:"));

    if (ACameraManager_getCameraCharacteristics(this->m_manager.data(),
                                                cameraId.toStdString().c_str(),
                                                &metaData) != ACAMERA_OK) {

        return {};
    }

    int32_t nTags = 0;
    const uint32_t *tags = nullptr;
    ACameraMetadata_getAllTags(metaData, &nTags, &tags);

    // Read controls related parameters

    QMap<int, QStringList> controlsMenu;
    QVector<int> booleanControls;
    QMap<int, QPair<int, int>> controlsRange;
    QMap<int, int> controlsStep;

    for (int i = 0; i < nTags; i++) {
        ACameraMetadata_const_entry entry;
        memset(&entry, 0, sizeof(ACameraMetadata_const_entry));

        if (ACameraMetadata_getConstEntry(metaData,
                                          tags[i],
                                          &entry) != ACAMERA_OK) {
            continue;
        }

        for (auto &control: controlsTable) {
            auto index = control.relTags.indexOf(acamera_metadata_tag(entry.tag));

            if (index < 0)
                continue;

            if (entry.count < 1)
                continue;

            switch (control.type) {
            case ControlType::Integer: {
                switch (index) {
                case 0: {
                    int min = 0;
                    int max = 0;
                    auto ok = this->readControlRange(entry, &min, &max);

                    if (ok)
                        controlsRange[control.tag] = {min, max};

                    break;
                }
                case 1:
                    controlsStep[control.tag] = this->readControlInteger(entry);

                    break;
                default:
                    break;
                }

                break;
            }
            case ControlType::Boolean: {
                if (this->readControlBool(entry))
                    booleanControls << control.tag;

                break;
            }
            case ControlType::Menu: {
                auto menuOptions =
                        this->readControlMenuOptions(entry, control.menuOptions);

                if (!menuOptions.isEmpty())
                    controlsMenu[control.tag] = menuOptions;

                break;
            }
            default:
                break;
            }
        }
    }

    // Read controls values

    QVariantList controlsList;

    for (int i = 0; i < nTags; i++) {
        ACameraMetadata_const_entry entry;
        memset(&entry, 0, sizeof(ACameraMetadata_const_entry));

        if (ACameraMetadata_getConstEntry(metaData,
                                          tags[i],
                                          &entry) != ACAMERA_OK) {
            continue;
        }

        for (auto &control: controlsTable) {
            if (control.tag != entry.tag)
                continue;

            if (entry.count < 1)
                continue;

            QVariantList params;

            switch (control.type) {
            case ControlType::Integer:
                if (controlsRange.contains(control.tag)) {
                    auto range = controlsRange.value(control.tag);

                    params = this->controlInteger(entry,
                                                  range.first,
                                                  range.second,
                                                  controlsStep.value(control.tag, 1),
                                                  control.description,
                                                  control.defaultValue.toInt());
                }

                break;

            case ControlType::Boolean:
                if (control.relTags.isEmpty()
                    || booleanControls.contains(control.tag))
                    params = this->controlBoolean(entry,
                                                  control.description,
                                                  control.defaultValue.toBool());

                break;

            case ControlType::Menu:
                params = this->controlMenu(entry,
                                           control.description,
                                           control.defaultValue.toString(),
                                           controlsMenu.value(control.tag));
                break;

            default:
                break;
            }

            if (!params.isEmpty())
                controlsList << QVariant(params);
        }
    }

    ACameraMetadata_free(metaData);

    return controlsList;
}

bool CaptureNdkCameraPrivate::setControls(const ControlVector &controlsTable,
                                          const QVariantMap &controls) const
{
    if (!this->m_captureRequest)
        return false;

    for (auto it = controls.cbegin(); it != controls.cend(); it++)
        for (auto &control: controlsTable)
            if (control.description == it.key()) {
                ACameraMetadata_const_entry entry;

                if (ACaptureRequest_getConstEntry(this->m_captureRequest,
                                                  control.tag,
                                                  &entry) != ACAMERA_OK) {
                    continue;
                }

                switch (entry.type) {
                case ACAMERA_TYPE_BYTE:
                case ACAMERA_TYPE_INT32: {
                    auto value = it.value().toInt();
                    ACaptureRequest_setEntry_i32(this->m_captureRequest,
                                                 control.tag,
                                                 1,
                                                 &value);

                    break;
                }
                case ACAMERA_TYPE_INT64: {
                    int64_t value = it.value().toLongLong();
                    ACaptureRequest_setEntry_i64(this->m_captureRequest,
                                                 control.tag,
                                                 1,
                                                 &value);

                    break;
                }
                case ACAMERA_TYPE_FLOAT: {
                    auto value = it.value().toFloat();
                    ACaptureRequest_setEntry_float(this->m_captureRequest,
                                                   control.tag,
                                                   1,
                                                   &value);

                    break;
                }
                case ACAMERA_TYPE_DOUBLE: {
                    auto value = it.value().toDouble();
                    ACaptureRequest_setEntry_double(this->m_captureRequest,
                                                    control.tag,
                                                    1,
                                                    &value);

                    break;
                }
                default:
                    break;
                }
            }

    return true;
}

bool CaptureNdkCameraPrivate::readControlRange(const ACameraMetadata_const_entry &entry,
                                               int *min,
                                               int *max) const
{
    if (entry.count < 2)
        return false;

    switch (entry.type) {
    case ACAMERA_TYPE_BYTE:
        *min = entry.data.u8[0];
        *max = entry.data.u8[1];

        return true;
    case ACAMERA_TYPE_INT32:
        *min = entry.data.i32[0];
        *max = entry.data.i32[1];

        return true;
    case ACAMERA_TYPE_INT64:
        *min = int(entry.data.i64[0]);
        *max = int(entry.data.i64[1]);

        return true;
    default:
        break;
    }

    return false;
}

int CaptureNdkCameraPrivate::readControlInteger(const ACameraMetadata_const_entry &entry) const
{
    switch (entry.type) {
    case ACAMERA_TYPE_BYTE:
        return entry.data.u8[0];
    case ACAMERA_TYPE_INT32:
        return entry.data.i32[0];
    case ACAMERA_TYPE_INT64:
        return entry.data.i64[0];
    default:
        break;
    }

    return 0;
}

bool CaptureNdkCameraPrivate::readControlBool(const ACameraMetadata_const_entry &entry) const
{
    switch (entry.type) {
    case ACAMERA_TYPE_BYTE:
        return entry.data.u8[0];
    case ACAMERA_TYPE_INT32:
        return entry.data.i32[0];
    case ACAMERA_TYPE_INT64:
        return entry.data.i64[0];
    default:
        break;
    }

    return false;
}

QStringList CaptureNdkCameraPrivate::readControlMenuOptions(const ACameraMetadata_const_entry &entry,
                                                            const MenuOptions &menuOptions) const
{
    QStringList options;

    for (uint32_t i = 0; i < entry.count; i++) {
        QString option;

        switch (entry.type) {
        case ACAMERA_TYPE_BYTE:
            option = menuOptions.value(entry.data.u8[i]);

            break;
        case ACAMERA_TYPE_INT32:
            option = menuOptions.value(entry.data.i32[i]);

            break;
        case ACAMERA_TYPE_INT64:
            option = menuOptions.value(entry.data.i64[i]);

            break;
        default:
            break;
        }

        if (!option.isEmpty())
            options << option;
    }

    return options;
}

QVariantList CaptureNdkCameraPrivate::controlBoolean(const ACameraMetadata_const_entry &entry,
                                                     const QString &description,
                                                     bool defaultValue) const
{
    bool value = false;

    switch (entry.type) {
    case ACAMERA_TYPE_BYTE:
        value = entry.data.u8[0];
        break;
    case ACAMERA_TYPE_INT32:
        value = entry.data.i32[0];
        break;
    case ACAMERA_TYPE_INT64:
        value = entry.data.i64[0];
        break;
    default:
        break;
    }

    return QVariantList {
        description,
        "boolean",
        0,
        1,
        1,
        defaultValue,
        value,
        QStringList()
    };
}

QVariantList CaptureNdkCameraPrivate::controlMenu(const ACameraMetadata_const_entry &entry,
                                                  const QString &description,
                                                  const QString &defaultValue,
                                                  const QStringList &menuOptions) const
{
    QString value;

    switch (entry.type) {
    case ACAMERA_TYPE_BYTE:
        value = menuOptions.value(entry.data.u8[0]);

        break;
    case ACAMERA_TYPE_INT32:
        value = menuOptions.value(entry.data.i32[0]);

        break;
    case ACAMERA_TYPE_INT64:
        value = menuOptions.value(entry.data.i64[0]);

        break;
    default:
        break;
    }

    return QVariantList {
        description,
        "menu",
        0,
        menuOptions.size() - 1,
        1,
        qMax(menuOptions.indexOf(defaultValue), 0),
        qMax(menuOptions.indexOf(value), 0),
        menuOptions
    };
}

QVariantList CaptureNdkCameraPrivate::controlInteger(const ACameraMetadata_const_entry &entry,
                                                     int min,
                                                     int max,
                                                     int step,
                                                     const QString &description,
                                                     int defaultValue) const
{
    int value = 0;

    switch (entry.type) {
    case ACAMERA_TYPE_BYTE:
        value = entry.data.u8[0];
        break;
    case ACAMERA_TYPE_INT32:
        value = entry.data.i32[0];
        break;
    case ACAMERA_TYPE_INT64:
        value = int(entry.data.i64[0]);
        break;
    default:
        break;
    }

    return QVariantList {
        description,
        "integer",
        min,
        max,
        step,
        defaultValue,
        value,
        QStringList()
    };
}

QVariantMap CaptureNdkCameraPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlName = params[0].toString();
        controlStatus[controlName] = params[6];
    }

    return controlStatus;
}

QVariantMap CaptureNdkCameraPrivate::mapDiff(const QVariantMap &map1,
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

bool CaptureNdkCameraPrivate::canUseCamera()
{
    static bool done = false;
    static bool result = false;

    if (done)
        return result;

    QStringList permissions {
        "android.permission.CAMERA"
    };
    QStringList neededPermissions;

    for (auto &permission: permissions)
        if (QtAndroid::checkPermission(permission) == QtAndroid::PermissionResult::Denied)
            neededPermissions << permission;

    if (!neededPermissions.isEmpty()) {
        auto results = QtAndroid::requestPermissionsSync(neededPermissions);

        for (auto it = results.constBegin(); it != results.constEnd(); it++)
            if (it.value() == QtAndroid::PermissionResult::Denied) {
                done = true;

                return false;
            }
    }

    done = true;
    result = true;

    return true;
}

bool CaptureNdkCameraPrivate::isFlashSupported() const
{
    auto context = QtAndroid::androidContext();
    auto packageManager =
            context.callObjectMethod("getPackageManager",
                                     "()Landroid/content/pm/PackageManager;");

    if (packageManager.isValid()) {
        auto feature = packageManager.getObjectField("FEATURE_CAMERA_FLASH",
                                                     "java/lang/String");

        if (feature.isValid()) {
            return packageManager.callMethod<jboolean>("hasSystemFeature",
                                                       "(Ljava/lang/String;)Z",
                                                       feature.object());
        }
    }

    return false;
}

void CaptureNdkCameraPrivate::updateDevices()
{
    if (!this->canUseCamera())
        return;

    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;
    decltype(this->m_supportedFlashModes) supportedFlashModes;

    bool hasFlash = this->isFlashSupported();
    ACameraIdList *cameras = nullptr;

    if (ACameraManager_getCameraIdList(this->m_manager.data(),
                                       &cameras) == ACAMERA_OK) {
        static const QVector<AIMAGE_FORMATS> unsupportedFormats {
            AIMAGE_FORMAT_RAW_PRIVATE,
            AIMAGE_FORMAT_DEPTH16,
            AIMAGE_FORMAT_DEPTH_POINT_CLOUD,
            AIMAGE_FORMAT_PRIVATE,
        };

        static const QMap<acamera_metadata_enum_android_lens_facing_t, QString> facingToStr {
            {ACAMERA_LENS_FACING_FRONT   , "Front"},
            {ACAMERA_LENS_FACING_BACK    , "Back"},
            {ACAMERA_LENS_FACING_EXTERNAL, "External"},
        };

        for (int i = 0; i < cameras->numCameras; i++) {
            QString cameraId = cameras->cameraIds[i];
            ACameraMetadata *metaData = nullptr;

            if (ACameraManager_getCameraCharacteristics(this->m_manager.data(),
                                                        cameras->cameraIds[i],
                                                        &metaData) != ACAMERA_OK) {
                continue;
            }

            ACameraMetadata_const_entry frameRates;

            if (ACameraMetadata_getConstEntry(metaData,
                                              ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
                                              &frameRates) != ACAMERA_OK) {
                continue;
            }

            QList<AkFrac> supportedFrameRates;

            for (uint32_t i = 0; i < frameRates.count; i += 2) {
                AkFrac fps(frameRates.data.i32[i + 0]
                           + frameRates.data.i32[i + 1],
                           2);

                if (!supportedFrameRates.contains(fps))
                    supportedFrameRates << fps;
            }

            ACameraMetadata_const_entry formats;

            if (ACameraMetadata_getConstEntry(metaData,
                                              ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                              &formats) != ACAMERA_OK) {
                continue;
            }

            CaptureVideoCaps supportedFormats;

            for (uint32_t i = 0; i < formats.count; i += 4) {
                if (formats.data.i32[i + 3])
                    continue;

                auto width = formats.data.i32[i + 1];
                auto height = formats.data.i32[i + 2];

                if (width < 1 || height < 1)
                    continue;

                auto format = AIMAGE_FORMATS(formats.data.i32[i + 0]);

                if (unsupportedFormats.contains(format))
                    continue;

                if (rawFmtToAkMap->contains(format)) {
                    AkVideoCaps videoCaps(rawFmtToAkMap->value(format),
                                          width,
                                          height,
                                          {});

                    if (!supportedFormats.contains(videoCaps))
                        supportedFormats << videoCaps;
                } else if (compressedFmtToAkMap->contains(format)) {
                    AkCompressedVideoCaps videoCaps(compressedFmtToAkMap->value(format),
                                                    width,
                                                    height,
                                                    {});

                    if (!supportedFormats.contains(videoCaps))
                        supportedFormats << videoCaps;
                }
            }

            CaptureVideoCaps caps;

            for (auto &format: supportedFormats)
                for (auto &fps: supportedFrameRates) {
                    AkVideoCaps videoCaps(format);
                    videoCaps.setFps(fps);
                    caps << videoCaps;
                }

            if (!caps.isEmpty()) {
                auto deviceId = "NdkCamera:" + cameraId;
                ACameraMetadata_const_entry lensFacing;
                ACameraMetadata_getConstEntry(metaData,
                                              ACAMERA_LENS_FACING,
                                              &lensFacing);
                auto facing =
                        acamera_metadata_enum_android_lens_facing_t(lensFacing.data.u8[0]);
                auto description = QString("%1 Camera %2")
                                   .arg(facingToStr[facing], cameraId);
                devices << deviceId;
                descriptions[deviceId] = description;
                devicesCaps[deviceId] = caps;

                ACameraMetadata_const_entry flashAvailableEntry;
                ACameraMetadata_getConstEntry(metaData,
                                              ACAMERA_FLASH_INFO_AVAILABLE,
                                              &flashAvailableEntry);

                if (hasFlash && flashAvailableEntry.data.u8[0]) {
                    ACameraMetadata_const_entry entry;

                    if (ACameraMetadata_getConstEntry(metaData,
                                                      ACAMERA_CONTROL_AE_AVAILABLE_MODES,
                                                      &entry) == ACAMERA_OK) {
                        Capture::FlashModeList modes;

                        for (jint i = 0; i < entry.count; i++) {
                            auto mode =
                                    acamera_metadata_enum_android_control_ae_mode_t(entry.data.u8[i]);

                            if (aeFlashModeMap->contains(mode))
                                modes << aeFlashModeMap->value(mode);
                        }

                        supportedFlashModes[deviceId] = modes;
                    }
                }
            }

            ACameraMetadata_free(metaData);
        }

        ACameraManager_deleteCameraIdList(cameras);
    }

    if (devicesCaps.isEmpty()) {
        devices.clear();
        descriptions.clear();
        supportedFlashModes.clear();
    }

    this->m_descriptions = descriptions;
    this->m_devicesCaps = devicesCaps;
    this->m_supportedFlashModes = supportedFlashModes;

    if (this->m_devices != devices) {
        this->m_devices = devices;
        emit self->webcamsChanged(this->m_devices);
    }
}

bool CaptureNdkCamera::init()
{
    this->d->m_localImageControls.clear();
    this->d->m_localCameraControls.clear();
    this->uninit();

    QList<int> streams;
    CaptureVideoCaps supportedCaps;
    AIMAGE_FORMATS format;
    AkCaps caps;
    int32_t fpsRange[2];
    int width = 0;
    int height = 0;
    AkFrac fps;

    auto cameraId = this->d->m_device;
    cameraId.remove(QRegExp("^NdkCamera:"));
    ACameraDevice_StateCallbacks deviceStateCb {
        this->d,
        CaptureNdkCameraPrivate::deviceDisconnected,
        CaptureNdkCameraPrivate::deviceError,
    };
    AImageReader_ImageListener imageListenerCb {
        this->d,
        CaptureNdkCameraPrivate::imageAvailable
    };
    ACameraCaptureSession_stateCallbacks sessionStateCb {
        this->d,
        CaptureNdkCameraPrivate::sessionClosed,
        CaptureNdkCameraPrivate::sessionReady,
        CaptureNdkCameraPrivate::sessionActive,
    };

    this->d->m_curDeviceId = cameraId;

    if (ACameraManager_openCamera(this->d->m_manager.data(),
                                  cameraId.toStdString().c_str(),
                                  &deviceStateCb,
                                  &this->d->m_camera) != ACAMERA_OK) {
        this->uninit();

        return false;
    }

    if (ACameraDevice_createCaptureRequest(this->d->m_camera,
                                           TEMPLATE_PREVIEW,
                                           &this->d->m_captureRequest) != ACAMERA_OK) {
        this->uninit();

        return false;
    }

    streams = this->streams();

    if (streams.isEmpty()) {
        this->uninit();

        return false;
    }

    supportedCaps = this->caps(this->d->m_device);
    caps = supportedCaps[streams[0]];

    if (caps.type() == AkCaps::CapsVideo) {
        AkVideoCaps videoCaps(caps);
        format = rawFmtToAkMap->key(videoCaps.format(),
                                    AIMAGE_FORMAT_PRIVATE);
        width = videoCaps.width();
        height = videoCaps.height();
        fps = videoCaps.fps();
    } else {
        AkCompressedVideoCaps videoCaps(caps);
        format = compressedFmtToAkMap->key(videoCaps.format(),
                                           AIMAGE_FORMAT_PRIVATE);
        width = videoCaps.width();
        height = videoCaps.height();
        fps = videoCaps.fps();
    }

    if (!this->d->nearestFpsRangue(cameraId, fps, fpsRange[0], fpsRange[1])) {
        this->uninit();

        return false;
    }

    if (ACaptureRequest_setEntry_i32(this->d->m_captureRequest,
                                     ACAMERA_CONTROL_AE_TARGET_FPS_RANGE,
                                     2,
                                     fpsRange) != ACAMERA_OK) {
        this->uninit();

        return false;
    }

    if (AImageReader_new(width,
                         height,
                         format,
                         this->d->m_nBuffers,
                         &this->d->m_imageReader) != AMEDIA_OK) {
        this->uninit();

        return false;
    }

    if (AImageReader_setImageListener(this->d->m_imageReader,
                                      &imageListenerCb) != AMEDIA_OK) {
        this->uninit();

        return false;
    }

    if (AImageReader_getWindow(this->d->m_imageReader,
                               &this->d->m_imageReaderWindow) != AMEDIA_OK) {
        this->uninit();

        return false;
    }

    ANativeWindow_acquire(this->d->m_imageReaderWindow);

    if (ACaptureSessionOutput_create(this->d->m_imageReaderWindow,
                                     &this->d->m_sessionOutput) != ACAMERA_OK) {
        this->uninit();

        return false;
    }

    if (ACaptureSessionOutputContainer_create(&this->d->m_outputContainer) != ACAMERA_OK) {
        this->uninit();

        return false;
    }

    if (ACaptureSessionOutputContainer_add(this->d->m_outputContainer,
                                           this->d->m_sessionOutput) != ACAMERA_OK) {
        this->uninit();

        return false;
    }

    if (ACameraOutputTarget_create(this->d->m_imageReaderWindow,
                                   &this->d->m_outputTarget) != ACAMERA_OK) {
        this->uninit();

        return false;
    }

    if (ACaptureRequest_addTarget(this->d->m_captureRequest,
                                  this->d->m_outputTarget) != ACAMERA_OK) {
        this->uninit();

        return false;
    }

    if (ACameraDevice_createCaptureSession(this->d->m_camera,
                                           this->d->m_outputContainer,
                                           &sessionStateCb,
                                           &this->d->m_captureSession) != ACAMERA_OK) {
        this->uninit();

        return false;
    }

    if (ACameraCaptureSession_setRepeatingRequest(this->d->m_captureSession,
                                                  nullptr,
                                                  1,
                                                  &this->d->m_captureRequest,
                                                  nullptr) != ACAMERA_OK) {
        this->uninit();

        return false;
    }

    this->d->m_id = Ak::id();
    this->d->m_caps = caps;
    this->d->m_fps = fps;

    auto angle = -this->d->cameraRotation(this->d->m_curDeviceId);
    this->d->m_rotate->setProperty("angle", angle);

    return true;
}

void CaptureNdkCamera::uninit()
{
    if (this->d->m_captureSession) {
        ACameraCaptureSession_stopRepeating(this->d->m_captureSession);
        ACameraCaptureSession_close(this->d->m_captureSession);
        this->d->m_captureSession = nullptr;
    }

    if (this->d->m_outputTarget) {
        if (this->d->m_captureRequest)
            ACaptureRequest_removeTarget(this->d->m_captureRequest, this->d->m_outputTarget);

        ACameraOutputTarget_free(this->d->m_outputTarget);
        this->d->m_outputTarget = nullptr;
    }

    if (this->d->m_sessionOutput) {
        if (this->d->m_outputContainer)
            ACaptureSessionOutputContainer_remove(this->d->m_outputContainer,
                                                  this->d->m_sessionOutput);

        ACaptureSessionOutput_free(this->d->m_sessionOutput);
        this->d->m_sessionOutput = nullptr;
    }

    if (this->d->m_outputContainer) {
        ACaptureSessionOutputContainer_free(this->d->m_outputContainer);
        this->d->m_outputContainer = nullptr;
    }

    if (this->d->m_imageReaderWindow) {
        ANativeWindow_release(this->d->m_imageReaderWindow);
        this->d->m_imageReaderWindow = nullptr;
    }

    if (this->d->m_captureRequest) {
        ACaptureRequest_free(this->d->m_captureRequest);
        this->d->m_captureRequest = nullptr;
    }

    if (this->d->m_camera) {
        ACameraDevice_close(this->d->m_camera);
        this->d->m_camera = nullptr;
    }

    QThread::sleep(1);

    if (this->d->m_imageReader) {
        AImageReader_delete(this->d->m_imageReader);
        this->d->m_imageReader = nullptr;
    }

    this->d->m_curDeviceId = QString();
}

void CaptureNdkCamera::setDevice(const QString &device)
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
        this->d->m_globalImageControls =
                this->d->controls(*globalImageControls, device);
        this->d->m_globalCameraControls =
                this->d->controls(*globalCameraControls, device);
        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lockForWrite();
    auto imageStatus = this->d->controlStatus(this->d->m_globalImageControls);
    auto cameraStatus = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    emit this->deviceChanged(device);
    emit this->imageControlsChanged(imageStatus);
    emit this->cameraControlsChanged(cameraStatus);
}

void CaptureNdkCamera::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    auto supportedCaps = this->caps(this->d->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams {stream};

    if (this->streams() == inputStreams)
        return;

    this->d->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void CaptureNdkCamera::setIoMethod(const QString &ioMethod)
{
    Q_UNUSED(ioMethod)
}

void CaptureNdkCamera::setNBuffers(int nBuffers)
{
    if (this->d->m_nBuffers == nBuffers)
        return;

    this->d->m_nBuffers = nBuffers;
    emit this->nBuffersChanged(nBuffers);
}

void CaptureNdkCamera::setFlashMode(FlashMode mode)
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_captureRequest) {
        ACameraMetadata_const_entry aeModeEntry;

        if (ACaptureRequest_getConstEntry(this->d->m_captureRequest,
                                          ACAMERA_CONTROL_AE_MODE,
                                          &aeModeEntry) == ACAMERA_OK) {
            auto aeMode = acamera_metadata_enum_android_control_ae_mode_t(aeModeEntry.data.u8[0]);

            if (aeFlashModeMap->contains(aeMode)) {
                ACameraMetadata_const_entry flashModeEntry;

                if (ACaptureRequest_getConstEntry(this->d->m_captureRequest,
                                                  ACAMERA_FLASH_MODE,
                                                  &flashModeEntry) == ACAMERA_OK) {
                    auto curAeMode = aeFlashModeMap->value(aeMode);
                    auto curFlashMode = flashModeMap->value(mode);

                    if  (curAeMode != mode || curFlashMode != flashModeEntry.data.u8[0]) {
                        uint8_t aeMode = aeFlashModeMap->key(mode);
                        ACaptureRequest_setEntry_u8(this->d->m_captureRequest,
                                                    ACAMERA_CONTROL_AE_MODE,
                                                    1,
                                                    &aeMode);
                        uint8_t flashMode = flashModeMap->value(mode);
                        ACaptureRequest_setEntry_u8(this->d->m_captureRequest,
                                                    ACAMERA_FLASH_MODE,
                                                    1,
                                                    &flashMode);
                        emit this->flashModeChanged(mode);
                    }
                }
            } else {
                uint8_t aeMode = aeFlashModeMap->key(mode);
                ACaptureRequest_setEntry_u8(this->d->m_captureRequest,
                                            ACAMERA_CONTROL_AE_MODE,
                                            1,
                                            &aeMode);
                uint8_t flashMode = flashModeMap->value(mode);
                ACaptureRequest_setEntry_u8(this->d->m_captureRequest,
                                            ACAMERA_FLASH_MODE,
                                            1,
                                            &flashMode);
                emit this->flashModeChanged(mode);
            }
        }
    }

    this->d->m_mutex.unlock();
}

void CaptureNdkCamera::resetDevice()
{
    this->setDevice("");
}

void CaptureNdkCamera::resetStreams()
{
    auto supportedCaps = this->caps(this->d->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void CaptureNdkCamera::resetIoMethod()
{
    this->setIoMethod("any");
}

void CaptureNdkCamera::resetNBuffers()
{
    this->setNBuffers(4);
}

void CaptureNdkCamera::resetFlashMode()
{
    this->setFlashMode(FlashMode_Off);
}

void CaptureNdkCamera::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

#include "moc_capturendkcamera.cpp"

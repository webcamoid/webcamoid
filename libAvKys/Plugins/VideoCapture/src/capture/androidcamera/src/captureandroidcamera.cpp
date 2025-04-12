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
#include <QJniObject>
#include <QMutex>
#include <QReadWriteLock>
#include <QSet>
#include <QThread>
#include <QtConcurrent>
#include <QVariant>
#include <QWaitCondition>
#include <QtConcurrent>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideoformatspec.h>
#include <akvideopacket.h>
#include <iak/akelement.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QPermissions>
#endif

#include "captureandroidcamera.h"

#define BUFFER_SIZE 4

#define JNAMESPACE "org/webcamoid/plugins/VideoCapture/submodules/androidcamera"
#define JCLASS(jclass) JNAMESPACE "/" #jclass

#define LENS_FACING_FRONT    0
#define LENS_FACING_BACK     1
#define LENS_FACING_EXTERNAL 2

#define SURFACE_ROTATION_0   0
#define SURFACE_ROTATION_90  1
#define SURFACE_ROTATION_180 2
#define SURFACE_ROTATION_270 3

#define MAKE_FOURCC(a, b, c, d) AK_MAKE_FOURCC(d, c, b, a)

enum ImageFormat
{
    TRANSLUCENT       = -3,
    TRANSPARENT       = -2,
    OPAQUE            = -1,
    UNKNOWN           = 0,
    RGBA_8888         = 1,
    RGBX_8888         = 2,
    RGB_888           = 3,
    RGB_565           = 4,
    RGBA_5551         = 6,
    RGBA_4444         = 7,
    A_8               = 8,
    L_8               = 9,
    LA_88             = 10,
    RGB_332           = 11,
    NV16              = 16,
    NV21              = 17,
    YUY2              = 20,
    RGBA_F16          = 22,
    RAW_SENSOR        = 32,
    YUV_420_888       = 35,
    PRIVATE           = 34,
    RAW_PRIVATE       = 36,
    RAW10             = 37,
    RAW12             = 38,
    YUV_422_888       = 39,
    FLEX_RGB_888      = 41,
    FLEX_RGBA_8888    = 42,
    RGBA_1010102      = 43,
    JPEG              = 256,
    DEPTH_POINT_CLOUD = 257,
    Y8                = MAKE_FOURCC('Y', '8', ' ', ' '),
    YV12              = MAKE_FOURCC('Y', 'V', '1', '2'),
    DEPTH16           = MAKE_FOURCC('Y', '1', '6', 'D'),
    DEPTH_JPEG        = MAKE_FOURCC('c', 'i', 'e', 'i'),
};

using AndroidFmtToAkFmtMap = QMap<__u32, AkVideoCaps::PixelFormat>;

inline AndroidFmtToAkFmtMap initAndroidFmtToAkFmt()
{
    AndroidFmtToAkFmtMap androidFmtToAkFmt {
        {RGBA_8888     , AkVideoCaps::Format_rgba       },
        {RGBX_8888     , AkVideoCaps::Format_rgbx       },
        {RGB_888       , AkVideoCaps::Format_rgb24      },
        {RGB_565       , AkVideoCaps::Format_rgb565     },
        {RGBA_5551     , AkVideoCaps::Format_rgba5551   },
        {RGBA_4444     , AkVideoCaps::Format_rgba4444   },
        {A_8           , AkVideoCaps::Format_y8         },
        {L_8           , AkVideoCaps::Format_y8         },
        {LA_88         , AkVideoCaps::Format_ya88       },
        {RGB_332       , AkVideoCaps::Format_rgb332     },
        {NV16          , AkVideoCaps::Format_nv16       },
        {NV21          , AkVideoCaps::Format_nv21       },
        {YUY2          , AkVideoCaps::Format_yuyv422    },
        {YUV_420_888   , AkVideoCaps::Format_yuv420p    },
        {YUV_422_888   , AkVideoCaps::Format_yuv422p    },
        {FLEX_RGB_888  , AkVideoCaps::Format_rgb24p     },
        {FLEX_RGBA_8888, AkVideoCaps::Format_rgbap      },
        {RGBA_1010102  , AkVideoCaps::Format_rgba1010102},
        {Y8            , AkVideoCaps::Format_y8         },
        {YV12          , AkVideoCaps::Format_yvu420p    },
    };

    return androidFmtToAkFmt;
}

Q_GLOBAL_STATIC_WITH_ARGS(AndroidFmtToAkFmtMap,
                          androidFmtToAkFmt,
                          (initAndroidFmtToAkFmt()))

enum ControlType
{
    Integer,
    Boolean,
    Menu,
};

using JniObjectKeyFunc =
    std::function<void (const QJniObject &requestBuilder, jint value)>;
using JniObjectCharacteristicsKeyFunc =
    std::function<QJniObject (const QJniObject &characteristics)>;
using VariantValueFunc = std::function<QVariant ()>;

struct ControlMenu
{
    VariantValueFunc keyFunc;
    QString description;
};

using ControlMenuVector = QVector<ControlMenu>;

#define REQUEST_KEY_BOOLEAN(key) \
({ \
    auto func = [] (const QJniObject &requestBuilder, \
                    jint value) { \
    auto controlKey = \
        QJniObject::getStaticObjectField("android/hardware/camera2/CaptureRequest", \
                                         #key, \
                                         "Landroid/hardware/camera2/CaptureRequest$Key;"); \
    QJniObject valueObject("java/lang/Boolean",\
                           "(Z)V", \
                           jboolean(value));\
    requestBuilder.callMethod<void>("set", \
                                    "(Landroid/hardware/camera2/CaptureRequest$Key;Ljava/lang/Object;)V", \
                                    controlKey.object(), \
                                    valueObject.object()); \
    }; \
    \
    func; \
})

#define REQUEST_KEY_INTEGER(key) \
({ \
    auto func = [] (const QJniObject &requestBuilder, \
                    jint value) { \
    auto controlKey = \
        QJniObject::getStaticObjectField("android/hardware/camera2/CaptureRequest", \
                                         #key, \
                                         "Landroid/hardware/camera2/CaptureRequest$Key;"); \
    QJniObject valueObject("java/lang/Integer",\
                           "(I)V", \
                           value);\
    requestBuilder.callMethod<void>("set", \
                                    "(Landroid/hardware/camera2/CaptureRequest$Key;Ljava/lang/Object;)V", \
                                    controlKey.object(), \
                                    valueObject.object()); \
    }; \
    \
    func; \
})

#define CHARACTERISTICS_VALUE(key) \
({ \
    auto func = [] (const QJniObject &characteristics) -> QJniObject { \
            auto jkey = QJniObject::getStaticObjectField("android/hardware/camera2/CameraCharacteristics", \
                                                         #key, \
                                                         "Landroid/hardware/camera2/CameraCharacteristics$Key;"); \
            auto value = characteristics.callObjectMethod("get", \
                                                          "(Landroid/hardware/camera2/CameraCharacteristics$Key;)Ljava/lang/Object;", \
                                                          jkey.object()); \
            \
            return value; \
        }; \
    \
    func; \
})

#define CONTROL_VALUE(key) \
({ \
    auto func = [] () -> QVariant { \
            return QJniObject::getStaticField<jint>("android/hardware/camera2/CameraMetadata", \
                                                    #key); \
        }; \
    \
    func; \
})

#define CONTROL_VALUE_NUM(value) \
({ \
    auto func = [] () -> QVariant { \
            return value; \
        }; \
    \
    func; \
})

inline const ControlMenuVector &antibandingModes()
{
    static const ControlMenuVector antibandingModes {
        {CONTROL_VALUE(CONTROL_AE_ANTIBANDING_MODE_50HZ), "50 Hz"},
        {CONTROL_VALUE(CONTROL_AE_ANTIBANDING_MODE_60HZ), "60 Hz"},
        {CONTROL_VALUE(CONTROL_AE_ANTIBANDING_MODE_AUTO), "Auto" },
        {CONTROL_VALUE(CONTROL_AE_ANTIBANDING_MODE_OFF) , "Off"  },
    };

    return antibandingModes;
}

inline const ControlMenuVector &aeModes()
{
    static const ControlMenuVector aeModes {
        {CONTROL_VALUE(CONTROL_AE_MODE_OFF)                 , "Off"              },
        {CONTROL_VALUE(CONTROL_AE_MODE_ON)                  , "On"               },
        {CONTROL_VALUE(CONTROL_AE_MODE_ON_ALWAYS_FLASH)     , "Always Flash"     },
        {CONTROL_VALUE(CONTROL_AE_MODE_ON_AUTO_FLASH)       , "Auto Flash"       },
        {CONTROL_VALUE(CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE), "Auto Flash Redeye"},
        {CONTROL_VALUE(CONTROL_AE_MODE_ON_EXTERNAL_FLASH)   , "External Flash"   },
    };

    return aeModes;
}

inline const ControlMenuVector &afModes()
{
    static const ControlMenuVector afModes {
        {CONTROL_VALUE(CONTROL_AF_MODE_AUTO)              , "Auto"                   },
        {CONTROL_VALUE(CONTROL_AF_MODE_CONTINUOUS_PICTURE), "Continuous Picture"     },
        {CONTROL_VALUE(CONTROL_AF_MODE_CONTINUOUS_VIDEO)  , "Continuous Video"       },
        {CONTROL_VALUE(CONTROL_AF_MODE_EDOF)              , "Extended depth of field"},
        {CONTROL_VALUE(CONTROL_AF_MODE_MACRO)             , "Close-up"               },
        {CONTROL_VALUE(CONTROL_AF_MODE_OFF)               , "Off"                    },
    };

    return afModes;
}

inline const ControlMenuVector &awbModes()
{
    static const ControlMenuVector awbModes {
        {CONTROL_VALUE(CONTROL_AWB_MODE_AUTO)            , "Auto"            },
        {CONTROL_VALUE(CONTROL_AWB_MODE_CLOUDY_DAYLIGHT) , "Cloudy Daylight" },
        {CONTROL_VALUE(CONTROL_AWB_MODE_DAYLIGHT)        , "Daylight"        },
        {CONTROL_VALUE(CONTROL_AWB_MODE_FLUORESCENT)     , "Fluorescent"     },
        {CONTROL_VALUE(CONTROL_AWB_MODE_INCANDESCENT)    , "Incandescent"    },
        {CONTROL_VALUE(CONTROL_AWB_MODE_OFF)             , "Off"             },
        {CONTROL_VALUE(CONTROL_AWB_MODE_SHADE)           , "Shade"           },
        {CONTROL_VALUE(CONTROL_AWB_MODE_TWILIGHT)        , "Twilight"        },
        {CONTROL_VALUE(CONTROL_AWB_MODE_WARM_FLUORESCENT), "Warm Fluorescent"},
    };

    return awbModes;
}

inline const ControlMenuVector &effectModes()
{
    static const ControlMenuVector effectModes {
        {CONTROL_VALUE(CONTROL_EFFECT_MODE_AQUA)      , "Aqua"      },
        {CONTROL_VALUE(CONTROL_EFFECT_MODE_BLACKBOARD), "Blackboard"},
        {CONTROL_VALUE(CONTROL_EFFECT_MODE_MONO)      , "Mon"       },
        {CONTROL_VALUE(CONTROL_EFFECT_MODE_NEGATIVE)  , "Negative"  },
        {CONTROL_VALUE(CONTROL_EFFECT_MODE_OFF)       , "Off"       },
        {CONTROL_VALUE(CONTROL_EFFECT_MODE_POSTERIZE) , "Posterize" },
        {CONTROL_VALUE(CONTROL_EFFECT_MODE_SEPIA)     , "Sepia"     },
        {CONTROL_VALUE(CONTROL_EFFECT_MODE_SOLARIZE)  , "Solarize"  },
        {CONTROL_VALUE(CONTROL_EFFECT_MODE_WHITEBOARD), "Whiteboard"},
    };

    return effectModes;
}

inline const ControlMenuVector &controlModes()
{
    static const ControlMenuVector controlModes {
        {CONTROL_VALUE(CONTROL_MODE_AUTO)          , "Auto"      },
        {CONTROL_VALUE(CONTROL_MODE_OFF)           , "Off"       },
        {CONTROL_VALUE(CONTROL_MODE_OFF_KEEP_STATE), "Keep State"},
        {CONTROL_VALUE(CONTROL_MODE_USE_SCENE_MODE), "Scene Mode"},
    };

    return controlModes;
}

inline const ControlMenuVector &sceneModes()
{
    static const ControlMenuVector sceneModes {
        {CONTROL_VALUE(CONTROL_SCENE_MODE_DISABLED)        , "Disabled"        },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_ACTION)          , "Action"          },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_BARCODE)         , "Barcode"         },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_BEACH)           , "Beach"           },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_CANDLELIGHT)     , "Candlelight"     },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_FACE_PRIORITY)   , "Face Priority"   },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_FIREWORKS)       , "Fireworks"       },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_HDR)             , "Hdr"             },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_HIGH_SPEED_VIDEO), "High Speed Video"},
        {CONTROL_VALUE(CONTROL_SCENE_MODE_LANDSCAPE)       , "Landscape"       },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_NIGHT)           , "Night"           },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_NIGHT_PORTRAIT)  , "Night Portrait"  },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_PARTY)           , "Party"           },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_PORTRAIT)        , "Portrait"        },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_SNOW)            , "Snow"            },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_SPORTS)          , "Sports"          },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_STEADYPHOTO)     , "Steadyphoto"     },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_SUNSET)          , "Sunset"          },
        {CONTROL_VALUE(CONTROL_SCENE_MODE_THEATRE)         , "Theatre"         },
    };

    return sceneModes;
}

inline const ControlMenuVector &stabilizationModes()
{
    static const ControlMenuVector stabilizationModes {
        {CONTROL_VALUE(CONTROL_VIDEO_STABILIZATION_MODE_OFF), "Off"},
        {CONTROL_VALUE(CONTROL_VIDEO_STABILIZATION_MODE_ON) , "On" },
    };

    return stabilizationModes;
}

struct DeviceControl
{
    JniObjectKeyFunc requestKeyFunc;
    QString description;
    ControlType type;
    JniObjectCharacteristicsKeyFunc range;
    JniObjectCharacteristicsKeyFunc step;
    VariantValueFunc defaultValueFunc;
    JniObjectCharacteristicsKeyFunc menuKeyFunc;
    ControlMenuVector menuOptions;
};

using ControlVector = QVector<DeviceControl>;

inline const ControlVector &initImageControls()
{
    static const ControlVector controls {
        {REQUEST_KEY_INTEGER(CONTROL_AE_ANTIBANDING_MODE)     , "Antibanding Mode"          , ControlType::Menu   , {}                                                  , {}                                                 , CONTROL_VALUE(CONTROL_AE_ANTIBANDING_MODE_AUTO)   , CHARACTERISTICS_VALUE(CONTROL_AE_AVAILABLE_ANTIBANDING_MODES)     , antibandingModes()  },
        {REQUEST_KEY_INTEGER(CONTROL_AE_EXPOSURE_COMPENSATION), "Auto Exposure Compensation", ControlType::Integer, CHARACTERISTICS_VALUE(CONTROL_AE_COMPENSATION_RANGE), CHARACTERISTICS_VALUE(CONTROL_AE_COMPENSATION_STEP), CONTROL_VALUE_NUM(0)                              , {}                                                                , {}                  },
        {REQUEST_KEY_BOOLEAN(CONTROL_AE_LOCK)                 , "Auto Exposure Lock"        , ControlType::Boolean, CHARACTERISTICS_VALUE(CONTROL_AE_LOCK_AVAILABLE)    , {}                                                 , CONTROL_VALUE_NUM(false)                          , {}                                                                , {}                  },
        {REQUEST_KEY_INTEGER(CONTROL_AE_MODE)                 , "Auto Exposure Mode"        , ControlType::Menu   , {}                                                  , {}                                                 , CONTROL_VALUE(CONTROL_AE_MODE_ON)                 , CHARACTERISTICS_VALUE(CONTROL_AE_AVAILABLE_MODES)                 , aeModes()           },
        {REQUEST_KEY_BOOLEAN(CONTROL_AWB_LOCK)                , "Auto White Balance Lock"   , ControlType::Boolean, CHARACTERISTICS_VALUE(CONTROL_AWB_LOCK_AVAILABLE)   , {}                                                 , CONTROL_VALUE_NUM(false)                          , {}                                                                , {}                  },
        {REQUEST_KEY_INTEGER(CONTROL_AWB_MODE)                , "Auto White Balance Mode"   , ControlType::Menu   , {}                                                  , {}                                                 , CONTROL_VALUE(CONTROL_AWB_MODE_AUTO)              , CHARACTERISTICS_VALUE(CONTROL_AWB_AVAILABLE_MODES)                , awbModes()          },
        {REQUEST_KEY_INTEGER(CONTROL_EFFECT_MODE)             , "Effect Mode"               , ControlType::Menu   , {}                                                  , {}                                                 , CONTROL_VALUE(CONTROL_EFFECT_MODE_OFF)            , CHARACTERISTICS_VALUE(CONTROL_AVAILABLE_EFFECTS)                  , effectModes()       },
        {REQUEST_KEY_INTEGER(CONTROL_MODE)                    , "Mode"                      , ControlType::Menu   , {}                                                  , {}                                                 , CONTROL_VALUE(CONTROL_MODE_AUTO)                  , CHARACTERISTICS_VALUE(CONTROL_AVAILABLE_MODES)                    , controlModes()      },
        {REQUEST_KEY_INTEGER(CONTROL_SCENE_MODE)              , "Scene Mode"                , ControlType::Menu   , {}                                                  , {}                                                 , CONTROL_VALUE(CONTROL_SCENE_MODE_DISABLED)        , CHARACTERISTICS_VALUE(CONTROL_AVAILABLE_SCENE_MODES)              , sceneModes()        },
        {REQUEST_KEY_INTEGER(CONTROL_VIDEO_STABILIZATION_MODE), "Video Stabilization Mode"  , ControlType::Menu   , {}                                                  , {}                                                 , CONTROL_VALUE(CONTROL_VIDEO_STABILIZATION_MODE_ON), CHARACTERISTICS_VALUE(CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES), stabilizationModes()},
    };

    return controls;
}

Q_GLOBAL_STATIC_WITH_ARGS(ControlVector,
                          globalImageControls,
                          (initImageControls()))

inline const ControlVector &initCameraControls()
{
    static const ControlVector controls {
        {REQUEST_KEY_INTEGER(CONTROL_AF_MODE), "Auto Focus Mode", ControlType::Menu, {}, {}, CONTROL_VALUE(CONTROL_AF_MODE_AUTO), CHARACTERISTICS_VALUE(CONTROL_AF_AVAILABLE_MODES), afModes()},
    };

    return controls;
}

Q_GLOBAL_STATIC_WITH_ARGS(ControlVector,
                          globalCameraControls,
                          (initCameraControls()))

using FpsRanges = QVector<QPair<int, int>>;

class CaptureAndroidCameraPrivate
{
    public:
        CaptureAndroidCamera *self;
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkCapsList> m_devicesCaps;
        QMap<QString, FpsRanges> m_availableFpsRanges;
        QMap<QString, bool> m_isTorchSupported;
        QReadWriteLock m_controlsMutex;
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;
        QReadWriteLock m_mutex;
        QByteArray m_curBuffer;
        bool m_cameraOpened {false};
        QWaitCondition m_cameraOpenedReady;
        bool m_sessionConfigured {false};
        QWaitCondition m_sessionConfiguredReady;
        QWaitCondition m_packetReady;
        AkFrac m_fps;
        AkFrac m_timeBase;
        AkVideoCaps m_caps;
        QString m_curDeviceId;
        AkPacket m_curPacket;
        qint64 m_id {-1};
        QThreadPool m_threadPool;
        QJniEnvironment m_jenv;
        QJniObject m_cameraManager;
        QJniObject m_context;
        QJniObject m_camera;
        QJniObject m_cameraSession;
        QJniObject m_requestBuilder;
        QJniObject m_callbacks;
        QJniObject m_imageReader;

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        QCameraPermission m_cameraPermission;
        bool m_permissionResultReady {false};
#endif

        AkElementPtr m_rotate {akPluginManager->create<AkElement>("VideoFilter/Rotate")};
        Capture::TorchMode m_torchMode {Capture::Torch_Off};

        explicit CaptureAndroidCameraPrivate(CaptureAndroidCamera *self);
        void registerNatives();
        AkCapsList caps(const QString &deviceId);
        AkCapsList caps(const QJniObject &characteristics,
                              const FpsRanges &fpsRanges);
        QString deviceId(const QString &device) const;
        bool nearestFpsRangue(const AkFrac &fps,
                              jint &min,
                              jint &max);
        QVariantList controlBoolean(const QJniObject &characteristics,
                                    const DeviceControl &control) const;
        bool setControlBoolean(QJniObject &requestBuilder,
                               const DeviceControl &control,
                               bool value) const;
        QVariantList controlMenu(const QJniObject &characteristics,
                                 const DeviceControl &control) const;
        bool setControlMenu(QJniObject &requestBuilder,
                            const DeviceControl &control,
                            int index) const;
        QVariantList controlInteger(const QJniObject &characteristics,
                                    const DeviceControl &control) const;
        bool setControlInteger(QJniObject &requestBuilder,
                               const DeviceControl &control,
                               int value) const;
        QVariantList controls(const QJniObject &characteristics,
                              const ControlVector &controls) const;
        bool setControls(QJniObject &requestBuilder,
                         const ControlVector &controls,
                         const QVariantMap &values);
        QVariantList imageControls(const QJniObject &characteristics) const;
        bool setImageControls(QJniObject &requestBuilder,
                              const QVariantMap &imageControls);
        QVariantList cameraControls(const QJniObject &characteristics) const;
        bool setCameraControls(QJniObject &requestBuilder,
                               const QVariantMap &cameraControls);
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        qreal cameraRotation(const QString &cameraId) const;
        static void sessionConfigured(JNIEnv *env,
                                      jobject obj,
                                      jlong userPtr,
                                      jobject session);
        static void sessionConfigureFailed(JNIEnv *env,
                                           jobject obj,
                                           jlong userPtr,
                                           jobject session);
        static void cameraOpened(JNIEnv *env,
                                 jobject obj,
                                 jlong userPtr,
                                 jobject device);
        static void cameraDisconnected(JNIEnv *env,
                                       jobject obj,
                                       jlong userPtr,
                                       jobject device);
        static void cameraFailed(JNIEnv *env,
                                 jobject obj,
                                 jlong userPtr,
                                 jobject device,
                                 jint error);
        static void imageAvailable(JNIEnv *env,
                                   jobject obj,
                                   jlong userPtr,
                                   jobject image);
        FpsRanges availableFpsRanges(const QJniObject &cameraCharacteristics);
        bool isTorchSupported(const QJniObject &cameraCharacteristics);
        void setTorchMode(Capture::TorchMode mode);
        jint cameraFacing(const QJniObject &cameraCharacteristics) const;
        void updateDevices();
        template<typename T>
        static inline T alignUp(const T &value, const T &align)
        {
            return (value + align - 1) & ~(align - 1);
        }
};

CaptureAndroidCamera::CaptureAndroidCamera(QObject *parent):
    Capture(parent)
{
    this->d = new CaptureAndroidCameraPrivate(this);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto permissionStatus = qApp->checkPermission(this->d->m_cameraPermission);

    if (permissionStatus != Qt::PermissionStatus::Granted) {
        this->d->m_permissionResultReady = false;
        qApp->requestPermission(this->d->m_cameraPermission,
                                this,
                                [this, permissionStatus] (const QPermission &permission) {
                                    if (permissionStatus != permission.status()) {
                                        PermissionStatus curStatus = PermissionStatus_Granted;

                                        switch (permission.status()) {
                                        case Qt::PermissionStatus::Undetermined:
                                            curStatus = PermissionStatus_Undetermined;
                                            break;

                                        case Qt::PermissionStatus::Granted:
                                            curStatus = PermissionStatus_Granted;
                                            break;

                                        case Qt::PermissionStatus::Denied:
                                            curStatus = PermissionStatus_Denied;
                                            break;

                                        default:
                                            break;
                                        }

                                        emit this->permissionStatusChanged(curStatus);
                                    }

                                    this->d->m_permissionResultReady = true;
                                });

        while (!this->d->m_permissionResultReady) {
            auto eventDispatcher = QThread::currentThread()->eventDispatcher();

            if (eventDispatcher)
                eventDispatcher->processEvents(QEventLoop::AllEvents);
        }
    }
#endif

    this->d->updateDevices();
}

CaptureAndroidCamera::~CaptureAndroidCamera()
{
    delete this->d;
}

QStringList CaptureAndroidCamera::webcams() const
{
    return this->d->m_devices;
}

QString CaptureAndroidCamera::device() const
{
    return this->d->m_device;
}

QList<int> CaptureAndroidCamera::streams()
{
    if (!this->d->m_streams.isEmpty())
        return this->d->m_streams;

    auto caps = this->caps(this->d->m_device);

    if (caps.isEmpty())
        return {};

    return {0};
}

QList<int> CaptureAndroidCamera::listTracks(AkCaps::CapsType type)
{
    if (type != AkCaps::CapsVideo && type != AkCaps::CapsUnknown)
        return {};

    auto caps = this->caps(this->d->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CaptureAndroidCamera::ioMethod() const
{
    return {};
}

int CaptureAndroidCamera::nBuffers() const
{
    return 0;
}

QString CaptureAndroidCamera::description(const QString &webcam) const
{
    return this->d->m_descriptions.value(webcam);
}

AkCapsList CaptureAndroidCamera::caps(const QString &webcam) const
{
    return this->d->m_devicesCaps.value(webcam);
}

QVariantList CaptureAndroidCamera::imageControls() const
{
    return this->d->m_globalImageControls;
}

bool CaptureAndroidCamera::setImageControls(const QVariantMap &imageControls)
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

bool CaptureAndroidCamera::resetImageControls()
{
    QVariantMap controls;

    for (auto &control: this->imageControls()) {
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList CaptureAndroidCamera::cameraControls() const
{
    return this->d->m_globalCameraControls;
}

bool CaptureAndroidCamera::setCameraControls(const QVariantMap &cameraControls)
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

bool CaptureAndroidCamera::resetCameraControls()
{
    QVariantMap controls;

    for (auto &control: this->cameraControls()) {
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket CaptureAndroidCamera::readFrame()
{
    if (this->d->m_camera.isValid()) {
        if (this->d->m_requestBuilder.isValid()) {
            this->d->m_controlsMutex.lockForRead();
            auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
            this->d->m_controlsMutex.unlock();
            bool apply = false;

            if (this->d->m_localImageControls != imageControls) {
                auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                                 imageControls);
                apply |= this->d->setImageControls(this->d->m_requestBuilder, controls);
                this->d->m_localImageControls = imageControls;
            }

            this->d->m_controlsMutex.lockForRead();
            auto cameraControls = this->d->controlStatus(this->d->m_globalCameraControls);
            this->d->m_controlsMutex.unlock();

            if (this->d->m_localCameraControls != cameraControls) {
                auto controls = this->d->mapDiff(this->d->m_localCameraControls,
                                                 cameraControls);
                apply |= this->d->setCameraControls(this->d->m_requestBuilder, controls);
                this->d->m_localCameraControls = cameraControls;
            }

            if (apply) {
                this->d->m_cameraSession.callMethod<void>("stopRepeating", "()V");
                auto request =
                        this->d->m_requestBuilder.callObjectMethod("build",
                                                                   "()Landroid/hardware/camera2/CaptureRequest;");
                auto captureSessionCB =
                        this->d->m_callbacks.callObjectMethod("captureSessionCB",
                                                              "()L" JCLASS(AkAndroidCameraCallbacks) "$CaptureSessionCallback;");
                auto looper =
                        this->d->m_context.callObjectMethod("getMainLooper",
                                                            "()Landroid/os/Looper;");
                QJniObject handler("android/os/Handler",
                                   "(Landroid/os/Looper;)V",
                                   looper.object());
                this->d->m_cameraSession.callMethod<jint>("setRepeatingRequest",
                                                          "(Landroid/hardware/camera2/CaptureRequest;"
                                                          "Landroid/hardware/camera2/CameraCaptureSession$CaptureCallback;"
                                                          "Landroid/os/Handler;)I",
                                                          request.object(),
                                                          captureSessionCB.object(),
                                                          handler.object());
            }
        }
    }

    this->d->m_mutex.lockForWrite();

    if (!this->d->m_curPacket)
        if (!this->d->m_packetReady.wait(&this->d->m_mutex, 1000)) {
            this->d->m_mutex.unlock();

            return {};
        }

    auto packet = this->d->m_curPacket;
    this->d->m_curPacket = {};
    this->d->m_mutex.unlock();

    if (!this->d->m_rotate)
        return packet;

    auto angle = this->d->cameraRotation(this->d->m_curDeviceId);
    this->d->m_rotate->setProperty("angle", angle);

    return this->d->m_rotate->iStream(packet);
}

bool CaptureAndroidCamera::isTorchSupported() const
{
    return this->d->m_isTorchSupported.value(this->d->m_device);
}

Capture::TorchMode CaptureAndroidCamera::torchMode() const
{
    return this->d->m_torchMode;
}

Capture::PermissionStatus CaptureAndroidCamera::permissionStatus() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    switch (qApp->checkPermission(this->d->m_cameraPermission)) {
    case Qt::PermissionStatus::Undetermined:
        return PermissionStatus_Undetermined;

    case Qt::PermissionStatus::Granted:
        return PermissionStatus_Granted;

    case Qt::PermissionStatus::Denied:
        return PermissionStatus_Denied;

    default:
        break;
    }
#endif
    return PermissionStatus_Granted;
}

bool CaptureAndroidCamera::init()
{
    if (!this->d->m_cameraManager.isValid())
        return false;

    this->d->m_localImageControls.clear();
    this->d->m_localCameraControls.clear();

    this->uninit();

    this->d->m_cameraOpened = false;
    this->d->m_curDeviceId = this->d->deviceId(this->d->m_device);
    auto deviceId = QJniObject::fromString(this->d->m_curDeviceId);
    auto deviceStateCB =
        this->d->m_callbacks.callObjectMethod("deviceStateCB",
                                              "()L" JCLASS(AkAndroidCameraCallbacks) "$DeviceStateCallback;");
    auto looper =
        this->d->m_context.callObjectMethod("getMainLooper",
                                            "()Landroid/os/Looper;");
    QJniObject handler("android/os/Handler",
                       "(Landroid/os/Looper;)V",
                       looper.object());
    this->d->m_cameraManager.callMethod<void>("openCamera",
                                              "(Ljava/lang/String;"
                                              "Landroid/hardware/camera2/CameraDevice$StateCallback;"
                                              "Landroid/os/Handler;)V",
                                              deviceId.object(),
                                              deviceStateCB.object(),
                                              handler.object());
    this->d->m_mutex.lockForRead();
    this->d->m_cameraOpenedReady.wait(&this->d->m_mutex);
    this->d->m_mutex.unlock();

    if (!this->d->m_cameraOpened) {
        this->uninit();

        return false;
    }

    this->d->m_mutex.lockForWrite();
    this->d->m_sessionConfiguredReady.wait(&this->d->m_mutex);
    this->d->m_mutex.unlock();

    if (!this->d->m_sessionConfigured) {
        this->uninit();

        return false;
    }

    this->setTorchMode(Capture::Torch_Off);

    this->d->m_id = Ak::id();
    this->d->m_timeBase = this->d->m_fps.invert();

    return true;
}

void CaptureAndroidCamera::uninit()
{
    this->d->m_mutex.lockForWrite();

    if (this->d->m_cameraSession.isValid()) {
        this->d->m_cameraSession.callMethod<void>("stopRepeating", "()V");
        this->d->m_cameraSession.callMethod<void>("close", "()V");
        this->d->m_cameraSession = {};
    }

    if (this->d->m_camera.isValid()) {
        this->d->m_camera.callMethod<void>("close", "()V");
        this->d->m_camera = {};
    }

    if (this->d->m_imageReader.isValid()) {
        this->d->m_imageReader.callMethod<void>("close", "()V");
        this->d->m_imageReader = {};
    }

    this->d->m_curDeviceId = {};
    this->d->m_mutex.unlock();
}

void CaptureAndroidCamera::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    auto wasTorchSupported =
            this->d->m_isTorchSupported.value(this->d->m_device);
    this->d->m_device = device;

    if (device.isEmpty()) {
        this->d->m_controlsMutex.lockForWrite();
        this->d->m_globalImageControls.clear();
        this->d->m_globalCameraControls.clear();
        this->d->m_controlsMutex.unlock();
    } else {
        this->d->m_controlsMutex.lockForWrite();

        if (this->d->m_cameraManager.isValid()) {
            auto cameraId = QJniObject::fromString(this->d->deviceId(device));
            auto characteristics =
                    this->d->m_cameraManager.callObjectMethod("getCameraCharacteristics",
                                                              "(Ljava/lang/String;)Landroid/hardware/camera2/CameraCharacteristics;",
                                                              cameraId.object());

            if (characteristics.isValid()) {
                this->d->m_globalImageControls =
                        this->d->imageControls(characteristics);
                this->d->m_globalCameraControls =
                        this->d->cameraControls(characteristics);
            }
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

    bool isTorchSupported =
            this->d->m_isTorchSupported.value(this->d->m_device);

    if (wasTorchSupported != isTorchSupported)
        emit this->isTorchSupportedChanged(isTorchSupported);

    this->setTorchMode(Torch_Off);
}

void CaptureAndroidCamera::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    auto stream = streams[0];

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

void CaptureAndroidCamera::setIoMethod(const QString &ioMethod)
{
    Q_UNUSED(ioMethod)
}

void CaptureAndroidCamera::setNBuffers(int nBuffers)
{
    Q_UNUSED(nBuffers)
}

void CaptureAndroidCamera::setTorchMode(TorchMode mode)
{
    if (this->d->m_torchMode == mode)
        return;

    this->d->m_torchMode = mode;
    this->d->setTorchMode(mode);
    emit this->torchModeChanged(mode);
}

void CaptureAndroidCamera::resetDevice()
{
    this->setDevice("");
}

void CaptureAndroidCamera::resetStreams()
{
    auto supportedCaps = this->caps(this->d->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void CaptureAndroidCamera::resetIoMethod()
{
    this->setIoMethod("any");
}

void CaptureAndroidCamera::resetNBuffers()
{
    this->setNBuffers(32);
}

void CaptureAndroidCamera::resetTorchMode()
{
    this->setTorchMode(Torch_Off);
}

void CaptureAndroidCamera::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

CaptureAndroidCameraPrivate::CaptureAndroidCameraPrivate(CaptureAndroidCamera *self):
    self(self)
{
    this->m_context =
        qApp->nativeInterface<QNativeInterface::QAndroidApplication>()->context();
    this->m_threadPool.setMaxThreadCount(16);
    this->registerNatives();
    jlong userPtr = intptr_t(this);
    this->m_callbacks =
            QJniObject(JCLASS(AkAndroidCameraCallbacks), "(J)V", userPtr);
}

void CaptureAndroidCameraPrivate::registerNatives()
{
    static bool ready = false;

    if (ready)
        return;

    if (auto jclass = this->m_jenv.findClass(JCLASS(AkAndroidCameraCallbacks))) {
        static const QVector<JNINativeMethod> androidCameraMethods {
            {"sessionConfigured"     , "(JLandroid/hardware/camera2/CameraCaptureSession;)V", reinterpret_cast<void *>(CaptureAndroidCameraPrivate::sessionConfigured)     },
            {"sessionConfigureFailed", "(JLandroid/hardware/camera2/CameraCaptureSession;)V", reinterpret_cast<void *>(CaptureAndroidCameraPrivate::sessionConfigureFailed)},
            {"cameraOpened"          , "(JLandroid/hardware/camera2/CameraDevice;)V"        , reinterpret_cast<void *>(CaptureAndroidCameraPrivate::cameraOpened)          },
            {"cameraDisconnected"    , "(JLandroid/hardware/camera2/CameraDevice;)V"        , reinterpret_cast<void *>(CaptureAndroidCameraPrivate::cameraDisconnected)    },
            {"cameraFailed"          , "(JLandroid/hardware/camera2/CameraDevice;I)V"       , reinterpret_cast<void *>(CaptureAndroidCameraPrivate::cameraFailed)          },
            {"imageAvailable"        , "(JLandroid/media/Image;)V"                          , reinterpret_cast<void *>(CaptureAndroidCameraPrivate::imageAvailable)        },
        };

        this->m_jenv->RegisterNatives(jclass,
                                      androidCameraMethods.data(),
                                      androidCameraMethods.size());
    }

    ready = true;
}

AkCapsList CaptureAndroidCameraPrivate::caps(const QString &deviceId)
{
    if (!this->m_cameraManager.isValid())
        return {};

    auto cameraId = QJniObject::fromString(deviceId);
    auto characteristics =
            this->m_cameraManager.callObjectMethod("getCameraCharacteristics",
                                                   "(Ljava/lang/String;)Landroid/hardware/camera2/CameraCharacteristics;",
                                                   cameraId.object());
    auto fpsRanges = this->availableFpsRanges(characteristics);

    return this->caps(characteristics, fpsRanges);
}

AkCapsList CaptureAndroidCameraPrivate::caps(const QJniObject &characteristics,
                                                   const FpsRanges &fpsRanges)
{
    auto configMapKey =
        QJniObject::getStaticObjectField("android/hardware/camera2/CameraCharacteristics",
                                         "SCALER_STREAM_CONFIGURATION_MAP",
                                         "Landroid/hardware/camera2/CameraCharacteristics$Key;");
    auto configMap =
            characteristics.callObjectMethod("get",
                                             "(Landroid/hardware/camera2/CameraCharacteristics$Key;)Ljava/lang/Object;",
                                             configMapKey.object());

    QVector<AkFrac> supportedFrameRates;

    for (auto &fpsRange: fpsRanges) {
        AkFrac fps(fpsRange.first, 1);

        if (!supportedFrameRates.contains(fps))
            supportedFrameRates << fps;

        fps = {fpsRange.second, 1};

        if (!supportedFrameRates.contains(fps))
            supportedFrameRates << fps;
    }

    if (supportedFrameRates.isEmpty())
        supportedFrameRates << AkFrac(30, 1);

    std::sort(supportedFrameRates.rbegin(), supportedFrameRates.rend());

    auto formats =
            configMap.callObjectMethod("getOutputFormats", "()[I");
    auto numFormats =
            this->m_jenv->GetArrayLength(jobjectArray(formats.object()));
    QVector<jint> formatsList(numFormats);
    this->m_jenv->GetIntArrayRegion(jintArray(formats.object()),
                                    0,
                                    numFormats,
                                    formatsList.data());

    QVector<AkVideoCaps> rawCaps;

    for (auto &format: formatsList) {
        if (!androidFmtToAkFmt->contains(ImageFormat(format)))
            continue;

        auto sizes =
                configMap.callObjectMethod("getOutputSizes",
                                           "(I)[Landroid/util/Size;",
                                           format);
        auto numSizes =
                this->m_jenv->GetArrayLength(jobjectArray(sizes.object()));

        for (jsize i = 0; i < numSizes; i++) {
            QJniObject size =
                this->m_jenv->GetObjectArrayElement(jobjectArray(sizes.object()),
                                                    i);

            if (!size.isValid())
                continue;

            auto width = size.callMethod<jint>("getWidth", "()I");
            auto height = size.callMethod<jint>("getHeight", "()I");

            for (auto &fps: supportedFrameRates) {
                auto akFormat =
                        androidFmtToAkFmt->value(format,
                                                 AkVideoCaps::Format_none);

                if (akFormat != AkVideoCaps::Format_none)
                    rawCaps << AkVideoCaps(akFormat, width, height, fps);
            }
        }
    }

    std::sort(rawCaps.begin(), rawCaps.end());
    AkCapsList caps;

    for (auto &rcaps: rawCaps)
        caps << rcaps;

    return caps;
}

QString CaptureAndroidCameraPrivate::deviceId(const QString &device) const
{
    auto idStr = device;

    return idStr.remove("JniCamera:");
}

bool CaptureAndroidCameraPrivate::nearestFpsRangue(const AkFrac &fps,
                                                   jint &min,
                                                   jint &max)
{
    auto fpsValue = fps.value();
    bool ok = false;
    min = 0;
    max = 0;

    for (auto &fpsRange: this->m_availableFpsRanges[this->m_device]) {
        if (fpsValue >= qreal(fpsRange.first)
            && fpsValue <= qreal(fpsRange.second)) {
            min = fpsRange.first;
            max = fpsRange.second;
            ok = true;
        }

        if (ok)
            break;
    }

    return ok;
}

QVariantList CaptureAndroidCameraPrivate::controlBoolean(const QJniObject &characteristics,
                                                         const DeviceControl &control) const
{
    auto isAvailable =
            control.range(characteristics)
                   .callMethod<jboolean>("booleanValue", "()Z");

    if (!isAvailable)
        return {};

    auto value = control.defaultValueFunc().toBool();

    return QVariantList {
        control.description,
        "boolean",
        0,
        1,
        1,
        value,
        value,
        QStringList()
    };
}

bool CaptureAndroidCameraPrivate::setControlBoolean(QJniObject &requestBuilder,
                                                    const DeviceControl &control,
                                                    bool value) const
{
    control.requestKeyFunc(requestBuilder, value);

    return true;
}

QVariantList CaptureAndroidCameraPrivate::controlMenu(const QJniObject &characteristics,
                                                      const DeviceControl &control) const
{
    auto options = control.menuKeyFunc(characteristics);

    if (!options.isValid())
        return {};

    auto numOptions =
            this->m_jenv->GetArrayLength(jintArray(options.object()));

    if (numOptions < 2)
        return {};

    QStringList menuOptions;
    QVector<jint> intMenuOptions(numOptions);
    this->m_jenv->GetIntArrayRegion(jintArray(options.object()),
                                    0,
                                    numOptions,
                                    intMenuOptions.data());
    auto findOptionStr = [&control] (jint optionValue) -> QString {
        auto it = std::find_if(control.menuOptions.begin(),
                               control.menuOptions.end(),
                               [optionValue] (const ControlMenu &opt) {
            return opt.keyFunc().toInt() == optionValue;
        });

        return it != control.menuOptions.end()? it->description: QString();
    };

    for (auto &option: intMenuOptions) {
        auto optionStr = findOptionStr(option);

        if (!optionStr.isEmpty() && !menuOptions.contains(optionStr))
            menuOptions << optionStr;
    }

    auto value = control.defaultValueFunc().toInt();
    value = qMax(intMenuOptions.indexOf(value), 0);

    return QVariantList {
        control.description,
        "menu",
        0,
        menuOptions.size() - 1,
        1,
        value,
        value,
        menuOptions
    };
}

bool CaptureAndroidCameraPrivate::setControlMenu(QJniObject &requestBuilder,
                                                 const DeviceControl &control,
                                                 int index) const
{
    if (index < 0)
        return false;

    auto findOptionValue = [&control] (const QVariantList &controls,
                                       int index) -> jint {
        // Get option description from the menu index.
        auto it = std::find_if(controls.begin(),
                               controls.end(),
                               [&control] (const QVariant &cameraControl) -> bool {
            auto ctrl = cameraControl.toList();

            return ctrl.value(0).toString() == control.description;
        });

        if (it == controls.end())
            return -1;

        auto ctrl = it->toList();
        auto menuOptions = ctrl.value(7).toStringList();

        if (index >= menuOptions.size())
            return -1;

        auto optionDescription = menuOptions.value(index);

        // Get Option value from the description.
        auto mit = std::find_if(control.menuOptions.begin(),
                                control.menuOptions.end(),
                                [&optionDescription] (const ControlMenu &menu) -> bool {
            return menu.description == optionDescription;
        });

        if (mit == control.menuOptions.end())
            return -1;

        return mit->keyFunc().toInt();
    };

    auto value = findOptionValue(this->m_globalImageControls, index);

    if (value < 0)
        value = findOptionValue(this->m_globalCameraControls, index);

    if (value < 0)
        return false;

    control.requestKeyFunc(requestBuilder, value);

    return true;
}

QVariantList CaptureAndroidCameraPrivate::controlInteger(const QJniObject &characteristics,
                                                         const DeviceControl &control) const
{
    auto range = control.range(characteristics);
    auto lower = range.callObjectMethod("getLower", "()Ljava/lang/Comparable;");
    auto upper = range.callObjectMethod("getUpper", "()Ljava/lang/Comparable;");

    auto min = lower.callMethod<jint>("intValue", "()I");
    auto max = upper.callMethod<jint>("intValue", "()I");

    if (min == max)
        return {};

    auto value = control.defaultValueFunc().toInt();
    value = qBound(min, value, max);
    auto step = control.step(characteristics).callMethod<jint>("intValue", "()I");

    return QVariantList {
        control.description,
        "integer",
        min,
        max,
        step,
        value,
        value,
        QStringList()
    };
}

bool CaptureAndroidCameraPrivate::setControlInteger(QJniObject &requestBuilder,
                                                    const DeviceControl &control,
                                                    int value) const
{
    control.requestKeyFunc(requestBuilder, value);

    return true;
}

QVariantList CaptureAndroidCameraPrivate::controls(const QJniObject &characteristics,
                                                   const ControlVector &controls) const
{
    QVariantList controlsList;

    for (auto &control: controls) {
        QVariantList params;

        switch (control.type) {
        case ControlType::Integer:
            params = this->controlInteger(characteristics, control);
            break;

        case ControlType::Boolean:
            params = this->controlBoolean(characteristics, control);
            break;

        case ControlType::Menu:
            params = this->controlMenu(characteristics, control);
            break;
        }

        if (!params.isEmpty())
            controlsList << QVariant(params);
    }

    return controlsList;
}

bool CaptureAndroidCameraPrivate::setControls(QJniObject &requestBuilder,
                                              const ControlVector &controls,
                                              const QVariantMap &values)
{
    bool ok = true;

    for (auto &control: controls) {
        if (!values.contains(control.description))
            continue;

        int value = values.value(control.description).toInt();

        switch (control.type) {
        case ControlType::Integer:
            ok &= this->setControlInteger(requestBuilder, control, value);
            break;

        case ControlType::Boolean:
            ok &= this->setControlBoolean(requestBuilder, control, value);
            break;

        case ControlType::Menu:
            ok &= this->setControlMenu(requestBuilder, control, value);
            break;
        }
    }

    return ok;
}

QVariantList CaptureAndroidCameraPrivate::imageControls(const QJniObject &characteristics) const
{
    return this->controls(characteristics, *globalImageControls);
}

bool CaptureAndroidCameraPrivate::setImageControls(QJniObject &requestBuilder,
                                                   const QVariantMap &imageControls)
{
    return this->setControls(requestBuilder, *globalImageControls, imageControls);
}

QVariantList CaptureAndroidCameraPrivate::cameraControls(const QJniObject &characteristics) const
{
    return this->controls(characteristics, *globalCameraControls);
}

bool CaptureAndroidCameraPrivate::setCameraControls(QJniObject &requestBuilder,
                                                    const QVariantMap &cameraControls)
{
    return this->setControls(requestBuilder, *globalCameraControls, cameraControls);
}

QVariantMap CaptureAndroidCameraPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlName = params[0].toString();
        controlStatus[controlName] = params[6];
    }

    return controlStatus;
}

QVariantMap CaptureAndroidCameraPrivate::mapDiff(const QVariantMap &map1,
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

qreal CaptureAndroidCameraPrivate::cameraRotation(const QString &cameraId) const
{
    auto windowManager =
        this->m_context.callObjectMethod("getWindowManager",
                                         "()Landroid/view/WindowManager;");
    auto display =
            windowManager.callObjectMethod("getDefaultDisplay",
                                           "()Landroid/view/Display;");
    int screenRotation = 0;

    switch (display.callMethod<jint>("getRotation", "()I")) {
    case SURFACE_ROTATION_0:
        screenRotation = 0;

        break;
    case SURFACE_ROTATION_90:
        screenRotation = 90;

        break;
    case SURFACE_ROTATION_180:
        screenRotation = 180;

        break;
    case SURFACE_ROTATION_270:
        screenRotation = 270;

        break;
    default:
        break;
    }

    auto camId = QJniObject::fromString(cameraId);

    if (!camId.isValid() || !this->m_cameraManager.isValid())
        return 0.0;

    auto characteristics =
            this->m_cameraManager.callObjectMethod("getCameraCharacteristics",
                                                   "(Ljava/lang/String;)"
                                                   "Landroid/hardware/camera2/CameraCharacteristics;",
                                                   camId.object());
    auto facing = this->cameraFacing(characteristics);
    auto sensorOrientationKey =
        QJniObject::getStaticObjectField("android/hardware/camera2/CameraCharacteristics",
                                         "SENSOR_ORIENTATION",
                                         "Landroid/hardware/camera2/CameraCharacteristics$Key;");
    auto sensorOrientation =
            characteristics.callObjectMethod("get",
                                             "(Landroid/hardware/camera2/CameraCharacteristics$Key;)"
                                             "Ljava/lang/Object;",
                                             sensorOrientationKey.object());
    jint cameraRotation = 0;

    if (sensorOrientation.isValid())
        cameraRotation =
            sensorOrientation.callMethod<jint>("intValue", "()I");

    int rotation = 0;

    switch (facing) {
    case LENS_FACING_FRONT:
        rotation = (cameraRotation + screenRotation) % 360;

        break;

    case LENS_FACING_BACK:
        rotation = (cameraRotation - screenRotation + 360) % 360;

        break;

    default:
        break;
    }

    return -rotation;
}

void CaptureAndroidCameraPrivate::sessionConfigured(JNIEnv *env,
                                                    jobject obj,
                                                    jlong userPtr,
                                                    jobject session)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)

    auto self = reinterpret_cast<CaptureAndroidCameraPrivate *>(intptr_t(userPtr));

    self->m_cameraSession = session;
    auto previewTemplate =
        QJniObject::getStaticField<jint>("android/hardware/camera2/CameraDevice",
                                         "TEMPLATE_PREVIEW");
    self->m_requestBuilder =
            self->m_camera.callObjectMethod("createCaptureRequest",
                                            "(I)Landroid/hardware/camera2/CaptureRequest$Builder;",
                                            previewTemplate);
    auto surface =
            self->m_imageReader.callObjectMethod("getSurface",
                                                 "()Landroid/view/Surface;");
    self->m_requestBuilder.callMethod<void>("addTarget",
                                            "(Landroid/view/Surface;)V",
                                            surface.object());

    jint fpsMin = 0;
    jint fpsMax = 0;

    if (!self->nearestFpsRangue(self->m_fps, fpsMin, fpsMax)) {
        if (self->m_cameraSession.isValid()) {
            self->m_cameraSession.callMethod<void>("close", "()V");
            self->m_cameraSession = {};
        }

        if (self->m_camera.isValid()) {
            self->m_camera.callMethod<void>("close", "()V");
            self->m_camera = {};
        }

        if (self->m_imageReader.isValid()) {
            self->m_imageReader.callMethod<void>("close", "()V");
            self->m_imageReader = {};
        }

        self->m_mutex.lockForWrite();
        self->m_sessionConfiguredReady.wakeAll();
        self->m_mutex.unlock();

        return;
    }

    auto lower =
        QJniObject::callStaticObjectMethod("java/lang/Integer",
                                           "valueOf",
                                           "(I)Ljava/lang/Integer;",
                                           fpsMin);
    auto upper =
        QJniObject::callStaticObjectMethod("java/lang/Integer",
                                           "valueOf",
                                           "(I)Ljava/lang/Integer;",
                                           fpsMax);
    QJniObject fpsRange("android/util/Range",
                        "(Ljava/lang/Comparable;"
                        "Ljava/lang/Comparable;)V",
                        lower.object(),
                        upper.object());
    auto fpsRangeKey =
        QJniObject::getStaticObjectField("android/hardware/camera2/CaptureRequest",
                                         "CONTROL_AE_TARGET_FPS_RANGE",
                                         "Landroid/hardware/camera2/CaptureRequest$Key;");
    self->m_requestBuilder.callMethod<void>("set",
                                            "(Landroid/hardware/camera2/CaptureRequest$Key;Ljava/lang/Object;)V",
                                            fpsRangeKey.object(),
                                            fpsRange.object());
    auto request =
            self->m_requestBuilder.callObjectMethod("build",
                                                    "()Landroid/hardware/camera2/CaptureRequest;");
    auto captureSessionCB =
        self->m_callbacks.callObjectMethod("captureSessionCB",
                                           "()L" JCLASS(AkAndroidCameraCallbacks) "$CaptureSessionCallback;");
    auto looper =
        self->m_context.callObjectMethod("getMainLooper",
                                         "()Landroid/os/Looper;");
    QJniObject handler("android/os/Handler",
                       "(Landroid/os/Looper;)V",
                       looper.object());
    self->m_cameraSession.callMethod<jint>("setRepeatingRequest",
                                           "(Landroid/hardware/camera2/CaptureRequest;"
                                           "Landroid/hardware/camera2/CameraCaptureSession$CaptureCallback;"
                                           "Landroid/os/Handler;)I",
                                           request.object(),
                                           captureSessionCB.object(),
                                           handler.object());
    self->m_sessionConfigured = true;

    self->m_mutex.lockForWrite();
    self->m_sessionConfiguredReady.wakeAll();
    self->m_mutex.unlock();
}

void CaptureAndroidCameraPrivate::sessionConfigureFailed(JNIEnv *env,
                                                         jobject obj,
                                                         jlong userPtr,
                                                         jobject session)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)
    Q_UNUSED(session)

    auto self = reinterpret_cast<CaptureAndroidCameraPrivate *>(intptr_t(userPtr));

    if (self->m_cameraSession.isValid()) {
        self->m_cameraSession.callMethod<void>("close", "()V");
        self->m_cameraSession = {};
    }

    if (self->m_camera.isValid()) {
        self->m_camera.callMethod<void>("close", "()V");
        self->m_camera = {};
    }

    if (self->m_imageReader.isValid()) {
        self->m_imageReader.callMethod<void>("close", "()V");
        self->m_imageReader = {};
    }

    self->m_mutex.lockForWrite();
    self->m_sessionConfiguredReady.wakeAll();
    self->m_mutex.unlock();
}

void CaptureAndroidCameraPrivate::cameraOpened(JNIEnv *env,
                                               jobject obj,
                                               jlong userPtr,
                                               jobject device)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)

    auto self = reinterpret_cast<CaptureAndroidCameraPrivate *>(intptr_t(userPtr));
    self->m_camera = device;
    auto streams = self->self->streams();

    if (streams.isEmpty()) {
        self->m_mutex.lockForWrite();
        self->m_cameraOpenedReady.wakeAll();
        self->m_mutex.unlock();

        return;
    }

    auto supportedCaps = self->self->caps(self->m_device);
    self->m_caps = supportedCaps[streams[0]];
    self->m_fps = self->m_caps.fps();

    self->m_imageReader =
            QJniObject::callStaticObjectMethod("android/media/ImageReader",
                                               "newInstance",
                                               "(IIII)Landroid/media/ImageReader;",
                                               jint(self->m_caps.width()),
                                               jint(self->m_caps.height()),
                                               androidFmtToAkFmt->key(self->m_caps.format(),
                                                                      ImageFormat::UNKNOWN),
                                               BUFFER_SIZE);

    if (!self->m_imageReader.isValid()) {
        self->m_mutex.lockForWrite();
        self->m_cameraOpenedReady.wakeAll();
        self->m_mutex.unlock();

        return;
    }

    auto surface =
            self->m_imageReader.callObjectMethod("getSurface",
                                                 "()Landroid/view/Surface;");

    if (!surface.isValid()) {
        self->m_mutex.lockForWrite();
        self->m_cameraOpenedReady.wakeAll();
        self->m_mutex.unlock();

        return;
    }

    auto looper =
        self->m_context.callObjectMethod("getMainLooper",
                                         "()Landroid/os/Looper;");
    QJniObject handler("android/os/Handler",
                       "(Landroid/os/Looper;)V",
                       looper.object());
    self->m_imageReader.callMethod<void>("setOnImageAvailableListener",
                                         "(Landroid/media/ImageReader$OnImageAvailableListener;"
                                         "Landroid/os/Handler;)V",
                                         self->m_callbacks.object(),
                                         handler.object());
    auto surfaces = self->m_callbacks.callObjectMethod("createSurfaceList",
                                                       "()Ljava/util/List;");
    surfaces.callMethod<jboolean>("add",
                                  "(Ljava/lang/Object;)Z",
                                  surface.object());
    self->m_sessionConfigured = false;
    auto captureSessionStateCB =
        self->m_callbacks.callObjectMethod("captureSessionStateCB",
                                           "()L" JCLASS(AkAndroidCameraCallbacks) "$CaptureSessionStateCallback;");
    self->m_camera.callMethod<void>("createCaptureSession",
                                    "(Ljava/util/List;"
                                    "Landroid/hardware/camera2/CameraCaptureSession$StateCallback;"
                                    "Landroid/os/Handler;)V",
                                    surfaces.object(),
                                    captureSessionStateCB.object(),
                                    handler.object());

    self->m_cameraOpened = true;

    self->m_mutex.lockForWrite();
    self->m_cameraOpenedReady.wakeAll();
    self->m_mutex.unlock();
}

void CaptureAndroidCameraPrivate::cameraDisconnected(JNIEnv *env,
                                                     jobject obj,
                                                     jlong userPtr,
                                                     jobject device)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)

    auto self = reinterpret_cast<CaptureAndroidCameraPrivate *>(intptr_t(userPtr));

    QJniObject(device).callMethod<void>("close", "()V");

    self->m_mutex.lockForWrite();
    self->m_cameraOpenedReady.wakeAll();
    self->m_mutex.unlock();
}

void CaptureAndroidCameraPrivate::cameraFailed(JNIEnv *env,
                                               jobject obj,
                                               jlong userPtr,
                                               jobject device,
                                               jint error)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)
    Q_UNUSED(device)
    Q_UNUSED(error)

    auto self = reinterpret_cast<CaptureAndroidCameraPrivate *>(intptr_t(userPtr));

    self->m_mutex.lockForWrite();
    self->m_cameraOpenedReady.wakeAll();
    self->m_mutex.unlock();
}

void CaptureAndroidCameraPrivate::imageAvailable(JNIEnv *env,
                                                 jobject obj,
                                                 jlong userPtr,
                                                 jobject image)
{
    Q_UNUSED(obj)

    auto self = reinterpret_cast<CaptureAndroidCameraPrivate *>(intptr_t(userPtr));

    if (!image)
        return;

    QJniObject src = image;
    auto planesArray = src.callObjectMethod("getPlanes",
                                            "()[Landroid/media/Image$Plane;");

    if (!planesArray.isValid())
        return;

    auto planes = env->GetArrayLength(jobjectArray(planesArray.object()));

    if (planes < 1)
        return;

    auto format = src.callMethod<jint>("getFormat", "()I");
    auto fmt = androidFmtToAkFmt->value(format, AkVideoCaps::Format_none);

    if (fmt == AkVideoCaps::Format_none)
        return;

    auto width = src.callMethod<jint>("getWidth", "()I");

    if (width < 1)
        return;

    auto height = src.callMethod<jint>("getHeight", "()I");

    if (height < 1)
        return;

    AkVideoPacket packet({fmt, width, height, self->m_fps}, true);

    for(jsize i = 0; i < planes; i++) {
        QJniObject plane =
                env->GetObjectArrayElement(jobjectArray(planesArray.object()),
                                           i);

        if (!plane.isValid())
            continue;

        auto iLineSize = plane.callMethod<jint>("getRowStride", "()I");

        if (iLineSize < 1)
            continue;

        auto pixelStride =  plane.callMethod<jint>("getPixelStride", "()I");

        if (iLineSize < 1)
            continue;

        auto pixelSize = packet.pixelSize(i);
        auto lineSize = qMin<size_t>(iLineSize, packet.lineSize(i));

        if (lineSize < 1)
            continue;

        auto byteBuffer = plane.callObjectMethod("getBuffer",
                                                 "()Ljava/nio/ByteBuffer;");

        if (!byteBuffer.isValid())
            continue;

        auto planeData =
                reinterpret_cast<quint8 *>(env->GetDirectBufferAddress(byteBuffer.object()));

        if (!planeData)
            continue;

        auto widthDiv = packet.widthDiv(i);
        auto heightDiv = packet.heightDiv(i);

        if (pixelStride == pixelSize)
            for (int y = 0; y < packet.caps().height(); ++y) {
                int ys = y >> heightDiv;
                auto srcLine = planeData + ys * iLineSize;
                auto dstLine = packet.line(i, y);
                memcpy(dstLine, srcLine, lineSize);
            }
        else if (pixelSize == 1)
            for (int y = 0; y < packet.caps().height(); ++y) {
                int ys = y >> heightDiv;
                auto srcLine = planeData + ys * iLineSize;
                auto dstLine = packet.line(i, y);

                for (int x = 0; x < packet.caps().width(); ++x) {
                    int xs = x >> widthDiv;
                    dstLine[xs] = srcLine[xs * pixelStride];
                }
            }
        else
            for (int y = 0; y < packet.caps().height(); ++y) {
                int ys = y >> heightDiv;
                auto srcLine = planeData + ys * iLineSize;
                auto dstLine = packet.line(i, y);

                for (int x = 0; x < packet.caps().width(); ++x) {
                    int xs = x >> widthDiv;
                    auto iPixel = srcLine + xs * pixelStride;
                    auto oPixel = dstLine + xs * pixelSize;

                    for (int i = 0; i < pixelSize; ++i)
                        oPixel[i] = iPixel[i];
                }
            }
    }

    jlong timestampNs = src.callMethod<jlong>("getTimestamp", "()J");

    auto pts = qint64(timestampNs * self->m_fps.value() / 1e9);
    packet.setPts(pts);
    packet.setDuration(1);
    packet.setTimeBase(self->m_fps.invert());
    packet.setIndex(0);
    packet.setId(self->m_id);

    self->m_mutex.lockForWrite();
    self->m_curPacket = packet;
    self->m_packetReady.wakeAll();
    self->m_mutex.unlock();
}

FpsRanges CaptureAndroidCameraPrivate::availableFpsRanges(const QJniObject &cameraCharacteristics)
{
    auto availableFpsRangesKey =
        QJniObject::getStaticObjectField("android/hardware/camera2/CameraCharacteristics",
                                         "CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES",
                                         "Landroid/hardware/camera2/CameraCharacteristics$Key;");
    auto availableFpsRanges =
            cameraCharacteristics.callObjectMethod("get",
                                                   "(Landroid/hardware/camera2/CameraCharacteristics$Key;)"
                                                   "Ljava/lang/Object;",
                                                   availableFpsRangesKey.object());

    if (!availableFpsRanges.isValid())
        return {};

    auto numFrameRates =
            this->m_jenv->GetArrayLength(jobjectArray(availableFpsRanges.object()));
    FpsRanges frameRates;

    for (jsize i = 0; i < numFrameRates; i++) {
        QJniObject frameRate =
            this->m_jenv->GetObjectArrayElement(jobjectArray(availableFpsRanges.object()),
                                                i);

        if (frameRate.isValid()) {
            auto lower = frameRate.callObjectMethod("getLower",
                                                    "()Ljava/lang/Comparable;");
            jint lowerInt = 0;

            if (lower.isValid())
                lowerInt = lower.callMethod<jint>("intValue", "()I");

            auto upper = frameRate.callObjectMethod("getUpper",
                                                    "()Ljava/lang/Comparable;");
            jint upperInt = 0;

            if (upper.isValid())
                upperInt = upper.callMethod<jint>("intValue", "()I");

            if (lowerInt > 0 && upperInt > 0)
                frameRates << QPair<int, int>(lowerInt, upperInt);
        }
    }

    return frameRates;
}

bool CaptureAndroidCameraPrivate::isTorchSupported(const QJniObject &cameraCharacteristics)
{
    // Check if the flash feature is available in this device.

    if (!this->m_context.isValid())
        return false;

    auto packageManager =
        this->m_context.callObjectMethod("getPackageManager",
                                         "()Landroid/content/pm/PackageManager;");

    if (!packageManager.isValid())
        return false;

    auto feature =
        QJniObject::getStaticObjectField("android/content/pm/PackageManager",
                                         "FEATURE_CAMERA_FLASH",
                                         "Ljava/lang/String;");

    if (!feature.isValid())
        return false;

    bool isSupported =
        packageManager.callMethod<jboolean>("hasSystemFeature",
                                            "(Ljava/lang/String;)Z",
                                            feature.object());

    if (!isSupported)
        return false;

    // Check if the camera has a flash unit.

    auto flashAvailableKey =
        QJniObject::getStaticObjectField("android/hardware/camera2/CameraCharacteristics",
                                         "FLASH_INFO_AVAILABLE",
                                         "Landroid/hardware/camera2/CameraCharacteristics$Key;");
    auto flashAvailable =
            cameraCharacteristics.callObjectMethod("get",
                                                   "(Landroid/hardware/camera2/CameraCharacteristics$Key;)"
                                                   "Ljava/lang/Object;",
                                                   flashAvailableKey.object());

    if (!flashAvailable.isValid())
        return false;

    return flashAvailable.callMethod<jboolean>("booleanValue", "()Z");
}

void CaptureAndroidCameraPrivate::setTorchMode(Capture::TorchMode mode)
{
    if (!this->m_isTorchSupported.value(this->m_device))
        return;

    if (!this->m_camera.isValid()) {
        if (!this->m_cameraManager.isValid())
            return;

        auto deviceId = QJniObject::fromString(this->deviceId(this->m_device));
        this->m_cameraManager.callMethod<void>("setTorchMode",
                                               "(Ljava/lang/String;Z)V",
                                               deviceId.object(),
                                               jboolean(mode == Capture::Torch_On));

        return;
    }

    this->m_cameraSession.callMethod<void>("stopRepeating", "()V");
    jint flashMode =
            mode == Capture::Torch_On?
                QJniObject::getStaticField<jint>("android/hardware/camera2/CameraMetadata",
                                                 "FLASH_MODE_TORCH"):
                QJniObject::getStaticField<jint>("android/hardware/camera2/CameraMetadata",
                                                 "FLASH_MODE_OFF");
    auto flashModeKey =
        QJniObject::getStaticObjectField("android/hardware/camera2/CaptureRequest",
                                         "FLASH_MODE",
                                         "Landroid/hardware/camera2/CaptureRequest$Key;");
    auto flashModeInteger =
        QJniObject::callStaticObjectMethod("java/lang/Integer",
                                           "valueOf",
                                           "(I)Ljava/lang/Integer;",
                                           flashMode);
    this->m_requestBuilder.callMethod<void>("set",
                                            "(Landroid/hardware/camera2/CaptureRequest$Key;Ljava/lang/Object;)V",
                                            flashModeKey.object(),
                                            flashModeInteger.object());

    auto request =
            this->m_requestBuilder.callObjectMethod("build",
                                                    "()Landroid/hardware/camera2/CaptureRequest;");
    auto captureSessionCB =
            this->m_callbacks.callObjectMethod("captureSessionCB",
                                               "()L" JCLASS(AkAndroidCameraCallbacks) "$CaptureSessionCallback;");
    auto looper =
            this->m_context.callObjectMethod("getMainLooper",
                                             "()Landroid/os/Looper;");
    QJniObject handler("android/os/Handler",
                       "(Landroid/os/Looper;)V",
                       looper.object());
    this->m_cameraSession.callMethod<jint>("setRepeatingRequest",
                                           "(Landroid/hardware/camera2/CaptureRequest;"
                                           "Landroid/hardware/camera2/CameraCaptureSession$CaptureCallback;"
                                           "Landroid/os/Handler;)I",
                                           request.object(),
                                           captureSessionCB.object(),
                                           handler.object());
}

jint CaptureAndroidCameraPrivate::cameraFacing(const QJniObject &cameraCharacteristics) const
{
    auto lensFacingKey =
        QJniObject::getStaticObjectField("android/hardware/camera2/CameraCharacteristics",
                                         "LENS_FACING",
                                         "Landroid/hardware/camera2/CameraCharacteristics$Key;");
    auto lensFacing =
            cameraCharacteristics.callObjectMethod("get",
                                                   "(Landroid/hardware/camera2/CameraCharacteristics$Key;)"
                                                   "Ljava/lang/Object;",
                                                   lensFacingKey.object());
    jint facing = LENS_FACING_EXTERNAL;

    if (lensFacing.isValid())
        facing =
            lensFacing.callMethod<jint>("intValue", "()I");

    return facing;
}

void CaptureAndroidCameraPrivate::updateDevices()
{
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;
    decltype(this->m_availableFpsRanges) availableFpsRanges;
    decltype(this->m_isTorchSupported) isTorchSupported;

    if (!this->m_cameraManager.isValid()) {
        auto cameraService =
            QJniObject::getStaticObjectField("android/content/Context",
                                             "CAMERA_SERVICE",
                                             "Ljava/lang/String;");
        this->m_cameraManager =
                this->m_context.callObjectMethod("getSystemService",
                                                 "(Ljava/lang/String;)Ljava/lang/Object;",
                                                 cameraService.object());
    }

    if (this->m_cameraManager.isValid()) {
        auto camerasArray =
                this->m_cameraManager.callObjectMethod("getCameraIdList",
                                                       "()[Ljava/lang/String;");

        if (camerasArray.isValid()) {
            static const QMap<jint, QString> facingToStr {
                {LENS_FACING_FRONT   , "Front"   },
                {LENS_FACING_BACK    , "Back"    },
                {LENS_FACING_EXTERNAL, "External"},
            };

            auto cameras =
                    this->m_jenv->GetArrayLength(jobjectArray(camerasArray.object()));

            for(jsize i = 0; i < cameras; i++) {
                QJniObject cameraId =
                        this->m_jenv->GetObjectArrayElement(jobjectArray(camerasArray.object()),
                                                            i);

                if (!cameraId.isValid())
                    continue;

                auto characteristics =
                        this->m_cameraManager.callObjectMethod("getCameraCharacteristics",
                                                               "(Ljava/lang/String;)"
                                                               "Landroid/hardware/camera2/CameraCharacteristics;",
                                                               cameraId.object());

                if (characteristics.isValid()) {
                    auto fpsRanges = this->availableFpsRanges(characteristics);
                    auto caps = this->caps(characteristics, fpsRanges);

                    if (!caps.empty()) {
                        auto index =
                                Capture::nearestResolution({DEFAULT_FRAME_WIDTH,
                                                            DEFAULT_FRAME_HEIGHT},
                                                           DEFAULT_FRAME_FPS,
                                                           caps);

                        if (index > 0)
                            caps.move(index, 0);

                        auto deviceId =
                                QString("JniCamera:%1").arg(cameraId.toString());
                        auto facing = this->cameraFacing(characteristics);
                        auto facingStr = facingToStr.value(facing, "External");
                        devices << deviceId;
                        descriptions[deviceId] =
                                QString("%1 Camera %2").arg(facingStr).arg(i);
                        devicesCaps[deviceId] = caps;
                        availableFpsRanges[deviceId] = fpsRanges;
                        isTorchSupported[deviceId] =
                                this->isTorchSupported(characteristics);
                    }
                }
            }
        }
    }

    if (devicesCaps.isEmpty()) {
        devices.clear();
        descriptions.clear();
        availableFpsRanges.clear();
        isTorchSupported.clear();
    }

    this->m_descriptions = descriptions;
    this->m_devicesCaps = devicesCaps;
    this->m_availableFpsRanges = availableFpsRanges;
    this->m_isTorchSupported = isTorchSupported;

    if (this->m_devices != devices) {
        this->m_devices = devices;
        emit self->webcamsChanged(this->m_devices);
    }
}

#include "moc_captureandroidcamera.cpp"

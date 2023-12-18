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
#include <QCoreApplication>
#include <QDateTime>
#include <QJniObject>
#include <QReadWriteLock>
#include <QScreen>
#include <QSet>
#include <QThread>
#include <QVariant>
#include <QWaitCondition>
#include <QWindow>
#include <QtConcurrent>
#include <QtCore/private/qandroidextras_p.h>
#include <ak.h>
#include <akcaps.h>
#include <akelement.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideoformatspec.h>
#include <akvideopacket.h>

#if defined(Q_OS_ANDROID) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QPermissions>
#endif

#include "captureandroidcamera.h"

#define JNAMESPACE "org/webcamoid/plugins/VideoCapture/submodules/androidcamera"
#define JCLASS(jclass) JNAMESPACE "/" #jclass

#define CAMERA_FACING_BACK     0
#define CAMERA_FACING_FRONT    1
#define CAMERA_FACING_EXTERNAL 2

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
    Zoom
};

struct DeviceControl
{
    ControlType type;
    QString name;
    QString description;
    QVariant defaultValue;
};

using ControlVector = QVector<DeviceControl>;

inline const ControlVector &initImageControls()
{
    static const ControlVector controls {
        {ControlType::Menu   , "SceneMode/s"         , "Scene Mode"             , "auto"},
        {ControlType::Boolean, "AutoWhiteBalanceLock", "Auto White Balance Lock", true  },
        {ControlType::Menu   , "WhiteBalance"        , "White Balance"          , "auto"},
        {ControlType::Boolean, "AutoExposureLock"    , "Auto Exposure Lock"     , true  },
        {ControlType::Integer, "ExposureCompensation", "Exposure Compensation"  , 0     },
        {ControlType::Menu   , "Antibanding"         , "Antibanding"            , "auto"},
        {ControlType::Boolean, "VideoStabilization"  , "Video Stabilization"    , true  },
        {ControlType::Menu   , "ColorEffect/s"       , "Color Effect"           , "none"},
    };

    return controls;
}

Q_GLOBAL_STATIC_WITH_ARGS(ControlVector,
                          globalImageControls,
                          (initImageControls()))

inline const ControlVector &initCameraControls()
{
    static const ControlVector controls {
        {ControlType::Menu, "FocusMode/s", "Focus Mode", "auto"},
        {ControlType::Zoom,            {},           {},      0},
    };

    return controls;
}

Q_GLOBAL_STATIC_WITH_ARGS(ControlVector,
                          globalCameraControls,
                          (initCameraControls()))

class CaptureAndroidCameraPrivate
{
    public:
        CaptureAndroidCamera *self;
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, CaptureVideoCaps> m_devicesCaps;
        QReadWriteLock m_controlsMutex;
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;
        QReadWriteLock m_mutex;
        QByteArray m_curBuffer;
        QWaitCondition m_waitCondition;
        AkFrac m_fps;
        AkFrac m_timeBase;
        AkVideoCaps m_caps;
        jint m_curDeviceId {-1};
        qint64 m_id {-1};
        QThreadPool m_threadPool;
        QJniObject m_context;
        QJniObject m_camera;
        QJniObject m_callbacks;
        QJniObject m_surfaceView;
        AkElementPtr m_rotate {akPluginManager->create<AkElement>("VideoFilter/Rotate")};
        bool m_isTorchSupported {false};
        Capture::TorchMode m_torchMode {Capture::Torch_Off};
        bool m_isCapturing {false};

        explicit CaptureAndroidCameraPrivate(CaptureAndroidCamera *self);
        void registerNatives();
        CaptureVideoCaps caps(jint device);
        jint deviceId(const QString &device) const;
        bool nearestFpsRangue(const QJniObject &parameters,
                              const AkFrac &fps,
                              jint &min,
                              jint &max);
        QVariantList controlBoolean(const QJniObject &parameters,
                                    const QString &name,
                                    const QString &description,
                                    bool defaultValue=false) const;
        bool setControlBoolean(QJniObject &parameters,
                               const QString &name,
                               bool value) const;
        QVariantList controlMenu(const QJniObject &parameters,
                                 const QString &name,
                                 const QString &description,
                                 const QString &defaultValue={}) const;
        bool setControlMenu(QJniObject &parameters,
                            const QString &name,
                            int index) const;
        QVariantList controlInteger(const QJniObject &parameters,
                                    const QString &name,
                                    const QString &description,
                                    int defaultValue=0) const;
        bool setControlInteger(QJniObject &parameters,
                               const QString &name,
                               int value) const;
        QVariantList controlZoom(const QJniObject &parameters,
                                 int defaultValue=0) const;
        bool setControlZoom(QJniObject &parameters,
                            int value) const;
        QVariantList controls(const QJniObject &parameters,
                              const ControlVector &controls) const;
        bool setControls(QJniObject &parameters,
                         const ControlVector &controls,
                         const QVariantMap &values);
        QVariantList imageControls(const QJniObject &parameters) const;
        bool setImageControls(QJniObject &parameters,
                              const QVariantMap &imageControls);
        QVariantList cameraControls(const QJniObject &parameters) const;
        bool setCameraControls(QJniObject &parameters,
                               const QVariantMap &cameraControls);
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        qreal cameraRotation(jint cameraId) const;
        static void previewFrameReady(JNIEnv *env,
                                      jobject obj,
                                      jlong userPtr,
                                      jbyteArray data);
        static void surfaceCreated(JNIEnv *env, jobject obj, jlong userPtr);
        static void surfaceDestroyed(JNIEnv *env, jobject obj, jlong userPtr);
        static void shutterActivated(JNIEnv *env, jobject obj, jlong userPtr);
        static void pictureTaken(JNIEnv *env,
                                 jobject obj,
                                 jlong userPtr,
                                 jint index,
                                 jbyteArray data);
        QString torchModeStr(const char *field) const;
        bool isTorchSupported();
        Capture::TorchMode torchMode();
        void setTorchMode(Capture::TorchMode mode);
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

    auto rotateFunc = [this] () {
        if (this->d->m_curDeviceId >= 0) {
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

#if QT_CONFIG(permissions) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QCameraPermission cameraPermission;

    switch (qApp->checkPermission(cameraPermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(cameraPermission,
                                this,
                                [this] (const QPermission &permission) {
                                    if (permission.status() == Qt::PermissionStatus::Granted)
                                        this->d->updateDevices();
                                });

        break;

    case Qt::PermissionStatus::Granted:
        this->d->updateDevices();

        break;

    default:
        break;
    }
#else
    this->d->updateDevices();
#endif
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

CaptureVideoCaps CaptureAndroidCamera::caps(const QString &webcam) const
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
        auto parameters =
                this->d->m_camera.callObjectMethod("getParameters",
                                                   "()Landroid/hardware/Camera$Parameters;");

        if (parameters.isValid()) {
            this->d->m_controlsMutex.lockForRead();
            auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
            this->d->m_controlsMutex.unlock();
            bool apply = false;

            if (this->d->m_localImageControls != imageControls) {
                auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                                 imageControls);
                apply |= this->d->setImageControls(parameters, controls);
                this->d->m_localImageControls = imageControls;
            }

            this->d->m_controlsMutex.lockForRead();
            auto cameraControls = this->d->controlStatus(this->d->m_globalCameraControls);
            this->d->m_controlsMutex.unlock();

            if (this->d->m_localCameraControls != cameraControls) {
                auto controls = this->d->mapDiff(this->d->m_localCameraControls,
                                                 cameraControls);
                apply |= this->d->setCameraControls(parameters, controls);
                this->d->m_localCameraControls = cameraControls;
            }

            if (apply)
                this->d->m_camera.callMethod<void>("setParameters",
                                                   "(Landroid/hardware/Camera$Parameters;)V",
                                                   parameters.object());
        }
    }

    AkPacket packet;
    auto timestamp = QDateTime::currentMSecsSinceEpoch();
    auto pts =
            qint64(timestamp
                   * this->d->m_timeBase.invert().value()
                   / 1e3);

    this->d->m_mutex.lockForWrite();

    if (this->d->m_curBuffer.isEmpty())
        this->d->m_waitCondition.wait(&this->d->m_mutex, 1000);

    if (!this->d->m_curBuffer.isEmpty()) {
        AkVideoPacket videoPacket(this->d->m_caps);
        auto iData = this->d->m_curBuffer.constData();

        for (int plane = 0; plane < videoPacket.planes(); ++plane) {
            auto iLineSize = CaptureAndroidCameraPrivate::alignUp<size_t>(videoPacket.bytesUsed(plane), 16);
            auto oLineSize = videoPacket.lineSize(plane);
            auto lineSize = qMin<size_t>(iLineSize, oLineSize);
            auto heightDiv = videoPacket.heightDiv(plane);

            for (int y = 0; y < videoPacket.caps().height(); ++y) {
                int ys = y >> heightDiv;
                memcpy(videoPacket.line(plane, y),
                       iData + ys * iLineSize,
                       lineSize);
            }

            iData += iLineSize * (videoPacket.caps().height() >> heightDiv);
        }

        videoPacket.setPts(pts);
        videoPacket.setTimeBase(this->d->m_timeBase);
        videoPacket.setIndex(0);
        videoPacket.setId(this->d->m_id);
        this->d->m_curBuffer.clear();
        packet = videoPacket;
    }

    this->d->m_mutex.unlock();

    return this->d->m_rotate->iStream(packet);
}

bool CaptureAndroidCamera::isTorchSupported() const
{
    return this->d->m_isTorchSupported;
}

Capture::TorchMode CaptureAndroidCamera::torchMode() const
{
    return this->d->m_torchMode;
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
            QJniObject(JCLASS(AkAndroidCameraCallbacks),
                              "(J)V",
                              userPtr);
}

void CaptureAndroidCameraPrivate::registerNatives()
{
    static bool ready = false;

    if (ready)
        return;

    QJniEnvironment jenv;

    if (auto jclass = jenv.findClass(JCLASS(AkAndroidCameraCallbacks))) {
        static const QVector<JNINativeMethod> methods {
            {"previewFrameReady"     , "(J[B)V" , reinterpret_cast<void *>(CaptureAndroidCameraPrivate::previewFrameReady)},
            {"notifySurfaceCreated"  , "(J)V"   , reinterpret_cast<void *>(CaptureAndroidCameraPrivate::surfaceCreated)   },
            {"notifySurfaceDestroyed", "(J)V"   , reinterpret_cast<void *>(CaptureAndroidCameraPrivate::surfaceDestroyed) },
            {"shutterActivated"      , "(J)V"   , reinterpret_cast<void *>(CaptureAndroidCameraPrivate::shutterActivated) },
            {"pictureTaken"          , "(JI[B)V", reinterpret_cast<void *>(CaptureAndroidCameraPrivate::pictureTaken)     },
        };

        jenv->RegisterNatives(jclass, methods.data(), methods.size());
    }

    ready = true;
}

CaptureVideoCaps CaptureAndroidCameraPrivate::caps(jint device)
{
    auto camera =
            QJniObject::callStaticObjectMethod("android/hardware/Camera",
                                                      "open",
                                                      "(I)Landroid/hardware/Camera;",
                                                      device);

    if (!camera.isValid())
        return {};

    auto parameters =
            camera.callObjectMethod("getParameters",
                                    "()Landroid/hardware/Camera$Parameters;");

    if (!parameters.isValid())
        return {};

    QSet<ImageFormat> supportedFormats;
    auto formats = parameters.callObjectMethod("getSupportedPreviewFormats",
                                               "()Ljava/util/List;");
    auto numFormats = formats.callMethod<jint>("size");

    for (jint i = 0; i < numFormats; i++) {
        auto jformat = formats.callObjectMethod("get",
                                                "(I)Ljava/lang/Object;",
                                                i);
        auto format = jformat.callMethod<jint>("intValue");

        if (!androidFmtToAkFmt->contains(ImageFormat(format)))
            continue;

        supportedFormats << ImageFormat(format);
    }

    QList<QSize> supportedSizes;
    auto sizes = parameters.callObjectMethod("getSupportedPreviewSizes",
                                             "()Ljava/util/List;");
    auto numSizes = sizes.callMethod<jint>("size");

    for (jint i = 0; i < numSizes; i++) {
        auto jsize = sizes.callObjectMethod("get",
                                            "(I)Ljava/lang/Object;",
                                            i);
        auto width = jsize.getField<jint>("width");
        auto height = jsize.getField<jint>("height");

        if (width < 1 || height < 1)
            continue;

        QSize size(width, height);

        if (!supportedSizes.contains(size))
            supportedSizes << size;
    }

    QList<AkFrac> supportedFrameRates;
    auto frameRates = parameters.callObjectMethod("getSupportedPreviewFpsRange",
                                                  "()Ljava/util/List;");
    auto numFps = frameRates.callMethod<jint>("size");
    QJniEnvironment jenv;

    for (jint i = 0; i < numFps; i++) {
        auto jfpsRange = frameRates.callObjectMethod("get",
                                                     "(I)Ljava/lang/Object;",
                                                     i);
        auto jrange = static_cast<jintArray>(jfpsRange.object());
        auto range = jenv->GetIntArrayElements(jrange, nullptr);
        AkFrac fps(range[0] + range[1], 2000);

        if (!supportedFrameRates.contains(fps))
            supportedFrameRates << fps;

        jenv->ReleaseIntArrayElements(jrange, range, 0);
    }

    camera.callMethod<void>("release");
    CaptureVideoCaps caps;

    for (auto &format: supportedFormats)
        for (auto &size: supportedSizes)
            for (auto &fps: supportedFrameRates) {
                auto akFormat = androidFmtToAkFmt->value(format,
                                                         AkVideoCaps::Format_none);

                if (akFormat != AkVideoCaps::Format_none)
                    caps << AkVideoCaps({akFormat,
                                         size.width(),
                                         size.height(),
                                         fps});
            }

    return caps;
}

jint CaptureAndroidCameraPrivate::deviceId(const QString &device) const
{
    auto idStr = device;
    bool ok = false;
    int id = idStr.remove("/dev/video").toInt(&ok);

    return ok? id: -1;
}

bool CaptureAndroidCameraPrivate::nearestFpsRangue(const QJniObject &parameters,
                                                   const AkFrac &fps,
                                                   jint &min,
                                                   jint &max)
{
    QJniEnvironment jenv;
    auto frameRates = parameters.callObjectMethod("getSupportedPreviewFpsRange",
                                                  "()Ljava/util/List;");
    auto numFps = frameRates.callMethod<jint>("size");
    auto fpsValue = fps.value();
    bool ok = false;
    min = 0;
    max = 0;

    for (jint i = 0; i < numFps; i++) {
        auto jfpsRange = frameRates.callObjectMethod("get",
                                                     "(I)Ljava/lang/Object;",
                                                     i);
        auto jrange = static_cast<jintArray>(jfpsRange.object());
        auto range = jenv->GetIntArrayElements(jrange, nullptr);

        if (1e3 * fpsValue >= qreal(range[0])
            && 1e3 * fpsValue <= qreal(range[1])) {
            min = range[0];
            max = range[1];
            ok = true;
        }

        jenv->ReleaseIntArrayElements(jrange, range, 0);

        if (ok)
            break;
    }

    return ok;
}

QVariantList CaptureAndroidCameraPrivate::controlBoolean(const QJniObject &parameters,
                                                         const QString &name,
                                                         const QString &description,
                                                         bool defaultValue) const
{
    auto supported =
            parameters.callMethod<jboolean>(QString("is%1Supported")
                                            .arg(name)
                                            .toStdString()
                                            .c_str());

    if (!supported)
        return {};

    auto value = parameters.callMethod<jboolean>(QString("get%1")
                                                 .arg(name)
                                                 .toStdString()
                                                 .c_str());

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

bool CaptureAndroidCameraPrivate::setControlBoolean(QJniObject &parameters,
                                                    const QString &name,
                                                    bool value) const
{
    auto supported =
            parameters.callMethod<jboolean>(QString("is%1Supported")
                                            .arg(name)
                                            .toStdString()
                                            .c_str());

    if (!supported)
        return false;

    parameters.callMethod<void>(QString("set%1")
                                .arg(name)
                                .toStdString()
                                .c_str(),
                                "(Z)V",
                                value);

    return true;
}

QVariantList CaptureAndroidCameraPrivate::controlMenu(const QJniObject &parameters,
                                                      const QString &name,
                                                      const QString &description,
                                                      const QString &defaultValue) const
{
    auto optionName = name;
    auto opt = name;

    if (optionName.endsWith("/s")) {
        optionName = optionName.left(optionName.size() - 2);
        opt = optionName + 's';
    }

    auto options =
            parameters.callObjectMethod(QString("getSupported%1")
                                        .arg(opt)
                                        .toStdString()
                                        .c_str(),
                                        "()Ljava/util/List;");

    if (!options.isValid())
        return {};

    auto numOptions = options.callMethod<jint>("size");

    if (numOptions < 2)
        return {};

    QStringList menuOptions;

    for (jint i = 0; i < numOptions; i++) {
        auto joption = options.callObjectMethod("get",
                                                "(I)Ljava/lang/Object;",
                                                i);
        auto option = joption.toString();

        if (!menuOptions.contains(option))
            menuOptions << option;
    }

    auto value = parameters.callObjectMethod(QString("get%1")
                                             .arg(optionName)
                                             .toStdString()
                                             .c_str(),
                                             "()Ljava/lang/String;").toString();

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

bool CaptureAndroidCameraPrivate::setControlMenu(QJniObject &parameters,
                                                 const QString &name,
                                                 int index) const
{
    if (index < 0)
        return false;

    auto optionName = name;
    auto opt = name;

    if (optionName.endsWith("/s")) {
        optionName = optionName.left(optionName.size() - 2);
        opt = optionName + 's';
    }

    auto options =
            parameters.callObjectMethod(QString("getSupported%1")
                                        .arg(opt)
                                        .toStdString()
                                        .c_str(),
                                        "()Ljava/util/List;");

    if (!options.isValid())
        return false;

    auto numOptions = options.callMethod<jint>("size");

    if (numOptions < 2 || index >= numOptions)
        return false;

    auto joption = options.callObjectMethod("get",
                                            "(I)Ljava/lang/Object;",
                                            index);
    parameters.callMethod<void>(QString("set%1")
                                .arg(optionName)
                                .toStdString()
                                .c_str(),
                                "(Ljava/lang/String;)V",
                                joption.object());

    return true;
}

QVariantList CaptureAndroidCameraPrivate::controlInteger(const QJniObject &parameters,
                                                         const QString &name,
                                                         const QString &description,
                                                         int defaultValue) const
{
    auto min = parameters.callMethod<jint>(QString("getMin%1")
                                           .arg(name)
                                           .toStdString()
                                           .c_str());
    auto max = parameters.callMethod<jint>(QString("getMax%1")
                                           .arg(name)
                                           .toStdString()
                                           .c_str());

    if (min == max)
        return {};

    auto value = parameters.callMethod<jint>(QString("get%1")
                                             .arg(name)
                                             .toStdString()
                                             .c_str());
    auto step = parameters.callMethod<jfloat>(QString("get%1Step")
                                              .arg(name)
                                              .toStdString()
                                              .c_str());

    return QVariantList {
        description,
        "integer",
        int(min / step),
        int(max / step),
        1,
        int(qBound(min, defaultValue, max) / step),
        int(qBound(min, value, max) / step),
        QStringList()
    };
}

bool CaptureAndroidCameraPrivate::setControlInteger(QJniObject &parameters,
                                                    const QString &name,
                                                    int value) const
{
    auto min = parameters.callMethod<jint>(QString("getMin%1")
                                           .arg(name)
                                           .toStdString()
                                           .c_str());
    auto max = parameters.callMethod<jint>(QString("getMax%1")
                                           .arg(name)
                                           .toStdString()
                                           .c_str());

    if (min == max)
        return false;

    parameters.callMethod<void>(QString("set%1")
                                .arg(name)
                                .toStdString()
                                .c_str(),
                                "(I)V",
                                value);

    return true;
}

QVariantList CaptureAndroidCameraPrivate::controlZoom(const QJniObject &parameters,
                                                      int defaultValue) const
{
    auto supported = parameters.callMethod<jboolean>("isZoomSupported");

    if (!supported)
        return {};

    auto smooth = parameters.callMethod<jboolean>("isSmoothZoomSupported");
    auto value = parameters.callMethod<jint>("getZoom");

    if (smooth) {
        auto max = parameters.callMethod<jint>("getMaxZoom");

        if (max == 0)
            return {};

        return QVariantList {
            "Zoom",
            "integer",
            0,
            max,
            1,
            qBound(0, defaultValue, max),
            qBound(0, value, max),
            QStringList()
        };
    }

    auto options =
            parameters.callObjectMethod("getZoomRatios",
                                        "()Ljava/util/List;");

    if (!options.isValid())
        return {};

    auto numOptions = options.callMethod<jint>("size");

    if (numOptions < 2)
        return {};

    QStringList menuOptions;

    for (jint i = 0; i < numOptions; i++) {
        auto joption = options.callObjectMethod("get",
                                                "(I)Ljava/lang/Object;",
                                                i);
        auto option = joption.callMethod<jint>("intValue");
        auto optionStr = QString("%1").arg(option);

        if (!menuOptions.contains(optionStr))
            menuOptions << optionStr;
    }

    return QVariantList {
        "Zoom",
        "menu",
        0,
        menuOptions.size() - 1,
        1,
        qMax(menuOptions.indexOf(QString("%1").arg(defaultValue)), 0),
        qMax(menuOptions.indexOf(QString("%1").arg(value)), 0),
        menuOptions
    };
}

bool CaptureAndroidCameraPrivate::setControlZoom(QJniObject &parameters,
                                                 int value) const
{
    auto supported = parameters.callMethod<jboolean>("isZoomSupported");

    if (!supported)
        return false;

    parameters.callMethod<void>("setZoom", "(I)V", value);

    return true;
}

QVariantList CaptureAndroidCameraPrivate::controls(const QJniObject &parameters,
                                                   const ControlVector &controls) const
{
    QVariantList controlsList;

    for (auto &control: controls) {
        QVariantList params;

        switch (control.type) {
        case ControlType::Integer:
            params = this->controlInteger(parameters,
                                          control.name,
                                          control.description,
                                          control.defaultValue.toInt());
            break;

        case ControlType::Boolean:
            params = this->controlBoolean(parameters,
                                          control.name,
                                          control.description,
                                          control.defaultValue.toBool());
            break;

        case ControlType::Menu:
            params = this->controlMenu(parameters,
                                       control.name,
                                       control.description,
                                       control.defaultValue.toString());
            break;

        case ControlType::Zoom:
            params = this->controlZoom(parameters,
                                       control.defaultValue.toInt());
            break;
        }

        if (!params.isEmpty())
            controlsList << QVariant(params);
    }

    return controlsList;
}

bool CaptureAndroidCameraPrivate::setControls(QJniObject &parameters,
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
            ok &= this->setControlInteger(parameters, control.name, value);
            break;

        case ControlType::Boolean:
            ok &= this->setControlBoolean(parameters, control.name, value);
            break;

        case ControlType::Menu:
            ok &= this->setControlMenu(parameters, control.name, value);
            break;

        case ControlType::Zoom:
            ok &= this->setControlZoom(parameters, value);
            break;
        }
    }

    return ok;
}

QVariantList CaptureAndroidCameraPrivate::imageControls(const QJniObject &parameters) const
{
    return this->controls(parameters, *globalImageControls);
}

bool CaptureAndroidCameraPrivate::setImageControls(QJniObject &parameters,
                                                   const QVariantMap &imageControls)
{
    return this->setControls(parameters, *globalImageControls, imageControls);
}

QVariantList CaptureAndroidCameraPrivate::cameraControls(const QJniObject &parameters) const
{
    return this->controls(parameters, *globalCameraControls);
}

bool CaptureAndroidCameraPrivate::setCameraControls(QJniObject &parameters,
                                                    const QVariantMap &cameraControls)
{
    return this->setControls(parameters, *globalCameraControls, cameraControls);
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

qreal CaptureAndroidCameraPrivate::cameraRotation(jint cameraId) const
{
    auto info = QJniObject("android/hardware/Camera$CameraInfo", "()V");
    QJniObject::callStaticMethod<void>("android/hardware/Camera",
                                       "getCameraInfo",
                                       "(ILandroid/hardware/Camera$CameraInfo;)V",
                                       cameraId,
                                       info.object());
    auto windowManager =
        this->m_context.callObjectMethod("getWindowManager",
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

    auto facing = info.getField<jint>("facing");
    auto orientation = info.getField<jint>("orientation");
    int rotation = 0;

    switch (facing) {
    case CAMERA_FACING_FRONT:
        rotation = (orientation + degrees) % 360;
        rotation = (360 - rotation) % 360;

        break;

    case CAMERA_FACING_BACK:
        rotation = (orientation - degrees + 360) % 360;
        break;

    default:
        break;
    }

    return rotation;
}

void CaptureAndroidCameraPrivate::previewFrameReady(JNIEnv *env,
                                                    jobject obj,
                                                    jlong userPtr,
                                                    jbyteArray data)
{
    Q_UNUSED(obj)

    if (!data)
        return;

    auto dataSize = env->GetArrayLength(data);

    if (dataSize < 1)
        return;

    QByteArray buffer(dataSize, Qt::Uninitialized);
    env->GetByteArrayRegion(data,
                            0,
                            dataSize,
                            reinterpret_cast<jbyte *>(buffer.data()));
    auto self = reinterpret_cast<CaptureAndroidCameraPrivate *>(intptr_t(userPtr));

    self->m_mutex.lockForWrite();
    self->m_curBuffer = buffer;
    self->m_waitCondition.wakeAll();
    self->m_mutex.unlock();
}

void CaptureAndroidCameraPrivate::surfaceCreated(JNIEnv *env,
                                                 jobject obj,
                                                 jlong userPtr)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)
    Q_UNUSED(userPtr)
}

void CaptureAndroidCameraPrivate::surfaceDestroyed(JNIEnv *env,
                                                   jobject obj,
                                                   jlong userPtr)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)
    Q_UNUSED(userPtr)
}

void CaptureAndroidCameraPrivate::shutterActivated(JNIEnv *env,
                                                   jobject obj,
                                                   jlong userPtr)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)
    Q_UNUSED(userPtr)
}

void CaptureAndroidCameraPrivate::pictureTaken(JNIEnv *env,
                                               jobject obj,
                                               jlong userPtr,
                                               jint index,
                                               jbyteArray data)
{
    Q_UNUSED(obj)
    auto dataSize = env->GetArrayLength(data);

    if (dataSize < 1)
        return;

    QByteArray buffer(dataSize, Qt::Uninitialized);
    env->GetByteArrayRegion(data,
                            0,
                            dataSize,
                            reinterpret_cast<jbyte *>(buffer.data()));
    auto self = reinterpret_cast<CaptureAndroidCameraPrivate *>(intptr_t(userPtr));

    auto timestamp = QDateTime::currentMSecsSinceEpoch();
    auto pts =
            qint64(timestamp
                   * self->m_timeBase.invert().value()
                   / 1e3);

    self->m_mutex.lockForWrite();

    AkVideoPacket videoPacket(self->m_caps);
    auto iData = buffer.constData();

    for (int plane = 0; plane < videoPacket.planes(); ++plane) {
        auto iLineSize = CaptureAndroidCameraPrivate::alignUp<size_t>(videoPacket.bytesUsed(plane), 16);
        auto oLineSize = videoPacket.lineSize(plane);
        auto lineSize = qMin<size_t>(iLineSize, oLineSize);
        auto heightDiv = videoPacket.heightDiv(plane);

        for (int y = 0; y < videoPacket.caps().height(); ++y) {
            int ys = y >> heightDiv;
            memcpy(videoPacket.line(plane, y),
                   iData + ys * iLineSize,
                   lineSize);
        }

        iData += iLineSize * (videoPacket.caps().height() >> heightDiv);
    }

    videoPacket.setPts(pts);
    videoPacket.setTimeBase(self->m_timeBase);
    videoPacket.setIndex(0);
    videoPacket.setId(self->m_id);

    self->m_mutex.unlock();

    emit self->self->pictureTaken(index, self->m_rotate->iStream(videoPacket));
}

QString CaptureAndroidCameraPrivate::torchModeStr(const char *field) const
{
    return QJniObject::getStaticObjectField("android/hardware/Camera$Parameters",
                                            field,
                                            "java/lang/String").toString();
}

bool CaptureAndroidCameraPrivate::isTorchSupported()
{
    if (!this->m_context.isValid())
        return false;

    auto packageManager =
        this->m_context.callObjectMethod("getPackageManager",
                                         "()Landroid/content/pm/PackageManager;");

    if (packageManager.isValid()) {
        auto feature =
            QJniObject::getStaticObjectField("android/content/pm/PackageManager",
                                             "FEATURE_CAMERA_FLASH",
                                             "java/lang/String");

        if (feature.isValid()) {
            bool isSupported =
                packageManager.callMethod<jboolean>("hasSystemFeature",
                                                    "(Ljava/lang/String;)Z",
                                                    feature.object());

            if (isSupported) {
                bool closeCamera = false;
                this->m_mutex.lockForWrite();

                if (!this->m_camera.isValid()) {
                    auto deviceId = this->deviceId(this->m_device);
                    this->m_camera =
                        QJniObject::callStaticObjectMethod("android/hardware/Camera",
                                                           "open",
                                                           "(I)Landroid/hardware/Camera;",
                                                           deviceId);

                    if (!this->m_camera.isValid()) {
                        this->m_mutex.unlock();

                        return false;
                    }

                    closeCamera = true;
                }

                auto parameters =
                    this->m_camera.callObjectMethod("getParameters",
                                                    "()Landroid/hardware/Camera$Parameters;");

                bool isTorchOnSupported = false;
                bool isTorchOffSupported = false;

                if (parameters.isValid()) {
                    auto flashModes =
                        parameters.callObjectMethod("getSupportedFlashModes",
                                                    "()Ljava/util/List;");

                    if (flashModes.isValid()) {
                        auto numFlashModes = flashModes.callMethod<jint>("size");

                        for (jint i = 0; i < numFlashModes; i++) {
                            auto mode = flashModes.callObjectMethod("get",
                                                                    "(I)Ljava/lang/Object;",
                                                                    i).toString();

                            if (mode == this->torchModeStr("FLASH_MODE_TORCH"))
                                isTorchOnSupported = true;
                            else if (mode == this->torchModeStr("FLASH_MODE_OFF"))
                                isTorchOffSupported = true;
                        }
                    }
                }

                if (closeCamera)
                    this->m_camera.callMethod<void>("release");

                this->m_mutex.unlock();

                return isTorchOnSupported && isTorchOffSupported;
            }
        }
    }

    return false;
}

Capture::TorchMode CaptureAndroidCameraPrivate::torchMode()
{
    if (!this->isTorchSupported())
        return Capture::Torch_Off;

    bool closeCamera = false;
    this->m_mutex.lockForWrite();

    if (!this->m_camera.isValid()) {
        auto deviceId = this->deviceId(this->m_device);
        this->m_camera =
            QJniObject::callStaticObjectMethod("android/hardware/Camera",
                                               "open",
                                               "(I)Landroid/hardware/Camera;",
                                               deviceId);

        if (!this->m_camera.isValid()) {
            this->m_mutex.unlock();

            return Capture::Torch_Off;
        }

        closeCamera = true;
    }

    auto parameters =
        this->m_camera.callObjectMethod("getParameters",
                                        "()Landroid/hardware/Camera$Parameters;");

    if (parameters.isValid()) {
        auto mode =
            parameters.callObjectMethod("flashMode",
                                        "()Ljava/lang/String;").toString();
        this->m_mutex.unlock();

        return mode == this->torchModeStr("FLASH_MODE_TORCH")?
                   Capture::Torch_On:
                   Capture::Torch_Off;
    }

    if (closeCamera) {
        this->m_camera.callMethod<void>("release");
        this->m_camera = {};
    }

    this->m_mutex.unlock();

    return Capture::Torch_Off;
}

void CaptureAndroidCameraPrivate::setTorchMode(Capture::TorchMode mode)
{
    this->m_mutex.lockForWrite();

    if (this->m_camera.isValid()) {
        auto isCapturing = this->m_isCapturing;

        if (isCapturing)
            this->m_camera.callMethod<void>("stopPreview");

        auto parameters =
            this->m_camera.callObjectMethod("getParameters",
                                            "()Landroid/hardware/Camera$Parameters;");
        auto jmode =
            parameters.callObjectMethod("flashMode",
                                        "()Ljava/lang/String;").toString();

        if (jmode == "FLASH_MODE_TORCH" || jmode == "FLASH_MODE_OFF") {
            auto curMode = jmode == "FLASH_MODE_TORCH"?
                            Capture::Torch_On:
                            Capture::Torch_Off;

            if (curMode != mode) {
                auto jNewMode = mode == Capture::Torch_On?
                                    QJniObject::fromString("FLASH_MODE_TORCH"):
                                    QJniObject::fromString("FLASH_MODE_OFF");
                parameters.callMethod<void>("setFlashMode",
                                            "(Ljava/lang/String;)V",
                                            jNewMode.object());
                this->m_camera.callMethod<void>("setParameters",
                                                "(Landroid/hardware/Camera$Parameters;)V",
                                                parameters.object());
            }
        } else {
            auto jNewMode = mode == Capture::Torch_On?
                                QJniObject::fromString("FLASH_MODE_TORCH"):
                                QJniObject::fromString("FLASH_MODE_OFF");
            parameters.callMethod<void>("setFlashMode",
                                        "(Ljava/lang/String;)V",
                                        jNewMode.object());
            this->m_camera.callMethod<void>("setParameters",
                                            "(Landroid/hardware/Camera$Parameters;)V",
                                            parameters.object());
        }

        if (isCapturing)
            this->m_camera.callMethod<void>("startPreview");
    }

    this->m_mutex.unlock();
}

void CaptureAndroidCameraPrivate::updateDevices()
{
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;

    auto numCameras =
            QJniObject::callStaticMethod<jint>("android/hardware/Camera",
                                                      "getNumberOfCameras");

    static const QMap<jint, QString> facingToStr {
        {CAMERA_FACING_FRONT   , "Front"},
        {CAMERA_FACING_BACK    , "Back"},
        {CAMERA_FACING_EXTERNAL, "External"},
    };

    for (jint i = 0; i < numCameras; i++) {
        auto caps = this->caps(i);

        if (!caps.empty()) {
            auto deviceId = QString("/dev/video%1").arg(i);
            QJniObject cameraInfo("android/hardware/Camera$CameraInfo");
            QJniObject::callStaticMethod<void>("android/hardware/Camera",
                                               "getCameraInfo",
                                               "(ILandroid/hardware/Camera$CameraInfo;)V",
                                               i,
                                               cameraInfo.object());
            auto facing = cameraInfo.getField<jint>("facing");
            auto description = QString("%1 Camera %2")
                               .arg(facingToStr.value(facing, "External"))
                               .arg(i);

            devices << deviceId;
            descriptions[deviceId] = description;
            devicesCaps[deviceId] = caps;
        }
    }

    if (devicesCaps.isEmpty()) {
        devices.clear();
        descriptions.clear();
    }

    this->m_descriptions = descriptions;
    this->m_devicesCaps = devicesCaps;

    if (this->m_devices != devices) {
        this->m_devices = devices;
        emit self->webcamsChanged(this->m_devices);
    }
}

bool CaptureAndroidCamera::init()
{
    this->d->m_localImageControls.clear();
    this->d->m_localCameraControls.clear();
    this->uninit();

    QJniObject surfaceHolder;
    QList<int> streams;
    CaptureVideoCaps supportedCaps;
    AkVideoCaps caps;
    QJniObject parameters;
    jint min = 0;
    jint max = 0;
    AkFrac fps;

    this->d->m_curDeviceId = this->d->deviceId(this->d->m_device);
    this->d->m_camera =
            QJniObject::callStaticObjectMethod("android/hardware/Camera",
                                               "open",
                                               "(I)Landroid/hardware/Camera;",
                                               this->d->m_curDeviceId);

    if (!this->d->m_camera.isValid()) {
        this->uninit();

        return false;
    }

    auto result =
        qApp->nativeInterface<QNativeInterface::QAndroidApplication>()->runOnAndroidMainThread([this] {
        this->d->m_surfaceView =
                QJniObject("android/view/SurfaceView",
                           "(Landroid/content/Context;)V",
                           this->d->m_context.object());
        auto window = QWindow::fromWinId(WId(this->d->m_surfaceView.object()));
        window->setVisible(true);
        window->setGeometry(0, 0, 0, 0);
    });

    result.waitForFinished();

    if (!this->d->m_surfaceView.isValid()) {
        this->uninit();

        return false;
    }

    surfaceHolder =
            this->d->m_surfaceView.callObjectMethod("getHolder",
                                                    "()Landroid/view/SurfaceHolder;");

    if (!surfaceHolder.isValid()) {
        this->uninit();

        return false;
    }

    QThread::sleep(1);

    this->d->m_camera.callMethod<void>("setPreviewDisplay",
                                       "(Landroid/view/SurfaceHolder;)V",
                                       surfaceHolder.object());
    surfaceHolder.callMethod<void>("addCallback",
                                   "(Landroid/view/SurfaceHolder$Callback;)V",
                                   this->d->m_callbacks.object());
    this->d->m_camera.callMethod<void>("setPreviewCallback",
                                       "(Landroid/hardware/Camera$PreviewCallback;)V",
                                       this->d->m_callbacks.object());
    parameters =
            this->d->m_camera.callObjectMethod("getParameters",
                                               "()Landroid/hardware/Camera$Parameters;");

    if (!parameters.isValid()) {
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

    parameters.callMethod<void>("setPreviewFormat",
                                "(I)V",
                                androidFmtToAkFmt->key(caps.format(),
                                                       ImageFormat::UNKNOWN));
    parameters.callMethod<void>("setPreviewSize",
                                "(II)V",
                                caps.width(),
                                caps.height());

    min = 0;
    max = 0;
    fps = caps.fps();

    if (!this->d->nearestFpsRangue(parameters,
                                   fps,
                                   min,
                                   max)) {
        this->uninit();

        return false;
    }

    parameters.callMethod<void>("setPreviewFpsRange",
                                "(II)V",
                                min,
                                max);
    this->d->m_camera.callMethod<void>("setParameters",
                                       "(Landroid/hardware/Camera$Parameters;)V",
                                       parameters.object());

    if (this->d->m_torchMode == Torch_Off
        && this->d->torchMode() == Capture::Torch_On)
        this->d->setTorchMode(Capture::Torch_Off);
    else if (this->d->m_torchMode == Torch_On
             && this->d->torchMode() == Capture::Torch_Off)
        this->d->setTorchMode(Capture::Torch_On);

    this->d->m_camera.callMethod<void>("startPreview");

    this->d->m_id = Ak::id();
    this->d->m_caps = caps;
    this->d->m_fps = fps;
    this->d->m_timeBase = this->d->m_fps.invert();
    this->d->m_isCapturing = true;

    auto angle = -this->d->cameraRotation(this->d->m_curDeviceId);
    this->d->m_rotate->setProperty("angle", angle);

    return true;
}

void CaptureAndroidCamera::uninit()
{
    if (!this->d->m_camera.isValid())
        return;

    this->d->m_camera.callMethod<void>("stopPreview");

    if (this->d->m_surfaceView.isValid()) {
        auto result =
            qApp->nativeInterface<QNativeInterface::QAndroidApplication>()->runOnAndroidMainThread([this] {
                this->d->m_surfaceView = {};
            });

        result.waitForFinished();
    }

    this->d->m_camera.callMethod<void>("release");
    this->d->m_camera = {};
    this->d->m_curDeviceId = -1;
    this->d->m_isCapturing = false;
}

void CaptureAndroidCamera::setDevice(const QString &device)
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

        auto camera =
                QJniObject::callStaticObjectMethod("android/hardware/Camera",
                                                   "open",
                                                   "(I)Landroid/hardware/Camera;",
                                                   this->d->deviceId(device));

        if (camera.isValid()) {
            auto parameters =
                    camera.callObjectMethod("getParameters",
                                            "()Landroid/hardware/Camera$Parameters;");

            if (parameters.isValid()) {
                this->d->m_globalImageControls = this->d->imageControls(parameters);
                this->d->m_globalCameraControls = this->d->cameraControls(parameters);
            }

            camera.callMethod<void>("release");
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

    bool isTorchSupported = this->d->isTorchSupported();
    auto torchMode = this->d->torchMode();

    if (this->d->m_isTorchSupported != isTorchSupported) {
        this->d->m_isTorchSupported = isTorchSupported;
        emit this->isTorchSupportedChanged(isTorchSupported);
    }

    if (this->d->m_torchMode != torchMode) {
        this->d->m_torchMode = torchMode;
        emit this->torchModeChanged(torchMode);
    }
}

void CaptureAndroidCamera::setStreams(const QList<int> &streams)
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
    emit this->torchModeChanged(mode);

    if (this->d->m_torchMode == Torch_Off
        && this->d->torchMode() == Capture::Torch_On)
        this->d->setTorchMode(Capture::Torch_Off);
    else if (this->d->m_torchMode == Torch_On
             && this->d->torchMode() == Capture::Torch_Off)
        this->d->setTorchMode(Capture::Torch_On);
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

void CaptureAndroidCamera::takePictures(int count, int delayMsecs)
{
    auto result = QtConcurrent::run(&this->d->m_threadPool,
                      [this, count, delayMsecs] () {
        this->d->m_callbacks.callMethod<void>("resetPictureIndex", "(V)V");

        for (int i = 0; i < count; i++) {
            if (this->d->m_camera.isValid())
                this->d->m_camera.callMethod<void>("takePicture",
                                                   "(Landroid/hardware/Camera$ShutterCallback;"
                                                   "Landroid/hardware/Camera$PictureCallback;"
                                                   "Landroid/hardware/Camera$PictureCallback;)V",
                                                   this->d->m_callbacks.object(),
                                                   this->d->m_callbacks.object(),
                                                   nullptr);
            QThread::msleep(delayMsecs);
        }
    });
    Q_UNUSED(result)
}

#include "moc_captureandroidcamera.cpp"

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

#include <QDateTime>
#include <QMutex>
#include <QSet>
#include <QThread>
#include <QVariant>
#include <QWaitCondition>
#include <QWindow>
#include <QtAndroid>
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideopacket.h>

#include "captureandroidcamera.h"

#define JNAMESPACE "org/webcamoid/plugins/VideoCapture/submodules/androidcamera"
#define JCLASS(jclass) JNAMESPACE "/" #jclass

#define CAMERA_FACING_BACK  0
#define CAMERA_FACING_FRONT 1

enum ImageFormat
{
    TRANSLUCENT       = -3,
    TRANSPARENT       = -2,
    OPAQUE            = -1,
    UNKNOWN           = AK_FOURCC_NULL,
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
    Y8                = AkFourCCR('Y', '8', ' ', ' '),
    YV12              = AkFourCCR('Y', 'V', '1', '2'),
    DEPTH16           = AkFourCCR('Y', '1', '6', 'D'),
    DEPTH_JPEG        = AkFourCCR('c', 'i', 'e', 'i'),
};

typedef QMap<ImageFormat, QString> ImageFormatToStrMap;

inline const ImageFormatToStrMap initImageFormatToStrMap()
{
    const ImageFormatToStrMap imgFmtToStrMap = {
        {ImageFormat::TRANSLUCENT      , "TRANSLUCENT"      },
        {ImageFormat::TRANSPARENT      , "TRANSPARENT"      },
        {ImageFormat::OPAQUE           , "OPAQUE"           },
        {ImageFormat::UNKNOWN          , "UNKNOWN"          },
        {ImageFormat::RGBA_8888        , "RGBA"             },
        {ImageFormat::RGBX_8888        , "RGBX"             },
        {ImageFormat::RGB_888          , "RGB"              },
        {ImageFormat::RGB_565          , "RGB565"           },
        {ImageFormat::RGBA_5551        , "RGB555"           },
        {ImageFormat::RGBA_4444        , "RGBA4444"         },
        {ImageFormat::A_8              , "GRAY8"            },
        {ImageFormat::L_8              , "GRAY8"            },
        {ImageFormat::LA_88            , "GRAYA8"           },
        {ImageFormat::RGB_332          , "RGB332"           },
        {ImageFormat::NV16             , "NV16"             },
        {ImageFormat::NV21             , "NV21"             },
        {ImageFormat::YUY2             , "YUY2"             },
        {ImageFormat::RGBA_F16         , "RGBAF16"          },
        {ImageFormat::RAW_SENSOR       , "RAW_SENSOR"       },
        {ImageFormat::YUV_420_888      , "YUY420P16_888"    },
        {ImageFormat::PRIVATE          , "PRIVATE"          },
        {ImageFormat::RAW_PRIVATE      , "RAW_PRIVATE"      },
        {ImageFormat::RAW10            , "SGRBG10"          },
        {ImageFormat::RAW12            , "SGRBG12"          },
        {ImageFormat::YUV_422_888      , "YUV422P"          },
        {ImageFormat::FLEX_RGB_888     , "FLEX_RGB_888"     },
        {ImageFormat::FLEX_RGBA_8888   , "FLEX_RGBA_8888"   },
        {ImageFormat::RGBA_1010102     , "RGBA_1010102"     },
        {ImageFormat::JPEG             , "JPEG"             },
        {ImageFormat::DEPTH_POINT_CLOUD, "DEPTH_POINT_CLOUD"},
        {ImageFormat::Y8               , "GRAY8"            },
        {ImageFormat::YV12             , "YV12"             },
        {ImageFormat::DEPTH16          , "DEPTH16"          },
        {ImageFormat::DEPTH_JPEG       , "DEPTH_JPEG"       },
    };

    return imgFmtToStrMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(ImageFormatToStrMap, imgFmtToStrMap, (initImageFormatToStrMap()))

enum ControlType
{
    Integer,
    Boolean,
    Menu,
    Zoom
};

struct Control
{
    ControlType type;
    QString name;
    QString description;
    QVariant defaultValue;
};

using ControlVector = QVector<Control>;

inline const ControlVector &initImageControls()
{
    static const ControlVector controls {
        {ControlType::Menu   ,          "SceneMode/s",              "Scene Mode", "auto"},
        {ControlType::Boolean, "AutoWhiteBalanceLock", "Auto White Balance Lock",   true},
        {ControlType::Menu   ,         "WhiteBalance",           "White Balance", "auto"},
        {ControlType::Boolean,     "AutoExposureLock",      "Auto Exposure Lock",   true},
        {ControlType::Integer, "ExposureCompensation",   "Exposure Compensation",      0},
        {ControlType::Menu   ,          "Antibanding",             "Antibanding", "auto"},
        {ControlType::Boolean,   "VideoStabilization",     "Video Stabilization",   true},
        {ControlType::Menu   ,        "ColorEffect/s",            "Color Effect", "none"},
    };

    return controls;
}

Q_GLOBAL_STATIC_WITH_ARGS(ControlVector, globalImageControls, (initImageControls()))

inline const ControlVector &initCameraControls()
{
    static const ControlVector controls {
        {ControlType::Menu, "FlashMode/s", "Flash Mode", "auto"},
        {ControlType::Menu, "FocusMode/s", "Focus Mode", "auto"},
        {ControlType::Zoom,            {},           {},      0},
    };

    return controls;
}

Q_GLOBAL_STATIC_WITH_ARGS(ControlVector, globalCameraControls, (initCameraControls()))

class CaptureAndroidCameraPrivate
{
    public:
        CaptureAndroidCamera *self;
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, QVariantList> m_devicesCaps;
        QMutex m_controlsMutex;
        QVariantList m_globalImageControls;
        QVariantList m_globalCameraControls;
        QVariantMap m_localImageControls;
        QVariantMap m_localCameraControls;
        QMutex m_mutex;
        QByteArray m_curBuffer;
        QWaitCondition m_waitCondition;
        AkFrac m_fps;
        AkFrac m_timeBase;
        AkCaps m_caps;
        qint64 m_id {-1};
        QAndroidJniObject m_camera;
        QAndroidJniObject m_callbacks;
        QAndroidJniObject m_surfaceView;

        explicit CaptureAndroidCameraPrivate(CaptureAndroidCamera *self);
        void registerNatives();
        QVariantList caps(jint device);
        jint deviceId(const QString &device) const;
        bool nearestFpsRangue(const QAndroidJniObject &parameters,
                              const AkFrac &fps,
                              jint &min,
                              jint &max);
        QVariantList controlBoolean(const QAndroidJniObject &parameters,
                                    const QString &name,
                                    const QString &description,
                                    bool defaultValue=false) const;
        bool setControlBoolean(QAndroidJniObject &parameters,
                               const QString &name,
                               bool value) const;
        QVariantList controlMenu(const QAndroidJniObject &parameters,
                                 const QString &name,
                                 const QString &description,
                                 const QString &defaultValue={}) const;
        bool setControlMenu(QAndroidJniObject &parameters,
                            const QString &name,
                            int index) const;
        QVariantList controlInteger(const QAndroidJniObject &parameters,
                                    const QString &name,
                                    const QString &description,
                                    int defaultValue=0) const;
        bool setControlInteger(QAndroidJniObject &parameters,
                               const QString &name,
                               int value) const;
        QVariantList controlZoom(const QAndroidJniObject &parameters,
                                 int defaultValue=0) const;
        bool setControlZoom(QAndroidJniObject &parameters,
                            int value) const;
        QVariantList controls(const QAndroidJniObject &parameters,
                              const ControlVector &controls) const;
        bool setControls(QAndroidJniObject &parameters,
                         const ControlVector &controls,
                         const QVariantMap &values);
        QVariantList imageControls(const QAndroidJniObject &parameters) const;
        bool setImageControls(QAndroidJniObject &parameters,
                              const QVariantMap &imageControls);
        QVariantList cameraControls(const QAndroidJniObject &parameters) const;
        bool setCameraControls(QAndroidJniObject &parameters,
                               const QVariantMap &cameraControls);
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        static void previewFrameReady(JNIEnv *env,
                                      jobject obj,
                                      jlong userPtr,
                                      jbyteArray data);
        static void surfaceCreated(JNIEnv *env, jobject obj, jlong userPtr);
        static void surfaceDestroyed(JNIEnv *env, jobject obj, jlong userPtr);
        static bool canUseCamera();
        void updateDevices();
};

CaptureAndroidCamera::CaptureAndroidCamera(QObject *parent):
    Capture(parent)
{
    this->d = new CaptureAndroidCameraPrivate(this);
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

QList<int> CaptureAndroidCamera::listTracks(const QString &mimeType)
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

QVariantList CaptureAndroidCamera::caps(const QString &webcam) const
{
    return this->d->m_devicesCaps.value(webcam);
}

QString CaptureAndroidCamera::capsDescription(const AkCaps &caps) const
{
    if (caps.mimeType() != "video/unknown")
        return {};

    AkFrac fps = caps.property("fps").toString();

    return QString("%1, %2x%3, %4 FPS")
                .arg(caps.property("fourcc").toString(),
                     caps.property("width").toString(),
                     caps.property("height").toString())
                .arg(qRound(fps.value()));
}

QVariantList CaptureAndroidCamera::imageControls() const
{
    return this->d->m_globalImageControls;
}

bool CaptureAndroidCamera::setImageControls(const QVariantMap &imageControls)
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
            this->d->m_controlsMutex.lock();
            auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
            this->d->m_controlsMutex.unlock();
            bool apply = false;

            if (this->d->m_localImageControls != imageControls) {
                auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                                 imageControls);
                apply |= this->d->setImageControls(parameters, controls);
                this->d->m_localImageControls = imageControls;
            }

            this->d->m_controlsMutex.lock();
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

    this->d->m_mutex.lock();

    if (this->d->m_curBuffer.isEmpty())
        this->d->m_waitCondition.wait(&this->d->m_mutex, 1000);

    if (!this->d->m_curBuffer.isEmpty()) {
        int bufferSize = this->d->m_curBuffer.size();
        QByteArray oBuffer(bufferSize, 0);
        memcpy(oBuffer.data(),
               this->d->m_curBuffer.constData(),
               size_t(bufferSize));

        packet = AkPacket(this->d->m_caps);
        packet.setBuffer(oBuffer);
        packet.setPts(pts);
        packet.setTimeBase(this->d->m_timeBase);
        packet.setIndex(0);
        packet.setId(this->d->m_id);
        this->d->m_curBuffer.clear();
    }

    this->d->m_mutex.unlock();

    return packet;
}

CaptureAndroidCameraPrivate::CaptureAndroidCameraPrivate(CaptureAndroidCamera *self):
    self(self)
{
    this->registerNatives();
    this->m_callbacks =
            QAndroidJniObject(JCLASS(AkAndroidCameraCallbacks),
                              "(J)V",
                              this);
}

void CaptureAndroidCameraPrivate::registerNatives()
{
    static bool ready = false;

    if (ready)
        return;

    QAndroidJniEnvironment jenv;

    if (auto jclass = jenv.findClass(JCLASS(AkAndroidCameraCallbacks))) {
        QVector<JNINativeMethod> methods {
            {"previewFrameReady"     , "(J[B)V", reinterpret_cast<void *>(CaptureAndroidCameraPrivate::previewFrameReady)},
            {"notifySurfaceCreated"  , "(J)V"  , reinterpret_cast<void *>(CaptureAndroidCameraPrivate::surfaceCreated)   },
            {"notifySurfaceDestroyed", "(J)V"  , reinterpret_cast<void *>(CaptureAndroidCameraPrivate::surfaceDestroyed) },
        };

        jenv->RegisterNatives(jclass, methods.data(), methods.size());
    }

    ready = true;
}

QVariantList CaptureAndroidCameraPrivate::caps(jint device)
{
    auto camera =
            QAndroidJniObject::callStaticObjectMethod("android/hardware/Camera",
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

        if (!imgFmtToStrMap->contains(ImageFormat(format)))
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
    QAndroidJniEnvironment jenv;

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
    QVariantList caps;

    for (auto &format: supportedFormats)
        for (auto &size: supportedSizes)
            for (auto &fps: supportedFrameRates) {
                AkCaps videoCaps;
                videoCaps.setMimeType("video/unknown");
                videoCaps.setProperty("fourcc", imgFmtToStrMap->value(ImageFormat(format)));
                videoCaps.setProperty("width", size.width());
                videoCaps.setProperty("height", size.height());
                videoCaps.setProperty("fps", fps.toString());
                caps << QVariant::fromValue(videoCaps);
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

bool CaptureAndroidCameraPrivate::nearestFpsRangue(const QAndroidJniObject &parameters,
                                                   const AkFrac &fps,
                                                   jint &min,
                                                   jint &max)
{
    QAndroidJniEnvironment jenv;
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

QVariantList CaptureAndroidCameraPrivate::controlBoolean(const QAndroidJniObject &parameters,
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

bool CaptureAndroidCameraPrivate::setControlBoolean(QAndroidJniObject &parameters,
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

QVariantList CaptureAndroidCameraPrivate::controlMenu(const QAndroidJniObject &parameters,
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

bool CaptureAndroidCameraPrivate::setControlMenu(QAndroidJniObject &parameters,
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

QVariantList CaptureAndroidCameraPrivate::controlInteger(const QAndroidJniObject &parameters,
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

bool CaptureAndroidCameraPrivate::setControlInteger(QAndroidJniObject &parameters,
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

QVariantList CaptureAndroidCameraPrivate::controlZoom(const QAndroidJniObject &parameters,
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

bool CaptureAndroidCameraPrivate::setControlZoom(QAndroidJniObject &parameters,
                                                 int value) const
{
    auto supported = parameters.callMethod<jboolean>("isZoomSupported");

    if (!supported)
        return false;

    parameters.callMethod<void>("setZoom", "(I)V", value);

    return true;
}

QVariantList CaptureAndroidCameraPrivate::controls(const QAndroidJniObject &parameters,
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

bool CaptureAndroidCameraPrivate::setControls(QAndroidJniObject &parameters,
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

QVariantList CaptureAndroidCameraPrivate::imageControls(const QAndroidJniObject &parameters) const
{
    return this->controls(parameters, *globalImageControls);
}

bool CaptureAndroidCameraPrivate::setImageControls(QAndroidJniObject &parameters,
                                                   const QVariantMap &imageControls)
{
    return this->setControls(parameters, *globalImageControls, imageControls);
}

QVariantList CaptureAndroidCameraPrivate::cameraControls(const QAndroidJniObject &parameters) const
{
    return this->controls(parameters, *globalCameraControls);
}

bool CaptureAndroidCameraPrivate::setCameraControls(QAndroidJniObject &parameters,
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

void CaptureAndroidCameraPrivate::previewFrameReady(JNIEnv *env,
                                                    jobject obj,
                                                    jlong userPtr,
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
    auto self = reinterpret_cast<CaptureAndroidCameraPrivate *>(userPtr);

    self->m_mutex.lock();
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

bool CaptureAndroidCameraPrivate::canUseCamera()
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

void CaptureAndroidCameraPrivate::updateDevices()
{
    if (!this->canUseCamera())
        return;

    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;

    auto numCameras =
            QAndroidJniObject::callStaticMethod<jint>("android/hardware/Camera",
                                                      "getNumberOfCameras");

    for (jint i = 0; i < numCameras; i++) {
        auto caps = this->caps(i);

        if (!caps.empty()) {
            auto deviceId = QString("/dev/video%1").arg(i);
            QAndroidJniObject cameraInfo("android/hardware/Camera$CameraInfo");
            QAndroidJniObject::callStaticMethod<void>("android/hardware/Camera",
                                                      "getCameraInfo",
                                                      "(ILandroid/hardware/Camera$CameraInfo;)V",
                                                      i,
                                                      cameraInfo.object());
            auto facing = cameraInfo.getField<jint>("facing");
            auto description = QString("%1 Camera %2")
                               .arg(facing == CAMERA_FACING_BACK? "Back": "Front")
                               .arg(i);

            devices << deviceId;
            descriptions[deviceId] = description;
            devicesCaps[deviceId] = caps;
        }
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

    QAndroidJniObject surfaceHolder;
    QList<int> streams;
    QVariantList supportedCaps;
    AkCaps caps;
    QAndroidJniObject parameters;
    jint min = 0;
    jint max = 0;
    AkFrac fps;

    this->d->m_camera =
            QAndroidJniObject::callStaticObjectMethod("android/hardware/Camera",
                                                      "open",
                                                      "(I)Landroid/hardware/Camera;",
                                                      this->d->deviceId(this->d->m_device));

    if (!this->d->m_camera.isValid())
        goto init_failed;

    QtAndroid::runOnAndroidThreadSync([this] {
        this->d->m_surfaceView =
                QAndroidJniObject("android/view/SurfaceView",
                                  "(Landroid/content/Context;)V",
                                  QtAndroid::androidActivity().object());
        auto window = QWindow::fromWinId(WId(this->d->m_surfaceView.object()));
        window->setVisible(true);
        window->setGeometry(0, 0, 0, 0);
    });

    if (!this->d->m_surfaceView.isValid())
        goto init_failed;

    surfaceHolder =
            this->d->m_surfaceView.callObjectMethod("getHolder",
                                                    "()Landroid/view/SurfaceHolder;");

    if (!surfaceHolder.isValid())
        goto init_failed;

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

    if (!parameters.isValid())
        goto init_failed;

    streams = this->streams();

    if (streams.isEmpty())
        goto init_failed;

    supportedCaps = this->caps(this->d->m_device);
    caps = supportedCaps[streams[0]].value<AkCaps>();
    caps.setProperty("align", 16);

    parameters.callMethod<void>("setPreviewFormat",
                                "(I)V",
                                imgFmtToStrMap->key(caps.property("fourcc").toString(),
                                                    ImageFormat::UNKNOWN));
    parameters.callMethod<void>("setPreviewSize",
                                "(II)V",
                                caps.property("width").toInt(),
                                caps.property("height").toInt());

    min = 0;
    max = 0;
    fps = caps.property("fps").toString();

    if (!this->d->nearestFpsRangue(parameters,
                                   fps,
                                   min,
                                   max)) {
        goto init_failed;
    }

    parameters.callMethod<void>("setPreviewFpsRange",
                                "(II)V",
                                min,
                                max);
    this->d->m_camera.callMethod<void>("setParameters",
                                       "(Landroid/hardware/Camera$Parameters;)V",
                                       parameters.object());
    this->d->m_camera.callMethod<void>("startPreview");

    this->d->m_id = Ak::id();
    this->d->m_caps = caps;
    this->d->m_fps = fps;
    this->d->m_timeBase = this->d->m_fps.invert();

    return true;

init_failed:
    this->uninit();

    return false;
}

void CaptureAndroidCamera::uninit()
{
    if (!this->d->m_camera.isValid())
        return;

    this->d->m_camera.callMethod<void>("stopPreview");

    if (this->d->m_surfaceView.isValid())
        QtAndroid::runOnAndroidThreadSync([this] {
            this->d->m_surfaceView = {};
        });

    this->d->m_camera.callMethod<void>("release");
    this->d->m_camera = {};
}

void CaptureAndroidCamera::setDevice(const QString &device)
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

        auto camera =
                QAndroidJniObject::callStaticObjectMethod("android/hardware/Camera",
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

    this->d->m_controlsMutex.lock();
    auto imageStatus = this->d->controlStatus(this->d->m_globalImageControls);
    auto cameraStatus = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    emit this->deviceChanged(device);
    emit this->imageControlsChanged(imageStatus);
    emit this->cameraControlsChanged(cameraStatus);
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

void CaptureAndroidCamera::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

#include "moc_captureandroidcamera.cpp"

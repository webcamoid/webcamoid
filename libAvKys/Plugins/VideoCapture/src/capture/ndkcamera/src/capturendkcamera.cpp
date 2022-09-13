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
#include <QReadWriteLock>
#include <QThread>
#include <QVariant>
#include <QVector>
#include <QWaitCondition>
#include <QtAndroid>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideopacket.h>
#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>

#include "capturendkcamera.h"

using ImageFormatToStrMap = QMap<AIMAGE_FORMATS, QString>;

inline const ImageFormatToStrMap initImageFormatToStrMap()
{
    const ImageFormatToStrMap imgFmtToStrMap = {
        {AIMAGE_FORMAT_RGBA_8888        , "RGBA"             },
        {AIMAGE_FORMAT_RGBX_8888        , "RGBX"             },
        {AIMAGE_FORMAT_RGB_888          , "RGB"              },
        {AIMAGE_FORMAT_RGB_565          , "RGB565"           },
        {AIMAGE_FORMAT_RGBA_FP16        , "RGBA_FP16"        },
        {AIMAGE_FORMAT_YUV_420_888      , "YU12"             },
        {AIMAGE_FORMAT_JPEG             , "JPEG"             },
        {AIMAGE_FORMAT_RAW16            , "SGRBG16"          },
        {AIMAGE_FORMAT_RAW_PRIVATE      , "RAW_PRIVATE"      },
        {AIMAGE_FORMAT_RAW10            , "SGRBG10"          },
        {AIMAGE_FORMAT_RAW12            , "SGRBG12"          },
        {AIMAGE_FORMAT_DEPTH16          , "DEPTH16"          },
        {AIMAGE_FORMAT_DEPTH_POINT_CLOUD, "DEPTH_POINT_CLOUD"},
        {AIMAGE_FORMAT_PRIVATE          , "PRIVATE"          },
    };

    return imgFmtToStrMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(ImageFormatToStrMap, imgFmtToStrMap, (initImageFormatToStrMap()))

using CameraManagerPtr = QSharedPointer<ACameraManager>;

class CaptureNdkCameraPrivate
{
    public:
        CaptureNdkCamera *self;
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, QVariantList> m_devicesCaps;
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
        static void sessionClosed(void *context,
                                  ACameraCaptureSession *session);
        static void sessionReady(void *context,
                                 ACameraCaptureSession *session);
        static void sessionActive(void *context,
                                  ACameraCaptureSession *session);
        static bool canUseCamera();
        static QByteArray readBuffer(AImage *image);
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

    QVariantList caps = this->caps(this->d->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int> {0};
}

QList<int> CaptureNdkCamera::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return QList<int>();

    QVariantList caps = this->caps(this->d->m_device);
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

QString CaptureNdkCamera::capsDescription(const AkCaps &caps) const
{
    if (caps.mimeType() != "video/unknown")
        return QString();

    AkFrac fps = caps.property("fps").toString();

    return QString("%1, %2x%3, %4 FPS")
                .arg(caps.property("fourcc").toString(),
                     caps.property("width").toString(),
                     caps.property("height").toString())
                .arg(qRound(fps.value()));
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
        QVariantList control = globalImageControls[i].toList();
        QString controlName = control[0].toString();

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
        QVariantList params = control.toList();

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
        QVariantList control = globalCameraControls[i].toList();
        QString controlName = control[0].toString();

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

AkPacket CaptureNdkCamera::readFrame()
{
    AkPacket packet;

    this->d->m_mutex.lockForWrite();

    if (!this->d->m_curPacket)
        this->d->m_waitCondition.wait(&this->d->m_mutex, 1000);

    if (this->d->m_curPacket) {
        packet = this->d->m_curPacket;
        this->d->m_curPacket = {};
    }

    this->d->m_mutex.unlock();

    return packet;
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
    int32_t format = 0;
    int32_t width = 0;
    int32_t height = 0;
    int64_t timestampNs = 0;
    QByteArray oBuffer;
    AkCaps caps;
    AkPacket packet;

    if (AImageReader_acquireLatestImage(reader, &image) != AMEDIA_OK)
        return;

    if (AImage_getFormat(image, &format) != AMEDIA_OK)
        goto imageAvailable_error;

    if (AImage_getWidth(image, &width) != AMEDIA_OK)
        goto imageAvailable_error;

    if (AImage_getHeight(image, &height) != AMEDIA_OK)
        goto imageAvailable_error;

    if (AImage_getTimestamp(image, &timestampNs) != AMEDIA_OK)
        goto imageAvailable_error;

    oBuffer = CaptureNdkCameraPrivate::readBuffer(image);

    if (oBuffer.isEmpty())
        goto imageAvailable_error;

    caps.setMimeType("video/unknown");
    caps.setProperty("fourcc", imgFmtToStrMap->value(AIMAGE_FORMATS(format)));
    caps.setProperty("width", width);
    caps.setProperty("height", height);
    caps.setProperty("fps", self->m_fps.toString());

    packet = AkPacket(caps);
    packet.setBuffer(oBuffer);
    packet.setPts(timestampNs);
    packet.setTimeBase({1, qint64(1e9)});
    packet.setIndex(0);
    packet.setId(self->m_id);

    self->m_mutex.lockForWrite();
    self->m_curPacket = packet;
    self->m_waitCondition.wakeAll();
    self->m_mutex.unlock();

imageAvailable_error:
    AImage_delete(image);
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

QByteArray CaptureNdkCameraPrivate::readBuffer(AImage *image)
{
    int32_t format = 0;
    AImage_getFormat(image, &format);
    int32_t width = 0;
    AImage_getWidth(image, &width);
    int32_t height = 0;
    AImage_getHeight(image, &height);
    int32_t numPlanes = 0;
    AImage_getNumberOfPlanes(image, &numPlanes);
    QByteArray oBuffer;

    if (format == AIMAGE_FORMAT_YUV_420_888) {
        for (int32_t i = 0; i < numPlanes; i++) {
            uint8_t *data = nullptr;
            int dataLength = 0;

            if (AImage_getPlaneData(image, i, &data, &dataLength) != AMEDIA_OK)
                continue;

            int32_t pixelStride = 0;
            int32_t rowStride = 0;
            AImage_getPlanePixelStride(image, i, &pixelStride);
            AImage_getPlaneRowStride(image, i, &rowStride);
            auto _width = width / (i > 0? 2: 1);
            auto _height = height / (i > 0? 2: 1);
            QByteArray buffer(_width * _height, Qt::Uninitialized);

            for (int y = 0; y < _height; y++) {
                auto srcLine = data + y * rowStride;
                auto dstLine = reinterpret_cast<quint8 *>(buffer.data())
                               + y * _width;

                for (int x = 0; x < _width; x++)
                    dstLine[x] = srcLine[pixelStride * x];
            }

            oBuffer += buffer;
        }
    } else {
        for (int32_t i = 0; i < numPlanes; i++) {
            uint8_t *data = nullptr;
            int dataLength = 0;

            if (AImage_getPlaneData(image, i, &data, &dataLength) != AMEDIA_OK)
                continue;

            oBuffer += QByteArray(reinterpret_cast<char *>(data), dataLength);
        }
    }

    return oBuffer;
}

void CaptureNdkCameraPrivate::updateDevices()
{
    if (!this->canUseCamera())
        return;

    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;

    ACameraIdList *cameras = nullptr;

    if (ACameraManager_getCameraIdList(this->m_manager.data(),
                                       &cameras) == ACAMERA_OK) {
        QVector<AIMAGE_FORMATS> unsupportedFormats {
            AIMAGE_FORMAT_RAW_PRIVATE,
            AIMAGE_FORMAT_DEPTH16,
            AIMAGE_FORMAT_DEPTH_POINT_CLOUD,
            AIMAGE_FORMAT_PRIVATE,
        };

        QMap<acamera_metadata_enum_android_lens_facing_t, QString> facingToStr {
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

            QList<AkCaps> supportedFormats;

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

                AkCaps videoCaps;
                videoCaps.setMimeType("video/unknown");
                videoCaps.setProperty("fourcc", imgFmtToStrMap->value(format));
                videoCaps.setProperty("width", width);
                videoCaps.setProperty("height", height);

                if (!supportedFormats.contains(videoCaps))
                    supportedFormats << videoCaps;
            }

            QVariantList caps;

            for (auto &format: supportedFormats)
                for (auto &fps: supportedFrameRates) {
                    auto videoCaps = format;
                    videoCaps.setProperty("fps", fps.toString());
                    caps << QVariant::fromValue(videoCaps);
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
            }

            ACameraMetadata_free(metaData);
        }

        ACameraManager_deleteCameraIdList(cameras);
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

bool CaptureNdkCamera::init()
{
    this->d->m_localImageControls.clear();
    this->d->m_localCameraControls.clear();
    this->uninit();

    QList<int> streams;
    QVariantList supportedCaps;
    AIMAGE_FORMATS format;
    AkCaps caps;
    int32_t fpsRange[2];
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

    if (ACameraManager_openCamera(this->d->m_manager.data(),
                                  cameraId.toStdString().c_str(),
                                  &deviceStateCb,
                                  &this->d->m_camera) != ACAMERA_OK) {
        goto init_failed;
    }

    if (ACameraDevice_createCaptureRequest(this->d->m_camera,
                                           TEMPLATE_PREVIEW,
                                           &this->d->m_captureRequest) != ACAMERA_OK) {
        goto init_failed;
    }

    streams = this->streams();

    if (streams.isEmpty())
        goto init_failed;

    supportedCaps = this->caps(this->d->m_device);
    caps = supportedCaps[streams[0]].value<AkCaps>();
    caps.setProperty("align", 32);
    fps = caps.property("fps").toString();

    if (!this->d->nearestFpsRangue(cameraId, fps, fpsRange[0], fpsRange[1]))
        goto init_failed;

    if (ACaptureRequest_setEntry_i32(this->d->m_captureRequest,
                                     ACAMERA_CONTROL_AE_TARGET_FPS_RANGE,
                                     2,
                                     fpsRange) != ACAMERA_OK) {
        goto init_failed;
    }

    format = imgFmtToStrMap->key(caps.property("fourcc").toString(),
                                 AIMAGE_FORMAT_PRIVATE);

    if (AImageReader_new(caps.property("width").toInt(),
                         caps.property("height").toInt(),
                         format,
                         this->d->m_nBuffers,
                         &this->d->m_imageReader) != AMEDIA_OK) {
        goto init_failed;
    }

    if (AImageReader_setImageListener(this->d->m_imageReader,
                                      &imageListenerCb) != AMEDIA_OK) {
        goto init_failed;
    }

    if (AImageReader_getWindow(this->d->m_imageReader,
                               &this->d->m_imageReaderWindow) != AMEDIA_OK) {
        goto init_failed;
    }

    ANativeWindow_acquire(this->d->m_imageReaderWindow);

    if (ACaptureSessionOutput_create(this->d->m_imageReaderWindow,
                                     &this->d->m_sessionOutput) != ACAMERA_OK) {
        goto init_failed;
    }

    if (ACaptureSessionOutputContainer_create(&this->d->m_outputContainer) != ACAMERA_OK) {
        goto init_failed;
    }

    if (ACaptureSessionOutputContainer_add(this->d->m_outputContainer,
                                           this->d->m_sessionOutput) != ACAMERA_OK) {
        goto init_failed;
    }

    if (ACameraOutputTarget_create(this->d->m_imageReaderWindow,
                                   &this->d->m_outputTarget) != ACAMERA_OK) {
        goto init_failed;
    }

    if (ACaptureRequest_addTarget(this->d->m_captureRequest,
                                  this->d->m_outputTarget) != ACAMERA_OK) {
        goto init_failed;
    }

    if (ACameraDevice_createCaptureSession(this->d->m_camera,
                                           this->d->m_outputContainer,
                                           &sessionStateCb,
                                           &this->d->m_captureSession) != ACAMERA_OK) {
        goto init_failed;
    }

    if (ACameraCaptureSession_setRepeatingRequest(this->d->m_captureSession,
                                                  nullptr,
                                                  1,
                                                  &this->d->m_captureRequest,
                                                  nullptr) != ACAMERA_OK) {
        goto init_failed;
    }

    this->d->m_id = Ak::id();
    this->d->m_caps = caps;
    this->d->m_fps = fps;

    return true;

init_failed:
    this->uninit();

    return false;
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


        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lockForWrite();
    this->d->m_controlsMutex.unlock();

    emit this->deviceChanged(device);
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

void CaptureNdkCamera::resetDevice()
{
    this->setDevice("");
}

void CaptureNdkCamera::resetStreams()
{
    QVariantList supportedCaps = this->caps(this->d->m_device);
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

void CaptureNdkCamera::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

#include "moc_capturendkcamera.cpp"

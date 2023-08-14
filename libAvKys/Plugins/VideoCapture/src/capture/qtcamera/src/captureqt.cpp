/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#include <QDebug>
#include <QApplication>
#include <QCameraImageCapture>
#include <QCameraInfo>
#include <QMediaRecorder>
#include <QReadWriteLock>
#include <QTimer>
#include <ak.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akelement.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoformatspec.h>
#include <akvideopacket.h>
#include <akcompressedvideocaps.h>
#include <akcompressedvideopacket.h>

#include "captureqt.h"
#include "videosurface.h"

#define CONTROL_BRIGHTNESS         "Brightness"
#define CONTROL_CONTRAST           "Contrast"
#define CONTROL_SATURATION         "Saturation"
#define CONTROL_SHARPENING         "Sharpening"
#define CONTROL_DENOISING          "Denoising"
#define CONTROL_WHITE_BALANCE_MODE "White Balance Mode"
#define CONTROL_COLOR_FILTER       "Color Filter"

static const int minControlValue = 0;
static const int maxControlValue = 255;

using WhiteBalanceModeMap = QMap<QCameraImageProcessing::WhiteBalanceMode, QString>;

inline const WhiteBalanceModeMap initWhiteBalanceModeMap()
{
    static const WhiteBalanceModeMap whiteBalanceModeMap {
        {QCameraImageProcessing::WhiteBalanceAuto       , "Auto"       },
        {QCameraImageProcessing::WhiteBalanceManual     , "Manual"     },
        {QCameraImageProcessing::WhiteBalanceSunlight   , "Sunlight"   },
        {QCameraImageProcessing::WhiteBalanceCloudy     , "Cloudy"     },
        {QCameraImageProcessing::WhiteBalanceShade      , "Shade"      },
        {QCameraImageProcessing::WhiteBalanceTungsten   , "Tungsten"   },
        {QCameraImageProcessing::WhiteBalanceFluorescent, "Fluorescent"},
        {QCameraImageProcessing::WhiteBalanceFlash      , "Flash"      },
        {QCameraImageProcessing::WhiteBalanceSunset     , "Sunset"     },
        {QCameraImageProcessing::WhiteBalanceVendor     , "Vendor"     },
    };

    return whiteBalanceModeMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(WhiteBalanceModeMap,
                          whiteBalanceModeMap,
                          (initWhiteBalanceModeMap()))

using ColorFilterMap = QMap<QCameraImageProcessing::ColorFilter, QString>;

inline const ColorFilterMap initColorFilterMap()
{
    static const ColorFilterMap colorFilterMap {
        {QCameraImageProcessing::ColorFilterNone      , "None"      },
        {QCameraImageProcessing::ColorFilterGrayscale , "Grayscale" },
        {QCameraImageProcessing::ColorFilterNegative  , "Negative"  },
        {QCameraImageProcessing::ColorFilterSolarize  , "Solarize"  },
        {QCameraImageProcessing::ColorFilterSepia     , "Sepia"     },
        {QCameraImageProcessing::ColorFilterPosterize , "Posterize" },
        {QCameraImageProcessing::ColorFilterWhiteboard, "Whiteboard"},
        {QCameraImageProcessing::ColorFilterBlackboard, "Blackboard"},
        {QCameraImageProcessing::ColorFilterAqua      , "Aqua"      },
        {QCameraImageProcessing::ColorFilterVendor    , "Vendor"    },
    };

    return colorFilterMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(ColorFilterMap,
                          colorFilterMap,
                          (initColorFilterMap()))

using CameraPtr = QSharedPointer<QCamera>;

class CaptureQtPrivate
{
    public:
        CaptureQt *self;
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
        CameraPtr m_camera;
        VideoSurface m_surface;
        QTimer m_timer;
        AkElementPtr m_hslFilter {akPluginManager->create<AkElement>("VideoFilter/AdjustHSL")};
        AkElementPtr m_contrastFilter {akPluginManager->create<AkElement>("VideoFilter/Contrast")};
        AkElementPtr m_gammaFilter {akPluginManager->create<AkElement>("VideoFilter/Gamma")};

        explicit CaptureQtPrivate(CaptureQt *self);
        ~CaptureQtPrivate();
        QSize nearestResolution(const QSize &resolution,
                                const QList<QSize> &resolutions) const;
        QVariantList imageControls(const CameraPtr &camera) const;
        bool setImageControls(const CameraPtr &camera,
                              const QVariantMap &imageControls) const;
        QVariantList cameraControls(const CameraPtr &camera) const;
        bool setCameraControls(const CameraPtr &camera,
                               const QVariantMap &cameraControls) const;
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        void updateDevices();
};

CaptureQt::CaptureQt(QObject *parent):
    Capture(parent)
{
    this->d = new CaptureQtPrivate(this);
    this->d->m_timer.setInterval(3000);

    QObject::connect(&this->d->m_timer,
                     &QTimer::timeout,
                     this,
                     [this] () {
                         this->d->updateDevices();
                     });

    this->d->updateDevices();
    this->d->m_timer.start();
}

CaptureQt::~CaptureQt()
{
    delete this->d;
}

QStringList CaptureQt::webcams() const
{
    return this->d->m_devices;
}

QString CaptureQt::device() const
{
    return this->d->m_device;
}

QList<int> CaptureQt::streams()
{
    if (!this->d->m_streams.isEmpty())
        return this->d->m_streams;

    auto caps = this->d->m_devicesCaps.value(this->d->m_device);

    if (caps.isEmpty())
        return {};

    return {0};
}

QList<int> CaptureQt::listTracks(AkCaps::CapsType type)
{
    if (type != AkCaps::CapsVideo && type != AkCaps::CapsUnknown)
        return {};

    auto caps = this->d->m_devicesCaps.value(this->d->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CaptureQt::ioMethod() const
{
    return {};
}

int CaptureQt::nBuffers() const
{
    return 0;
}

QString CaptureQt::description(const QString &webcam) const
{
    return this->d->m_descriptions.value(webcam);
}

CaptureVideoCaps CaptureQt::caps(const QString &webcam) const
{
    CaptureVideoCaps caps;

    for (auto &videoCaps: this->d->m_devicesCaps.value(webcam))
        caps << videoCaps;

    return caps;
}

QVariantList CaptureQt::imageControls() const
{
    return this->d->m_globalImageControls;
}

bool CaptureQt::setImageControls(const QVariantMap &imageControls)
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

bool CaptureQt::resetImageControls()
{
    QVariantMap controls;

    for (auto &control: this->imageControls()) {
        QVariantList params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList CaptureQt::cameraControls() const
{
    return this->d->m_globalCameraControls;
}

bool CaptureQt::setCameraControls(const QVariantMap &cameraControls)
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

bool CaptureQt::resetCameraControls()
{
    QVariantMap controls;

    for (auto &control: this->cameraControls()) {
        auto params = control.toList();
        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket CaptureQt::readFrame()
{
    if (!this->d->m_camera)
        return {};

    this->d->m_controlsMutex.lockForRead();
    auto imageControls = this->d->controlStatus(this->d->m_globalImageControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localImageControls != imageControls) {
        auto controls = this->d->mapDiff(this->d->m_localImageControls,
                                         imageControls);
        this->d->setImageControls(this->d->m_camera, controls);
        this->d->m_localImageControls = imageControls;
    }

    this->d->m_controlsMutex.lockForRead();
    auto cameraControls = this->d->controlStatus(this->d->m_globalCameraControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localCameraControls != cameraControls) {
        auto controls = this->d->mapDiff(this->d->m_localCameraControls,
                                         cameraControls);
        this->d->setCameraControls(this->d->m_camera, controls);
        this->d->m_localCameraControls = cameraControls;
    }

    auto packet = this->d->m_surface.readFrame();

    auto imageProcessing = this->d->m_camera->imageProcessing();

    if (!imageProcessing || !imageProcessing->isAvailable()) {
        packet = this->d->m_hslFilter->iStream(packet);
        packet = this->d->m_gammaFilter->iStream(packet);
        packet = this->d->m_contrastFilter->iStream(packet);
    }

    return packet;
}

bool CaptureQt::init()
{
    this->d->m_localImageControls.clear();
    this->d->m_localCameraControls.clear();

    if (!this->d->m_camera)
        return false;

    auto streams = this->streams();

    if (streams.isEmpty()) {
        qDebug() << "VideoCapture: No streams available.";

        return false;
    }

    auto supportedCaps = this->d->m_devicesCaps.value(this->d->m_device);
    auto caps = supportedCaps[streams[0]];
    QVideoFrame::PixelFormat pixelFormat = QVideoFrame::Format_Invalid;
    int width = 0;
    int height = 0;
    AkFrac fps;

    if (caps.type() == AkCaps::CapsVideo) {
        AkVideoCaps videoCaps(caps);
        pixelFormat = VideoSurface::fromRaw(videoCaps.format());
        width = videoCaps.width();
        height = videoCaps.height();
        fps = videoCaps.fps();
    } else {
        AkCompressedVideoCaps videoCaps(caps);
        pixelFormat = VideoSurface::fromCompressed(videoCaps.format());
        width = videoCaps.width();
        height = videoCaps.height();
        fps = videoCaps.fps();
    }

    this->d->m_camera->load();
    QMediaRecorder mediaRecorder(this->d->m_camera.data());
    auto frameRates = mediaRecorder.supportedFrameRates();
    auto minimumFrameRate = *std::min_element(frameRates.begin(), frameRates.end());
    auto maximumFrameRate = *std::max_element(frameRates.begin(), frameRates.end());
    this->d->m_camera->unload();

    this->d->m_surface.setId(Ak::id());
    this->d->m_surface.setFps(fps);

    auto settings = this->d->m_camera->viewfinderSettings();
    settings.setResolution(width, height);
    settings.setMinimumFrameRate(minimumFrameRate);
    settings.setMaximumFrameRate(maximumFrameRate);
    this->d->m_camera->setViewfinderSettings(settings);
    this->d->m_camera->start();

    return true;
}

void CaptureQt::uninit()
{
    this->d->m_camera->stop();
}

void CaptureQt::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;

    this->d->m_camera = CameraPtr(new QCamera(device.toUtf8()));
    this->d->m_camera->setCaptureMode(QCamera::CaptureViewfinder);
    this->d->m_camera->setViewfinder(&this->d->m_surface);

    if (device.isEmpty()) {
        this->d->m_controlsMutex.lockForWrite();
        this->d->m_globalImageControls.clear();
        this->d->m_globalCameraControls.clear();
        this->d->m_controlsMutex.unlock();
    } else {
        this->d->m_controlsMutex.lockForWrite();
        this->d->m_camera->load();
        this->d->m_globalImageControls = this->d->imageControls(this->d->m_camera);
        this->d->m_globalCameraControls = this->d->cameraControls(this->d->m_camera);
        this->d->m_camera->unload();
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

void CaptureQt::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    auto supportedCaps = this->d->m_devicesCaps.value(this->d->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams {stream};

    if (this->streams() == inputStreams)
        return;

    this->d->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void CaptureQt::setIoMethod(const QString &ioMethod)
{
}

void CaptureQt::setNBuffers(int nBuffers)
{
}

void CaptureQt::resetDevice()
{
    this->setDevice("");
}

void CaptureQt::resetStreams()
{
    auto supportedCaps = this->d->m_devicesCaps.value(this->d->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void CaptureQt::resetIoMethod()
{
    this->setIoMethod("any");
}

void CaptureQt::resetNBuffers()
{
    this->setNBuffers(32);
}

void CaptureQt::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

CaptureQtPrivate::CaptureQtPrivate(CaptureQt *self):
    self(self)
{
}

CaptureQtPrivate::~CaptureQtPrivate()
{
}

QSize CaptureQtPrivate::nearestResolution(const QSize &resolution, const QList<QSize> &resolutions) const
{
    if (resolutions.isEmpty())
        return {};

    QSize nearestResolution;
    qreal q = std::numeric_limits<qreal>::max();

    for (auto &size: resolutions) {
        qreal dw = size.width() - resolution.width();
        qreal dh = size.height() - resolution.height();
        qreal k = dw * dw + dh * dh;

        if (k < q) {
            nearestResolution = size;
            q = k;

            if (k == 0.)
                break;
        }
    }

    return nearestResolution;
}

QVariantList CaptureQtPrivate::imageControls(const CameraPtr &camera) const
{
    QVariantList controlsList;
    auto imageProcessing = camera->imageProcessing();

    if (imageProcessing && imageProcessing->isAvailable()) {
        QMap<QString, qreal> integerControls {
            {CONTROL_BRIGHTNESS, imageProcessing->brightness()     },
            {CONTROL_CONTRAST  , imageProcessing->contrast()       },
            {CONTROL_SATURATION, imageProcessing->saturation()     },
            {CONTROL_SHARPENING, imageProcessing->sharpeningLevel()},
            {CONTROL_DENOISING , imageProcessing->denoisingLevel() },
        };

        for (auto it = integerControls.begin(); it != integerControls.end(); it++) {
            auto value = ((it.value() + 1.0) * (maxControlValue - minControlValue) + 2.0 * minControlValue) / 2.0;
            QVariantList params {
                it.key(),
                "integer",
                minControlValue,
                maxControlValue,
                1,
                (minControlValue + maxControlValue) / 2,
                qRound(value),
                QStringList()
            };
            controlsList << QVariant(params);
        }

        QStringList wbModes;

        for (auto it = whiteBalanceModeMap->begin(); it != whiteBalanceModeMap->end(); it++)
            if (imageProcessing->isWhiteBalanceModeSupported(it.key()))
                wbModes << it.value();

        if (wbModes.size() > 1) {
            auto wbMode = whiteBalanceModeMap->value(imageProcessing->whiteBalanceMode());
            QVariantList params {
                CONTROL_WHITE_BALANCE_MODE,
                "menu",
                0,
                wbModes.size() - 1,
                1,
                0,
                qMax(wbModes.indexOf(wbMode), 0),
                wbModes
            };
            controlsList << QVariant(params);
         }

        QStringList colorFilters;

        for (auto it = colorFilterMap->begin(); it != colorFilterMap->end(); it++)
            if (imageProcessing->isColorFilterSupported(it.key()))
                colorFilters << it.value();

        if (colorFilters.size() > 1) {
            auto colorFilter = colorFilterMap->value(imageProcessing->colorFilter());
            QVariantList params {
                CONTROL_COLOR_FILTER,
                "menu",
                0,
                colorFilters.size() - 1,
                1,
                0,
                qMax(colorFilters.indexOf(colorFilter), 0),
                colorFilter
            };
            controlsList << QVariant(params);
        }
    } else {
        return {
            QVariant(QVariantList {
                                  "Brightness",
                                  "integer",
                                  -255,
                                  255,
                                  1,
                                  0,
                                  this->m_hslFilter->property("luminance").toInt(),
                                  QStringList()}),
            QVariant(QVariantList {
                                  "Contrast",
                                  "integer",
                                  -255,
                                  255,
                                  1,
                                  0,
                                  this->m_contrastFilter->property("contrast").toInt(),
                                  QStringList()}),
            QVariant(QVariantList {
                                  "Saturation",
                                  "integer",
                                  -255,
                                  255,
                                  1,
                                  0,
                                  this->m_hslFilter->property("saturation").toInt(),
                                  QStringList()}),
            QVariant(QVariantList {
                                  "Hue",
                                  "integer",
                                  -359,
                                  359,
                                  1,
                                  0,
                                  this->m_hslFilter->property("hue").toInt(),
                                  QStringList()}),
            QVariant(QVariantList {
                                  "Gamma",
                                  "integer",
                                  -255,
                                  255,
                                  1,
                                  0,
                                  this->m_gammaFilter->property("gamma").toInt(),
                                  QStringList()}),
        };
    }

    return controlsList;
}

bool CaptureQtPrivate::setImageControls(const CameraPtr &camera,
                                        const QVariantMap &imageControls) const
{
    if (!camera)
        return false;

    static const QStringList integerControls {
        CONTROL_BRIGHTNESS,
        CONTROL_CONTRAST,
        CONTROL_SATURATION,
        CONTROL_SHARPENING,
        CONTROL_DENOISING,
    };

    static const QStringList menuControls {
        CONTROL_WHITE_BALANCE_MODE,
        CONTROL_COLOR_FILTER,
    };

    auto imageProcessing = camera->imageProcessing();
    bool ok = true;

    if (imageProcessing && imageProcessing->isAvailable()) {
        for (auto it = imageControls.cbegin(); it != imageControls.cend(); it++) {
            if (integerControls.contains(it.key())) {
                auto value = qreal(2 * it.value().toInt()
                                   - maxControlValue - minControlValue)
                             / qreal(maxControlValue - minControlValue);

                if (it.key() == CONTROL_BRIGHTNESS)
                    imageProcessing->setBrightness(value);
                else if (it.key() == CONTROL_CONTRAST)
                    imageProcessing->setContrast(value);
                else if (it.key() == CONTROL_SATURATION)
                    imageProcessing->setSaturation(value);
                else if (it.key() == CONTROL_SHARPENING)
                    imageProcessing->setSharpeningLevel(value);
                else if (it.key() == CONTROL_DENOISING)
                    imageProcessing->setDenoisingLevel(value);
            } else if (menuControls.contains(it.key())) {
                if (it.key() == CONTROL_WHITE_BALANCE_MODE) {
                    QStringList wbModes;

                    for (auto it = whiteBalanceModeMap->begin(); it != whiteBalanceModeMap->end(); it++)
                        if (imageProcessing->isWhiteBalanceModeSupported(it.key()))
                            wbModes << it.value();

                    auto value = wbModes.value(it.value().toInt());
                    auto mode = whiteBalanceModeMap->key(value, QCameraImageProcessing::WhiteBalanceAuto);
                    imageProcessing->setWhiteBalanceMode(mode);
                } else if (it.key() == CONTROL_COLOR_FILTER) {
                    QStringList colorFilters;

                    for (auto it = colorFilterMap->begin(); it != colorFilterMap->end(); it++)
                        if (imageProcessing->isColorFilterSupported(it.key()))
                            colorFilters << it.value();

                    auto value = colorFilters.value(it.value().toInt());
                    auto filter = colorFilterMap->key(value, QCameraImageProcessing::ColorFilterNone);
                    imageProcessing->setColorFilter(filter);
                }
            } else {
                ok = false;
            }
        }
    } else {
        for (auto it = imageControls.cbegin(); it != imageControls.cend(); it++) {
            if (it.key() == "Brightness")
                this->m_hslFilter->setProperty("luminance", it.value());
            else if (it.key() == "Contrast")
                this->m_contrastFilter->setProperty("contrast", it.value());
            else if (it.key() == "Saturation")
                this->m_hslFilter->setProperty("saturation", it.value());
            else if (it.key() == "Hue")
                this->m_hslFilter->setProperty("hue", it.value());
            else if (it.key() == "Gamma")
                this->m_gammaFilter->setProperty("gamma", it.value());
            else
                ok = false;
        }
    }

    return ok;
}

QVariantList CaptureQtPrivate::cameraControls(const CameraPtr &camera) const
{
    Q_UNUSED(camera)

    return {};
}

bool CaptureQtPrivate::setCameraControls(const CameraPtr &camera,
                                         const QVariantMap &cameraControls) const
{
    Q_UNUSED(camera)
    Q_UNUSED(cameraControls)

    return false;
}

QVariantMap CaptureQtPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlName = params[0].toString();
        controlStatus[controlName] = params[6];
    }

    return controlStatus;
}

QVariantMap CaptureQtPrivate::mapDiff(const QVariantMap &map1,
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

void CaptureQtPrivate::updateDevices()
{
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;

    auto cameras = QCameraInfo::availableCameras();
    VideoSurface surface;

    for (auto &cameraInfo: cameras) {
        auto deviceName = cameraInfo.deviceName();
        QCamera camera(cameraInfo);
        QCameraImageCapture imageCapture(&camera);
        QMediaRecorder mediaRecorder(&camera);

        camera.setCaptureMode(QCamera::CaptureViewfinder);
        camera.setViewfinder(&surface);
        camera.load();

        while (camera.state() == QCamera::UnloadedState)
            qApp->processEvents();

        auto formats = imageCapture.supportedBufferFormats();
        auto resolutions = imageCapture.supportedResolutions();
        auto frameRates = mediaRecorder.supportedFrameRates();

        // Set 640x480 as default most common resolution.
        auto defaultResolution = this->nearestResolution({640, 480},
                                                         resolutions);
        resolutions.removeAll(defaultResolution);
        resolutions.prepend(defaultResolution);

        // Sort frame rates in descending order.
        std::sort(frameRates.begin(), frameRates.end(), std::greater<qreal>());

        CaptureVideoCaps caps;

        for (auto &format: formats)
            for (auto &resolution: resolutions)
                for (auto &rate: frameRates) {
                    AkFrac fps(qRound(rate), 1);

                    if (VideoSurface::isRaw(format)) {
                        AkVideoCaps videoCaps(VideoSurface::rawFormat(format),
                                              resolution.width(),
                                              resolution.height(),
                                              fps);
                        caps << videoCaps;
                    } else if (VideoSurface::isCompessed(format)) {
                        AkCompressedVideoCaps videoCaps(VideoSurface::compressedFormat(format),
                                                        resolution.width(),
                                                        resolution.height(),
                                                        fps);
                        caps << videoCaps;
                    }
                }

        camera.unload();

        if (!caps.isEmpty()) {
            devices << deviceName;
            descriptions[deviceName] = cameraInfo.description();
            devicesCaps[deviceName] = caps;
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

#include "moc_captureqt.cpp"

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
#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QQmlContext>
#include <QSharedPointer>
#include <QMutex>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideopacket.h>

#include "virtualcameraelement.h"
#include "ipcbridge.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"

#define MAX_CAMERAS 64
#define PREFERRED_ROUNDING 32

using PixelFormatMap = QMap<AkVideoCaps::PixelFormat, AkVCam::PixelFormat>;
using ScalingMap = QMap<VirtualCameraElement::Scaling, AkVCam::Scaling>;
using AspectRatioMap = QMap<VirtualCameraElement::AspectRatio, AkVCam::AspectRatio>;

class VirtualCameraElementPrivate
{
    public:
        AkVCam::IpcBridge m_ipcBridge;
        AkCaps m_streamCaps;
        QMutex m_mutex;
        QString m_curDevice;
        QDir m_applicationDir;
        int m_streamIndex {-1};
        bool m_playing {false};

        VirtualCameraElementPrivate();
        ~VirtualCameraElementPrivate();
        std::vector<std::wstring> *driverPaths();
        QString convertToAbsolute(const QString &path) const;
        static void serverStateChanged(void *userData,
                                       AkVCam::IpcBridge::ServerState state);
        static inline int roundTo(int value, int n);
        static inline const PixelFormatMap &akToVCamPixelFormatMap();
        static inline const ScalingMap &akToVCamScalingMap();
        static inline const AspectRatioMap &akToVCamAspectRatioMap();
};

VirtualCameraElement::VirtualCameraElement():
    AkElement()
{
    this->d = new VirtualCameraElementPrivate;
}

VirtualCameraElement::~VirtualCameraElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QString VirtualCameraElement::errorMessage() const
{
    return QString::fromStdWString(this->d->m_ipcBridge.errorMessage());
}

QStringList VirtualCameraElement::driverPaths() const
{
    QStringList driverPaths;

    for (auto &path: *this->d->driverPaths())
        driverPaths << QString::fromStdWString(path);

    return driverPaths;
}

QStringList VirtualCameraElement::medias() const
{
    QStringList webcams;

    for (auto &device: this->d->m_ipcBridge.listDevices())
        webcams << QString::fromStdString(device);

    return webcams;
}

QString VirtualCameraElement::media() const
{
    return this->d->m_curDevice;
}

QList<int> VirtualCameraElement::streams() const
{
    return QList<int> {0};
}

int VirtualCameraElement::maxCameras() const
{
    return MAX_CAMERAS;
}

QString VirtualCameraElement::driver() const
{
    return QString::fromStdString(this->d->m_ipcBridge.driver());
}

QStringList VirtualCameraElement::availableDrivers() const
{
    QStringList drivers;

    for (auto &driver: this->d->m_ipcBridge.availableDrivers())
        drivers << QString::fromStdString(driver);

    return drivers;
}

QString VirtualCameraElement::rootMethod() const
{
    return QString::fromStdString(this->d->m_ipcBridge.rootMethod());
}

QStringList VirtualCameraElement::availableMethods() const
{
    QStringList methods;

    for (auto &method: this->d->m_ipcBridge.availableRootMethods())
        methods << QString::fromStdString(method);

    return methods;
}

AkVideoCaps::PixelFormatList VirtualCameraElement::supportedOutputPixelFormats() const
{
    AkVideoCaps::PixelFormatList formats;
    auto &formatConvert = VirtualCameraElementPrivate::akToVCamPixelFormatMap();

    for (auto &format: this->d->m_ipcBridge.supportedOutputPixelFormats()) {
        auto akFormat = formatConvert.key(format, AkVideoCaps::Format_none);

        if (akFormat != AkVideoCaps::Format_none)
            formats << akFormat;
    }

    return formats;
}

AkVideoCaps::PixelFormat VirtualCameraElement::defaultOutputPixelFormat() const
{
    auto format = this->d->m_ipcBridge.defaultOutputPixelFormat();
    auto &formatConvert = VirtualCameraElementPrivate::akToVCamPixelFormatMap();

    return formatConvert.key(format, AkVideoCaps::Format_none);
}

bool VirtualCameraElement::horizontalMirrored() const
{
    return this->d->m_ipcBridge.isHorizontalMirrored(this->d->m_curDevice.toStdString());
}

bool VirtualCameraElement::verticalMirrored() const
{
    return this->d->m_ipcBridge.isVerticalMirrored(this->d->m_curDevice.toStdString());
}

VirtualCameraElement::Scaling VirtualCameraElement::scalingMode() const
{
    auto scalingMode =
            this->d->m_ipcBridge.scalingMode(this->d->m_curDevice.toStdString());
    auto vcamScalingMode =
            VirtualCameraElementPrivate::akToVCamScalingMap().key(scalingMode,
                                                                  ScalingFast);

    return vcamScalingMode;
}

VirtualCameraElement::AspectRatio VirtualCameraElement::aspectRatioMode() const
{
    auto aspectRatio =
            this->d->m_ipcBridge.aspectRatioMode(this->d->m_curDevice.toStdString());
    auto vcamAspectRatio =
            VirtualCameraElementPrivate::akToVCamAspectRatioMap().key(aspectRatio,
                                                                      AspectRatioIgnore);

    return vcamAspectRatio;
}

bool VirtualCameraElement::swapRgb() const
{
    return this->d->m_ipcBridge.swapRgb(this->d->m_curDevice.toStdString());
}

int VirtualCameraElement::defaultStream(const QString &mimeType) const
{
    if (mimeType == "video/x-raw")
        return 0;

    return -1;
}

QString VirtualCameraElement::description(const QString &media) const
{
    return QString::fromStdWString(this->d->m_ipcBridge.description(media.toStdString()));
}

AkCaps VirtualCameraElement::caps(int stream) const
{
    if (stream != 0)
        return AkCaps();

    return this->d->m_streamCaps;
}

AkVideoCapsList VirtualCameraElement::outputCaps(const QString &webcam) const
{
    AkVideoCapsList caps;
    auto formats = this->d->m_ipcBridge.formats(webcam.toStdString());
    auto &formatConvert =
            VirtualCameraElementPrivate::akToVCamPixelFormatMap();

    for (auto &format: formats) {
        auto pixelFormat =
                formatConvert.key(AkVCam::PixelFormat(format.fourcc()),
                                  AkVideoCaps::Format_none);

        if (pixelFormat != AkVideoCaps::Format_none)
            caps << AkVideoCaps {
                pixelFormat,
                format.width(),
                format.height(),
                {format.frameRates()[0].num(),
                 format.frameRates()[0].den()}
            };
    }

    return caps;
}

QVariantMap VirtualCameraElement::addStream(int streamIndex,
                                            const AkCaps &streamCaps,
                                            const QVariantMap &streamParams)
{
    Q_UNUSED(streamParams)

    if (streamIndex != 0)
        return {};

    AkVideoCaps videoCaps(streamCaps);
    videoCaps.setFormat(AkVideoCaps::Format_rgb24);
    videoCaps.setWidth(VirtualCameraElementPrivate::roundTo(videoCaps.width(),
                                                            PREFERRED_ROUNDING));
    videoCaps.setHeight(VirtualCameraElementPrivate::roundTo(videoCaps.height(),
                                                             PREFERRED_ROUNDING));

    this->d->m_streamIndex = streamIndex;
    this->d->m_streamCaps = videoCaps;

    QVariantMap outputParams = {
        {"caps", QVariant::fromValue(streamCaps)}
    };

    return outputParams;
}

QVariantMap VirtualCameraElement::updateStream(int streamIndex,
                                               const QVariantMap &streamParams)
{
    if (streamIndex != 0)
        return {};

    auto streamCaps = streamParams.value("caps").value<AkCaps>();

    if (!streamCaps)
        return {};

    AkVideoCaps videoCaps(streamCaps);
    videoCaps.setFormat(AkVideoCaps::Format_rgb24);
    videoCaps.setWidth(VirtualCameraElementPrivate::roundTo(videoCaps.width(),
                                                            PREFERRED_ROUNDING));
    videoCaps.setHeight(VirtualCameraElementPrivate::roundTo(videoCaps.height(),
                                                             PREFERRED_ROUNDING));

    this->d->m_streamIndex = streamIndex;
    this->d->m_streamCaps = videoCaps;

    QVariantMap outputParams {
        {"caps", QVariant::fromValue(streamCaps)}
    };

    return outputParams;
}

QString VirtualCameraElement::createWebcam(const QString &description,
                                           const AkVideoCapsList &formats)
{
    std::vector<AkVCam::VideoFormat> vcamFormats;

    for (auto &format: formats)
        vcamFormats.push_back({
            AkVCam::FourCC(format.fourCC()),
            format.width(),
            format.height(),
            {{format.fps().num(), format.fps().den()}}
        });

    auto webcam =
            this->d->m_ipcBridge.deviceCreate(description.toStdWString(),
                                              vcamFormats);

    if (webcam.empty()) {
        auto error = this->d->m_ipcBridge.errorMessage();
        emit this->errorMessageChanged(QString::fromStdWString(error));

        return {};
    }

    emit this->mediasChanged(this->medias());

    return QString::fromStdString(webcam);
}

bool VirtualCameraElement::changeDescription(const QString &webcam,
                                             const QString &description) const
{
    bool ok = this->d->m_ipcBridge.changeDescription(webcam.toStdString(),
                                                     description.toStdWString());

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

bool VirtualCameraElement::removeWebcam(const QString &webcam)
{
    bool ok = this->d->m_ipcBridge.deviceDestroy(webcam.toStdString());

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

bool VirtualCameraElement::removeAllWebcams()
{
    bool ok = this->d->m_ipcBridge.destroyAllDevices();

    if (ok)
        emit this->mediasChanged(this->medias());

    return ok;
}

QString VirtualCameraElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VirtualCamera/share/qml/main.qml");
}

void VirtualCameraElement::controlInterfaceConfigure(QQmlContext *context,
                                                     const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("virtualCamera", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", controlId);
}

AkPacket VirtualCameraElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_mutex.lock();

    if (this->state() == AkElement::ElementStatePlaying) {
        auto videoPacket = packet.convert(AkVideoCaps::Format_rgb24, 32);
        auto fps = AkVCam::Fraction {uint32_t(videoPacket.caps().fps().num()),
                                     uint32_t(videoPacket.caps().fps().den())};
        AkVCam::VideoFormat format(videoPacket.caps().fourCC(),
                                   videoPacket.caps().width(),
                                   videoPacket.caps().height(),
                                   {fps});
        AkVCam::VideoFrame frame(format);
        memcpy(frame.data().data(),
               videoPacket.buffer().constData(),
               size_t(videoPacket.buffer().size()));
        this->d->m_ipcBridge.write(this->d->m_curDevice.toStdString(), frame);
    }

    this->d->m_mutex.unlock();

    akSend(packet)
}

void VirtualCameraElement::setDriverPaths(const QStringList &driverPaths)
{
    std::vector<std::wstring> paths;

    for (auto &path: driverPaths)
        if (QFileInfo::exists(path))
            paths.push_back(path.toStdWString());

    if (paths != *this->d->driverPaths()) {
        *this->d->driverPaths() = paths;
        this->d->m_ipcBridge.setDriverPaths(paths);
        emit this->driverPathsChanged(this->driverPaths());
    }
}

void VirtualCameraElement::addDriverPath(const QString &driverPath)
{
    if (QFileInfo::exists(driverPath))
        return;

    auto paths = *this->d->driverPaths();
    paths.push_back(driverPath.toStdWString());
    *this->d->driverPaths() = paths;
    this->d->m_ipcBridge.setDriverPaths(paths);
    emit this->driverPathsChanged(this->driverPaths());
}

void VirtualCameraElement::addDriverPaths(const QStringList &driverPaths)
{
    auto paths = *this->d->driverPaths();

    for (auto &path: driverPaths)
        if (QFileInfo::exists(path))
            paths.push_back(path.toStdWString());

    if (paths != *this->d->driverPaths()) {
        *this->d->driverPaths() = paths;
        this->d->m_ipcBridge.setDriverPaths(paths);
        emit this->driverPathsChanged(this->driverPaths());
    }
}

void VirtualCameraElement::removeDriverPath(const QString &driverPath)
{
    std::vector<std::wstring> paths;

    for (auto &path: *this->d->driverPaths())
        if (QString::fromStdWString(path) != driverPath)
            paths.push_back(path);

    if (paths != *this->d->driverPaths()) {
        *this->d->driverPaths() = paths;
        this->d->m_ipcBridge.setDriverPaths(paths);
        emit this->driverPathsChanged(this->driverPaths());
    }
}

void VirtualCameraElement::removeDriverPaths(const QStringList &driverPaths)
{
    std::vector<std::wstring> paths;

    for (auto &path: *this->d->driverPaths())
        if (!driverPaths.contains(QString::fromStdWString(path)))
            paths.push_back(path);

    if (paths != *this->d->driverPaths()) {
        *this->d->driverPaths() = paths;
        this->d->m_ipcBridge.setDriverPaths(paths);
        emit this->driverPathsChanged(this->driverPaths());
    }
}

void VirtualCameraElement::setMedia(const QString &media)
{
    if (this->d->m_curDevice == media)
        return;

    this->d->m_curDevice = media;
    emit this->mediaChanged(media);
}

void VirtualCameraElement::setDriver(const QString &driver)
{
    if (this->driver() == driver)
        return;

    this->d->m_ipcBridge.setDriver(driver.toStdString());
    emit this->driverChanged(driver);
}

void VirtualCameraElement::setRootMethod(const QString &rootMethod)
{
    if (this->rootMethod() == rootMethod)
        return;

    this->d->m_ipcBridge.setRootMethod(rootMethod.toStdString());
    emit this->rootMethodChanged(rootMethod);
}

void VirtualCameraElement::setHorizontalMirrored(bool horizontalMirrored)
{
    auto device = this->d->m_curDevice.toStdString();
    auto horizontalMirrored_ = this->d->m_ipcBridge.isHorizontalMirrored(device);

    if (horizontalMirrored_ == horizontalMirrored)
        return;

    auto verticalMirrored = this->d->m_ipcBridge.isVerticalMirrored(device);
    this->d->m_ipcBridge.setMirroring(device,
                                      horizontalMirrored,
                                      verticalMirrored);
    emit horizontalMirroredChanged(horizontalMirrored);
}

void VirtualCameraElement::setVerticalMirrored(bool verticalMirrored)
{
    auto device = this->d->m_curDevice.toStdString();
    auto verticalMirrored_ = this->d->m_ipcBridge.isVerticalMirrored(device);

    if (verticalMirrored_ == verticalMirrored)
        return;

    auto horizontalMirrored = this->d->m_ipcBridge.isHorizontalMirrored(device);
    this->d->m_ipcBridge.setMirroring(device,
                                      horizontalMirrored,
                                      verticalMirrored);
    emit verticalMirroredChanged(verticalMirrored);
}

void VirtualCameraElement::setScalingMode(Scaling scalingMode)
{
    auto device = this->d->m_curDevice.toStdString();
    auto scalingMode_ = this->d->m_ipcBridge.scalingMode(device);
    auto vcamScalingMode =
            VirtualCameraElementPrivate::akToVCamScalingMap().value(scalingMode,
                                                                    AkVCam::ScalingFast);

    if (scalingMode_ == vcamScalingMode)
        return;

    this->d->m_ipcBridge.setScaling(device, vcamScalingMode);
    emit scalingModeChanged(scalingMode);
}

void VirtualCameraElement::setAspectRatioMode(AspectRatio aspectRatioMode)
{
    auto device = this->d->m_curDevice.toStdString();
    auto aspectRatioMode_ = this->d->m_ipcBridge.aspectRatioMode(device);
    auto vcamAspectRatioMode =
            VirtualCameraElementPrivate::akToVCamAspectRatioMap().value(aspectRatioMode,
                                                                        AkVCam::AspectRatioIgnore);

    if (aspectRatioMode_ == vcamAspectRatioMode)
        return;

    this->d->m_ipcBridge.setAspectRatio(device, vcamAspectRatioMode);
    emit aspectRatioModeChanged(aspectRatioMode);
}

void VirtualCameraElement::setSwapRgb(bool swapRgb)
{
    auto device = this->d->m_curDevice.toStdString();
    auto swapRgb_ = this->d->m_ipcBridge.swapRgb(device);

    if (swapRgb_ == swapRgb)
        return;

    this->d->m_ipcBridge.setSwapRgb(device, swapRgb);
    emit swapRgbChanged(swapRgb);
}

void VirtualCameraElement::resetDriverPaths()
{
#if defined(Q_OS_OSX) || defined(Q_OS_WIN32)
    std::vector<std::wstring> paths = {
        QString(DATAROOTDIR).toStdWString(),
        this->d->convertToAbsolute("../share").toStdWString(),
#ifdef Q_OS_OSX
        this->d->convertToAbsolute("../Resources").toStdWString(),
#endif
    };
#else
    std::vector<std::wstring> paths;
#endif

    if (paths != *this->d->driverPaths()) {
        *this->d->driverPaths() = paths;
        this->d->m_ipcBridge.setDriverPaths(paths);
        emit this->driverPathsChanged(this->driverPaths());
    }
}

void VirtualCameraElement::resetMedia()
{
    auto devices = this->d->m_ipcBridge.listDevices();

    if (devices.empty())
        this->d->m_curDevice.clear();
    else
        this->d->m_curDevice = QString::fromStdString(devices.front());
}

void VirtualCameraElement::resetDriver()
{
    auto drivers = this->d->m_ipcBridge.availableDrivers();

    if (drivers.empty())
        this->d->m_ipcBridge.setDriver({});
    else
        this->d->m_ipcBridge.setDriver(drivers.front());

    auto driver = this->d->m_ipcBridge.driver();
    emit this->driverChanged(QString::fromStdString(driver));
}

void VirtualCameraElement::resetRootMethod()
{
    auto rootMethods = this->d->m_ipcBridge.availableRootMethods();

    if (rootMethods.empty())
        this->d->m_ipcBridge.setRootMethod({});
    else
        this->d->m_ipcBridge.setRootMethod(rootMethods.front());
}

void VirtualCameraElement::resetHorizontalMirrored()
{
    this->setHorizontalMirrored(false);
}

void VirtualCameraElement::resetVerticalMirrored()
{
    this->setVerticalMirrored(false);
}

void VirtualCameraElement::resetScalingMode()
{
    this->setScalingMode(ScalingFast);
}

void VirtualCameraElement::resetAspectRatioMode()
{
    this->setAspectRatioMode(AspectRatioIgnore);
}

void VirtualCameraElement::resetSwapRgb()
{
    this->setSwapRgb(false);
}

void VirtualCameraElement::clearStreams()
{
    this->d->m_streamIndex = -1;
    this->d->m_streamCaps.clear();
}

bool VirtualCameraElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
        case AkElement::ElementStatePlaying: {
            this->d->m_mutex.lock();
            if (this->d->m_curDevice.isEmpty()) {
                QStringList webcams = this->medias();

                if (webcams.isEmpty()) {
                    this->d->m_mutex.unlock();

                    return false;
                }

                this->d->m_curDevice = webcams.front();
            }

            AkVideoCaps caps = this->d->m_streamCaps;
            auto fps = AkVCam::Fraction {uint32_t(caps.fps().num()),
                                         uint32_t(caps.fps().den())};
            AkVCam::VideoFormat format(AkVCam::PixelFormatRGB24,
                                       caps.width(),
                                       caps.height(),
                                       {fps});

            if (!this->d->m_ipcBridge.deviceStart(this->d->m_curDevice.toStdString(),
                                                  format)) {
                this->d->m_mutex.unlock();

                return false;
            }

            this->d->m_mutex.unlock();
            this->d->m_playing = true;

            return AkElement::setState(state);
        }
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->m_playing = false;

            this->d->m_mutex.lock();
            this->d->m_ipcBridge.deviceStop(this->d->m_curDevice.toStdString());
            this->d->m_mutex.unlock();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->m_playing = false;

            this->d->m_mutex.lock();
            this->d->m_ipcBridge.deviceStop(this->d->m_curDevice.toStdString());
            this->d->m_mutex.unlock();

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

void VirtualCameraElement::rootMethodUpdated(const QString &rootMethod)
{
    this->d->m_ipcBridge.setRootMethod(rootMethod.toStdString());
}

VirtualCameraElementPrivate::VirtualCameraElementPrivate()
{
    this->m_applicationDir.setPath(QCoreApplication::applicationDirPath());
    this->m_ipcBridge.connectServerStateChanged(this,
                                                &VirtualCameraElementPrivate::serverStateChanged);
    this->m_ipcBridge.connectService(false);
    auto devices = this->m_ipcBridge.listDevices();

    if (!devices.empty())
        this->m_curDevice = QString::fromStdString(devices.front());
}

VirtualCameraElementPrivate::~VirtualCameraElementPrivate()
{
    this->m_ipcBridge.disconnectService();
}

std::vector<std::wstring> *VirtualCameraElementPrivate::driverPaths()
{
#if defined(Q_OS_OSX) || defined(Q_OS_WIN32)
    static std::vector<std::wstring> paths = {
        QString(DATAROOTDIR).toStdWString(),
        this->convertToAbsolute("../share").toStdWString(),
#ifdef Q_OS_OSX
        this->convertToAbsolute("../Resources").toStdWString(),
#endif
    };
#else
    static std::vector<std::wstring> paths;
#endif

    static bool driverPathsSet = false;

    if (!driverPathsSet) {
        this->m_ipcBridge.setDriverPaths(paths);
        driverPathsSet = true;
    }

    return &paths;
}

QString VirtualCameraElementPrivate::convertToAbsolute(const QString &path) const
{
    if (!QDir::isRelativePath(path))
        return QDir::cleanPath(path);

    auto absPath = this->m_applicationDir.absoluteFilePath(path);

    return QDir::cleanPath(absPath);
}

void VirtualCameraElementPrivate::serverStateChanged(void *userData,
                                                     AkVCam::IpcBridge::ServerState state)
{
    auto self = reinterpret_cast<VirtualCameraElementPrivate *>(userData);

    switch (state) {
        case AkVCam::IpcBridge::ServerStateAvailable:
            self->m_ipcBridge.deviceStop(self->m_curDevice.toStdString());

            if (self->m_playing) {
                AkVideoCaps caps = self->m_streamCaps;
                auto fps = AkVCam::Fraction {uint32_t(caps.fps().num()),
                                             uint32_t(caps.fps().den())};
                AkVCam::VideoFormat format(AkVCam::PixelFormatRGB24,
                                           caps.width(),
                                           caps.height(),
                                           {fps});
                self->m_ipcBridge.deviceStart(self->m_curDevice.toStdString(),
                                              format);
            }

            break;

        case AkVCam::IpcBridge::ServerStateGone:
            break;
    }
}

int VirtualCameraElementPrivate::roundTo(int value, int n)
{
    return n * qRound(value / qreal(n));
}

const PixelFormatMap &VirtualCameraElementPrivate::akToVCamPixelFormatMap()
{
    static const PixelFormatMap pixelFormatMap {
        {AkVideoCaps::Format_0rgb    , AkVCam::PixelFormatRGB32},
        {AkVideoCaps::Format_rgb24   , AkVCam::PixelFormatRGB24},
        {AkVideoCaps::Format_rgb565le, AkVCam::PixelFormatRGB16},
        {AkVideoCaps::Format_rgb555le, AkVCam::PixelFormatRGB15},
        {AkVideoCaps::Format_0bgr    , AkVCam::PixelFormatBGR32},
        {AkVideoCaps::Format_bgr24   , AkVCam::PixelFormatBGR24},
        {AkVideoCaps::Format_bgr565le, AkVCam::PixelFormatBGR16},
        {AkVideoCaps::Format_bgr555le, AkVCam::PixelFormatBGR15},
        {AkVideoCaps::Format_uyvy422 , AkVCam::PixelFormatUYVY },
        {AkVideoCaps::Format_yuyv422 , AkVCam::PixelFormatYUY2 },
        {AkVideoCaps::Format_nv12    , AkVCam::PixelFormatNV12 },
        {AkVideoCaps::Format_nv21    , AkVCam::PixelFormatNV21 }
    };

    return pixelFormatMap;
}

const ScalingMap &VirtualCameraElementPrivate::akToVCamScalingMap()
{
    static const ScalingMap scalingMap {
        {VirtualCameraElement::ScalingFast  , AkVCam::ScalingFast  },
        {VirtualCameraElement::ScalingLinear, AkVCam::ScalingLinear}
    };

    return scalingMap;
}

const AspectRatioMap &VirtualCameraElementPrivate::akToVCamAspectRatioMap()
{
    static const AspectRatioMap aspectRatioMap {
        {VirtualCameraElement::AspectRatioIgnore   , AkVCam::AspectRatioIgnore   },
        {VirtualCameraElement::AspectRatioKeep     , AkVCam::AspectRatioKeep     },
        {VirtualCameraElement::AspectRatioExpanding, AkVCam::AspectRatioExpanding}
    };

    return aspectRatioMap;
}

#include "moc_virtualcameraelement.cpp"

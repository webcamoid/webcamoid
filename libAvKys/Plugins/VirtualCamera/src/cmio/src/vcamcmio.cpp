/* Webcamoid, webcam capture application.
 * Copyright (C) 2018  Gonzalo Exequiel Pedone
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
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibrary>
#include <QMap>
#include <QMutex>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVariant>
#include <akfrac.h>
#include <akvideoconverter.h>

#include "vcamcmio.h"

using vcam_id_fn = void (*)(char *id, size_t *id_len);
using vcam_version_fn = void (*)(int *major, int *minor, int *patch);
using vcam_open_fn = void *(*)();
using vcam_close_fn = void (*)(void *vcam);
using vcam_devices_fn = int (*)(void *vcam, char *devs, size_t *buffer_size);
using vcam_add_device_fn = int (*)(void *vcam,
                                   const char *description,
                                   char *device_id,
                                   size_t buffer_size);
using vcam_remove_device_fn = int (*)(void *vcam, const char *device_id);
using vcam_remove_devices_fn = int (*)(void *vcam);
using vcam_description_fn = int (*)(void *vcam,
                                    const char *device_id,
                                    char *device_description,
                                    size_t *buffer_size);
using vcam_set_description_fn = int (*)(void *vcam,
                                        const char *device_id,
                                        const char *description);
using vcam_supported_input_formats_fn = int (*)(void *vcam,
                                                char *formats,
                                                size_t *buffer_size);
using vcam_supported_output_formats_fn = int (*)(void *vcam,
                                                 char *formats,
                                                 size_t *buffer_size);
using vcam_default_input_format_fn = int (*)(void *vcam,
                                             char *format,
                                             size_t *buffer_size);
using vcam_default_output_format_fn = int (*)(void *vcam,
                                              char *format,
                                              size_t *buffer_size);
using vcam_format_fn = int (*)(void *vcam,
                               const char *device_id,
                               int index,
                               char *format,
                               size_t *format_bfsz,
                               int *width,
                               int *height,
                               int *fps_num,
                               int *fps_den);
using vcam_add_format_fn = int (*)(void *vcam,
                                   const char *device_id,
                                   const char *format,
                                   int width,
                                   int height,
                                   int fps_num,
                                   int fps_den,
                                   int *index);
using vcam_remove_format_fn = int (*)(void *vcam,
                                      const char *device_id,
                                      int index);
using vcam_remove_formats_fn = int (*)(void *vcam, const char *device_id);
using vcam_update_fn = int (*)(void *vcam);
using vcam_load_fn = int (*)(void *vcam, const char *settings_ini);
using vcam_stream_start_fn = int (*)(void *vcam, const char *device_id);
using vcam_stream_send_fn = int (*)(void *vcam,
                                    const char *device_id,
                                    const char *format,
                                    int width,
                                    int height,
                                    const char **data,
                                    size_t *line_size);
using vcam_stream_stop_fn = int (*)(void *vcam, const char *device_id);
using vcam_event_fn = void (*)(void *context, const char *event);
using vcam_set_event_listener_fn = int (*)(void *vcam,
                                           void *context,
                                           vcam_event_fn event_listener);
using vcam_control_fn = int (*)(void *vcam,
                                const char *device_id,
                                int index,
                                char *name,
                                size_t *name_bfsz,
                                char *description,
                                size_t *description_bfsz,
                                char *type,
                                size_t *type_bfsz,
                                int *min,
                                int *max,
                                int *step,
                                int *value,
                                int *default_value,
                                char *menu,
                                size_t *menu_bfsz);
using vcam_set_controls_fn = int (*)(void *vcam,
                                     const char *device_id,
                                     const char **controls,
                                     int *values,
                                     size_t n_controls);
using vcam_picture_fn = int (*)(void *vcam,
                                char *file_path,
                                size_t *buffer_size);
using vcam_set_picture_fn = int (*)(void *vcam, const char *file_path);
using vcam_loglevel_fn = int (*)(void *vcam, int *level);
using vcam_set_loglevel_fn = int (*)(void *vcam, int level);
using vcam_clients_fn = int (*)(void *vcam, uint64_t *pids, size_t npids);
using vcam_client_path_fn = int (*)(void *vcam,
                                    uint64_t pid,
                                    char *path,
                                    size_t *buffer_size);

struct AkFormatStr
{
    AkVideoCaps::PixelFormat format;
    const char *str;

    AkFormatStr(AkVideoCaps::PixelFormat format, const char *str):
        format(format),
        str(str)
    {
    }

    inline static const AkFormatStr *table()
    {
        static const AkFormatStr akVCamDShowFormatStrMap[] = {
            // RGB formats
            {AkVideoCaps::Format_xrgb    , "RGB32"},
            {AkVideoCaps::Format_rgb24   , "RGB24"},

            // RGB formats
            {AkVideoCaps::Format_xbgr    , "BGR32"},
            {AkVideoCaps::Format_bgr24   , "BGR24"},

            // YUV formats
            {AkVideoCaps::Format_uyvy422 , "UYVY"},
            {AkVideoCaps::Format_yuyv422 , "YUY2"},
            {AkVideoCaps::Format_nv12    , "NV12"},
            {AkVideoCaps::Format_nv21    , "NV21"},

            {AkVideoCaps::Format_none    , ""    },
        };

        return akVCamDShowFormatStrMap;
    }

    inline static const AkFormatStr *byFormat(AkVideoCaps::PixelFormat format)
    {
        auto fmt = table();

        for (; fmt->format != AkVideoCaps::Format_none; ++fmt)
            if (fmt->format == format)
                return fmt;

        return fmt;
    }

    inline static const AkFormatStr *byStr(const QString &str)
    {
        auto fmt = table();

        for (; fmt->format != AkVideoCaps::Format_none; ++fmt)
            if (fmt->str == str)
                return fmt;

        return fmt;
    }
};

class VCamCMIOPrivate
{
    public:
        VCamCMIO *self;
        QString m_device;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkVideoCapsList> m_devicesFormats;
        QList<AkVideoCaps::PixelFormat> m_supportedOutputPixelFormats;
        AkVideoCaps::PixelFormat m_defaultOutputPixelFormat;
        QVariantList m_globalControls;
        QVariantMap m_localControls;
        QMutex m_controlsMutex;
        QLibrary m_vcamApi;
        QString m_error;
        AkVideoCaps m_currentCaps;
        AkVideoConverter m_videoConverter;
        QString m_picture;
        QString m_rootMethod;
        bool m_isInitialized {false};
        void *m_vcam {nullptr};

        // Virtual camera API functions

#define DECLARE_VCAM_FN(name) vcam_##name##_fn m_vcam_##name {nullptr}

        DECLARE_VCAM_FN(id);
        DECLARE_VCAM_FN(version);
        DECLARE_VCAM_FN(open);
        DECLARE_VCAM_FN(close);
        DECLARE_VCAM_FN(devices);
        DECLARE_VCAM_FN(add_device);
        DECLARE_VCAM_FN(remove_device);
        DECLARE_VCAM_FN(remove_devices);
        DECLARE_VCAM_FN(description);
        DECLARE_VCAM_FN(set_description);
        DECLARE_VCAM_FN(supported_input_formats);
        DECLARE_VCAM_FN(supported_output_formats);
        DECLARE_VCAM_FN(default_input_format);
        DECLARE_VCAM_FN(default_output_format);
        DECLARE_VCAM_FN(format);
        DECLARE_VCAM_FN(add_format);
        DECLARE_VCAM_FN(remove_format);
        DECLARE_VCAM_FN(remove_formats);
        DECLARE_VCAM_FN(update);
        DECLARE_VCAM_FN(load);
        DECLARE_VCAM_FN(stream_start);
        DECLARE_VCAM_FN(stream_send);
        DECLARE_VCAM_FN(stream_stop);
        DECLARE_VCAM_FN(event);
        DECLARE_VCAM_FN(set_event_listener);
        DECLARE_VCAM_FN(control);
        DECLARE_VCAM_FN(set_controls);
        DECLARE_VCAM_FN(picture);
        DECLARE_VCAM_FN(set_picture);
        DECLARE_VCAM_FN(loglevel);
        DECLARE_VCAM_FN(set_loglevel);
        DECLARE_VCAM_FN(clients);
        DECLARE_VCAM_FN(client_path);

        VCamCMIOPrivate(VCamCMIO *self=nullptr);
        ~VCamCMIOPrivate();

        bool loadVCamApi();
        void setupEventListener();
        void disableEventListener();
        static void handleEvent(void *context, const char *event);
        QStringList availableRootMethods() const;
        QString whereBin(const QString &binary) const;
        void fillSupportedFormats();
        QVariantMap controlStatus(const QVariantList &controls) const;
        QVariantMap mapDiff(const QVariantMap &map1,
                            const QVariantMap &map2) const;
        QVariantList controls(const QString &device);
        bool setControls(const QString &device,
                         const QVariantMap &controls);
        QString readPicturePath() const;
        QString vcamLib() const;
        void updateDevices();
};

VCamCMIO::VCamCMIO(QObject *parent):
    VCam(parent)
{
    this->d = new VCamCMIOPrivate(this);
    QStringList preferredRootMethod {
        "osascript",
    };

    auto availableMethods = this->d->availableRootMethods();

    for (auto &method: preferredRootMethod)
        if (availableMethods.contains(method)) {
            this->d->m_rootMethod = method;

            break;
        }
}

VCamCMIO::~VCamCMIO()
{
    this->uninit();
    delete this->d;
}

QString VCamCMIO::error() const
{
    return this->d->m_error;
}

bool VCamCMIO::isInstalled() const
{
    return !this->d->vcamLib().isEmpty();
}

QString VCamCMIO::installedVersion() const
{
    if (!this->d->m_vcam_version)
        return {};

    int major = 0;
    int minor = 0;
    int patch = 0;
    this->d->m_vcam_version(&major, &minor, &patch);

    return QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
}

QStringList VCamCMIO::webcams() const
{
    return this->d->m_devices;
}

QString VCamCMIO::device() const
{
    return this->d->m_device;
}

QString VCamCMIO::description(const QString &deviceId) const
{
    return this->d->m_descriptions.value(deviceId);
}

QList<AkVideoCaps::PixelFormat> VCamCMIO::supportedOutputPixelFormats() const
{
    return this->d->m_supportedOutputPixelFormats;
}

AkVideoCaps::PixelFormat VCamCMIO::defaultOutputPixelFormat() const
{
    return this->d->m_defaultOutputPixelFormat;
}

AkVideoCapsList VCamCMIO::caps(const QString &deviceId) const
{
    if (!this->d->m_devicesFormats.contains(deviceId))
        return {};

    return this->d->m_devicesFormats[deviceId];
}

AkVideoCaps VCamCMIO::currentCaps() const
{
    return this->d->m_currentCaps;
}

QVariantList VCamCMIO::controls() const
{
    QVariantList controls;

    for (auto &control: this->d->m_globalControls)
        controls << QVariant(control.toList().mid(1));

    return controls;
}

bool VCamCMIO::setControls(const QVariantMap &controls)
{
    this->d->m_controlsMutex.lock();
    auto globalControls = this->d->m_globalControls;
    this->d->m_controlsMutex.unlock();

    for (int i = 0; i < globalControls.count(); i++) {
        auto control = globalControls[i].toList();
        auto controlDescription = control[1].toString();

        if (controls.contains(controlDescription)) {
            control[7] = controls[controlDescription];
            globalControls[i] = control;
        }
    }

    this->d->m_controlsMutex.lock();

    if (this->d->m_globalControls == globalControls) {
        this->d->m_controlsMutex.unlock();

        return false;
    }

    this->d->m_globalControls = globalControls;
    this->d->m_controlsMutex.unlock();

    this->d->setControls(this->d->m_device, controls);
    emit this->controlsChanged(controls);

    return true;
}

QList<quint64> VCamCMIO::clientsPids() const
{
    if (!this->d->m_vcam || !this->d->m_vcam_clients)
        return {};

    auto npids = this->d->m_vcam_clients(this->d->m_vcam, nullptr, 0);

    QVector<uint64_t> pids(npids);
    npids = this->d->m_vcam_clients(this->d->m_vcam, pids.data(), pids.size());

    QList<quint64> clients;

    for (int i = 0; i < npids; ++i)
        clients << pids[i];

    return clients;
}

QString VCamCMIO::clientExe(quint64 pid) const
{
    if (!this->d->m_vcam || !this->d->m_vcam_client_path)
        return {};

    size_t pathSize = 0;

    if (this->d->m_vcam_client_path(this->d->m_vcam, pid, nullptr, &pathSize) < 0)
        return {};

    if (pathSize < 1)
        return {};

    QByteArray path(pathSize, Qt::Uninitialized);

    if (this->d->m_vcam_client_path(this->d->m_vcam, pid, path.data(), &pathSize) < 0)
        return {};

    return QString::fromUtf8(path.constData());
}

QString VCamCMIO::picture() const
{
    return this->d->m_picture;
}

QString VCamCMIO::rootMethod() const
{
    return this->d->m_rootMethod;
}

QStringList VCamCMIO::availableRootMethods() const
{
    return this->d->availableRootMethods();
}

QString VCamCMIO::deviceCreate(const QString &description,
                               const AkVideoCapsList &formats)
{
    this->d->m_error = "";

    // Validate vcam and required functions
    if (!this->d->m_vcam
        || !this->d->m_vcam_load
        || !this->d->m_vcam_devices) {
        this->d->m_error = "Invalid vcam or functions";
        qCritical() << this->d->m_error.toStdString().c_str();

        return {};
    }

    // Get current devices
    size_t devicesBufferSize = 0;
    auto nDevices = this->d->m_vcam_devices(this->d->m_vcam,
                                            nullptr,
                                            &devicesBufferSize);
    QStringList oldDevices;

    if (nDevices > 0 && devicesBufferSize > 0) {
        QByteArray devicesBuffer(devicesBufferSize, Qt::Uninitialized);
        nDevices = this->d->m_vcam_devices(this->d->m_vcam,
                                           devicesBuffer.data(),
                                           &devicesBufferSize);

        if (nDevices >= 0) {
            size_t offset = 0;

            while (offset < devicesBufferSize - 1 && devicesBuffer[offset]) {
                oldDevices << QString::fromUtf8(devicesBuffer.data() + offset);
                offset += strlen(devicesBuffer.data() + offset) + 1;
            }
        }
    }

    // Create config.ini
    QTemporaryDir tempDir;

    if (!tempDir.isValid()) {
        this->d->m_error = "Failed to create temporary directory";
        qCritical() << this->d->m_error.toStdString().c_str();

        return {};
    }

    QSettings settings(tempDir.path() + "/config.ini", QSettings::IniFormat);
    int i = 0;
    int j = 0;

    // Write existing devices
    for (auto it = this->d->m_descriptions.constBegin();
         it != this->d->m_descriptions.constEnd();
         ++it) {
        const auto &deviceId = it.key();
        const auto &deviceDesc = it.value();
        const auto &deviceFormats = this->d->m_devicesFormats.value(deviceId);

        QStringList formatsIndex;

        for (int k = 0; k < deviceFormats.size(); k++)
            formatsIndex << QString("%1").arg(k + j + 1);

        settings.beginGroup("Cameras");
        settings.beginWriteArray("cameras");
        settings.setArrayIndex(i);
        settings.setValue("description", deviceDesc);
        settings.setValue("formats", formatsIndex);
        settings.endArray();
        settings.endGroup();

        settings.beginGroup("Formats");
        settings.beginWriteArray("formats");

        for (const auto &format: deviceFormats) {
            settings.setArrayIndex(j);
            settings.setValue("format", AkFormatStr::byFormat(format.format())->str);
            settings.setValue("width", format.width());
            settings.setValue("height", format.height());
            settings.setValue("fps", format.fps().toString());
            j++;
        }

        settings.endArray();
        settings.endGroup();

        i++;
    }

    // Write new device
    QStringList formatsIndex;

    for (int k = 0; k < formats.size(); k++)
        formatsIndex << QString("%1").arg(k + j + 1);

    settings.beginGroup("Cameras");
    settings.beginWriteArray("cameras");
    settings.setArrayIndex(i);
    settings.setValue("description", description);
    settings.setValue("formats", formatsIndex);
    settings.endArray();
    settings.endGroup();

    settings.beginGroup("Formats");
    settings.beginWriteArray("formats");

    for (const auto &format: formats) {
        settings.setArrayIndex(j);
        settings.setValue("format", AkFormatStr::byFormat(format.format())->str);
        settings.setValue("width", format.width());
        settings.setValue("height", format.height());
        settings.setValue("fps", format.fps().toString());
        j++;
    }

    settings.endArray();
    settings.endGroup();

    // Set default frame if available
    if (!this->d->m_picture.isEmpty())
        settings.setValue("default_frame", this->d->m_picture);

#ifdef QT_DEBUG
    settings.setValue("loglevel", "7");
#endif

    settings.sync();

    // Load config with vcam_load
    auto configPath = tempDir.path() + "/config.ini";
    auto configPathUtf8 = configPath.toUtf8();
    auto result = this->d->m_vcam_load(this->d->m_vcam, configPathUtf8.constData());

    if (result < 0) {
        this->d->m_error = QString("Failed to load config: %1").arg(configPath);
        qCritical() << this->d->m_error.toStdString().c_str();

        return {};
    }

    // Get new devices to find the new device ID
    devicesBufferSize = 0;
    nDevices = this->d->m_vcam_devices(this->d->m_vcam,
                                       nullptr,
                                       &devicesBufferSize);

    if (nDevices <= 0 || devicesBufferSize == 0) {
        this->d->m_error = "No devices found after loading config";
        qCritical() << this->d->m_error.toStdString().c_str();

        return {};
    }

    QByteArray newDevicesBuffer(devicesBufferSize, Qt::Uninitialized);
    nDevices = this->d->m_vcam_devices(this->d->m_vcam,
                                       newDevicesBuffer.data(),
                                       &devicesBufferSize);

    if (nDevices < 0) {
        this->d->m_error = "Error reading devices after loading config";
        qCritical() << this->d->m_error.toStdString().c_str();

        return {};
    }

    QStringList newDevices;
    size_t offset = 0;

    while (offset < devicesBufferSize - 1 && newDevicesBuffer[offset]) {
        newDevices << QString::fromUtf8(newDevicesBuffer.data() + offset);
        offset += strlen(newDevicesBuffer.data() + offset) + 1;
    }

    // Find the new device ID
    QString deviceId;

    for (const auto &id: newDevices)
        if (!oldDevices.contains(id)) {
            deviceId = id;

            break;
        }

    if (deviceId.isEmpty()) {
        this->d->m_error = "No new device created";
        qWarning() << this->d->m_error.toStdString().c_str();
    }

    return deviceId;
}

bool VCamCMIO::deviceEdit(const QString &deviceId,
                          const QString &description,
                          const AkVideoCapsList &formats)
{
    this->d->m_error = "";

    // Validate vcam and vcam_load
    if (!this->d->m_vcam || !this->d->m_vcam_load) {
        this->d->m_error = "Invalid vcam or vcam_load";
        qCritical() << this->d->m_error.toStdString().c_str();

        return false;
    }

    // Create config.ini
    QTemporaryDir tempDir;

    if (!tempDir.isValid()) {
        this->d->m_error = "Failed to create temporary directory";
        qCritical() << this->d->m_error.toStdString().c_str();

        return false;
    }

    QSettings settings(tempDir.path() + "/config.ini", QSettings::IniFormat);
    int i = 0;
    int j = 0;

    // Write all devices, updating the specified device
    for (auto it = this->d->m_descriptions.constBegin();
         it != this->d->m_descriptions.constEnd();
         ++it) {
        const auto &curDeviceId = it.key();
        QString deviceDesc = curDeviceId == deviceId? description: it.value();
        const auto &deviceFormats =
                curDeviceId == deviceId?
                    formats:
                    this->d->m_devicesFormats.value(curDeviceId);
        QStringList formatsIndex;

        for (int k = 0; k < deviceFormats.size(); k++)
            formatsIndex << QString("%1").arg(k + j + 1);

        settings.beginGroup("Cameras");
        settings.beginWriteArray("cameras");
        settings.setArrayIndex(i);
        settings.setValue("description", deviceDesc);
        settings.setValue("formats", formatsIndex);
        settings.endArray();
        settings.endGroup();

        settings.beginGroup("Formats");
        settings.beginWriteArray("formats");

        for (const auto &format : deviceFormats) {
            settings.setArrayIndex(j);
            settings.setValue("format", AkFormatStr::byFormat(format.format())->str);
            settings.setValue("width", format.width());
            settings.setValue("height", format.height());
            settings.setValue("fps", format.fps().toString());
            j++;
        }

        settings.endArray();
        settings.endGroup();

        i++;
    }

    // Set default frame if available
    if (!this->d->m_picture.isEmpty())
        settings.setValue("default_frame", this->d->m_picture);

#ifdef QT_DEBUG
    settings.setValue("loglevel", "7");
#endif

    settings.sync();

    // Load config with vcam_load
    auto configPath = tempDir.path() + "/config.ini";
    auto configPathUtf8 = configPath.toUtf8();
    auto result = this->d->m_vcam_load(this->d->m_vcam,
                                       configPathUtf8.constData());

    if (result < 0) {
        this->d->m_error = QString("Failed to load config: %1").arg(configPath);
        qCritical() << this->d->m_error.toStdString().c_str();

        return false;
    }

    return true;
}

bool VCamCMIO::changeDescription(const QString &deviceId,
                                 const QString &description)
{
    this->d->m_error = "";

    if (!this->d->m_vcam || !this->d->m_vcam_set_description) {
        this->d->m_error = "API library not found";

        return false;
    }

    if (deviceId.isEmpty()) {
        this->d->m_error = "The device ID is not valid";

        return false;
    }

    if (description.isEmpty()) {
        this->d->m_error = "The device ID can't be empty";

        return false;
    }

    int exitCode =
            this->d->m_vcam_set_description(this->d->m_vcam,
                                            deviceId.toStdString().c_str(),
                                            description.toStdString().c_str());

    if (exitCode < 0) {
        this->d->m_error = QString("Execution failed with code %1").arg(exitCode);
        qDebug() << this->d->m_error.toStdString().c_str();
    }

    return exitCode >= 0;
}

bool VCamCMIO::deviceDestroy(const QString &deviceId)
{
    this->d->m_error = "";

    if (!this->d->m_vcam || !this->d->m_vcam_remove_device)
        return false;

    int exitCode =
            this->d->m_vcam_remove_device(this->d->m_vcam,
                                          deviceId.toStdString().c_str());

    if (exitCode < 0) {
        this->d->m_error = QString("Execution failed with code %1").arg(exitCode);
        qDebug() << this->d->m_error.toStdString().c_str();
    }

    return exitCode >= 0;
}

bool VCamCMIO::destroyAllDevices()
{
    this->d->m_error = "";

    if (!this->d->m_vcam || !this->d->m_vcam_remove_devices)
        return false;

    int exitCode = this->d->m_vcam_remove_devices(this->d->m_vcam);

    if (exitCode < 0) {
        this->d->m_error = QString("Execution failed with code %1").arg(exitCode);
        qDebug() << this->d->m_error.toStdString().c_str();
    }

    return exitCode >= 0;
}

bool VCamCMIO::init()
{
    qInfo() << "Initialicing the virtual camera broadcast";

    if (!this->d->m_vcam || !this->d->m_vcam_stream_start) {
        qCritical() << "The virtual camera API library was not set";

        return false;
    }

    this->d->m_isInitialized = false;

    int exitCode =
            this->d->m_vcam_stream_start(this->d->m_vcam,
                                         this->d->m_device.toStdString().c_str());

    if (exitCode < 0) {
        qCritical() << "Virtual camera initialization failed with code" << exitCode;

        return false;
    }

    auto outputCaps = this->d->m_currentCaps;
    outputCaps.setFormat(AkVideoCaps::Format_rgb24);
    this->d->m_videoConverter.setOutputCaps(outputCaps);
    this->d->m_isInitialized = true;

    qInfo() << "The virtual camera is ready to receive frames";

    return true;
}

void VCamCMIO::uninit()
{
    if (!this->d->m_vcam
        || !this->d->m_vcam_stream_stop
        || !this->d->m_isInitialized) {
        return;
    }

    this->d->m_vcam_stream_stop(this->d->m_vcam,
                                this->d->m_device.toStdString().c_str());
}

void VCamCMIO::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;

    if (device.isEmpty()) {
        this->d->m_controlsMutex.lock();
        this->d->m_globalControls.clear();
        this->d->m_controlsMutex.unlock();
    } else {
        this->d->m_controlsMutex.lock();
        this->d->m_globalControls = this->d->controls(device);
        this->d->m_controlsMutex.unlock();
    }

    this->d->m_controlsMutex.lock();
    auto status = this->d->controlStatus(this->d->m_globalControls);
    this->d->m_controlsMutex.unlock();

    emit this->deviceChanged(device);
    emit this->controlsChanged(status);
}

void VCamCMIO::setCurrentCaps(const AkVideoCaps &currentCaps)
{
    if (this->d->m_currentCaps == currentCaps)
        return;

    this->d->m_currentCaps = currentCaps;
    emit this->currentCapsChanged(this->d->m_currentCaps);
}

void VCamCMIO::setPicture(const QString &picture)
{
    if (this->d->m_picture == picture)
        return;

    this->d->m_picture = picture;
    emit this->pictureChanged(this->d->m_picture);
}

void VCamCMIO::setRootMethod(const QString &rootMethod)
{
    if (this->d->m_rootMethod == rootMethod)
        return;

    this->d->m_rootMethod = rootMethod;
    emit this->rootMethodChanged(this->d->m_rootMethod);
}

bool VCamCMIO::applyPicture()
{
    // Validate vcam and vcam_set_picture
    if (!this->d->m_vcam || !this->d->m_vcam_set_picture || this->d->m_picture.isEmpty())
        return false;

    // Convert picture path to C-style string
    auto result =
            this->d->m_vcam_set_picture(this->d->m_vcam,
                                        this->d->m_picture.toStdString().c_str());

    if (result < 0) {
        this->d->m_error = QString("Failed to set picture: %1").arg(this->d->m_picture);
        qDebug() << this->d->m_error.toStdString().c_str();

        return false;
    }

    return true;
}

bool VCamCMIO::write(const AkVideoPacket &frame)
{
    if (!this->d->m_isInitialized)
        return false;

    if (!this->d->m_vcam || !this->d->m_vcam_stream_send)
        return false;

    this->d->m_controlsMutex.lock();
    auto curControls = this->d->controlStatus(this->d->m_globalControls);
    this->d->m_controlsMutex.unlock();

    if (this->d->m_localControls != curControls) {
        auto controls = this->d->mapDiff(this->d->m_localControls,
                                         curControls);
        this->d->setControls(this->d->m_device, controls);
        this->d->m_localControls = curControls;
    }

    this->d->m_videoConverter.begin();
    auto videoPacket = this->d->m_videoConverter.convert(frame);
    this->d->m_videoConverter.end();

    if (!videoPacket)
        return false;

    const char *data[4];
    size_t lineSize[4];

    memset(data, 0 , 4 * sizeof(char *));
    memset(lineSize, 0 , 4 * sizeof(size_t));

    for (int plane = 0; plane < videoPacket.planes(); plane++) {
        data[plane] = reinterpret_cast<const char *>(videoPacket.constPlane(plane));
        lineSize[plane] = videoPacket.lineSize(plane);
    }

    int result =
            this->d->m_vcam_stream_send(this->d->m_vcam,
                                        this->d->m_device.toStdString().c_str(),
                                        AkFormatStr::byFormat(videoPacket.caps().format())->str,
                                        videoPacket.caps().width(),
                                        videoPacket.caps().height(),
                                        data,
                                        lineSize);

    return result >= 0;
}

VCamCMIOPrivate::VCamCMIOPrivate(VCamCMIO *self):
    self(self)
{
    this->loadVCamApi();
    this->setupEventListener();
    this->fillSupportedFormats();
    this->m_picture = this->readPicturePath();
    this->updateDevices();
}

VCamCMIOPrivate::~VCamCMIOPrivate()
{
    this->disableEventListener();

    if (this->m_vcam && this->m_vcam_close)
        this->m_vcam_close(this->m_vcam);
}

#define RESOLVE_VCAM_FN(name) \
    this->m_vcam_##name = reinterpret_cast<vcam_##name##_fn>(this->m_vcamApi.resolve("vcam_" #name))

bool VCamCMIOPrivate::loadVCamApi()
{
    auto vcamLib = this->vcamLib();

    if (vcamLib.isEmpty())
        return false;

    this->m_vcamApi.setFileName(vcamLib);

    if (!this->m_vcamApi.load())
        return false;

    RESOLVE_VCAM_FN(id);
    RESOLVE_VCAM_FN(version);
    RESOLVE_VCAM_FN(open);
    RESOLVE_VCAM_FN(close);
    RESOLVE_VCAM_FN(devices);
    RESOLVE_VCAM_FN(add_device);
    RESOLVE_VCAM_FN(remove_device);
    RESOLVE_VCAM_FN(remove_devices);
    RESOLVE_VCAM_FN(description);
    RESOLVE_VCAM_FN(set_description);
    RESOLVE_VCAM_FN(supported_input_formats);
    RESOLVE_VCAM_FN(supported_output_formats);
    RESOLVE_VCAM_FN(default_input_format);
    RESOLVE_VCAM_FN(default_output_format);
    RESOLVE_VCAM_FN(format);
    RESOLVE_VCAM_FN(add_format);
    RESOLVE_VCAM_FN(remove_format);
    RESOLVE_VCAM_FN(remove_formats);
    RESOLVE_VCAM_FN(update);
    RESOLVE_VCAM_FN(load);
    RESOLVE_VCAM_FN(stream_start);
    RESOLVE_VCAM_FN(stream_send);
    RESOLVE_VCAM_FN(stream_stop);
    RESOLVE_VCAM_FN(event);
    RESOLVE_VCAM_FN(set_event_listener);
    RESOLVE_VCAM_FN(control);
    RESOLVE_VCAM_FN(set_controls);
    RESOLVE_VCAM_FN(picture);
    RESOLVE_VCAM_FN(set_picture);
    RESOLVE_VCAM_FN(loglevel);
    RESOLVE_VCAM_FN(set_loglevel);
    RESOLVE_VCAM_FN(clients);
    RESOLVE_VCAM_FN(client_path);

    if (this->m_vcam_open)
        this->m_vcam = this->m_vcam_open();

    return true;
}

void VCamCMIOPrivate::setupEventListener()
{
    if (!this->m_vcam_set_event_listener)
        return;

    qDebug() << "Start listening to the virtual camera events";

    this->m_vcam_set_event_listener(this->m_vcam,
                                    this,
                                    &VCamCMIOPrivate::handleEvent);
}

void VCamCMIOPrivate::disableEventListener()
{
    if (!this->m_vcam_set_event_listener)
        return;

    qDebug() << "Stop listening to the virtual camera events";

    this->m_vcam_set_event_listener(this->m_vcam,
                                    nullptr,
                                    nullptr);
}

void VCamCMIOPrivate::handleEvent(void *context, const char *event)
{
    auto self = reinterpret_cast<VCamCMIOPrivate *>(context);

    qDebug() << "Event:" << event;

    if (strcmp(event, "DevicesUpdated") == 0)
        self->updateDevices();
    else if (strcmp(event, "PictureUpdated") == 0)
        self->m_picture = self->readPicturePath();
}

QStringList VCamCMIOPrivate::availableRootMethods() const
{
    static const QStringList sus {
        "osascript"
    };

    QStringList methods;

    for (auto &su: sus)
        if (!this->whereBin(su).isEmpty())
            methods << su;

    return methods;
}

QString VCamCMIOPrivate::whereBin(const QString &binary) const
{
#ifdef FAKE_APPLE
    if (binary == "osascript")
        return {"/usr/bin/osascript"};
#endif

    auto paths =
            QProcessEnvironment::systemEnvironment().value("PATH").split(':');

    for (auto &path: paths)
        if (QDir(path).exists(binary))
            return QDir(path).filePath(binary);

    return {};
}

void VCamCMIOPrivate::fillSupportedFormats()
{
    if (!this->m_vcam)
        return;

    size_t bufferSize = 0;

    if (this->m_vcam_supported_output_formats) {
        qDebug() << "Listing supported virtual camera formats";

        auto result =
                this->m_vcam_supported_output_formats(this->m_vcam,
                                                      nullptr,
                                                      &bufferSize);

        if (result > 0) {
            QByteArray formats(bufferSize, Qt::Uninitialized);
            result = this->m_vcam_supported_output_formats(this->m_vcam, formats.data(), &bufferSize);

            if (result > 0) {
                int offset = 0;

                while (offset < formats.size() && formats[offset] != '\x0') {
                    auto formatStr = formats.constData() + offset;
                    auto format = AkFormatStr::byStr(formatStr)->format;

                    if (format != AkVideoCaps::Format_none) {
                        qDebug() << "    Format:" << format;
                        this->m_supportedOutputPixelFormats << format;
                    }

                    offset += strlen(formatStr) + 1;
                }
            }
        }
    }

    if (this->m_vcam_default_output_format) {
        qDebug() << "Reading the default virtual camera format";
        this->m_defaultOutputPixelFormat = AkVideoCaps::Format_none;

        auto result =
                this->m_vcam_default_output_format(this->m_vcam,
                                                   nullptr,
                                                   &bufferSize);

        if (result >= 0) {
            QByteArray format(bufferSize, Qt::Uninitialized);

            result = this->m_vcam_default_output_format(this->m_vcam,
                                                        format.data(),
                                                        &bufferSize);

            if (result > 0) {
                this->m_defaultOutputPixelFormat =
                        AkFormatStr::byStr(format.constData())->format;
                qDebug() << "    Format:" << this->m_defaultOutputPixelFormat;
            }
        }
    }
}

QVariantMap VCamCMIOPrivate::controlStatus(const QVariantList &controls) const
{
    QVariantMap controlStatus;

    for (auto &control: controls) {
        auto params = control.toList();
        auto controlDescription = params[1].toString();
        controlStatus[controlDescription] = params[7];
    }

    return controlStatus;
}

QVariantMap VCamCMIOPrivate::mapDiff(const QVariantMap &map1,
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

QVariantList VCamCMIOPrivate::controls(const QString &device)
{
    // Validate vcam and vcam_control
    if (!this->m_vcam || !this->m_vcam_control)
        return {};

    // Initialize buffer sizes

    size_t nameBfsz = 0;
    size_t descriptionBfsz = 0;
    size_t typeBfsz = 0;
    size_t menuBfsz = 0;

    // Get number of controls

    auto nControls =
            this->m_vcam_control(this->m_vcam,
                                 device.toStdString().c_str(),
                                 0,
                                 nullptr, &nameBfsz,
                                 nullptr, &descriptionBfsz,
                                 nullptr, &typeBfsz,
                                 nullptr,
                                 nullptr,
                                 nullptr,
                                 nullptr,
                                 nullptr,
                                 nullptr, &menuBfsz);

    if (nControls <= 0)
        return {};

    QVariantList controls;

    for (int i = 0; i < nControls; ++i) {
        // First call to get required buffer sizes
        auto result = this->m_vcam_control(this->m_vcam,
                                           device.toStdString().c_str(),
                                           i,
                                           nullptr, &nameBfsz,
                                           nullptr, &descriptionBfsz,
                                           nullptr, &typeBfsz,
                                           nullptr,
                                           nullptr,
                                           nullptr,
                                           nullptr,
                                           nullptr,
                                           nullptr, &menuBfsz);

        if (result < 0)
            continue;

        // Allocate buffers
        QByteArray nameBuffer(nameBfsz, Qt::Uninitialized);
        QByteArray descriptionBuffer(descriptionBfsz, Qt::Uninitialized);
        QByteArray typeBuffer(typeBfsz, Qt::Uninitialized);
        QByteArray menuBuffer(menuBfsz, Qt::Uninitialized);
        int min = 0;
        int max = 0;
        int step = 0;
        int value = 0;
        int defaultValue = 0;

        // Get control data
        result = this->m_vcam_control(this->m_vcam,
                                      device.toStdString().c_str(),
                                      i,
                                      nameBuffer.data(), &nameBfsz,
                                      descriptionBuffer.data(), &descriptionBfsz,
                                      typeBuffer.data(), &typeBfsz,
                                      &min,
                                      &max,
                                      &step,
                                      &value,
                                      &defaultValue,
                                      menuBuffer.data(), &menuBfsz);

        if (result < 0)
            continue;

        // Convert menu to QStringList
        QStringList menuItems;
        size_t offset = 0;

        while (offset < menuBfsz - 1 && menuBuffer[offset]) {
            menuItems << QString::fromUtf8(menuBuffer.constData() + offset);
            offset += strlen(menuBuffer.constData() + offset) + 1;
        }

        // Build control variant list
        QVariantList controlVar {
            QString::fromUtf8(nameBuffer.constData()),
            QString::fromUtf8(descriptionBuffer.constData()),
            QString::fromUtf8(typeBuffer.constData()).toLower(),
            min,
            max,
            step,
            defaultValue,
            value,
            QVariant(menuItems)
        };

        controls << QVariant(controlVar);
    }

    return controls;
}

bool VCamCMIOPrivate::setControls(const QString &device,
                                  const QVariantMap &controls)
{
    // Validate vcam and vcam_set_controls
    if (!this->m_vcam || !this->m_vcam_set_controls)
        return false;

    // Prepare control names and values
    QVector<QByteArray> controlNames;
    QVector<int> controlValues;
    bool result = true;

    for (const auto &control: this->m_globalControls) {
        auto controlParams = control.toList();

        if (controlParams.size() < 2) {
            result = false;

            continue;
        }

        auto description = controlParams[1].toString();

        if (!controls.contains(description)) {
            result = false;

            continue;
        }

        auto name = controlParams[0].toString();
        auto value = controls[description].toInt();
        controlNames << name.toUtf8(); // Store name as QByteArray
        controlValues << value;
    }

    // Set controls if any
    if (!controlNames.isEmpty()) {
        // Prepare pointers for control names
        QVector<const char *> controlNamePtrs(controlNames.size());

        for (const auto &name: controlNames)
            controlNamePtrs << name.constData();

        // Call vcam_set_controls
        auto status = this->m_vcam_set_controls(this->m_vcam,
                                                device.toStdString().c_str(),
                                                controlNamePtrs.data(),
                                                controlValues.data(),
                                                controlNames.size());

        if (status < 0) {
            this->m_error += QString("Failed to set controls for device %1").arg(device);
            result = false;
        }
    }

    return result;
}

QString VCamCMIOPrivate::readPicturePath() const
{
    // Validate vcam and vcam_picture
    if (!this->m_vcam || !this->m_vcam_picture)
        return {};

    qDebug() << "Reading the picture path";

    // Get required buffer size
    size_t bufferSize = 0;
    auto result = this->m_vcam_picture(this->m_vcam, nullptr, &bufferSize);

    if (result < 0 || bufferSize == 0)
        return {};

    // Allocate buffer and get picture path
    QByteArray pictureBuffer(bufferSize, Qt::Uninitialized);
    result = this->m_vcam_picture(this->m_vcam, pictureBuffer.data(), &bufferSize);

    if (result < 0)
        return {};

    // Convert to QString
    auto picturePath = QString::fromUtf8(pictureBuffer.constData()).trimmed();
    qDebug() << "Picture path" << picturePath;

    return picturePath;
}

QString VCamCMIOPrivate::vcamLib() const
{
    qInfo() << "Searching for the virtual camera API lib";

    auto apiLib = QString::fromUtf8(qgetenv("AKVCAM_APILIB"));

    if (!apiLib.isEmpty() && QFileInfo::exists(apiLib)) {
        qInfo() << "API lib found:" << apiLib;

        return apiLib;
    }

    QSettings config;
    config.beginGroup("VirtualCamera");
    apiLib = config.value("apiLib").toString();
    config.endGroup();

    if (!apiLib.isEmpty() && QFileInfo::exists(apiLib)) {
        qInfo() << "API lib found:" << apiLib;

        return apiLib;
    }

#ifdef FAKE_APPLE
    apiLib = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
             + "/AkVirtualCamera.plugin/Contents/Frameworks/libvcam_capi.dll";
#else
    apiLib = "/Library/CoreMediaIO/Plug-Ins/DAL/AkVirtualCamera.plugin/Contents/Frameworks/libvcam_capi.dll";
#endif

    if (!QFileInfo::exists(apiLib)) {
        qWarning() << "API lib not found:" << apiLib;

        return {};
    }

    qInfo() << "API lib found:" << apiLib;

    return apiLib;
}

void VCamCMIOPrivate::updateDevices()
{
    // Validate vcam and required functions
    if (!this->m_vcam
        || !this->m_vcam_devices
        || !this->m_vcam_description
        || !this->m_vcam_format) {
        return;
    }

    qDebug() << "Reading the configured virtual cameras";

    // Get devices
    size_t devicesBufferSize = 0;
    auto nDevices = this->m_vcam_devices(this->m_vcam,
                                         nullptr,
                                         &devicesBufferSize);

    if (nDevices <= 0 || devicesBufferSize == 0) {
        this->m_error = "No virtual cameras found";
        qCritical() << this->m_error;

        return;
    }

    QByteArray devicesBuffer(devicesBufferSize, Qt::Uninitialized);
    nDevices = this->m_vcam_devices(this->m_vcam,
                                    devicesBuffer.data(),
                                    &devicesBufferSize);

    if (nDevices < 0) {
        this->m_error = "Error reading virtual cameras";
        qCritical() << this->m_error;

        return;
    }

    // Parse device IDs
    QStringList devices;
    size_t offset = 0;

    while (offset < devicesBufferSize - 1 && devicesBuffer[offset]) {
        devices << QString::fromUtf8(devicesBuffer.data() + offset);
        offset += strlen(devicesBuffer.data() + offset) + 1;
    }

    // Get descriptions and formats for each device
    QMap<QString, QString> descriptions;
    QMap<QString, AkVideoCapsList> devicesFormats;

    for (const auto &device: devices) {
        // Get description
        size_t descBufferSize = 0;
        auto result = this->m_vcam_description(this->m_vcam,
                                               device.toStdString().c_str(),
                                               nullptr,
                                               &descBufferSize);

        if (result < 0 || descBufferSize == 0)
            continue;

        QByteArray descBuffer(descBufferSize, Qt::Uninitialized);
        result = this->m_vcam_description(this->m_vcam,
                                          device.toStdString().c_str(),
                                          descBuffer.data(),
                                          &descBufferSize);

        if (result < 0)
            continue;

        auto description = QString::fromUtf8(descBuffer.constData()).trimmed();

        if (description.isEmpty())
            continue;

        // Get formats
        AkVideoCapsList deviceCaps;

        for (int index = 0;; ++index) {
            size_t formatBfsz = 0;
            int width = 0;
            int height = 0;
            int fpsNum = 0;
            int fpsDen = 0;

            // First call to get format buffer size
            auto nFormats = this->m_vcam_format(this->m_vcam,
                                                device.toStdString().c_str(),
                                                index,
                                                nullptr,
                                                &formatBfsz,
                                                nullptr,
                                                nullptr,
                                                nullptr,
                                                nullptr);

            if (nFormats < 0 || formatBfsz == 0)
                break;

            // Allocate buffer and get format data
            QByteArray formatBuffer(formatBfsz, Qt::Uninitialized);
            result = this->m_vcam_format(this->m_vcam,
                                         device.toStdString().c_str(),
                                         index,
                                         formatBuffer.data(),
                                         &formatBfsz,
                                         &width,
                                         &height,
                                         &fpsNum,
                                         &fpsDen);

            if (result < 0)
                break;

            AkVideoCaps caps(AkFormatStr::byStr(formatBuffer.constData())->format,
                             width,
                             height,
                             AkFrac(fpsNum, fpsDen));

            if (caps)
                deviceCaps << caps;
        }

        if (!deviceCaps.isEmpty()) {
            devicesFormats[device] = deviceCaps;
            descriptions[device] = description;
        }
    }

    // Clear if no valid devices found
    if (devicesFormats.isEmpty()) {
        devices.clear();
        descriptions.clear();
    }

    qDebug() << "Virtual cameras found:";

    for (auto it = descriptions.constBegin(); it != descriptions.constEnd(); ++it)
        qDebug() << "    " << it.value() << '(' << it.key() << ')';

    // Update member variables
    this->m_descriptions = descriptions;
    this->m_devicesFormats = devicesFormats;

    if (this->m_devices != devices) {
        this->m_devices = devices;
        emit self->webcamsChanged(this->m_devices);
    }
}

#include "moc_vcamcmio.cpp"

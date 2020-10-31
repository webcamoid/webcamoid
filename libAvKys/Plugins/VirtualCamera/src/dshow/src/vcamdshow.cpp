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

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSharedPointer>
#include <fstream>
#include <dshow.h>
#include <dbt.h>
#include <dvdmedia.h>
#include <akfrac.h>

#include "vcamdshow.h"

#define TIME_BASE 1.0e7
#define DSHOW_PLUGIN_NAME "AkVirtualCamera"
#define DSHOW_PLUGIN_MANAGER_NAME "AkVCamManager"
#define DSHOW_PLUGIN_ASSISTANT_NAME "AkVCamAssistant"

struct DShowAkFormat
{
    GUID guid;
    AkVideoCaps::PixelFormat ak;
    QString str;
};

using DShowAkFormatMap = QVector<DShowAkFormat>;
using MonikerPtr = QSharedPointer<IMoniker>;
using BaseFilterPtr = QSharedPointer<IBaseFilter>;
using PropertyBagPtr = QSharedPointer<IPropertyBag>;
using PinPtr = QSharedPointer<IPin>;

class VCamDShowPrivate
{
    public:
        VCamDShow *self;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, AkVideoCapsList> m_devicesFormats;
        FILE *m_managerProc {nullptr};
        AkVideoCaps m_curFormat;
        QString m_error;

        explicit VCamDShowPrivate(VCamDShow *self);
        ~VCamDShowPrivate();

        inline const DShowAkFormatMap &dshowAkFormatMap() const;
        inline const DShowAkFormat &formatByGUID(const GUID &guid) const;
        inline const DShowAkFormat &formatByAk(AkVideoCaps::PixelFormat ak) const;
        QVector<MonikerPtr> listCameras() const;
        BaseFilterPtr filter(IMoniker *moniker) const;
        PropertyBagPtr propertyBag(IMoniker *moniker) const;
        QString cameraPath(const MonikerPtr &moniker) const;
        QString cameraPath(IPropertyBag *propertyBag) const;
        QString cameraDescription(const MonikerPtr &moniker) const;
        QString cameraDescription(IPropertyBag *propertyBag) const;
        QVector<PinPtr> enumPins(IBaseFilter *baseFilter) const;
        AkVideoCapsList enumVideoFormats(IPin *pin) const;
        AkVideoCapsList caps(const MonikerPtr &moniker) const;
        AkVideoCaps formatFromMediaType(const AM_MEDIA_TYPE *mediaType) const;
        void deleteMediaType(AM_MEDIA_TYPE **mediaType) const;
        bool isSubTypeSupported(const GUID &subType) const;
        QString servicePath(const QString &serviceName) const;
        QString manager(const QString &arch={}) const;
        QStringList devices() const;
        void updateDevices();
        bool sudo(const QString &command, const QStringList &argumments);
};

VCamDShow::VCamDShow(QObject *parent):
    VCam(parent),
    QAbstractNativeEventFilter()
{
    this->d = new VCamDShowPrivate(this);
    qApp->installNativeEventFilter(this);
    this->d->updateDevices();
}

VCamDShow::~VCamDShow()
{
    qApp->removeNativeEventFilter(this);
    delete this->d;
}

QString VCamDShow::error() const
{
    return this->d->m_error;
}

bool VCamDShow::isInstalled() const
{
    return !this->d->manager().isEmpty();
}

QStringList VCamDShow::webcams() const
{
    return this->d->m_devices;
}

QString VCamDShow::description(const QString &deviceId) const
{
    return this->d->m_descriptions.value(deviceId);
}

QList<AkVideoCaps::PixelFormat> VCamDShow::supportedOutputPixelFormats() const
{
    return {
        AkVideoCaps::Format_0rgb,
        AkVideoCaps::Format_rgb24,
        AkVideoCaps::Format_rgb565le,
        AkVideoCaps::Format_rgb555le,
        AkVideoCaps::Format_uyvy422,
        AkVideoCaps::Format_yuyv422,
        AkVideoCaps::Format_nv12,
    };
}

AkVideoCaps::PixelFormat VCamDShow::defaultOutputPixelFormat() const
{
    return AkVideoCaps::Format_yuyv422;
}

AkVideoCapsList VCamDShow::caps(const QString &deviceId) const
{
    if (!this->d->m_devicesFormats.contains(deviceId))
        return {};

    return this->d->m_devicesFormats[deviceId];
}

QVariantList VCamDShow::controls() const
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager,
               {"-p",
                "controls",
                this->m_device});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return {};
    }

    auto availableControls =
            QString(proc.readAllStandardOutput()).trimmed().split(QRegExp("\\s+"));
    QVariantList controls;

    for (auto &control: availableControls) {
        proc.start(manager,
                   {"-p",
                    "get-control",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto value = proc.readAllStandardOutput().trimmed().toInt();

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-c",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto description = QString(proc.readAllStandardOutput().trimmed());

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-t",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto type = QString(proc.readAllStandardOutput().trimmed().toLower());

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-m",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto minimum = proc.readAllStandardOutput().trimmed().toInt();

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-M",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto maximum = proc.readAllStandardOutput().trimmed().toInt();

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-s",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto step = proc.readAllStandardOutput().trimmed().toInt();

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-d",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto defaultValue = proc.readAllStandardOutput().trimmed().toInt();

        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-l",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode())
            continue;

        auto menu =
                QString(proc.readAllStandardOutput()).trimmed().split(QRegExp("\\s+"));

        QVariantList controlVar {
            description,
            type,
            minimum,
            maximum,
            step,
            defaultValue,
            value,
            menu
        };
        controls << QVariant(controlVar);
    }

    return controls;
}

bool VCamDShow::setControls(const QVariantMap &controls)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager, {"-p", "controls", this->m_device});
    proc.waitForFinished();

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return false;
    }

    auto availableControls =
            QString(proc.readAllStandardOutput()).trimmed().split(QRegExp("\\s+"));
    auto result = true;
    QStringList args;

    for (auto &control: availableControls) {
        proc.start(manager,
                   {"-p",
                    "get-control",
                    "-c",
                    this->m_device,
                    control});
        proc.waitForFinished();

        if (proc.exitCode()) {
            result = false;

            continue;
        }

        auto description = QString(proc.readAllStandardOutput().trimmed());

        if (!controls.contains(description)) {
            result = false;

            continue;
        }

        args << QString("%1=%2").arg(control).arg(controls[description].toInt());
    }

    proc.start(manager, QStringList {"set-controls", this->m_device} + args);
    proc.waitForFinished();

    if (proc.exitCode())
        result = false;

    return result;
}

QList<quint64> VCamDShow::clientsPids() const
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"-p", "clients"});
    proc.waitForFinished();

    if (proc.exitCode())
        return {};

    QList<quint64> pids;

    for (auto &line: proc.readAllStandardOutput().split('\n')) {
        auto pidExe = line.simplified().split(' ');
        auto pid = pidExe.value(0).toInt();

        if (pid != getpid())
            pids << quint64(pid);
    }

    return pids;
}

QString VCamDShow::clientExe(uint64_t pid) const
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"-p", "clients"});
    proc.waitForFinished();

    if (proc.exitCode())
        return {};

    for (auto &line: proc.readAllStandardOutput().split('\n')) {
        auto pidExe = line.simplified().split(' ');

        if (pidExe.value(0).toULongLong() == pid)
            return pidExe.value(1);
    }

    return {};
}

QString VCamDShow::deviceCreate(const QString &description,
                                const AkVideoCapsList &formats)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"-p", "add-device", description});
    proc.waitForFinished();
    auto result = proc.exitCode();
    QString deviceId;

    if (!result) {
        deviceId = QString::fromUtf8(proc.readAllStandardOutput());
        deviceId = deviceId.trimmed();
    }

    if (!result) {
        AkVideoCapsList outputformats;

        for (auto &format: formats) {
            auto width = format.width();
            auto height = format.height();
            auto fps = format.fps();
            auto ot = std::find(outputformats.begin(),
                                outputformats.end(),
                                format);
            auto pixFormat = this->d->formatByAk(format.format()).str;

            if (ot == outputformats.end() && !pixFormat.isEmpty()) {
                proc.start(manager,
                           {"add-format",
                            deviceId,
                            pixFormat,
                            QString::number(width),
                            QString::number(height),
                            fps.toString()});
                proc.waitForFinished();

                if (proc.exitCode()) {
                    result = proc.exitCode();

                    break;
                }

                outputformats.push_back(format);
            }
        }
    }

    if (!result) {
        proc.start(manager, {"update"});
        proc.waitForFinished();
        result = proc.exitCode();
    }

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return {};
    }

    return deviceId;
}

bool VCamDShow::deviceEdit(const QString &deviceId,
                           const QString &description,
                           const AkVideoCapsList &formats)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"set-description", deviceId, description});
    proc.waitForFinished();
    auto result = proc.exitCode();

    if (!result) {
        proc.start(manager, {"remove-formats", deviceId});
        proc.waitForFinished();
        result = proc.exitCode();
    }

    if (!result) {
        AkVideoCapsList outputformats;

        for (auto &format: formats) {
            auto width = format.width();
            auto height = format.height();
            auto fps = format.fps();
            auto ot = std::find(outputformats.begin(),
                                outputformats.end(),
                                format);
            auto pixFormat = this->d->formatByAk(format.format()).str;

            if (ot == outputformats.end() && !pixFormat.isEmpty()) {
                proc.start(manager,
                           {"add-format",
                            deviceId,
                            pixFormat,
                            QString::number(width),
                            QString::number(height),
                            fps.toString()});
                proc.waitForFinished();

                if (proc.exitCode()) {
                    result = proc.exitCode();

                    break;
                }

                outputformats << format;
            }
        }
    }

    if (!result) {
        proc.start(manager, {"update"});
        proc.waitForFinished();
        result = proc.exitCode();
    }

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return false;
    }

    return true;
}

bool VCamDShow::changeDescription(const QString &deviceId,
                                 const QString &description)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager, {"set-description", deviceId, description});
    proc.waitForFinished();

    if (!proc.exitCode()) {
        proc.start(manager, {"update"});
        proc.waitForFinished();
    }

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return false;
    }

    return true;
}

bool VCamDShow::deviceDestroy(const QString &deviceId)
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager, {"remove-device", deviceId});
    proc.waitForFinished();

    if (!proc.exitCode()) {
        proc.start(manager, {"update"});
        proc.waitForFinished();
    }

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return false;
    }

    return true;
}

bool VCamDShow::destroyAllDevices()
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QProcess proc;
    proc.start(manager, {"remove-devices"});
    proc.waitForFinished();

    if (!proc.exitCode()) {
        proc.start(manager, {"update"});
        proc.waitForFinished();
    }

    if (proc.exitCode()) {
        auto errorMsg = proc.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg;
            this->d->m_error += QString(errorMsg);
        }

        return false;
    }

    return true;
}

bool VCamDShow::nativeEventFilter(const QByteArray &eventType,
                                  void *message,
                                  long *result)
{
    Q_UNUSED(eventType)

    if (!message)
        return false;

    auto msg = reinterpret_cast<MSG *>(message);

    if (msg->message == WM_DEVICECHANGE) {
        switch (msg->wParam) {
        case DBT_DEVICEARRIVAL:
        case DBT_DEVICEREMOVECOMPLETE:
        case DBT_DEVNODES_CHANGED: {
            this->d->updateDevices();

            if (result)
                *result = TRUE;

            return true;
        }
        default:
            break;
        }
    }

    return false;
}

bool VCamDShow::init()
{
    auto manager = this->d->manager();

    if (manager.isEmpty())
        return false;

    QString params;
    QTextStream paramsStream(&params);
    paramsStream << manager
                 << " "
                 << "stream"
                 << " "
                 << this->m_device
                 << " "
                 << this->d->formatByAk(this->m_currentCaps.format()).str
                 << " "
                 << this->m_currentCaps.width()
                 << " "
                 << this->m_currentCaps.height();
    this->d->m_managerProc = popen(params.toStdString().c_str(), "w");

    return this->d->m_managerProc != nullptr;
}

void VCamDShow::uninit()
{
    if (this->d->m_managerProc) {
        pclose(this->d->m_managerProc);
        this->d->m_managerProc = nullptr;
    }

    this->d->m_curFormat.clear();
}

bool VCamDShow::write(const AkVideoPacket &frame)
{
    if (!this->d->m_managerProc)
        return false;

    auto scaled = frame.scaled(this->d->m_curFormat.width(),
                               this->d->m_curFormat.height())
                        .convert(this->d->m_curFormat.format());

    if (!scaled)
        return false;

    return fwrite(scaled.buffer().data(),
                  size_t(scaled.buffer().size()),
                  1,
                  this->d->m_managerProc) > 0;
}

VCamDShowPrivate::VCamDShowPrivate(VCamDShow *self):
    self(self)
{
}

VCamDShowPrivate::~VCamDShowPrivate()
{
}

const DShowAkFormatMap &VCamDShowPrivate::dshowAkFormatMap() const
{
    static const DShowAkFormatMap formatMap {
        {{}                 , AkVideoCaps::Format_none    , ""     },

        // RGB formats
        {MEDIASUBTYPE_RGB32 , AkVideoCaps::Format_0rgb    , "RGB32"},
        {MEDIASUBTYPE_RGB24 , AkVideoCaps::Format_rgb24   , "RGB24"},
        {MEDIASUBTYPE_RGB565, AkVideoCaps::Format_rgb565le, "RGB16"},
        {MEDIASUBTYPE_RGB555, AkVideoCaps::Format_rgb555le, "RGB15"},

        // YUV formats
        {MEDIASUBTYPE_UYVY  , AkVideoCaps::Format_uyvy422 , "UYVY"},
        {MEDIASUBTYPE_YUY2  , AkVideoCaps::Format_yuyv422 , "YUY2"},
        {MEDIASUBTYPE_NV12  , AkVideoCaps::Format_yuyv422 , "NV12"}
    };

    return formatMap;
}

const DShowAkFormat &VCamDShowPrivate::formatByGUID(const GUID &guid) const
{
    auto &formatMap = this->dshowAkFormatMap();

    for (auto &format: formatMap)
        if (IsEqualGUID(format.guid, guid))
            return format;

    return formatMap.first();
}

const DShowAkFormat &VCamDShowPrivate::formatByAk(AkVideoCaps::PixelFormat ak) const
{
    auto &formatMap = this->dshowAkFormatMap();

    for (auto &format: formatMap)
        if (format.ak == ak)
            return format;

    return formatMap.first();
}

QVector<MonikerPtr> VCamDShowPrivate::listCameras() const
{
    QVector<MonikerPtr> cameras;

    // Create the System Device Enumerator.
    ICreateDevEnum *deviceEnumerator = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                                  nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum,
                                  reinterpret_cast<void **>(&deviceEnumerator));

    if (FAILED(hr))
        return cameras;

    // Create an enumerator for the category.
    IEnumMoniker *enumMoniker = nullptr;

    if (deviceEnumerator->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                                &enumMoniker,
                                                0) == S_OK) {
        enumMoniker->Reset();
        IMoniker *moniker = nullptr;

        while (enumMoniker->Next(1, &moniker, nullptr) == S_OK)
            cameras.push_back(MonikerPtr(moniker, [](IMoniker *moniker) {
                moniker->Release();
            }));

        enumMoniker->Release();
    }

    deviceEnumerator->Release();

    return cameras;
}

BaseFilterPtr VCamDShowPrivate::filter(IMoniker *moniker) const
{
    if (!moniker)
        return {};

    IBaseFilter *baseFilter = nullptr;

    if (FAILED(moniker->BindToObject(nullptr,
                                     nullptr,
                                     IID_IBaseFilter,
                                     reinterpret_cast<void **>(&baseFilter)))) {
        return {};
    }

    return BaseFilterPtr(baseFilter, [] (IBaseFilter *baseFilter) {
        baseFilter->Release();
    });
}

PropertyBagPtr VCamDShowPrivate::propertyBag(IMoniker *moniker) const
{
    if (!moniker)
        return {};

    IPropertyBag *propertyBag = nullptr;

    if (FAILED(moniker->BindToStorage(nullptr,
                                      nullptr,
                                      IID_IPropertyBag,
                                      reinterpret_cast<void **>(&propertyBag)))) {
        return {};
    }

    return PropertyBagPtr(propertyBag, [] (IPropertyBag *propertyBag) {
        propertyBag->Release();
    });
}

QString VCamDShowPrivate::cameraPath(const MonikerPtr &moniker) const
{
    auto propertyBag = this->propertyBag(moniker.get());

    return this->cameraPath(propertyBag.get());
}

QString VCamDShowPrivate::cameraPath(IPropertyBag *propertyBag) const
{
    VARIANT var;
    VariantInit(&var);

    if (FAILED(propertyBag->Read(L"DevicePath", &var, nullptr)))
        return {};

    auto devicePath = QString::fromStdWString(var.bstrVal);
    VariantClear(&var);

    return devicePath;
}

QString VCamDShowPrivate::cameraDescription(const MonikerPtr &moniker) const
{
    auto propertyBag = this->propertyBag(moniker.get());

    return this->cameraDescription(propertyBag.get());
}

QString VCamDShowPrivate::cameraDescription(IPropertyBag *propertyBag) const
{
    VARIANT var;
    VariantInit(&var);

    if (FAILED(propertyBag->Read(L"Description", &var, nullptr)))
        if (FAILED(propertyBag->Read(L"FriendlyName", &var, nullptr)))
            return {};

    auto description = QString::fromStdWString(var.bstrVal);
    VariantClear(&var);

    return description;
}

QVector<PinPtr> VCamDShowPrivate::enumPins(IBaseFilter *baseFilter) const
{
    QVector<PinPtr> pins;
    IEnumPins *enumPins = nullptr;

    if (SUCCEEDED(baseFilter->EnumPins(&enumPins))) {
        enumPins->Reset();
        IPin *pin = nullptr;

        while (enumPins->Next(1, &pin, nullptr) == S_OK) {
            PIN_DIRECTION direction = PINDIR_INPUT;

            if (SUCCEEDED(pin->QueryDirection(&direction))
                && direction == PINDIR_OUTPUT) {
                pins << PinPtr(pin, [] (IPin *pin) {
                    pin->Release();
                });

                continue;
            }

            pin->Release();
        }

        enumPins->Release();
    }

    return pins;
}

AkVideoCapsList VCamDShowPrivate::enumVideoFormats(IPin *pin) const
{
    AkVideoCapsList mediaTypes;
    IEnumMediaTypes *pEnum = nullptr;

    if (FAILED(pin->EnumMediaTypes(&pEnum)))
        return mediaTypes;

    pEnum->Reset();
    AM_MEDIA_TYPE *mediaType = nullptr;

    while (pEnum->Next(1, &mediaType, nullptr) == S_OK) {
        auto format = this->formatFromMediaType(mediaType);
        this->deleteMediaType(&mediaType);

        if (!format)
            mediaTypes << format;
    }

    pEnum->Release();

    return mediaTypes;
}

AkVideoCapsList VCamDShowPrivate::caps(const MonikerPtr &moniker) const
{
    auto baseFilter = filter(moniker.get());
    AkVideoCapsList caps;

    for (auto &pin: this->enumPins(baseFilter.get()))
        caps << this->enumVideoFormats(pin.get());

    return caps;
}

AkVideoCaps VCamDShowPrivate::formatFromMediaType(const AM_MEDIA_TYPE *mediaType) const
{
    if (!mediaType)
        return {};

    if (!IsEqualGUID(mediaType->majortype, MEDIATYPE_Video))
        return {};

    if (!this->isSubTypeSupported(mediaType->subtype))
        return {};

    if (!mediaType->pbFormat)
        return {};

    if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo)) {
        auto format = reinterpret_cast<VIDEOINFOHEADER *>(mediaType->pbFormat);
        AkFrac fps(uint32_t(TIME_BASE),
                   uint32_t(format->AvgTimePerFrame));

        return AkVideoCaps(this->formatByGUID(mediaType->subtype).ak,
                           format->bmiHeader.biWidth,
                           std::abs(format->bmiHeader.biHeight),
                           fps);
    } else if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo2)) {
        auto format = reinterpret_cast<VIDEOINFOHEADER2 *>(mediaType->pbFormat);
        AkFrac fps(uint32_t(TIME_BASE),
                   uint32_t(format->AvgTimePerFrame));

        return AkVideoCaps(this->formatByGUID(mediaType->subtype).ak,
                           format->bmiHeader.biWidth,
                           std::abs(format->bmiHeader.biHeight),
                           fps);
    }

    return {};
}

void VCamDShowPrivate::deleteMediaType(AM_MEDIA_TYPE **mediaType) const
{
    if (!mediaType || !*mediaType)
        return;

    auto format = (*mediaType)->pbFormat;

    if (format && (*mediaType)->cbFormat)
        CoTaskMemFree(format);

    CoTaskMemFree(*mediaType);
    *mediaType = nullptr;
}

bool VCamDShowPrivate::isSubTypeSupported(const GUID &subType) const
{
    return this->formatByGUID(subType).ak != AkVideoCaps::Format_none;
}

QString VCamDShowPrivate::servicePath(const QString &serviceName) const
{
    QString path;
    auto manager = OpenSCManager(nullptr, nullptr, GENERIC_READ);

    if (manager) {
        auto service = OpenServiceA(manager,
                                    serviceName.toStdString().c_str(),
                                    SERVICE_QUERY_CONFIG);

        if (service) {
            DWORD bytesNeeded = 0;
            QueryServiceConfig(service, nullptr, 0, &bytesNeeded);
            auto bufSize = bytesNeeded;
            auto serviceConfig =
                    reinterpret_cast<LPQUERY_SERVICE_CONFIG>(LocalAlloc(LMEM_FIXED,
                                                                        bufSize));
            if (serviceConfig) {
                if (QueryServiceConfig(service,
                                       serviceConfig,
                                       bufSize,
                                       &bytesNeeded)) {
                    path = QString::fromStdWString(serviceConfig->lpBinaryPathName);
                }

                LocalFree(serviceConfig);
            }

            CloseServiceHandle(service);
        }

        CloseServiceHandle(manager);
    }

    return path;
}

QString VCamDShowPrivate::manager(const QString &arch) const
{
    auto assistant = this->servicePath(DSHOW_PLUGIN_ASSISTANT_NAME);

    if (assistant.isEmpty())
        return {};

    auto pluginDir = QFileInfo(assistant).absoluteDir();
    pluginDir.cdUp();

    if (!arch.isEmpty()) {
        auto manager = pluginDir.absoluteFilePath(arch + "\\" DSHOW_PLUGIN_MANAGER_NAME ".exe");

        return QFileInfo::exists(manager)? manager: QString();
    }

#ifdef DSHOW_PLUGIN_ARCH_X64
    QStringList archs {"x64", "x86"};
#else
    QStringList archs {"x86", "x64"};
#endif

    QString driverPath;

    for (auto &arch: archs) {
        auto manager = pluginDir.absoluteFilePath(arch + "\\" DSHOW_PLUGIN_MANAGER_NAME ".exe");

        if (QFileInfo::exists(manager))
            return manager;
    }

    return {};
}

QStringList VCamDShowPrivate::devices() const
{
    auto manager = this->manager();

    if (manager.isEmpty())
        return {};

    QProcess proc;
    proc.start(manager, {"-p", "devices"});
    proc.waitForFinished();

    if (proc.exitCode() != 0)
        return {};

    QStringList devices;

    for (auto &line: proc.readAllStandardOutput().split('\n'))
        devices << line.trimmed();

    return devices;
}

void VCamDShowPrivate::updateDevices()
{
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesFormats) devicesFormats;

    auto virtualDevices = this->devices();

    for (auto camera: this->listCameras()) {
        auto deviceId = this->cameraPath(camera);
        auto it = std::find(virtualDevices.begin(),
                            virtualDevices.end(),
                            deviceId);

        if (it == virtualDevices.end())
            continue;

        auto formatsList = this->caps(camera);

        if (!formatsList.isEmpty()) {
            devices << deviceId;
            descriptions[deviceId] = this->cameraDescription(camera);
            devicesFormats[deviceId] = formatsList;
        }
    }

    this->m_descriptions = descriptions;
    this->m_devicesFormats = devicesFormats;
    this->m_devices = devices;
}

bool VCamDShowPrivate::sudo(const QString &command, const QStringList &argumments)
{
    QProcess su;
    su.start(self->rootMethod(),
             QStringList {command} << argumments);
    su.waitForFinished(-1);

    if (su.exitCode()) {
        auto outMsg = su.readAllStandardOutput();
        this->m_error.clear();

        if (!outMsg.isEmpty()) {
            qDebug() << outMsg.toStdString().c_str();
            this->m_error += QString(outMsg) + " ";
        }

        auto errorMsg = su.readAllStandardError();

        if (!errorMsg.isEmpty()) {
            qDebug() << errorMsg.toStdString().c_str();
            this->m_error += QString(outMsg);
        }

        return false;
    }

    return true;
}

#include "moc_vcamdshow.cpp"

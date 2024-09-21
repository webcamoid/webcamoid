/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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
#include <QBitArray>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <libusb.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/usb/video.h>
#include <linux/uvcvideo.h>
#include <linux/videodev2.h>

#include "uvcextendedcontrols.h"
#include "guid.h"
#include "ioctldefs.h"

#define CS_INTERFACE 36

struct UvcControlTypes
{
    const char *name;
    int uvcType;
    v4l2_ctrl_type v4l2Type;

    static inline const UvcControlTypes *byName(const QString &name);
};

static const UvcControlTypes uvcControlTypesTable[] {
    {"signed"  , UVC_CTRL_DATA_TYPE_SIGNED  , V4L2_CTRL_TYPE_INTEGER},
    {"unsigned", UVC_CTRL_DATA_TYPE_UNSIGNED, V4L2_CTRL_TYPE_INTEGER},
    {"boolean" , UVC_CTRL_DATA_TYPE_BOOLEAN , V4L2_CTRL_TYPE_BOOLEAN},
    {"menu"    , UVC_CTRL_DATA_TYPE_UNSIGNED, V4L2_CTRL_TYPE_MENU   },
    {""        , UVC_CTRL_DATA_TYPE_RAW     , v4l2_ctrl_type(0)     },
};

const UvcControlTypes *UvcControlTypes::byName(const QString &name)
{
    auto type = uvcControlTypesTable;

    for (; type->v4l2Type != v4l2_ctrl_type(0); type++)
        if (type->name == name)
            return type;

    return type;
}

struct UvcMenuOption
{
    QString name;
    QVariant value;

    UvcMenuOption()
    {

    }

    UvcMenuOption(const QString &name,
                  const QVariant &value):
        name(name),
        value(value)
    {

    }

    UvcMenuOption(const UvcMenuOption &other):
        name(other.name),
        value(other.value)
    {

    }
};

struct UvcControl
{
    QString name;
    quint8 selector {0};
    quint8 size {0};
    quint8 readSize {0};
    quint8 offset {0};
    int uvcType {0};
    v4l2_ctrl_type v4l2Type {v4l2_ctrl_type(0)};
    QVector<UvcMenuOption> menu;

    UvcControl()
    {

    }

    UvcControl(const QString &name,
               quint8 selector,
               quint8 size,
               quint8 readSize,
               quint8 offset,
               int uvcType,
               v4l2_ctrl_type v4l2Type,
               const QVector<UvcMenuOption> &menu):
        name(name),
        selector(selector),
        size(size),
        readSize(readSize),
        offset(offset),
        uvcType(uvcType),
        v4l2Type(v4l2Type),
        menu(menu)
    {

    }

    UvcControl(const UvcControl &other):
        name(other.name),
        selector(other.selector),
        size(other.size),
        readSize(other.readSize),
        offset(other.offset),
        uvcType(other.uvcType),
        v4l2Type(other.v4l2Type),
        menu(other.menu)
    {

    }
};

struct UvcInterface
{
    Guid guid;
    QVector<UvcControl> controls;

    UvcInterface()
    {

    }

    UvcInterface(const Guid &guid, const QVector<UvcControl> &controls):
        guid(guid),
        controls(controls)
    {

    }

    UvcInterface(const UvcInterface &other):
        guid(other.guid),
        controls(other.controls)
    {

    }
};

struct UvcProduct
{
    QVector<quint16> ids;
    QVector<UvcInterface> interfaces;

    UvcProduct()
    {

    }

    UvcProduct(const QVector<quint16> &ids, const QVector<UvcInterface> &interfaces):
        ids(ids),
        interfaces(interfaces)
    {

    }

    UvcProduct(const UvcProduct &other):
        ids(other.ids),
        interfaces(other.interfaces)
    {

    }
};

struct UvcVendor
{
    quint16 id {0};
    QVector<UvcProduct> products;

    UvcVendor()
    {

    }

    UvcVendor(quint16 id, const QVector<UvcProduct> &products={}):
        id(id),
        products(products)
    {

    }

    UvcVendor(const UvcVendor &other):
        id(other.id),
        products(other.products)
    {

    }
};

struct UvcControlExt
{
    UvcControl control;
    quint8 unitId {0};

    UvcControlExt()
    {

    }

    UvcControlExt(const UvcControl &control, quint16 unitId):
        control(control),
        unitId(unitId)
    {

    }

    UvcControlExt(const UvcControlExt &other):
        control(other.control),
        unitId(other.unitId)
    {

    }
};

class UvcExtendedControlsPrivate
{
    public:
        QVector<UvcVendor> m_vendors;
        QMap<Guid, quint8> m_extensions;
        QVector<UvcControlExt> m_uvcControls;

        void loadDefinitions(const QString &filePath);
        void loadVendors(const QStringList &searchDirectories={});
        QMap<Guid, quint8> readExtensions(quint16 vendorId,
                                          quint16 productId,
                                          quint8 bus,
                                          quint8 port,
                                          quint8 address) const;
        QMap<Guid, quint8> readExtensions(const QString &devicePath) const;
        QMap<Guid, quint8> readExtensions(int fd) const;
        void loadControls(const QString &devicePath);
        void loadControls(int fd);
        quint16 controlDataSize(int fd, quint8 unitId, quint8 selector) const;
        int queryControl(int fd,
                         quint8 unitId,
                         quint8 selector,
                         quint8 query,
                         void *data,
                         quint16 dataSize=0) const;
        inline quint32 readValueU(const UvcControl &control,
                                  const QBitArray &value) const;
        inline qint32 readValueS(const UvcControl &control,
                                 const QBitArray &value) const;
        QVariantList readControlSigned(int fd, quint8 unitId, const UvcControl &control) const;
        QVariantList readControlUnsigned(int fd, quint8 unitId, const UvcControl &control) const;
        QVariantList readControlBoolean(int fd, quint8 unitId, const UvcControl &control) const;
        QVariantList readControlMenu(int fd, quint8 unitId, const UvcControl &control) const;
        inline QByteArray writeValueU(const UvcControl &control, const QBitArray &curValue, quint32 value) const;
        inline QByteArray writeValueS(const UvcControl &control, const QBitArray &curValue, qint32 value) const;
        bool writeControlSigned(int fd, quint8 unitId, const UvcControl &control, qint32 value) const;
        bool writeControlUnsigned(int fd, quint8 unitId, const UvcControl &control, quint32 value) const;
};

UvcExtendedControls::UvcExtendedControls(QObject *parent):
    QObject(parent)
{
    this->d = new UvcExtendedControlsPrivate;
    this->d->loadVendors();
}

UvcExtendedControls::UvcExtendedControls(const QString &devicePath):
    QObject()
{
    this->d = new UvcExtendedControlsPrivate;
    this->d->loadVendors();
    this->load(devicePath);
}

UvcExtendedControls::UvcExtendedControls(int fd):
    QObject()
{
    this->d = new UvcExtendedControlsPrivate;
    this->d->loadVendors();
    this->load(fd);
}

UvcExtendedControls::~UvcExtendedControls()
{
    delete this->d;
}

void UvcExtendedControls::load(const QString &devicePath)
{
    this->d->m_extensions = this->d->readExtensions(devicePath);
    this->d->loadControls(devicePath);
}

void UvcExtendedControls::load(int fd)
{
    this->d->m_extensions = this->d->readExtensions(fd);
    this->d->loadControls(fd);
}

QVariantList UvcExtendedControls::controls(int fd) const
{
    if (fd < 0)
        return {};

    QVariantList controls;

    for (auto &uvcControl: this->d->m_uvcControls) {
        QVariantList controlVar;

        switch (uvcControl.control.uvcType) {
        case UVC_CTRL_DATA_TYPE_SIGNED:
            controlVar = this->d->readControlSigned(fd,
                                                    uvcControl.unitId,
                                                    uvcControl.control);

            break;

        case UVC_CTRL_DATA_TYPE_UNSIGNED:
            if (uvcControl.control.v4l2Type == V4L2_CTRL_TYPE_MENU)
                controlVar = this->d->readControlMenu(fd,
                                                      uvcControl.unitId,
                                                      uvcControl.control);
            else
                controlVar = this->d->readControlUnsigned(fd,
                                                          uvcControl.unitId,
                                                          uvcControl.control);

            break;

        case UVC_CTRL_DATA_TYPE_BOOLEAN:
            controlVar = this->d->readControlBoolean(fd,
                                                     uvcControl.unitId,
                                                     uvcControl.control);

            break;

        default:
            break;
        }

        if (!controlVar.isEmpty())
            controls << QVariant(controlVar);
    }

    return controls;
}

QVariantList UvcExtendedControls::controls(const QString &devicePath) const
{
    auto fd = x_open(devicePath.toStdString().c_str(),
                     O_RDWR, // | O_NONBLOCK,
                     0);

    if (fd < 0)
        return {};

    auto controls = this->controls(fd);
    x_close(fd);

    return controls;
}

bool UvcExtendedControls::setControls(int fd, const QVariantMap &controls) const
{
    bool ok = true;

    for (auto it = controls.cbegin(); it != controls.cend(); it++) {
        for (auto &uvcControl: this->d->m_uvcControls) {
            if (it.key() != uvcControl.control.name)
                continue;

            switch (uvcControl.control.uvcType) {
            case UVC_CTRL_DATA_TYPE_SIGNED:
                ok &= this->d->writeControlSigned(fd,
                                                  uvcControl.unitId,
                                                  uvcControl.control,
                                                  it.value().toInt());

                break;

            case UVC_CTRL_DATA_TYPE_UNSIGNED:
            case UVC_CTRL_DATA_TYPE_BOOLEAN:
                ok &= this->d->writeControlUnsigned(fd,
                                                    uvcControl.unitId,
                                                    uvcControl.control,
                                                    it.value().toUInt());

            default:
                ok = false;

                break;
            }
        }
    }

    return ok;
}

bool UvcExtendedControls::setControls(const QString &devicePath,
                                      const QVariantMap &controls) const
{
    auto fd = x_open(devicePath.toStdString().c_str(),
                     O_RDWR, // | O_NONBLOCK,
                     0);

    if (fd < 0)
        return false;

    auto result = this->setControls(fd, controls);
    x_close(fd);

    return result;
}

void UvcExtendedControlsPrivate::loadDefinitions(const QString &filePath)
{
    QFile jsonFile(filePath);

    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    auto jsonData = jsonFile.readAll();
    jsonFile.close();
    QJsonParseError jsonError;
    auto json = QJsonDocument::fromJson(jsonData, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        static const QMap<QJsonParseError::ParseError, QString> errorMap {
            {QJsonParseError::UnterminatedObject   , "Unterminated object"    },
            {QJsonParseError::MissingNameSeparator , "Missing name separator" },
            {QJsonParseError::UnterminatedArray    , "Unterminated array"     },
            {QJsonParseError::MissingValueSeparator, "Missing value separator"},
            {QJsonParseError::IllegalValue         , "Illegal value"          },
            {QJsonParseError::TerminationByNumber  , "Termination by number"  },
            {QJsonParseError::IllegalNumber        , "Illegal number"         },
            {QJsonParseError::IllegalEscapeSequence, "Illegal escape sequence"},
            {QJsonParseError::IllegalUTF8String    , "Illegal UTF8 string"    },
            {QJsonParseError::UnterminatedString   , "Unterminated string"    },
            {QJsonParseError::MissingObject        , "Missing object"         },
            {QJsonParseError::DeepNesting          , "Deep nesting"           },
            {QJsonParseError::DocumentTooLarge     , "Document too large"     },
            {QJsonParseError::GarbageAtEnd         , "Garbage at end"         },
        };

        auto line = jsonData.mid(0, jsonError.offset).count('\n') + 1;
        qCritical() << QString("%1: in line %2: %3 (%4)")
                        .arg(errorMap.value(jsonError.error, "Error"))
                        .arg(line)
                        .arg(jsonError.errorString())
                        .arg(jsonFile.fileName()).toStdString().c_str();

        return;
    }

    for (auto vendorVal: json.array()) {
        auto vendorObj = vendorVal.toObject();
        bool ok = false;
        auto vendorId =
            vendorObj.value("vendor_id").toString().toUShort(&ok, 16);

        if (!ok)
            continue;

        auto it = std::find_if(this->m_vendors.begin(), this->m_vendors.end(),
                                [&vendorId] (const UvcVendor &vendor) -> bool {
                                    return vendor.id == vendorId;
                                });
        UvcVendor *vendor = nullptr;

        if (it == this->m_vendors.end()) {
            this->m_vendors << UvcVendor(vendorId);
            vendor = &this->m_vendors.last();
        } else {
            vendor = &(*it);
        }

        for (auto productVal: vendorObj.value("products").toArray()) {
            auto productObj = productVal.toObject();
            QVector<quint16> productIds;

            if (productObj.contains("product_id")) {
                for (auto idVal: productObj.value("product_id").toArray()) {
                    auto productId = idVal.toString().toUShort(&ok, 16);

                    if (ok)
                        productIds << productId;;
                }
            }

            QVector<UvcInterface> interfaces;

            for (auto interfaceVal: productObj.value("interfaces").toArray()) {
                auto interfaceObj = interfaceVal.toObject();
                auto interfaceGuid =
                    Guid::fromString(interfaceObj.value("guid").toString());

                if (!interfaceGuid)
                    continue;

                QVector<UvcControl> controls;

                for (auto controlVal: interfaceObj.value("controls").toArray()) {
                    auto controlObj = controlVal.toObject();
                    auto name = controlObj.value("name").toString();

                    if (name.isEmpty())
                        continue;

                    if (!controlObj.contains("selector"))
                        continue;

                    auto selector = controlObj.value("selector").toInt();

                    if (!controlObj.contains("size"))
                        continue;

                    auto size = controlObj.value("size").toInt();

                    if (size < 1)
                        continue;

                    if (!controlObj.contains("read_size"))
                        continue;

                    auto readSize = controlObj.value("read_size").toInt();

                    if (readSize < 1)
                        continue;

                    if (!controlObj.contains("offset"))
                        continue;

                    auto offset = controlObj.value("offset").toInt();
                    auto type =
                        UvcControlTypes::byName(controlObj.value("type").toString());

                    if (type->v4l2Type == v4l2_ctrl_type(0))
                        continue;

                    QVector<UvcMenuOption> menu;

                    if (type->v4l2Type == V4L2_CTRL_TYPE_MENU) {
                        if (!controlObj.contains("menu"))
                            continue;

                        for (auto optionVal: controlObj.value("menu").toArray()) {
                            auto optionObj = optionVal.toObject();
                            auto name = optionObj["name"].toString();

                            if (name.isEmpty())
                                continue;

                            if (!optionObj["value"].isDouble())
                                continue;

                            auto value = optionObj["value"].toDouble();

                            menu << UvcMenuOption(name, value);
                        }

                        if (menu.isEmpty())
                            continue;
                    } else {
                        if (controlObj.contains("menu"))
                            continue;
                    }

                    controls << UvcControl(name,
                                            selector,
                                            size,
                                            readSize,
                                            offset,
                                            type->uvcType,
                                            type->v4l2Type,
                                            menu);
                }

                if (!controls.isEmpty())
                    interfaces << UvcInterface(interfaceGuid, controls);
            }

            if (!interfaces.isEmpty())
                vendor->products << UvcProduct(productIds, interfaces);
        }
    }
}

void UvcExtendedControlsPrivate::loadVendors(const QStringList &searchDirectories)
{
    QStringList directories {
        ":/v4l2/share/cameras"
    };

    auto binDir = QDir(BINDIR).absolutePath();
    auto dataRootDir = QDir(DATAROOTDIR).absolutePath();
    auto relDataRootDir = QDir(binDir).relativeFilePath(dataRootDir);
    QDir appDir = QCoreApplication::applicationDirPath();

    if (appDir.cd(relDataRootDir)) {
        auto path = appDir.absolutePath() + "/cameras";
        path.replace("/", QDir::separator());
        directories << path;
    }

    auto standardDataLocations =
            QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);

    for (auto &path: standardDataLocations)
        directories << path + "/cameras";

    directories << searchDirectories;

    for (auto &dir: directories) {
        QDir configsDir(dir);
        auto configs = configsDir.entryList(QStringList() << "*.json",
                                            QDir::Files | QDir::Readable,
                                            QDir::Name);

        for (auto &config: configs)
            this->loadDefinitions(configsDir.absoluteFilePath(config));
    }
}

QMap<Guid, quint8> UvcExtendedControlsPrivate::readExtensions(quint16 vendorId,
                                                              quint16 productId,
                                                              quint8 bus,
                                                              quint8 port,
                                                              quint8 address) const
{
    QMap<Guid, quint8> extensions;

    libusb_context *context = nullptr;

    if (libusb_init_context(&context, nullptr, 0) != 0)
        return {};

    libusb_device **devices = nullptr;
    auto nDevices = libusb_get_device_list(context, &devices);

    if (nDevices < 1)
        return {};

    for (int dev = 0; dev < nDevices; dev++) {
        libusb_device_descriptor descriptor;
        auto device = devices[dev];

        if (libusb_get_device_descriptor(device, &descriptor))
            continue;

        if (descriptor.idVendor != vendorId
            || descriptor.idProduct != productId
            || libusb_get_bus_number(device) != bus
            || libusb_get_port_number(device) != port
            || libusb_get_device_address(device) != address) {
            continue;
        }

        libusb_config_descriptor *configDescriptor = nullptr;

        if (libusb_get_active_config_descriptor(device, &configDescriptor) != 0)
            continue;

        for (int ifc = 0; ifc < configDescriptor->bNumInterfaces; ifc++) {
            auto interface = configDescriptor->interface[ifc];

            for (int alt = 0; alt < interface.num_altsetting; alt++) {
                auto interfaceDescriptor = interface.altsetting[alt];

                if (interfaceDescriptor.extra_length > 0) {
                    int start = 0;

                    while (start < interfaceDescriptor.extra_length) {
                        auto descriptorHeader =
                            reinterpret_cast<const uvc_descriptor_header *>(interfaceDescriptor.extra + start);

                        if (descriptorHeader->bDescriptorType == CS_INTERFACE) {
                            switch (descriptorHeader->bDescriptorSubType) {
                            case UVC_VC_EXTENSION_UNIT: {
                                auto xuDescriptor = reinterpret_cast<const uvc_extension_unit_descriptor *>(interfaceDescriptor.extra + start);
                                Guid guid(reinterpret_cast<const char *>(xuDescriptor->guidExtensionCode), 16);
                                extensions[guid] = xuDescriptor->bUnitID;

                                break;
                            }

                            default:
                                break;
                            }
                        }

                        start += descriptorHeader->bLength;
                    }
                }
            }
        }

        libusb_free_config_descriptor(configDescriptor);
    }

    libusb_free_device_list(devices, nDevices);
    libusb_exit(context);

    return extensions;
}

QMap<Guid, quint8> UvcExtendedControlsPrivate::readExtensions(const QString &devicePath) const
{
    if (devicePath.isEmpty())
        return {};

    auto id = QFileInfo(devicePath).baseName();
    auto path = QString("/sys/class/video4linux/%1/../../..").arg(id);
    static const QStringList paramsKeys {
        "idVendor",
        "idProduct",
        "busnum",
        "devpath",
        "devnum"
    };
    QMap<QString, uint> params;

    for (auto &param: paramsKeys) {
        QFile file(path + "/" + param);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};

        bool ok = false;
        int base = param == "idVendor" || param == "idProduct"? 16: 10;
        auto value = file.readAll().trimmed().toUInt(&ok, base);
        file.close();

        if (!ok)
            return {};

        params[param] = value;
    }

    return this->readExtensions(quint16(params["idVendor"]),
                                quint16(params["idProduct"]),
                                quint8(params["busnum"]),
                                quint8(params["devpath"]),
                                quint8(params["devnum"]));
}

QMap<Guid, quint8> UvcExtendedControlsPrivate::readExtensions(int fd) const
{
    if (fd < 1)
        return {};

    auto devicePath =
        QFileInfo(QString("/proc/self/fd/%1").arg(fd)).symLinkTarget();

    return this->readExtensions(devicePath);
}

void UvcExtendedControlsPrivate::loadControls(const QString &devicePath)
{
    auto fd = x_open(devicePath.toStdString().c_str(),
                     O_RDWR, // | O_NONBLOCK,
                     0);

    if (fd < 0)
        return;

    this->loadControls(fd);
    x_close(fd);
}

void UvcExtendedControlsPrivate::loadControls(int fd)
{
    this->m_uvcControls.clear();

    if (fd < 0)
        return;

    auto devicePath =
        QFileInfo(QString("/proc/self/fd/%1").arg(fd)).symLinkTarget();
    auto id = QFileInfo(devicePath).baseName();
    auto path = QString("/sys/class/video4linux/%1/../../..").arg(id);
    static const QStringList paramsKeys {
        "idVendor",
        "idProduct"
    };
    QMap<QString, uint> params;

    for (auto &param: paramsKeys) {
        QFile file(path + "/" + param);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return;

        bool ok = false;
        auto value = file.readAll().trimmed().toUInt(&ok, 16);
        file.close();

        if (!ok)
            return;

        params[param] = value;
    }

    auto vendorId = params["idVendor"];
    auto productId = params["idProduct"];
    auto it = std::find_if(this->m_vendors.constBegin(),
                           this->m_vendors.constEnd(),
                            [&vendorId] (const UvcVendor &vendor) -> bool {
                                return vendor.id == vendorId;
                            });

    if (it == this->m_vendors.constEnd())
        return;

    for (auto &product: it->products) {
        if (!product.ids.isEmpty() && !product.ids.contains(productId))
            continue;

        for (auto &interface: product.interfaces) {
            auto unitId = this->m_extensions.value(interface.guid);

            for (auto &control: interface.controls) {
                auto dataSize = this->controlDataSize(fd,
                                                      unitId,
                                                      control.selector);

                if (dataSize < 1 || dataSize != control.size)
                    continue;

                if (control.uvcType != UVC_CTRL_DATA_TYPE_SIGNED
                    && control.uvcType != UVC_CTRL_DATA_TYPE_UNSIGNED
                    && control.uvcType != UVC_CTRL_DATA_TYPE_BOOLEAN) {
                    continue;
                }

                this->m_uvcControls << UvcControlExt(control, unitId);
            }
        }
    }
}

quint16 UvcExtendedControlsPrivate::controlDataSize(int fd,
                                                    quint8 unitId,
                                                    quint8 selector) const
{
    if (fd < 0)
        return 0;

    quint16 dataSize = 0;

    uvc_xu_control_query query;
    memset(&query, 0, sizeof(uvc_xu_control_query));
    query.unit = unitId;
    query.selector = selector;
    query.query = UVC_GET_LEN;
    query.size = sizeof(quint16);
    query.data = reinterpret_cast<quint8 *>(&dataSize);

    if (ioctl(fd, UVCIOC_CTRL_QUERY, &query) < 0)
        return 0;

    return dataSize;
}

int UvcExtendedControlsPrivate::queryControl(int fd,
                                             quint8 unitId,
                                             quint8 selector,
                                             quint8 query,
                                             void *data,
                                             quint16 dataSize) const
{
    if (fd < 0)
        return -EBADF;

    if (dataSize < 1)
        dataSize = this->controlDataSize(fd, unitId, selector);

    if (dataSize < 1)
        return -EINVAL;

    uvc_xu_control_query controlQuery;
    memset(&controlQuery, 0, sizeof(uvc_xu_control_query));
    controlQuery.unit = unitId;
    controlQuery.selector = selector;
    controlQuery.query = query;
    controlQuery.size = dataSize;
    controlQuery.data = reinterpret_cast<quint8 *>(data);

    return ioctl(fd, UVCIOC_CTRL_QUERY, &controlQuery);
}

quint32 UvcExtendedControlsPrivate::readValueU(const UvcControl &control,
                                               const QBitArray &value) const
{
    QBitArray v(8 * sizeof(quint32));
    auto offset = v.size() - control.readSize;

    for (int i = 0; i < control.readSize; i++)
        v[i + offset] = value[i + control.offset];

    return v.toUInt32(QSysInfo::BigEndian);
}

qint32 UvcExtendedControlsPrivate::readValueS(const UvcControl &control,
                                              const QBitArray &value) const
{
    return qint32(qint64(this->readValueU(control, value))
                  + std::numeric_limits<qint32>::min());
}

QVariantList UvcExtendedControlsPrivate::readControlSigned(int fd,
                                                           quint8 unitId,
                                                           const UvcControl &control) const
{
    auto dataSize = this->controlDataSize(fd, unitId, control.selector);

    if (dataSize < 1 || dataSize != control.size)
        return {};

    QMap<int, qint32> values {
        {UVC_GET_CUR, 0},
        {UVC_GET_DEF, 0},
        {UVC_GET_MIN, 0},
        {UVC_GET_MAX, 0},
    };

    for (auto it = values.begin(); it != values.end(); it++) {
        QByteArray rawData(dataSize, '\0');

        if (this->queryControl(fd,
                               unitId,
                               control.selector,
                               it.key(),
                               rawData.data(),
                               dataSize) < 0) {
            return {};
        }

        auto value =
            QBitArray::fromBits(rawData.constData(), 8 * rawData.size());
        values[it.key()] = this->readValueS(control, value);
    }

    return {control.name,
            "integer",
            values[UVC_GET_MIN],
            values[UVC_GET_MAX],
            1,
            values[UVC_GET_DEF],
            values[UVC_GET_CUR],
            QVariantList()};
}

QVariantList UvcExtendedControlsPrivate::readControlUnsigned(int fd,
                                                             quint8 unitId,
                                                             const UvcControl &control) const
{
    auto dataSize = this->controlDataSize(fd, unitId, control.selector);

    if (dataSize < 1 || dataSize != control.size)
        return {};

    QMap<int, quint32> values {
        {UVC_GET_CUR, 0},
        {UVC_GET_DEF, 0},
        {UVC_GET_MIN, 0},
        {UVC_GET_MAX, 0},
    };

    for (auto it = values.begin(); it != values.end(); it++) {
        QByteArray rawData(dataSize, '\0');

        if (this->queryControl(fd,
                               unitId,
                               control.selector,
                               it.key(),
                               rawData.data(),
                               dataSize) < 0) {
            return {};
        }

        auto value =
            QBitArray::fromBits(rawData.constData(), 8 * rawData.size());
        values[it.key()] = this->readValueU(control, value);
    }

    return {control.name,
            "integer",
            values[UVC_GET_MIN],
            values[UVC_GET_MAX],
            1,
            values[UVC_GET_DEF],
            values[UVC_GET_CUR],
            QVariantList()};
}

QVariantList UvcExtendedControlsPrivate::readControlBoolean(int fd,
                                                            quint8 unitId,
                                                            const UvcControl &control) const
{
    auto dataSize = this->controlDataSize(fd, unitId, control.selector);

    if (dataSize < 1 || dataSize != control.size)
        return {};

    QMap<int, bool> values {
        {UVC_GET_CUR, false},
        {UVC_GET_DEF, false},
        {UVC_GET_MIN, false},
        {UVC_GET_MAX, false},
    };

    for (auto it = values.begin(); it != values.end(); it++) {
        QByteArray rawData(dataSize, '\0');

        if (this->queryControl(fd,
                               unitId,
                               control.selector,
                               it.key(),
                               rawData.data(),
                               dataSize) < 0) {
            return {};
        }

        auto value =
            QBitArray::fromBits(rawData.constData(), 8 * rawData.size());
        values[it.key()] = this->readValueU(control, value);
    }

    return {control.name,
            "boolean",
            0,
            1,
            1,
            values[UVC_GET_DEF],
            values[UVC_GET_CUR],
            QVariantList()};
}

QVariantList UvcExtendedControlsPrivate::readControlMenu(int fd,
                                                         quint8 unitId,
                                                         const UvcControl &control) const
{
    auto dataSize = this->controlDataSize(fd, unitId, control.selector);

    if (dataSize < 1 || dataSize != control.size)
        return {};

    QMap<int, quint32> values {
        {UVC_GET_CUR, 0},
        {UVC_GET_DEF, 0},
        {UVC_GET_MIN, 0},
        {UVC_GET_MAX, 0},
    };

    for (auto it = values.begin(); it != values.end(); it++) {
        QByteArray rawData(dataSize, '\0');

        if (this->queryControl(fd,
                               unitId,
                               control.selector,
                               it.key(),
                               rawData.data(),
                               dataSize) < 0) {
            return {};
        }

        auto value =
            QBitArray::fromBits(rawData.constData(), 8 * rawData.size());
        values[it.key()] = this->readValueU(control, value);
    }

    auto it = std::find_if(control.menu.constBegin(),
                           control.menu.constEnd(),
                           [&values] (const UvcMenuOption &option) -> bool {
        return option.value.toUInt() == values[UVC_GET_DEF];
    });

    quint32 defaultValue =
            it == control.menu.constEnd()?
                0:
                std::distance(control.menu.constBegin(), it);

    it = std::find_if(control.menu.constBegin(),
                      control.menu.constEnd(),
                      [&values] (const UvcMenuOption &option) -> bool {
        return option.value.toUInt() == values[UVC_GET_CUR];
    });

    quint32 value =
            it == control.menu.constEnd()?
                0:
                std::distance(control.menu.constBegin(), it);

    QStringList menu;

    for (auto &option: control.menu)
        menu << option.name;

    return {control.name,
            "menu",
            0,
            menu.size(),
            1,
            defaultValue,
            value,
            menu};
}

inline QByteArray UvcExtendedControlsPrivate::writeValueU(const UvcControl &control,
                                                          const QBitArray &curValue,
                                                          quint32 value) const
{
    auto curValueCopy = curValue;

    for (int i = 0; i < control.readSize; i++)
        curValueCopy[i + control.offset] = (value >> i) & 0x1;

    return QByteArray(reinterpret_cast<const char *>(curValueCopy.bits()), control.size);
}

inline QByteArray UvcExtendedControlsPrivate::writeValueS(const UvcControl &control,
                                                          const QBitArray &curValue,
                                                          qint32 value) const
{
    return this->writeValueU(control,
                             curValue,
                             quint32(quint64(value) - std::numeric_limits<qint32>::min()));
}

bool UvcExtendedControlsPrivate::writeControlSigned(int fd,
                                                    quint8 unitId,
                                                    const UvcControl &control,
                                                    qint32 value) const
{
    return this->writeControlUnsigned(fd,
                                      unitId,
                                      control,
                                      quint32(quint64(value) - std::numeric_limits<qint32>::min()));
}

bool UvcExtendedControlsPrivate::writeControlUnsigned(int fd,
                                                      quint8 unitId,
                                                      const UvcControl &control,
                                                      quint32 value) const
{
    auto dataSize = this->controlDataSize(fd, unitId, control.selector);

    if (dataSize < 1 || dataSize != control.size)
        return false;

    QByteArray rawData(dataSize, '\0');

    if (this->queryControl(fd,
                           unitId,
                           control.selector,
                           UVC_GET_CUR,
                           rawData.data(),
                           dataSize) < 0) {
        return false;
    }

    auto curValue =
        QBitArray::fromBits(rawData.constData(), 8 * rawData.size());

    if (control.v4l2Type == V4L2_CTRL_TYPE_MENU) {
        value = value < control.menu.size()?
                    control.menu[value].value.toUInt():
                control.menu.size() > 0?
                    control.menu[0].value.toUInt():
                    0;
    }

    auto data = this->writeValueU(control, curValue, value);

    if (this->queryControl(fd,
                           unitId,
                           control.selector,
                           UVC_SET_CUR,
                           data.data(),
                           data.size()) < 0) {
        return false;
    }

    return true;
}

#include "moc_uvcextendedcontrols.cpp"

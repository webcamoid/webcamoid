/* Webcamoid, webcam capture application.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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
#include <QRegularExpression>
#include <windows.h>
#include <mfapi.h>
#include <initguid.h>
#include <usbiodef.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <winusb.h>

#include "uvcextendedcontrols.h"
#include "guid.h"

#define CS_INTERFACE 36
#define UVC_VC_EXTENSION_UNIT 0x4

#define UVC_CTRL_DATA_TYPE_UNKNOWN  0x0
#define UVC_CTRL_DATA_TYPE_SIGNED   0x1
#define UVC_CTRL_DATA_TYPE_UNSIGNED 0x2
#define UVC_CTRL_DATA_TYPE_BOOLEAN  0x3

#define CTRL_TYPE_UNKNOWN 0x0
#define CTRL_TYPE_INTEGER 0x1
#define CTRL_TYPE_BOOLEAN 0x2
#define CTRL_TYPE_MENU    0x3

#define UVC_SET_CUR  0x01
#define UVC_GET_CUR  0x81
#define UVC_GET_MIN  0x82
#define UVC_GET_MAX  0x83
#define UVC_GET_RES	 0x84
#define UVC_GET_LEN  0x85
#define UVC_GET_DEF  0x87

struct UvcDescriptorHeader
{
    quint8 length;
    quint8 descriptorType;
    quint8 descriptorSubType;
};


struct UvcExtensionUnitDescriptor
{
    quint8 length;
    quint8 descriptorType;
    quint8 descriptorSubType;
    quint8 unitID;
    quint8 guidExtensionCode[16];
};

struct UvcControlTypes
{
    const char *name;
    int uvcType;
    int ctrlType;

    static inline const UvcControlTypes *byName(const QString &name);
};

static const UvcControlTypes uvcControlTypesTable[] {
    {"signed"  , UVC_CTRL_DATA_TYPE_SIGNED  , CTRL_TYPE_INTEGER},
    {"unsigned", UVC_CTRL_DATA_TYPE_UNSIGNED, CTRL_TYPE_INTEGER},
    {"boolean" , UVC_CTRL_DATA_TYPE_BOOLEAN , CTRL_TYPE_BOOLEAN},
    {"menu"    , UVC_CTRL_DATA_TYPE_UNSIGNED, CTRL_TYPE_MENU   },
    {""        , UVC_CTRL_DATA_TYPE_UNKNOWN , CTRL_TYPE_UNKNOWN},
};

const UvcControlTypes *UvcControlTypes::byName(const QString &name)
{
    auto type = uvcControlTypesTable;

    for (; type->uvcType != UVC_CTRL_DATA_TYPE_UNKNOWN; ++type)
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

    UvcMenuOption(const QString &name, const QVariant &value):
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
    int ctrlType {0};
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
               int ctrlType,
               const QVector<UvcMenuOption> &menu):
        name(name),
        selector(selector),
        size(size),
        readSize(readSize),
        offset(offset),
        uvcType(uvcType),
        ctrlType(ctrlType),
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
        ctrlType(other.ctrlType),
        menu(other.menu)
    {
    }
};

struct UvcInterface
{
    Guid guid;
    int num {0};
    QVector<UvcControl> controls;

    UvcInterface()
    {
    }

    UvcInterface(const Guid &guid,
                 int num,
                 const QVector<UvcControl> &controls):
        guid(guid),
        num(num),
        controls(controls)
    {
    }

    UvcInterface(const UvcInterface &other):
        guid(other.guid),
        num(other.num),
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

    UvcProduct(const QVector<quint16> &ids,
               const QVector<UvcInterface> &interfaces):
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
    quint16 id{0};
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
    int interfaceNumber {0};
    quint8 unitId {0};

    UvcControlExt()
    {
    }

    UvcControlExt(const UvcControl &control,
                  int interfaceNumber,
                  quint16 unitId):
        control(control),
        interfaceNumber(interfaceNumber),
        unitId(unitId)
    {
    }

    UvcControlExt(const UvcControlExt &other):
        control(other.control),
        interfaceNumber(other.interfaceNumber),
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

        QString devicePath(IMFMediaSource *camera) const;
        void loadDefinitions(const QString &filePath);
        void loadVendors(const QStringList &searchDirectories={});
        QMap<Guid, quint8> readExtensions(quint16 vendorId,
                                          quint16 productId,
                                          quint8 bus,
                                          quint8 address) const;
        QMap<Guid, quint8> readExtensions(const QString &devicePath) const;
        bool readProperties(const QString &devicePath,
                            quint16 &vendorId,
                            quint16 &productId,
                            quint8 &bus,
                            quint8 &address) const;
        bool readProperties(const QString &devicePath,
                            quint16 &vendorId,
                            quint16 &productId) const;
        void loadControls(const QString &devicePath);
        quint16 controlDataSize(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                int interfaceNumber,
                                quint8 unitId,
                                quint8 selector) const;
        int queryControl(WINUSB_INTERFACE_HANDLE winUsbHandle,
                         int interfaceNumber,
                         quint8 unitId,
                         quint8 selector,
                         quint8 query,
                         void *data,
                         quint16 dataSize = 0) const;
        inline quint32 readValueU(const UvcControl &control,
                                  const QBitArray &value) const;
        inline qint32 readValueS(const UvcControl &control,
                                 const QBitArray &value) const;
        QVariantList readControlSigned(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                       int interfaceNumber,
                                       quint8 unitId,
                                       const UvcControl &control) const;
        QVariantList readControlUnsigned(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                         int interfaceNumber,
                                         quint8 unitId,
                                         const UvcControl &control) const;
        QVariantList readControlBoolean(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                        int interfaceNumber,
                                        quint8 unitId,
                                        const UvcControl &control) const;
        QVariantList readControlMenu(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                     int interfaceNumber,
                                     quint8 unitId,
                                     const UvcControl &control) const;
        inline QByteArray writeValueU(const UvcControl &control,
                                      const QBitArray &curValue,
                                      quint32 value) const;
        inline QByteArray writeValueS(const UvcControl &control,
                                      const QBitArray &curValue,
                                      qint32 value) const;
        bool writeControlSigned(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                int interfaceNumber,
                                quint8 unitId,
                                const UvcControl &control,
                                qint32 value) const;
        bool writeControlUnsigned(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                  int interfaceNumber,
                                  quint8 unitId,
                                  const UvcControl &control,
                                  quint32 value) const;
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

UvcExtendedControls::UvcExtendedControls(IMFMediaSource *filter):
    QObject()
{
    this->d = new UvcExtendedControlsPrivate;
    this->d->loadVendors();
    this->load(this->d->devicePath(filter));
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

void UvcExtendedControls::load(IMFMediaSource *filter)
{
    this->load(this->d->devicePath(filter));
}

QVariantList UvcExtendedControls::controls(IMFMediaSource *filter) const
{
    return this->controls(this->d->devicePath(filter));
}

QVariantList UvcExtendedControls::controls(const QString &devicePath) const
{
    quint16 vendorId = 0;
    quint16 productId = 0;

    if (!this->d->readProperties(devicePath, vendorId, productId))
        return {};

    auto it = std::find_if(this->d->m_vendors.constBegin(),
                           this->d->m_vendors.constEnd(),
                           [&vendorId](const UvcVendor &vendor) {
        return vendor.id == vendorId;
    });

    if (it == this->d->m_vendors.constEnd())
        return {};

    auto hDevInfo =
            SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE,
                                nullptr,
                                nullptr,
                                DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE)
        return {};

    SP_DEVICE_INTERFACE_DATA interfaceData = {sizeof(SP_DEVICE_INTERFACE_DATA)};
    SP_DEVINFO_DATA devInfoData = {sizeof(SP_DEVINFO_DATA)};
    HANDLE devHandle = INVALID_HANDLE_VALUE;
    WINUSB_INTERFACE_HANDLE winUsbHandle = nullptr;

    for (DWORD index = 0; ; ++index) {
        if (!SetupDiEnumDeviceInterfaces(hDevInfo,
                                         nullptr,
                                         &GUID_DEVINTERFACE_USB_DEVICE,
                                         index,
                                         &interfaceData)) {
            break;
        }

        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                        &interfaceData,
                                        nullptr,
                                        0,
                                        &requiredSize,
                                        nullptr);
        auto detailData =
                reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(requiredSize));

        if (!detailData)
            continue;

        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                             &interfaceData,
                                             detailData,
                                             requiredSize,
                                             nullptr,
                                             &devInfoData)) {
            free(detailData);

            continue;
        }

        quint16 devVendorId = 0;
        quint16 devProductId = 0;
        quint8 devBus = 0;
        quint8 devAddress = 0;

        if (!this->d->readProperties(QString::fromWCharArray(detailData->DevicePath),
                                     devVendorId,
                                     devProductId,
                                     devBus,
                                     devAddress)) {
            free(detailData);

            continue;
        }

        if (devVendorId != vendorId || devProductId != productId) {
            free(detailData);

            continue;
        }

        devHandle = CreateFile(detailData->DevicePath,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr,
                               OPEN_EXISTING,
                               FILE_FLAG_OVERLAPPED,
                               nullptr);
        free(detailData);

        if (devHandle == INVALID_HANDLE_VALUE)
            continue;

        if (!WinUsb_Initialize(devHandle, &winUsbHandle)) {
            CloseHandle(devHandle);
            devHandle = INVALID_HANDLE_VALUE;

            continue;
        }

        break;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    if (!winUsbHandle) {
        if (devHandle != INVALID_HANDLE_VALUE)
            CloseHandle(devHandle);

        return {};
    }

    QVariantList controls;

    for (auto &uvcControl: this->d->m_uvcControls) {
        QVariantList controlVar;

        switch (uvcControl.control.uvcType) {
            case UVC_CTRL_DATA_TYPE_SIGNED:
                controlVar =
                        this->d->readControlSigned(winUsbHandle,
                                                   uvcControl.interfaceNumber,
                                                   uvcControl.unitId,
                                                   uvcControl.control);
                break;
            case UVC_CTRL_DATA_TYPE_UNSIGNED:
                if (uvcControl.control.ctrlType == CTRL_TYPE_MENU)
                    controlVar =
                            this->d->readControlMenu(winUsbHandle,
                                                     uvcControl.interfaceNumber,
                                                     uvcControl.unitId,
                                                     uvcControl.control);
                else
                    controlVar =
                            this->d->readControlUnsigned(winUsbHandle,
                                                         uvcControl.interfaceNumber,
                                                         uvcControl.unitId,
                                                         uvcControl.control);
                break;
            case UVC_CTRL_DATA_TYPE_BOOLEAN:
                controlVar =
                        this->d->readControlBoolean(winUsbHandle,
                                                    uvcControl.interfaceNumber,
                                                    uvcControl.unitId,
                                                    uvcControl.control);
                break;
            default:
                break;
        }

        if (!controlVar.isEmpty())
            controls << QVariant(controlVar);
    }

    WinUsb_Free(winUsbHandle);
    CloseHandle(devHandle);

    return controls;
}

bool UvcExtendedControls::setControls(IMFMediaSource *filter,
                                      const QVariantMap &controls) const
{
    return this->setControls(this->d->devicePath(filter), controls);
}

bool UvcExtendedControls::setControls(const QString &devicePath,
                                      const QVariantMap &controls) const
{
    quint16 vendorId = 0;
    quint16 productId = 0;

    if (!this->d->readProperties(devicePath, vendorId, productId))
        return false;

    auto it = std::find_if(this->d->m_vendors.constBegin(),
                           this->d->m_vendors.constEnd(),
                           [&vendorId](const UvcVendor &vendor) {
        return vendor.id == vendorId;
    });

    if (it == this->d->m_vendors.constEnd())
        return false;

    auto hDevInfo =
            SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE,
                                nullptr,
                                nullptr,
                                DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE)
        return false;

    SP_DEVICE_INTERFACE_DATA interfaceData = {sizeof(SP_DEVICE_INTERFACE_DATA)};
    SP_DEVINFO_DATA devInfoData = {sizeof(SP_DEVINFO_DATA)};
    HANDLE devHandle = INVALID_HANDLE_VALUE;
    WINUSB_INTERFACE_HANDLE winUsbHandle = nullptr;

    for (DWORD index = 0; ; ++index) {
        if (!SetupDiEnumDeviceInterfaces(hDevInfo,
                                         nullptr,
                                         &GUID_DEVINTERFACE_USB_DEVICE,
                                         index,
                                         &interfaceData)) {
            break;
        }

        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                        &interfaceData,
                                        nullptr,
                                        0,
                                        &requiredSize,
                                        nullptr);
        auto detailData =
                reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(requiredSize));

        if (!detailData)
            continue;

        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                             &interfaceData,
                                             detailData,
                                             requiredSize,
                                             nullptr,
                                             &devInfoData)) {
            free(detailData);

            continue;
        }

        quint16 devVendorId = 0;
        quint16 devProductId = 0;
        quint8 devBus = 0;
        quint8 devAddress = 0;

        if (!this->d->readProperties(QString::fromWCharArray(detailData->DevicePath),
                                     devVendorId,
                                     devProductId,
                                     devBus,
                                     devAddress)) {
            free(detailData);

            continue;
        }

        if (devVendorId != vendorId || devProductId != productId) {
            free(detailData);

            continue;
        }

        devHandle = CreateFile(detailData->DevicePath,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr,
                               OPEN_EXISTING,
                               FILE_FLAG_OVERLAPPED,
                               nullptr);
        free(detailData);

        if (devHandle == INVALID_HANDLE_VALUE)
            continue;

        if (!WinUsb_Initialize(devHandle, &winUsbHandle)) {
            CloseHandle(devHandle);
            devHandle = INVALID_HANDLE_VALUE;

            continue;
        }

        break;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    if (!winUsbHandle) {
        if (devHandle != INVALID_HANDLE_VALUE)
            CloseHandle(devHandle);

        return false;
    }

    bool ok = true;

    for (auto it = controls.cbegin(); it != controls.cend(); it++) {
        for (auto &uvcControl: this->d->m_uvcControls) {
            if (it.key() != uvcControl.control.name)
                continue;

            switch (uvcControl.control.uvcType) {
                case UVC_CTRL_DATA_TYPE_SIGNED:
                    ok &= this->d->writeControlSigned(winUsbHandle,
                                                      uvcControl.interfaceNumber,
                                                      uvcControl.unitId,
                                                      uvcControl.control,
                                                      it.value().toInt());
                    break;
                case UVC_CTRL_DATA_TYPE_UNSIGNED:
                case UVC_CTRL_DATA_TYPE_BOOLEAN:
                    ok &= this->d->writeControlUnsigned(winUsbHandle,
                                                        uvcControl.interfaceNumber,
                                                        uvcControl.unitId,
                                                        uvcControl.control,
                                                        it.value().toUInt());
                    break;
                default:
                    ok = false;
                    break;
            }
        }
    }

    WinUsb_Free(winUsbHandle);
    CloseHandle(devHandle);

    return ok;
}

QString UvcExtendedControlsPrivate::devicePath(IMFMediaSource *camera) const
{
    if (!camera)
        return {};

    IMFAttributes *attributes = nullptr;
    HRESULT hr = camera->QueryInterface(IID_PPV_ARGS(&attributes));

    if (FAILED(hr) || !attributes)
        return {};

    WCHAR *symbolicLink = nullptr;
    UINT32 length = 0;
    hr = attributes->GetStringLength(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                     &length);

    if (SUCCEEDED(hr)) {
        symbolicLink = new WCHAR[length + 1];
        hr = attributes->GetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                   symbolicLink,
                                   length + 1,
                                   &length);
    }

    QString result;

    if (SUCCEEDED(hr) && symbolicLink)
        result = QString::fromWCharArray(symbolicLink);

    delete[] symbolicLink;
    attributes->Release();

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

            if (productObj.contains("product_id"))
                for (auto idVal: productObj.value("product_id").toArray()) {
                    auto productId = idVal.toString().toUShort(&ok, 16);

                    if (ok)
                        productIds << productId;
                }

            QVector<UvcInterface> interfaces;

            for (auto interfaceVal: productObj.value("interfaces").toArray()) {
                auto interfaceObj = interfaceVal.toObject();
                auto interfaceGuid =
                        Guid::fromString(interfaceObj.value("guid").toString());

                if (!interfaceGuid)
                    continue;

                auto interfaceNumber = interfaceObj.value("ifacenum").toInt();
                QVector<UvcControl> controls;

                for (auto controlVal: interfaceObj.value("controls").toArray()) {
                    auto controlObj = controlVal.toObject();
                    auto name = controlObj.value("name").toString();

                    if (name.isEmpty()
                        || !controlObj.contains("selector")
                        || !controlObj.contains("size")
                        || !controlObj.contains("read_size")
                        || !controlObj.contains("offset")) {
                        continue;
                    }

                    auto selector = controlObj.value("selector").toInt();
                    auto size = controlObj.value("size").toInt();
                    auto readSize = controlObj.value("read_size").toInt();
                    auto offset = controlObj.value("offset").toInt();
                    auto type = UvcControlTypes::byName(controlObj.value("type").toString());

                    if (type->ctrlType == CTRL_TYPE_UNKNOWN)
                        continue;

                    QVector<UvcMenuOption> menu;

                    if (type->ctrlType == CTRL_TYPE_MENU) {
                        if (!controlObj.contains("menu"))
                            continue;

                        for (auto optionVal: controlObj.value("menu").toArray()) {
                            auto optionObj = optionVal.toObject();
                            auto name = optionObj["name"].toString();

                            if (name.isEmpty() || !optionObj["value"].isDouble())
                                continue;

                            auto value = optionObj["value"].toDouble();
                            menu << UvcMenuOption(name, value);
                        }

                        if (menu.isEmpty())
                            continue;
                    } else if (controlObj.contains("menu")) {
                        continue;
                    }

                    controls << UvcControl(name,
                                           selector,
                                           size,
                                           readSize,
                                           offset,
                                           type->uvcType,
                                           type->ctrlType,
                                           menu);
                }

                if (!controls.isEmpty())
                    interfaces << UvcInterface(interfaceGuid,
                                               interfaceNumber,
                                               controls);
            }

            if (!interfaces.isEmpty())
                vendor->products << UvcProduct(productIds, interfaces);
        }
    }
}

void UvcExtendedControlsPrivate::loadVendors(const QStringList &searchDirectories)
{
    QStringList directories {
        ":/VideoCapture/share/cameras"
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
                                                              quint8 address) const
{
    QMap<Guid, quint8> extensions;
    auto hDevInfo =
            SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE,
                                nullptr,
                                nullptr,
                                DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE)
        return {};

    SP_DEVICE_INTERFACE_DATA interfaceData = {sizeof(SP_DEVICE_INTERFACE_DATA)};
    SP_DEVINFO_DATA devInfoData = {sizeof(SP_DEVINFO_DATA)};

    for (DWORD index = 0; ; ++index) {
        if (!SetupDiEnumDeviceInterfaces(hDevInfo,
                                         nullptr,
                                         &GUID_DEVINTERFACE_USB_DEVICE,
                                         index,
                                         &interfaceData)) {
            break;
        }

        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                        &interfaceData,
                                        nullptr,
                                        0,
                                        &requiredSize,
                                        nullptr);
        auto detailData =
                reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(requiredSize));

        if (!detailData)
            continue;

        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                             &interfaceData,
                                             detailData,
                                             requiredSize,
                                             nullptr,
                                             &devInfoData)) {
            free(detailData);

            continue;
        }

        quint16 devVendorId = 0;
        quint16 devProductId = 0;
        quint8 devBus = 0;
        quint8 devAddress = 0;

        if (!readProperties(QString::fromWCharArray(detailData->DevicePath),
                            devVendorId,
                            devProductId,
                            devBus,
                            devAddress)) {
            free(detailData);

            continue;
        }

        if (devVendorId != vendorId
            || devProductId != productId
            || devBus != bus
            || devAddress != address) {
            free(detailData);

            continue;
        }

        auto devHandle =
                CreateFile(detailData->DevicePath,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_FLAG_OVERLAPPED,
                           nullptr);

        if (devHandle == INVALID_HANDLE_VALUE) {
            free(detailData);

            continue;
        }

        WINUSB_INTERFACE_HANDLE winUsbHandle;

        if (!WinUsb_Initialize(devHandle, &winUsbHandle)) {
            CloseHandle(devHandle);
            free(detailData);

            continue;
        }

        USB_CONFIGURATION_DESCRIPTOR configDesc;
        ULONG lengthTransferred;

        if (!WinUsb_GetDescriptor(winUsbHandle,
                                  USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                  0,
                                  0,
                                  reinterpret_cast<PUCHAR>(&configDesc),
                                  sizeof(configDesc),
                                  &lengthTransferred)) {
            WinUsb_Free(winUsbHandle);
            CloseHandle(devHandle);
            free(detailData);

            continue;
        }

        auto configBuffer =
                reinterpret_cast<PUCHAR>(malloc(configDesc.wTotalLength));

        if (!configBuffer) {
            WinUsb_Free(winUsbHandle);
            CloseHandle(devHandle);
            free(detailData);

            continue;
        }

        if (WinUsb_GetDescriptor(winUsbHandle,
                                 USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 configBuffer,
                                 configDesc.wTotalLength,
                                 &lengthTransferred)) {
            int start = 0;

            while (start < configDesc.wTotalLength) {
                auto descriptorHeader =
                        reinterpret_cast<const UvcDescriptorHeader *>(configBuffer + start);

                if (descriptorHeader->descriptorType == CS_INTERFACE
                    && descriptorHeader->descriptorSubType == UVC_VC_EXTENSION_UNIT) {
                    auto xuDescriptor = reinterpret_cast<const UvcExtensionUnitDescriptor *>(configBuffer + start);
                    Guid guid(reinterpret_cast<const char *>(xuDescriptor->guidExtensionCode), 16);
                    extensions[guid] = xuDescriptor->unitID;
                }

                start += descriptorHeader->length;
            }
        }

        free(configBuffer);
        WinUsb_Free(winUsbHandle);
        CloseHandle(devHandle);
        free(detailData);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    return extensions;
}

QMap<Guid, quint8> UvcExtendedControlsPrivate::readExtensions(const QString &devicePath) const
{
    quint16 vendorId = 0;
    quint16 productId = 0;
    quint8 bus = 0;
    quint8 address = 0;

    if (!this->readProperties(devicePath,
                              vendorId,
                              productId,
                              bus,
                              address)) {
        return {};
    }

    return this->readExtensions(vendorId, productId, bus, address);
}

bool UvcExtendedControlsPrivate::readProperties(const QString &devicePath,
                                                quint16 &vendorId,
                                                quint16 &productId,
                                                quint8 &bus,
                                                quint8 &address) const
{
    vendorId = 0;
    productId = 0;
    bus = 0;
    address = 0;

    // Check that devicePath is not empty
    if (devicePath.isEmpty())
        return false;

    // Convert QString to std::wstring for use in Windows APIs
    auto path = devicePath.toStdWString();

    // Set up device access using Windows APIs (e.g., SetupAPI)
    auto deviceInfo =
        SetupDiGetClassDevs(nullptr,
                            nullptr,
                            nullptr,
                            DIGCF_ALLCLASSES | DIGCF_PRESENT);

    if (deviceInfo == INVALID_HANDLE_VALUE)
        return false;

    bool ok = false;
    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    // Iterate over devices to find the one matching devicePath
    for (DWORD i = 0;
         SetupDiEnumDeviceInfo(deviceInfo, i, &deviceInfoData);
         ++i) {
        // Get the device's symbolic link
        WCHAR symbolicLink[256];
        DWORD bufferSize = sizeof(symbolicLink);

        if (!SetupDiGetDeviceRegistryProperty(deviceInfo,
                                              &deviceInfoData,
                                              SPDRP_FRIENDLYNAME,
                                              nullptr,
                                              reinterpret_cast<PBYTE>(symbolicLink),
                                              bufferSize,
                                              nullptr)) {
            continue;
        }

        if (_wcsicmp(symbolicLink, path.c_str()) != 0)
            continue;

        // Get Vendor ID and Product ID (using SPDRP_HARDWAREID)
        WCHAR hardwareId[256];

        if (!SetupDiGetDeviceRegistryProperty(deviceInfo,
                                              &deviceInfoData,
                                              SPDRP_HARDWAREID,
                                              nullptr,
                                              reinterpret_cast<PBYTE>(hardwareId),
                                              sizeof(hardwareId),
                                              nullptr)) {
            break;
        }

        // Parse hardware ID and get bus/address
        QString hwId = QString::fromWCharArray(hardwareId);
        int vidPos = hwId.indexOf("VID_");
        int pidPos = hwId.indexOf("PID_");

        if (vidPos == -1 || pidPos == -1)
            break;

        vendorId = static_cast<quint16>(hwId.mid(vidPos + 4, 4).toInt(nullptr, 16));
        productId = static_cast<quint16>(hwId.mid(pidPos + 4, 4).toInt(nullptr, 16));

        ULONG busNumber;
        ULONG busSize = sizeof(busNumber);

        if (CM_Get_DevNode_Registry_Property(deviceInfoData.DevInst,
                                             CM_DRP_BUSNUMBER,
                                             nullptr,
                                             &busNumber,
                                             &busSize,
                                             0) == CR_SUCCESS) {
            bus = static_cast<quint8>(busNumber);
        }

        ULONG deviceAddress;
        ULONG addressSize = sizeof(deviceAddress);

        if (CM_Get_DevNode_Registry_Property(deviceInfoData.DevInst,
                                             CM_DRP_ADDRESS,
                                             nullptr,
                                             &deviceAddress,
                                             &addressSize,
                                             0) == CR_SUCCESS) {
            address = static_cast<quint8>(deviceAddress);
        }

        ok = true;

        break;
    }

    // Clean up device information set
    SetupDiDestroyDeviceInfoList(deviceInfo);

    return ok;
}

bool UvcExtendedControlsPrivate::readProperties(const QString &devicePath,
                                                quint16 &vendorId,
                                                quint16 &productId) const
{
    quint8 bus;
    quint8 address;

    return this->readProperties(devicePath, vendorId, productId, bus, address);
}

void UvcExtendedControlsPrivate::loadControls(const QString &devicePath)
{
    this->m_uvcControls.clear();

    quint16 vendorId = 0;
    quint16 productId = 0;
    quint8 bus = 0;
    quint8 address = 0;

    if (!readProperties(devicePath, vendorId, productId, bus, address))
        return;

    auto it = std::find_if(this->m_vendors.constBegin(),
                           this->m_vendors.constEnd(),
                           [&vendorId](const UvcVendor &vendor) {
        return vendor.id == vendorId;
    });

    if (it == this->m_vendors.constEnd())
        return;

    auto hDevInfo =
            SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE,
                                nullptr,
                                nullptr,
                                DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE)
        return;

    SP_DEVICE_INTERFACE_DATA interfaceData = {sizeof(SP_DEVICE_INTERFACE_DATA)};
    SP_DEVINFO_DATA devInfoData = {sizeof(SP_DEVINFO_DATA)};

    for (DWORD index = 0; ; ++index) {
        if (!SetupDiEnumDeviceInterfaces(hDevInfo,
                                         nullptr,
                                         &GUID_DEVINTERFACE_USB_DEVICE,
                                         index,
                                         &interfaceData)) {
            break;
        }

        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                        &interfaceData,
                                        nullptr,
                                        0,
                                        &requiredSize,
                                        nullptr);
        auto detailData =
                reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(requiredSize));

        if (!detailData)
            continue;

        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                             &interfaceData,
                                             detailData,
                                             requiredSize,
                                             nullptr,
                                             &devInfoData)) {
            free(detailData);

            continue;
        }

        if (QString::fromWCharArray(detailData->DevicePath).toLower() != devicePath.toLower()) {
            free(detailData);

            continue;
        }

        auto devHandle =
                CreateFile(detailData->DevicePath,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_FLAG_OVERLAPPED,
                           nullptr);

        if (devHandle == INVALID_HANDLE_VALUE) {
            free(detailData);

            continue;
        }

        WINUSB_INTERFACE_HANDLE winUsbHandle;

        if (!WinUsb_Initialize(devHandle, &winUsbHandle)) {
            CloseHandle(devHandle);
            free(detailData);

            continue;
        }

        for (const auto &product: it->products) {
            if (!product.ids.isEmpty() && !product.ids.contains(productId))
                continue;

            for (const auto &iface: product.interfaces) {
                USB_INTERFACE_DESCRIPTOR interfaceDesc;

                if (!WinUsb_QueryInterfaceSettings(winUsbHandle,
                                                   0,
                                                   &interfaceDesc))
                    continue;

                if (interfaceDesc.bInterfaceNumber != static_cast<UCHAR>(iface.num))
                    continue;

                auto unitId = this->m_extensions.value(iface.guid);

                for (const auto &control: iface.controls) {
                    if (control.uvcType != UVC_CTRL_DATA_TYPE_SIGNED
                        && control.uvcType != UVC_CTRL_DATA_TYPE_UNSIGNED
                        && control.uvcType != UVC_CTRL_DATA_TYPE_BOOLEAN) {
                        continue;
                    }

                    auto dataSize =
                            controlDataSize(winUsbHandle,
                                            iface.num,
                                            unitId,
                                            control.selector);

                    if (dataSize < 1 || dataSize != control.size)
                        continue;

                    this->m_uvcControls << UvcControlExt(control,
                                                         iface.num,
                                                         unitId);
                }
            }
        }

        WinUsb_Free(winUsbHandle);
        CloseHandle(devHandle);
        free(detailData);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
}

quint16 UvcExtendedControlsPrivate::controlDataSize(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                                    int interfaceNumber,
                                                    quint8 unitId,
                                                    quint8 selector) const
{
    quint8 data[2];
    WINUSB_SETUP_PACKET setupPacket = {
        0xA1,
        UVC_GET_LEN,
        static_cast<USHORT>(static_cast<USHORT>(selector) << 8U),
        static_cast<USHORT>((static_cast<USHORT>(unitId) << 8) | static_cast<USHORT>(interfaceNumber)),
        sizeof(data)
    };

    ULONG lengthTransferred;

    if (!WinUsb_ControlTransfer(winUsbHandle,
                                setupPacket,
                                data,
                                sizeof(data),
                                &lengthTransferred,
                                nullptr)) {
        return 0;
    }

    return (data[1] << 8) | data[0];
}

int UvcExtendedControlsPrivate::queryControl(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                             int interfaceNumber,
                                             quint8 unitId,
                                             quint8 selector,
                                             quint8 query,
                                             void *data,
                                             quint16 dataSize) const
{
    WINUSB_SETUP_PACKET setupPacket = {
        static_cast<UCHAR>(query == UVC_SET_CUR? 0x21: 0xA1),
        query,
        static_cast<USHORT>(static_cast<USHORT>(selector) << 8U),
        static_cast<USHORT>((static_cast<USHORT>(unitId) << 8) | static_cast<USHORT>(interfaceNumber)),
        dataSize
    };

    ULONG lengthTransferred;

    if (!WinUsb_ControlTransfer(winUsbHandle,
                                setupPacket,
                                (PUCHAR)data,
                                dataSize,
                                &lengthTransferred,
                                nullptr)) {
        return -1;
    }

    return static_cast<int>(lengthTransferred);
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

QVariantList UvcExtendedControlsPrivate::readControlSigned(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                                           int interfaceNumber,
                                                           quint8 unitId,
                                                           const UvcControl &control) const
{
    auto dataSize = this->controlDataSize(winUsbHandle,
                                          interfaceNumber,
                                          unitId,
                                          control.selector);

    if (dataSize < 1 || dataSize != control.size)
        return {};

    QMap<int, qint32> values {
        {UVC_GET_CUR, 0},
        {UVC_GET_DEF, 0},
        {UVC_GET_MIN, 0},
        {UVC_GET_MAX, 0},
        {UVC_GET_RES, 0},
    };

    for (auto it = values.begin(); it != values.end(); it++) {
        QByteArray rawData(dataSize, '\0');

        if (this->queryControl(winUsbHandle,
                               interfaceNumber,
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
            qMax<quint32>(values[UVC_GET_RES], 1),
            values[UVC_GET_DEF],
            values[UVC_GET_CUR],
            QVariantList()};
}

QVariantList UvcExtendedControlsPrivate::readControlUnsigned(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                                             int interfaceNumber,
                                                             quint8 unitId,
                                                             const UvcControl &control) const
{
    auto dataSize = controlDataSize(winUsbHandle,
                                    interfaceNumber,
                                    unitId,
                                    control.selector);

    if (dataSize < 1 || dataSize != control.size)
        return {};

    QMap<int, quint32> values {
        {UVC_GET_CUR, 0},
        {UVC_GET_DEF, 0},
        {UVC_GET_MIN, 0},
        {UVC_GET_MAX, 0},
        {UVC_GET_RES, 0},
    };

    for (auto it = values.begin(); it != values.end(); it++) {
        QByteArray rawData(dataSize, '\0');

        if (this->queryControl(winUsbHandle,
                               interfaceNumber,
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
            qMax<quint32>(values[UVC_GET_RES], 1),
            values[UVC_GET_DEF],
            values[UVC_GET_CUR],
            QVariantList()};
}

QVariantList UvcExtendedControlsPrivate::readControlBoolean(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                                            int interfaceNumber,
                                                            quint8 unitId,
                                                            const UvcControl &control) const
{
    auto dataSize = controlDataSize(winUsbHandle,
                                    interfaceNumber,
                                    unitId,
                                    control.selector);

    if (dataSize < 1 || dataSize != control.size)
        return {};

    QMap<int, bool> values {
        {UVC_GET_CUR, false},
        {UVC_GET_DEF, false},
        {UVC_GET_MIN, false},
        {UVC_GET_MAX, false},
        {UVC_GET_RES, false},
    };

    for (auto it = values.begin(); it != values.end(); it++) {
        QByteArray rawData(dataSize, '\0');

        if (this->queryControl(winUsbHandle,
                               interfaceNumber,
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

QVariantList UvcExtendedControlsPrivate::readControlMenu(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                                         int interfaceNumber,
                                                         quint8 unitId,
                                                         const UvcControl &control) const
{
    auto dataSize = controlDataSize(winUsbHandle,
                                    interfaceNumber,
                                    unitId,
                                    control.selector);

    if (dataSize < 1 || dataSize != control.size)
        return {};

    QMap<int, quint32> values {
        {UVC_GET_CUR, 0},
        {UVC_GET_DEF, 0},
        {UVC_GET_MIN, 0},
        {UVC_GET_MAX, 0},
        {UVC_GET_RES, 0},
    };

    for (auto it = values.begin(); it != values.end(); it++) {
        QByteArray rawData(dataSize, '\0');

        if (this->queryControl(winUsbHandle,
                               interfaceNumber,
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
                           [&values] (const UvcMenuOption &option) {
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

QByteArray UvcExtendedControlsPrivate::writeValueU(const UvcControl &control,
                                                   const QBitArray &curValue,
                                                   quint32 value) const
{
    auto curValueCopy = curValue;

    for (int i = 0; i < control.readSize; i++)
        curValueCopy[i + control.offset] = (value >> i) & 0x1;

    return QByteArray(reinterpret_cast<const char *>(curValueCopy.bits()), control.size);
}

QByteArray UvcExtendedControlsPrivate::writeValueS(const UvcControl &control,
                                                   const QBitArray &curValue,
                                                   qint32 value) const
{
    return this->writeValueU(control,
                             curValue,
                             quint32(quint64(value) - std::numeric_limits<qint32>::min()));
}

bool UvcExtendedControlsPrivate::writeControlSigned(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                                    int interfaceNumber,
                                                    quint8 unitId,
                                                    const UvcControl &control,
                                                    qint32 value) const
{
    return this->writeControlUnsigned(winUsbHandle,
                                      interfaceNumber,
                                      unitId,
                                      control,
                                      quint32(quint64(value) - std::numeric_limits<qint32>::min()));
}

bool UvcExtendedControlsPrivate::writeControlUnsigned(WINUSB_INTERFACE_HANDLE winUsbHandle,
                                                      int interfaceNumber,
                                                      quint8 unitId,
                                                      const UvcControl &control,
                                                      quint32 value) const
{
    auto dataSize = this->controlDataSize(winUsbHandle,
                                          interfaceNumber,
                                          unitId,
                                          control.selector);

    if (dataSize < 1 || dataSize != control.size)
        return false;

    QByteArray rawData(dataSize, '\0');

    if (this->queryControl(winUsbHandle,
                           interfaceNumber,
                           unitId,
                           control.selector,
                           UVC_GET_CUR,
                           rawData.data(), dataSize) < 0) {
        return false;
    }

    auto curValue =
            QBitArray::fromBits(rawData.constData(), 8 * rawData.size());

    if (control.ctrlType == CTRL_TYPE_MENU) {
        value = value < control.menu.size()?
                    control.menu[value].value.toUInt():
                control.menu.size() > 0?
                    control.menu[0].value.toUInt():
                    0;
    }

    auto data = this->writeValueU(control, curValue, value);

    if (this->queryControl(winUsbHandle,
                           interfaceNumber,
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

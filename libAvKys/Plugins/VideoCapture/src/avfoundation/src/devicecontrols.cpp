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

#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>

#include "devicecontrols.h"

#define INTERFACE 0x4

// Video Interface Class Code
#define CC_VIDEO 0xe

// Video Class-Specific Descriptor Types
#define CS_INTERFACE 0x24
#define CS_ENDPOINT  0x25

// Video Interface Subclass Codes
#define SC_UNDEFINED      0x0
#define SC_VIDEOCONTROL   0x1
#define SC_VIDEOSTREAMING 0x2

// Video Class-Specific VC Interface Descriptor Subtypes
#define VC_INPUT_TERMINAL  0x2
#define VC_PROCESSING_UNIT 0x5

// Processing Unit Control Selectors
#define PU_CONTROL_UNDEFINED                      0x00
#define PU_BACKLIGHT_COMPENSATION_CONTROL         0x01
#define PU_BRIGHTNESS_CONTROL                     0x02
#define PU_CONTRAST_CONTROL                       0x03
#define PU_GAIN_CONTROL                           0x04
#define PU_POWER_LINE_FREQUENCY_CONTROL           0x05
#define PU_HUE_CONTROL                            0x06
#define PU_SATURATION_CONTROL                     0x07
#define PU_SHARPNESS_CONTROL                      0x08
#define PU_GAMMA_CONTROL                          0x09
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL      0x0a
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0b
#define PU_WHITE_BALANCE_COMPONENT_CONTROL        0x0c
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL   0x0d
#define PU_DIGITAL_MULTIPLIER_CONTROL             0x0e
#define PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL       0x0f
#define PU_HUE_AUTO_CONTROL                       0x10
#define PU_ANALOG_VIDEO_STANDARD_CONTROL          0x11
#define PU_ANALOG_LOCK_STATUS_CONTROL             0x12
#define PU_CONTRAST_AUTO_CONTROL                  0x13

// Camera Terminal Control Selectors
#define CT_CONTROL_UNDEFINED              0x00
#define CT_SCANNING_MODE_CONTROL          0x01
#define CT_AE_MODE_CONTROL                0x02
#define CT_AE_PRIORITY_CONTROL            0x03
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL 0x04
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL 0x05
#define CT_FOCUS_ABSOLUTE_CONTROL         0x06
#define CT_FOCUS_RELATIVE_CONTROL         0x07
#define CT_FOCUS_AUTO_CONTROL             0x08
#define CT_IRIS_ABSOLUTE_CONTROL          0x09
#define CT_IRIS_RELATIVE_CONTROL          0x0a
#define CT_ZOOM_ABSOLUTE_CONTROL          0x0b
#define CT_ZOOM_RELATIVE_CONTROL          0x0c
#define CT_PANTILT_ABSOLUTE_CONTROL       0x0d
#define CT_PANTILT_RELATIVE_CONTROL       0x0e
#define CT_ROLL_ABSOLUTE_CONTROL          0x0f
#define CT_ROLL_RELATIVE_CONTROL          0x10
#define CT_PRIVACY_CONTROL                0x11
#define CT_FOCUS_SIMPLE_CONTROL           0x12
#define CT_WINDOW_CONTROL                 0x13
#define CT_REGION_OF_INTEREST_CONTROL     0x14

// Video Class-Specific Request Codes
#define RC_UNDEFINED 0x00
#define SET_CUR      0x01
#define SET_CUR_ALL  0x11
#define GET_CUR      0x81
#define GET_MIN      0x82
#define GET_MAX      0x83
#define GET_RES      0x84
#define GET_LEN      0x85
#define GET_INFO     0x86
#define GET_DEF      0x87
#define GET_CUR_ALL  0x91
#define GET_MIN_ALL  0x92
#define GET_MAX_ALL  0x93
#define GET_RES_ALL  0x94
#define GET_DEF_ALL  0x97

#define CAMERA_TERMINAL 0x0
#define PROCESSING_UNIT 0x1

#define EP_INTERRUPT 0x3

struct GenericDescriptor
{
    quint8 bLength;
    quint8 bDescriptorType;
};

struct StandardVCInterfaceDescriptor
{
    quint8 bLength;
    quint8 bDescriptorType;
    quint8 bInterfaceNumber;
    quint8 bAlternateSetting;
    quint8 bNumEndpoints;
    quint8 bInterfaceClass;
    quint8 bInterfaceSubClass;
    quint8 bInterfaceProtocol;
    qint8  iInterface;
};

struct CsInterfaceDescriptor
{
    quint8 bLength;
    quint8 bDescriptorType;
    quint8 bDescriptorSubtype;
};

struct VcProcessingUnit
{
    quint8  bLength;
    quint8  bDescriptorType;
    quint8  bDescriptorSubtype;
    quint8  bUnitID;
};

struct VcInputTerminal
{
    quint8  bLength;
    quint8  bDescriptorType;
    quint8  bDescriptorSubtype;
    quint8  bTerminalID;
};

class UvcControl
{
    public:
        quint16 controlType;
        quint16 selector;
        QString description;
        QString type;
        bool signd;
        QStringList menu;

        inline static const QVector<UvcControl> &controls();
        static inline const UvcControl *bySelector(quint16 controlType,
                                                   quint16 selector);
        static inline QVector<quint16> allSelectors(quint16 controlType);
};

using USBInterfacePtr = IOUSBInterfaceInterface182 *;

class DeviceControlsPrivate
{
    public:
        USBInterfacePtr *m_interface {nullptr};
        quint8 m_bInterfaceNumber {0};
        quint8 m_bUnitID {0};
        quint8 m_bTerminalID {0};

        USBInterfacePtr *controlInterface(IOUSBDeviceInterface **deviceInterface);
        void readConfigurationDescription(IOUSBDeviceInterface **deviceInterface);
        int sendRequest(USBInterfacePtr *interface,
                        IOUSBDevRequest &request) const;
        template<typename T>
        int readData(USBInterfacePtr *interface,
                     quint8 request,
                     quint16 controlType,
                     quint16 selector,
                     T *data) const;
        template<typename T>
        int writeData(USBInterfacePtr *interface,
                      quint16 controlType,
                      quint16 selector,
                      T *data) const;
        QVariantList controlsList(USBInterfacePtr *interface,
                                  quint16 controlID,
                                  quint16 request,
                                  quint16 selector) const;
        int setControls(USBInterfacePtr *interface,
                        quint16 controlID,
                        quint16 request,
                        quint16 selector,
                        const QVariantMap &values) const;
};

DeviceControls::DeviceControls()
{
    this->d = new DeviceControlsPrivate;
}

DeviceControls::~DeviceControls()
{
    this->close();
    delete this->d;
}

bool DeviceControls::open(const QString &deviceId)
{
    bool ok = false;
    auto deviceLocationId = quint32(deviceId.toULongLong(&ok, 16) >> 32);

    if (!ok)
        return false;

    auto serviceMatching = IOServiceMatching(kIOUSBDeviceClassName);
    io_iterator_t serviceIterator;
    IOServiceGetMatchingServices(kIOMasterPortDefault,
                                 serviceMatching,
                                 &serviceIterator);

    while (auto device = IOIteratorNext(serviceIterator)) {
        IOCFPlugInInterface **pluginInterface = nullptr;
        SInt32 score = 0;
        auto result = IOCreatePlugInInterfaceForService(device,
                                                        kIOUSBDeviceUserClientTypeID,
                                                        kIOCFPlugInInterfaceID,
                                                        &pluginInterface,
                                                        &score);

        if (result != kIOReturnSuccess || !pluginInterface)
            continue;

        IOUSBDeviceInterface **deviceInterface = nullptr;
        auto hr = (*pluginInterface)->QueryInterface(pluginInterface,
                                                     CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
                                                     reinterpret_cast<LPVOID *>(&deviceInterface));

        if (hr != 0 || !deviceInterface)
            continue;

        UInt32 locationId = 0;
        (*deviceInterface)->GetLocationID(deviceInterface, &locationId);

        if (locationId == deviceLocationId) {
            this->d->m_interface = this->d->controlInterface(deviceInterface);
            this->d->readConfigurationDescription(deviceInterface);

            return true;
        }
    }

    return false;
}

void DeviceControls::close()
{
    if (this->d->m_interface) {
        (*this->d->m_interface)->USBInterfaceClose(this->d->m_interface);
        (*this->d->m_interface)->Release(this->d->m_interface);
        this->d->m_interface = nullptr;
    }
}

QVariantList DeviceControls::imageControls()
{
    if (!this->d->m_interface)
        return {};

    QVariantList deviceControls;

    for (auto &control: UvcControl::allSelectors(PROCESSING_UNIT)) {
        if (control == PU_CONTROL_UNDEFINED)
            continue;

        auto controls =
                this->d->controlsList(this->d->m_interface,
                                      this->d->m_bUnitID,
                                      PROCESSING_UNIT,
                                      control);

        if (!controls.isEmpty())
            deviceControls << QVariant(controls);
    }

    return deviceControls;
}

QVariantList DeviceControls::cameraControls()
{
    if (!this->d->m_interface)
        return {};

    QVariantList deviceControls;

    for (auto &control: UvcControl::allSelectors(CAMERA_TERMINAL)) {
        if (control == CT_CONTROL_UNDEFINED)
            continue;

        auto controls =
                this->d->controlsList(this->d->m_interface,
                                      this->d->m_bTerminalID,
                                      CAMERA_TERMINAL,
                                      control);

        if (!controls.isEmpty())
            deviceControls << QVariant(controls);
    }

    return deviceControls;
}

void DeviceControls::setImageControls(const QVariantMap &imageControls)
{
    if (!this->d->m_interface)
        return;

    for (auto &control: UvcControl::allSelectors(PROCESSING_UNIT))
        this->d->setControls(this->d->m_interface,
                             this->d->m_bUnitID,
                             PROCESSING_UNIT,
                             control,
                             imageControls);
}

void DeviceControls::setCameraControls(const QVariantMap &cameraControls)
{
    if (!this->d->m_interface)
        return;

    for (auto &control: UvcControl::allSelectors(CAMERA_TERMINAL))
        this->d->setControls(this->d->m_interface,
                             this->d->m_bTerminalID,
                             CAMERA_TERMINAL,
                             control,
                             cameraControls);
}

USBInterfacePtr *DeviceControlsPrivate::controlInterface(IOUSBDeviceInterface **deviceInterface)
{
    io_iterator_t interfaceIterator;
    IOUSBFindInterfaceRequest interfaceRequest;
    interfaceRequest.bInterfaceClass = CC_VIDEO;
    interfaceRequest.bInterfaceSubClass = SC_VIDEOCONTROL;
    interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    auto hr = (*deviceInterface)->CreateInterfaceIterator(deviceInterface,
                                                          &interfaceRequest,
                                                          &interfaceIterator);

    if (hr != kIOReturnSuccess)
        return nullptr;

    auto usbInterface = IOIteratorNext(interfaceIterator);

    if (!usbInterface)
        return nullptr;

    IOCFPlugInInterface **pluginInterface = nullptr;
    SInt32 score = 0;
    auto result = IOCreatePlugInInterfaceForService(usbInterface,
                                                    kIOUSBInterfaceUserClientTypeID,
                                                    kIOCFPlugInInterfaceID,
                                                    &pluginInterface,
                                                    &score);
    IOObjectRelease(usbInterface);

    if (result != 0 || !pluginInterface)
        return nullptr;

    USBInterfacePtr *controlInterface = nullptr;
    hr = (*pluginInterface)->QueryInterface(pluginInterface,
                                            CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID),
                                            reinterpret_cast<LPVOID *>(&controlInterface));
    (*pluginInterface)->Release(pluginInterface);

    if (result != 0 || !controlInterface)
        return nullptr;

    return controlInterface;
}

void DeviceControlsPrivate::readConfigurationDescription(IOUSBDeviceInterface **deviceInterface)
{
    IOUSBConfigurationDescriptorPtr configDescriptor = nullptr;
    auto result =
            (*deviceInterface)->GetConfigurationDescriptorPtr(deviceInterface,
                                                              0,
                                                              &configDescriptor);

    if (result != kIOReturnSuccess)
        return;

    auto ptr = reinterpret_cast<quint8 *>(configDescriptor);
    bool endPoint = false;

    for (size_t bytes = configDescriptor->bLength;
         bytes < configDescriptor->wTotalLength && !endPoint;) {
        auto descriptor = reinterpret_cast<GenericDescriptor *>(ptr + bytes);

        switch (descriptor->bDescriptorType) {
        case INTERFACE: {
            auto desc = reinterpret_cast<StandardVCInterfaceDescriptor *>(descriptor);

            if (desc->bInterfaceClass == CC_VIDEO
                && desc->bInterfaceSubClass == SC_VIDEOSTREAMING)
                this->m_bInterfaceNumber = desc->bInterfaceNumber;

            break;
        }

        case CS_INTERFACE: {
            auto desc = reinterpret_cast<CsInterfaceDescriptor *>(descriptor);

            switch (desc->bDescriptorSubtype) {
            case VC_PROCESSING_UNIT: {
                auto puDesc = reinterpret_cast<VcProcessingUnit *>(desc);
                this->m_bUnitID = puDesc->bUnitID;

                break;
            }

            case VC_INPUT_TERMINAL: {
                auto itDesc = reinterpret_cast<VcInputTerminal *>(desc);
                this->m_bTerminalID = itDesc->bTerminalID;

                break;
            }

            default:
                break;
            }

            break;
        }

        case CS_ENDPOINT: {
            auto desc = reinterpret_cast<CsInterfaceDescriptor *>(descriptor);

            if (desc->bDescriptorSubtype == EP_INTERRUPT)
                endPoint = true;

            break;
        }

        default:
            break;
        }

        bytes += descriptor->bLength;
    }
}

int DeviceControlsPrivate::sendRequest(USBInterfacePtr *interface,
                                       IOUSBDevRequest &request) const
{
    if (!interface)
        return kIOReturnError;

    auto hr = (*interface)->USBInterfaceOpen(interface);

    if (hr != kIOReturnSuccess)
        return hr;

    hr = (*interface)->ControlRequest(interface, 0, &request);
    (*interface)->USBInterfaceClose(interface);

    return hr;
}

template<typename T>
int DeviceControlsPrivate::readData(USBInterfacePtr *interface,
                                    quint8 request,
                                    quint16 controlType,
                                    quint16 selector,
                                    T *data) const
{
    IOUSBDevRequest controlRequest;
    controlRequest.bmRequestType =
            USBmakebmRequestType(kUSBIn, kUSBClass, kUSBInterface);
    controlRequest.bRequest = request;
    controlRequest.wValue = UInt16(selector << 8);
    controlRequest.wIndex = UInt16(controlType << 8) | this->m_bInterfaceNumber;
    controlRequest.wLength = sizeof(T);
    controlRequest.wLenDone = 0;
    controlRequest.pData = reinterpret_cast<void *>(data);

    return this->sendRequest(interface, controlRequest);
}

template<typename T>
int DeviceControlsPrivate::writeData(USBInterfacePtr *interface,
                                     quint16 controlType,
                                     quint16 selector,
                                     T *data) const
{    
    IOUSBDevRequest controlRequest;
    controlRequest.bmRequestType =
            USBmakebmRequestType(kUSBOut, kUSBClass, kUSBInterface);
    controlRequest.bRequest = SET_CUR;
    controlRequest.wValue = UInt16(selector << 8);
    controlRequest.wIndex = UInt16(controlType << 8) | this->m_bInterfaceNumber;
    controlRequest.wLength = sizeof(T);
    controlRequest.wLenDone = 0;
    controlRequest.pData = reinterpret_cast<void *>(data);

    /* FIXME: For some unknown reason 'ControlRequest' always returns
     * kIOUSBPipeStalled, so 'writeData' fails and it's not possible to set the
     * control value.
     */

    return this->sendRequest(interface, controlRequest);
}

QVariantList DeviceControlsPrivate::controlsList(USBInterfacePtr *interface,
                                                 quint16 controlID,
                                                 quint16 request,
                                                 quint16 selector) const
{
    auto control = UvcControl::bySelector(request, selector);
    int min = 0;
    int max = 0;
    int step = 0;
    int defaultValue = 0;
    int value = 0;

    if (control->type == "integer") {
        if (control->signd) {
            int16_t val = 0;

            if (this->readData(interface, GET_CUR, controlID, selector, &val) != kIOReturnSuccess)
                return {};

            value = val;
            this->readData(interface, GET_MIN, controlID, selector, &val);
            min = val;
            this->readData(interface, GET_MAX, controlID, selector, &val);
            max = val;
            this->readData(interface, GET_RES, controlID, selector, &val);
            step = val;
            this->readData(interface, GET_DEF, controlID, selector, &val);
            defaultValue = val;
        } else {
            uint16_t val = 0;

            if (this->readData(interface, GET_CUR, controlID, selector, &val) != kIOReturnSuccess)
                return {};

            value = val;
            this->readData(interface, GET_MIN, controlID, selector, &val);
            min = val;
            this->readData(interface, GET_MAX, controlID, selector, &val);
            max = val;
            this->readData(interface, GET_RES, controlID, selector, &val);
            step = val;
            this->readData(interface, GET_DEF, controlID, selector, &val);
            defaultValue = val;
        }
    } else if (control->type == "boolean") {
        uint8_t val = false;

        if (this->readData(interface, GET_CUR, controlID, selector, &val) != kIOReturnSuccess)
            return {};

        value = val;
        this->readData(interface, GET_MIN, controlID, selector, &val);
        min = val;
        this->readData(interface, GET_MAX, controlID, selector, &val);
        max = val;
        this->readData(interface, GET_RES, controlID, selector, &val);
        step = val;
        this->readData(interface, GET_DEF, controlID, selector, &val);
        defaultValue = val;
    } else if (control->type == "menu") {
        uint8_t val = 0;

        if (this->readData(interface, GET_CUR, controlID, selector, &val) != kIOReturnSuccess)
            return {};

        value = val;
        this->readData(interface, GET_MIN, controlID, selector, &val);
        min = val;
        this->readData(interface, GET_MAX, controlID, selector, &val);
        max = val;
        this->readData(interface, GET_RES, controlID, selector, &val);
        step = val;
        this->readData(interface, GET_DEF, controlID, selector, &val);
        defaultValue = val;
    }

    return QVariantList {
        control->description,
        control->type,
        min,
        max,
        step,
        defaultValue,
        value,
        control->menu
    };
}

int DeviceControlsPrivate::setControls(USBInterfacePtr *interface,
                                       quint16 controlID,
                                       quint16 request,
                                       quint16 selector,
                                       const QVariantMap &values) const
{
    auto control = UvcControl::bySelector(request, selector);

    if (!values.contains(control->description))
        return kIOReturnError;

    int result = kIOReturnError;

    if (control->type == "integer") {
        if (control->signd) {
            auto val = int16_t(values[control->description].toInt());
            result = this->writeData(interface, controlID, selector, &val);
        } else {
            auto val = uint16_t(values[control->description].toUInt());
            result = this->writeData(interface, controlID, selector, &val);
        }
    } else if (control->type == "boolean") {
        uint8_t val = values[control->description].toBool();
        result = this->writeData(interface, controlID, selector, &val);
    } else if (control->type == "menu") {
        auto val = uint8_t(values[control->description].toUInt());
        result = this->writeData(interface, controlID, selector, &val);
    }

    return result;
}

const QVector<UvcControl> &UvcControl::controls()
{
    static const QVector<UvcControl> controls {
        // Processing Units
        {PROCESSING_UNIT, PU_CONTROL_UNDEFINED                     , ""                              , ""       , false, {}},
        {PROCESSING_UNIT, PU_BACKLIGHT_COMPENSATION_CONTROL        , "Backlight Compensation"        , "integer", false, {}},
        {PROCESSING_UNIT, PU_BRIGHTNESS_CONTROL                    , "Brightness"                    , "integer", true , {}},
        {PROCESSING_UNIT, PU_CONTRAST_CONTROL                      , "Contrast"                      , "integer", false, {}},
        {PROCESSING_UNIT, PU_GAIN_CONTROL                          , "Gain"                          , "integer", false, {}},
        {PROCESSING_UNIT, PU_POWER_LINE_FREQUENCY_CONTROL          , "Power Line Frequency"          , "menu"   , false, {"Disabled",
                                                                                                                          "50 Hz",
                                                                                                                          "60 Hz",
                                                                                                                          "Auto"}},
        {PROCESSING_UNIT, PU_HUE_CONTROL                           , "Hue"                           , "integer", true , {}},
        {PROCESSING_UNIT, PU_SATURATION_CONTROL                    , "Saturation"                    , "integer", false, {}},
        {PROCESSING_UNIT, PU_SHARPNESS_CONTROL                     , "Sharpness"                     , "integer", false, {}},
        {PROCESSING_UNIT, PU_GAMMA_CONTROL                         , "Gamma"                         , "integer", false, {}},
        {PROCESSING_UNIT, PU_WHITE_BALANCE_TEMPERATURE_CONTROL     , "White Balance Temperature"     , "integer", false, {}},
        {PROCESSING_UNIT, PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL, "White Balance Temperature Auto", "boolean", false, {}},
        //{PROCESSING_UNIT, PU_WHITE_BALANCE_COMPONENT_CONTROL       , "White Balance Component"       , "integer", false, {}},
        {PROCESSING_UNIT, PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL  , "White Balance Component Auto"  , "boolean", false, {}},
        {PROCESSING_UNIT, PU_DIGITAL_MULTIPLIER_CONTROL            , "Digital Multiplier"            , "integer", false, {}},
        {PROCESSING_UNIT, PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL      , "Digital Multiplier Limit"      , "integer", false, {}},
        {PROCESSING_UNIT, PU_HUE_AUTO_CONTROL                      , "Hue Auto"                      , "boolean", false, {}},
        {PROCESSING_UNIT, PU_ANALOG_VIDEO_STANDARD_CONTROL         , "Analog Video Standard"         , "menu"   , false, {"None",
                                                                                                                          "NTSC - 525/60",
                                                                                                                          "PAL - 625/50",
                                                                                                                          "SECAM - 625/50",
                                                                                                                          "NTSC - 625/50",
                                                                                                                          "PAL - 525/60"}},
        {PROCESSING_UNIT, PU_ANALOG_LOCK_STATUS_CONTROL            , "Analog Lock Status"            , "menu"   , false, {"Locked",
                                                                                                                          "Unlocked"}},
        {PROCESSING_UNIT, PU_CONTRAST_AUTO_CONTROL                 , "Contrast Auto"                 , "boolean", false, {}},

        // Camera Terminals
        {CAMERA_TERMINAL, CT_CONTROL_UNDEFINED                     , ""                              , ""       , false, {}},
        {CAMERA_TERMINAL, CT_SCANNING_MODE_CONTROL                 , "Scanning Mode"                 , "boolean", false, {}},
        //{CAMERA_TERMINAL, CT_AE_MODE_CONTROL                       , "AE Mode"                       , "", false, {}},
        //{CAMERA_TERMINAL, CT_AE_PRIORITY_CONTROL                   , "AE Priority"                   , "", false, {}},
        //{CAMERA_TERMINAL, CT_EXPOSURE_TIME_ABSOLUTE_CONTROL        , "Exposure Time Absolute"        , "", false, {}},
        //{CAMERA_TERMINAL, CT_EXPOSURE_TIME_RELATIVE_CONTROL        , "Exposure Time Relative"        , "", false, {}},
        {CAMERA_TERMINAL, CT_FOCUS_ABSOLUTE_CONTROL                , "Focus Absolute"                , "integer", false, {}},
        //{CAMERA_TERMINAL, CT_FOCUS_RELATIVE_CONTROL                , "Focus Relative"                , "", false, {}},
        {CAMERA_TERMINAL, CT_FOCUS_AUTO_CONTROL                    , "Focus Auto"                    , "boolean", false, {}},
        {CAMERA_TERMINAL, CT_IRIS_ABSOLUTE_CONTROL                 , "Iris Absolute"                 , "integer", false, {}},
        //{CAMERA_TERMINAL, CT_IRIS_RELATIVE_CONTROL                 , "Iris Relative"                 , "", false, {}},
        {CAMERA_TERMINAL, CT_ZOOM_ABSOLUTE_CONTROL                 , "Zoom Absolute"                 , "integer", false, {}},
        //{CAMERA_TERMINAL, CT_ZOOM_RELATIVE_CONTROL                 , "Zoom Relative"                 , "", false, {}},
        //{CAMERA_TERMINAL, CT_PANTILT_ABSOLUTE_CONTROL              , "Pantilt Absolute"              , "", false, {}},
        //{CAMERA_TERMINAL, CT_PANTILT_RELATIVE_CONTROL              , "Pantilt Relative"              , "", false, {}},
        {CAMERA_TERMINAL, CT_ROLL_ABSOLUTE_CONTROL                 , "Roll Absolute"                 , "integer", true, {}},
        //{CAMERA_TERMINAL, CT_ROLL_RELATIVE_CONTROL                 , "Roll Relative"                 , "", false, {}},
        {CAMERA_TERMINAL, CT_PRIVACY_CONTROL                       , "Privacy"                       , "boolean", false, {}},
        {CAMERA_TERMINAL, CT_FOCUS_SIMPLE_CONTROL                  , "Focus Simple"                  , "menu"   , false, {"Full Range",
                                                                                                                          "Macro",
                                                                                                                          "People",
                                                                                                                          "Scene"}},
        //{CAMERA_TERMINAL, CT_DIGITAL_WINDOW_CONTROL                , "Digital Window"                , "", false, {}},
        //{CAMERA_TERMINAL, CT_REGION_OF_INTEREST_CONTROL            , "Region of Interest"            , "", false, {}}
    };

    return controls;
}

const UvcControl *UvcControl::bySelector(quint16 controlType, quint16 selector)
{
    for (auto &control: controls())
        if (control.controlType == controlType
            && control.selector == selector)
            return &control;

    // Returns default for control type.
    for (auto &control: controls())
        if (control.controlType == controlType)
            return &control;

    return &controls().first();
}

QVector<quint16> UvcControl::allSelectors(quint16 controlType)
{
    QVector<quint16> selectors;

    for (auto &control: controls())
        if (control.controlType == controlType)
            selectors << control.selector;

    return selectors;
}

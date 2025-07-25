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

#include <windows.h>
#include <ks.h>
#include <ksproxy.h>
#include <comdef.h>

#include "uvcextendedcontrols.h"

#define AK_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = {l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}

AK_DEFINE_GUID(KSPROPERTYSETID_ExtendedCameraControl, 0x1cb79112, 0xb8f4, 0x4666, 0x80, 0xb3, 0x57, 0x9e, 0x35, 0x1f, 0x3c, 0x16);

#define KSPROPERTY_CAMERACONTROL_EXTENDED_SCENEMODE           7
#define KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSPRIORITY       19
#define KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM                24
#define KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOSTABILIZATION  27
#define KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOHDR            30
#define KSPROPERTY_CAMERACONTROL_EXTENDED_OIS                 32
#define KSPROPERTY_CAMERACONTROL_EXTENDED_ADVANCEDPHOTO       33
#define KSPROPERTY_CAMERACONTROL_EXTENDED_END                 0xFFFFFFFF

#define KSCAMERA_EXTENDEDPROP_SCENEMODE_AUTO           0x0ULL
#define KSCAMERA_EXTENDEDPROP_SCENEMODE_MACRO          0x1ULL
#define KSCAMERA_EXTENDEDPROP_SCENEMODE_PORTRAIT       0x2ULL
#define KSCAMERA_EXTENDEDPROP_SCENEMODE_SPORT          0x4ULL
#define KSCAMERA_EXTENDEDPROP_SCENEMODE_SNOW           0x8ULL
#define KSCAMERA_EXTENDEDPROP_SCENEMODE_NIGHT          0x10ULL
#define KSCAMERA_EXTENDEDPROP_SCENEMODE_BEACH          0x20ULL
#define KSCAMERA_EXTENDEDPROP_SCENEMODE_SUNSET         0x40ULL
#define KSCAMERA_EXTENDEDPROP_SCENEMODE_CANDLELIGHT    0x80ULL
#define KSCAMERA_EXTENDEDPROP_SCENEMODE_LANDSCAPE      0x100ULL
#define KSCAMERA_EXTENDEDPROP_SCENEMODE_NIGHTPORTRAIT  0x200ULL
#define KSCAMERA_EXTENDEDPROP_SCENEMODE_BACKLIT        0x400ULL
#define KSCAMERA_EXTENDEDPROP_SCENEMODE_MANUAL         0x80000000000000ULL

#define KSCAMERA_EXTENDEDPROP_FOCUSPRIORITY_OFF  0ULL
#define KSCAMERA_EXTENDEDPROP_FOCUSPRIORITY_ON   1ULL

#define KSCAMERA_EXTENDEDPROP_ZOOM_DEFAULT  0ULL
#define KSCAMERA_EXTENDEDPROP_ZOOM_DIRECT   1ULL
#define KSCAMERA_EXTENDEDPROP_ZOOM_SMOOTH   2ULL

#define KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_OFF   0ULL
#define KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_ON    1ULL
#define KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_AUTO  2ULL

#define KSCAMERA_EXTENDEDPROP_VIDEOHDR_OFF   0ULL
#define KSCAMERA_EXTENDEDPROP_VIDEOHDR_ON    1ULL
#define KSCAMERA_EXTENDEDPROP_VIDEOHDR_AUTO  2ULL

#define KSCAMERA_EXTENDEDPROP_OIS_OFF   0ULL
#define KSCAMERA_EXTENDEDPROP_OIS_ON    1ULL
#define KSCAMERA_EXTENDEDPROP_OIS_AUTO  2ULL

#define KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_OFF            0x0ULL
#define KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_AUTO           0x1ULL
#define KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_HDR            0x2ULL
#define KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_FNF            0x4ULL
#define KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_ULTRALOWLIGHT  0x8ULL

#define KSCAMERA_EXTENDEDPROP_FILTERSCOPE  0xFFFFFFFF

struct KSCAMERA_EXTENDEDPROP_HEADER
{
    ULONG Version;
    ULONG PinId;
    ULONG Size;
    ULONG Result;
    ULONGLONG Flags;
    ULONGLONG Capability;
};

struct KSCAMERA_EXTENDEDPROP_VALUE
{
    union
    {
        double dbl;
        ULONGLONG ull;
        ULONG ul;
        ULARGE_INTEGER ratio;
        LONG l;
        LONGLONG ll;
    } Value;
};

struct KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING
{
    ULONG Mode;
    LONG Min;
    LONG Max;
    LONG Step;
    KSCAMERA_EXTENDEDPROP_VALUE VideoProc;
    ULONGLONG Reserved;
};

union VariantValue
{
    ULONGLONG ull;
    LONG l;
};

struct MenuOption
{
    VariantValue value;
    const char *description;
};

struct UvcExtendedControl
{
    ULONG id;
    ULONG pinId;
    const char *description;
    MenuOption menu[32];

    inline static const UvcExtendedControl *table()
    {
        static const UvcExtendedControl captureMFUvcExtendedControlMap[] = {
            {KSPROPERTY_CAMERACONTROL_EXTENDED_SCENEMODE         , KSCAMERA_EXTENDEDPROP_FILTERSCOPE, "Scene Mode"                 , {
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_AUTO         , "Auto"          },
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_MACRO        , "Macro"         },
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_PORTRAIT     , "Portrait"      },
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_SPORT        , "Sport"         },
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_SNOW         , "Snow"          },
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_NIGHT        , "Night"         },
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_BEACH        , "Beach"         },
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_SUNSET       , "Sunset"        },
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_CANDLELIGHT  , "Candle Light"  },
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_LANDSCAPE    , "Landscape"     },
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_NIGHTPORTRAIT, "Night Portrait"},
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_BACKLIT      , "Backlit"       },
                {KSCAMERA_EXTENDEDPROP_SCENEMODE_MANUAL       , "Manual"        },
             }},
            {KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSPRIORITY     , KSCAMERA_EXTENDEDPROP_FILTERSCOPE, "Focus Priority"             , {
                {KSCAMERA_EXTENDEDPROP_FOCUSPRIORITY_OFF, "Off"},
                {KSCAMERA_EXTENDEDPROP_FOCUSPRIORITY_ON , "On" },
             }},
            {KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM              , KSCAMERA_EXTENDEDPROP_FILTERSCOPE, "Zoom"                       , {
                {KSCAMERA_EXTENDEDPROP_ZOOM_DEFAULT, "Default"},
                {KSCAMERA_EXTENDEDPROP_ZOOM_DIRECT , "Direct" },
                {KSCAMERA_EXTENDEDPROP_ZOOM_SMOOTH , "Smooth" },
             }},
            {KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOSTABILIZATION, 0                                , "Video Stabilization"        , {
                {KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_OFF , "Off" },
                {KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_ON  , "On"  },
                {KSCAMERA_EXTENDEDPROP_VIDEOSTABILIZATION_AUTO, "Auto"},
             }},
            {KSPROPERTY_CAMERACONTROL_EXTENDED_VIDEOHDR          , 0                                , "Video HDR"                  , {
                {KSCAMERA_EXTENDEDPROP_VIDEOHDR_OFF , "Off" },
                {KSCAMERA_EXTENDEDPROP_VIDEOHDR_ON  , "On"  },
                {KSCAMERA_EXTENDEDPROP_VIDEOHDR_AUTO, "Auto"},
             }},
            {KSPROPERTY_CAMERACONTROL_EXTENDED_OIS               , KSCAMERA_EXTENDEDPROP_FILTERSCOPE, "Optical Image Stabilization", {
                {KSCAMERA_EXTENDEDPROP_OIS_OFF , "Off" },
                {KSCAMERA_EXTENDEDPROP_OIS_ON  , "On"  },
                {KSCAMERA_EXTENDEDPROP_OIS_AUTO, "Auto"},
             }},
            {KSPROPERTY_CAMERACONTROL_EXTENDED_ADVANCEDPHOTO     , 0                                , "Advanced Photo"             , {
                {KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_OFF          , "OFF"          },
                {KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_AUTO         , "AUTO"         },
                {KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_HDR          , "HDR"          },
                {KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_FNF          , "FNF"          },
                {KSCAMERA_EXTENDEDPROP_ADVANCEDPHOTO_ULTRALOWLIGHT, "ULTRALOWLIGHT"},
             }},
            {KSPROPERTY_CAMERACONTROL_EXTENDED_END               , KSCAMERA_EXTENDEDPROP_FILTERSCOPE, ""                           , {}},
        };

        return captureMFUvcExtendedControlMap;
    }

    inline static const UvcExtendedControl *byId(int id)
    {
        auto it = table();

        for (; it->id != KSPROPERTY_CAMERACONTROL_EXTENDED_END; ++it)
            if (it->id == id)
                return it;

        return it;
    }
};

namespace UvcExtendedControlsPrivate
{
    size_t controlDataSize(IKsControl *ksControl, ULONG propId);
    bool control(IKsControl *ksControl,
                 ULONG propId,
                 LONG *min,
                 LONG *max,
                 LONG *step,
                 VariantValue *value,
                 ULONGLONG *flags,
                 ULONGLONG *capabilities);
    bool setControl(IKsControl *ksControl,
                    ULONG propId,
                    VariantValue value,
                    ULONGLONG flags);
}

QVariantList UvcExtendedControls::controls(IUnknown *device)
{
    if (!device)
        return {};

    LONG min = 0;
    LONG max = 0;
    LONG step = 0;
    VariantValue value = {0};
    ULONGLONG flags = 0;
    ULONGLONG capabilities = 0;

    QVariantList controls;
    IKsControl *ksControl = nullptr;

    if (SUCCEEDED(device->QueryInterface(IID_IKsControl,
                                         reinterpret_cast<void **>(&ksControl)))) {
        for (auto it = UvcExtendedControl::table();
             it->id != KSPROPERTY_CAMERACONTROL_EXTENDED_END;
             ++it) {
            auto ok =
                    UvcExtendedControlsPrivate::control(ksControl,
                                                        it->id,
                                                        &min,
                                                        &max,
                                                        &step,
                                                        &value,
                                                        &flags,
                                                        &capabilities);

            if (!ok)
                continue;

            if (it->id == KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM) {
                QStringList menuOptions;
                size_t currentIndex = 0;
                size_t i = 0;

                for (auto option = it->menu; option->description && option->description[0]; ++option) {
                    if (capabilities & option->value.ull) {
                        menuOptions << QString(option->description);

                        if (option->value.ull == flags)
                            currentIndex = i;
                    }

                    ++i;
                }

                QVariantList controlZoomMode {
                    QString(it->description) + " Mode",
                    "menu",
                    0,
                    menuOptions.size(),
                    1,
                    0,
                    currentIndex,
                    menuOptions
                };

                controls << QVariant(controlZoomMode);

                QVariantList controlZoom {
                    QString(it->description),
                    "integer",
                    static_cast<int>(min),
                    static_cast<int>(max),
                    static_cast<int>(step),
                    0,
                    static_cast<int>(value.l),
                    QStringList()
                };

                controls << QVariant(controlZoom);
            } else {
                QStringList menuOptions;
                size_t currentIndex = 0;
                size_t i = 0;

                for (auto option = it->menu; option->description && option->description[0]; ++option) {
                    if (capabilities & option->value.ull) {
                        menuOptions << QString(option->description);

                        if (option->value.ull == value.ull)
                            currentIndex = i;
                    }

                    ++i;
                }

                QVariantList control {
                    QString(it->description),
                    "menu",
                    0,
                    menuOptions.size(),
                    1,
                    0,
                    currentIndex,
                    menuOptions
                };

                controls << QVariant(control);
            }
        }

        ksControl->Release();
    }

    return controls;
}

bool UvcExtendedControls::setControls(IUnknown *device,
                                      const QVariantMap &controls)
{
    if (!device)
        return false;

    LONG min = 0;
    LONG max = 0;
    VariantValue value = {0};
    ULONGLONG flags = 0;

    IKsControl *ksControl = nullptr;

    if (SUCCEEDED(device->QueryInterface(IID_IKsControl,
                                         reinterpret_cast<void **>(&ksControl)))) {
        QString descZoomMode =
                UvcExtendedControl::byId(KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM)->description;
        descZoomMode += " Mode";

        for (auto it = UvcExtendedControl::table();
             it->id != KSPROPERTY_CAMERACONTROL_EXTENDED_END;
             ++it) {
            if (!controls.contains(it->description)
                && !controls.contains(descZoomMode))
                continue;

            auto ok =
                    UvcExtendedControlsPrivate::control(ksControl,
                                                        it->id,
                                                        &min,
                                                        &max,
                                                        nullptr,
                                                        &value,
                                                        &flags,
                                                        nullptr);

            if (!ok)
                continue;

            if (it->id == KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM) {
                if (controls.contains(it->description))
                    value.l =
                            qBound(min,
                                   controls.value(it->description).toInt(),
                                   max);

                if (controls.contains(descZoomMode))
                    flags = controls.value(it->description).toULongLong();
            } else {
                flags = value.ull =
                        controls.value(it->description).toULongLong();
            }

            UvcExtendedControlsPrivate::setControl(ksControl,
                                                   it->id,
                                                   value,
                                                   flags);
        }

        ksControl->Release();
    }

    return true;
}

size_t UvcExtendedControlsPrivate::controlDataSize(IKsControl *ksControl,
                                                   ULONG propId)
{
    KSPROPERTY prop;
    memset(&prop, 0, sizeof(KSPROPERTY));
    prop.Set = KSPROPERTYSETID_ExtendedCameraControl;
    prop.Id = propId;
    prop.Flags = KSPROPERTY_TYPE_BASICSUPPORT;

    // Get DescriptionSize

    KSPROPERTY_DESCRIPTION desc;
    ULONG bytesReturned = 0;
    auto hr = ksControl->KsProperty(&prop,
                                    sizeof(KSPROPERTY),
                                    &desc,
                                    sizeof(KSPROPERTY_DESCRIPTION),
                                    &bytesReturned);

    if (FAILED(hr))
        return 0;

    // Read the description buffer

    auto descBuffer = new BYTE[desc.DescriptionSize];

    hr = ksControl->KsProperty(&prop,
                               sizeof(KSPROPERTY),
                               descBuffer,
                               desc.DescriptionSize,
                               &bytesReturned);

    if (FAILED(hr)) {
        delete[] descBuffer;

        return 0;
    }

    // Check that there are at least one KSPROPERTY_MEMBERSHEADER

    if (desc.MembersListCount < 1
        || bytesReturned < sizeof(KSPROPERTY_DESCRIPTION) + sizeof(KSPROPERTY_MEMBERSHEADER)) {
        delete[] descBuffer;

        return 0;
    }

    // Read the data size from the first KSPROPERTY_MEMBERSHEADER

    auto membersHeader =
            reinterpret_cast<KSPROPERTY_MEMBERSHEADER *>(descBuffer
                                                         + sizeof(KSPROPERTY_DESCRIPTION));
    size_t dataSize = membersHeader->MembersSize;

    delete[] descBuffer;

    return dataSize;
}

bool UvcExtendedControlsPrivate::control(IKsControl *ksControl,
                                         ULONG propId,
                                         LONG *min,
                                         LONG *max,
                                         LONG *step,
                                         VariantValue *value,
                                         ULONGLONG *flags,
                                         ULONGLONG *capabilities)
{
    if (min)
        *min = 0;

    if (max)
        *max = 0;

    if (step)
        *step = 0;

    if (value)
        value->ull = 0;

    if (flags)
        *flags = 0;

    if (capabilities)
        *capabilities = 0;

    size_t dataSize = controlDataSize(ksControl, propId);

    if (dataSize < 1)
        return false;

    auto propertyValue = new BYTE[dataSize];

    KSPROPERTY prop;
    memset(&prop, 0, sizeof(KSPROPERTY));
    prop.Set = KSPROPERTYSETID_ExtendedCameraControl;
    prop.Id = propId;
    prop.Flags = KSPROPERTY_TYPE_GET;

    ULONG bytesReturned = 0;
    auto hr = ksControl->KsProperty(&prop,
                                    sizeof(KSPROPERTY),
                                    propertyValue,
                                    dataSize,
                                    &bytesReturned);

    if (FAILED(hr)) {
        delete[] propertyValue;

        return false;
    }

    if (dataSize < sizeof(KSCAMERA_EXTENDEDPROP_HEADER)) {
        delete[] propertyValue;

        return false;
    }

    auto header = reinterpret_cast<KSCAMERA_EXTENDEDPROP_HEADER *>(propertyValue);

    if (flags)
        *flags = header->Flags;

    if (capabilities)
        *capabilities = header->Capability;

    bool ok = false;

    if (propId == KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM
        && dataSize >= sizeof(KSCAMERA_EXTENDEDPROP_HEADER) + sizeof(KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING)) {
        auto val =
                reinterpret_cast<KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING *>(propertyValue
                                                                           + sizeof(KSCAMERA_EXTENDEDPROP_HEADER));

        if (min)
            *min = val->Min;

        if (max)
            *max = val->Max;

        if (step)
            *step = val->Step;

        if (value)
            value->l = val->VideoProc.Value.l;

        ok = true;
    } else if (dataSize >= sizeof(KSCAMERA_EXTENDEDPROP_HEADER) + sizeof(KSCAMERA_EXTENDEDPROP_VALUE)) {
        auto val =
                reinterpret_cast<KSCAMERA_EXTENDEDPROP_VALUE *>(propertyValue
                                                                + sizeof(KSCAMERA_EXTENDEDPROP_HEADER));

        if (value)
            value->ull = val->Value.ull;

        ok = true;
    }

    delete[] propertyValue;

    return ok;
}

bool UvcExtendedControlsPrivate::setControl(IKsControl *ksControl,
                                            ULONG propId,
                                            VariantValue value,
                                            ULONGLONG flags)
{
    if (!ksControl)
        return false;

    // Get the control data size
    size_t dataSize = controlDataSize(ksControl, propId);

    if (dataSize < sizeof(KSCAMERA_EXTENDEDPROP_HEADER))
        return false;

    auto propertyValue = new BYTE[dataSize];
    memset(propertyValue, 0, dataSize);

    // Set the control value according to the control type
    auto header = reinterpret_cast<KSCAMERA_EXTENDEDPROP_HEADER *>(propertyValue);
    header->Version = 1;
    header->PinId = UvcExtendedControl::byId(propId)->pinId;
    header->Size = static_cast<ULONG>(dataSize);
    header->Flags = flags;

    if (propId == KSPROPERTY_CAMERACONTROL_EXTENDED_ZOOM
        && dataSize >= sizeof(KSCAMERA_EXTENDEDPROP_HEADER) + sizeof(KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING)) {
        auto videoProc =
                reinterpret_cast<KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING *>(propertyValue
                                                                           + sizeof(KSCAMERA_EXTENDEDPROP_HEADER));
        videoProc->VideoProc.Value.l = value.l;
    } else if (dataSize >= sizeof(KSCAMERA_EXTENDEDPROP_HEADER) + sizeof(KSCAMERA_EXTENDEDPROP_VALUE)) {
        auto val =
                reinterpret_cast<KSCAMERA_EXTENDEDPROP_VALUE *>(propertyValue
                                                                + sizeof(KSCAMERA_EXTENDEDPROP_HEADER));
        val->Value.ull = value.ull;
    } else {
        delete[] propertyValue;

        return false;
    }

    // Configure KSPROPERTY
    KSPROPERTY prop;
    memset(&prop, 0, sizeof(KSPROPERTY));
    prop.Set = KSPROPERTYSETID_ExtendedCameraControl;
    prop.Id = propId;
    prop.Flags = KSPROPERTY_TYPE_SET;

    // Call KsProperty to set the property
    ULONG bytesReturned = 0;
    auto hr = ksControl->KsProperty(&prop,
                                    sizeof(KSPROPERTY),
                                    propertyValue,
                                    static_cast<ULONG>(dataSize),
                                    &bytesReturned);

    delete [] propertyValue;

    return SUCCEEDED(hr)? true: false;
}

#include "moc_uvcextendedcontrols.cpp"

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

#include "plugin.h"
#include "plugininterface.h"
#include "classfactory.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

inline AkVCam::PluginInterface *pluginInterface()
{
    static AkVCam::PluginInterface pluginInterface;

    return &pluginInterface;
}

// Filter entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    UNUSED(lpvReserved)

#if defined(QT_DEBUG) && 0
    // Turn on lights
    freopen("CONOUT$", "a", stdout);
    freopen("CONOUT$", "a", stderr);
    setbuf(stdout, nullptr);
#endif

    auto temp = AkVCam::tempPath();
    AkLoggerStart(std::string(temp.begin(), temp.end())
                  + "\\" DSHOW_PLUGIN_NAME, "log");
    AkLoggerLog(__FUNCTION__, "()");

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            AkLoggerLog("Reason Attach");
            AkLoggerLog("Module file name: ", AkVCam::moduleFileName(hinstDLL));
            DisableThreadLibraryCalls(hinstDLL);
            pluginInterface()->pluginHinstance() = hinstDLL;

            break;

        case DLL_PROCESS_DETACH:
            AkLoggerLog("Reason Detach");

            break;

        default:
            AkLoggerLog("Reason Unknown: ", fdwReason);

            break;
    }

    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    AkLoggerLog(__FUNCTION__, "()");
    AkLoggerLog("CLSID: ", AkVCam::stringFromClsid(rclsid));
    AkLoggerLog("IID: ", AkVCam::stringFromClsid(rclsid));

    if (!ppv)
        return E_INVALIDARG;

    *ppv = nullptr;

    if (!IsEqualIID(riid, IID_IUnknown)
        && !IsEqualIID(riid, IID_IClassFactory)
        && AkVCam::cameraFromId(riid) < 0)
            return CLASS_E_CLASSNOTAVAILABLE;

    auto classFactory = new AkVCam::ClassFactory(rclsid);
    classFactory->AddRef();
    *ppv = classFactory;

    return S_OK;
}

STDAPI DllCanUnloadNow()
{
    AkLoggerLog(__FUNCTION__, "()");

    return AkVCam::ClassFactory::locked()? S_FALSE: S_OK;
}

STDAPI DllRegisterServer()
{
    AkLoggerLog(__FUNCTION__, "()");

    DllUnregisterServer();

    bool ok = true;

    for (DWORD i = 0; i < AkVCam::camerasCount(); i++) {
        auto description = AkVCam::cameraDescription(i);
        auto path = AkVCam::cameraPath(i);

#ifdef QT_DEBUG
        auto clsid = AkVCam::createClsidFromStr(path);
#endif

        AkLoggerLog("Creating Camera");
        AkLoggerLog("\tDescription: ", std::string(description.begin(),
                                                   description.end()));
        AkLoggerLog("\tPath: ", std::string(path.begin(), path.end()));
        AkLoggerLog("\tCLSID: ", AkVCam::stringFromIid(clsid));

        ok &= pluginInterface()->createDevice(path, description);
    }

    return ok? S_OK: E_UNEXPECTED;
}

STDAPI DllUnregisterServer()
{
    AkLoggerLog(__FUNCTION__, "()");

    auto cameras =
            AkVCam::listRegisteredCameras(pluginInterface()->pluginHinstance());

    for (auto camera: cameras) {
        AkLoggerLog("Deleting ", AkVCam::stringFromClsid(camera));
        pluginInterface()->destroyDevice(camera);
    }

    // Unregister old virtual camera filter.
    // NOTE: This code must be removed in future versions.
    CLSID clsid;
    CLSIDFromString(L"{41764B79-7320-5669-7274-75616C43616D}", &clsid);
    AkLoggerLog("Deleting ", AkVCam::stringFromClsid(clsid));
    pluginInterface()->destroyDevice(clsid);

    return S_OK;
}

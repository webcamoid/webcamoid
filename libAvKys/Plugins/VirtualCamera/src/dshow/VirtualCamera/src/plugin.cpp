/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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
#include "utils.h"
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

    AkLoggerStart(AkVCam::tempPath() + "\\AkVirtualCamera", "log");
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
        && !IsEqualIID(riid, IID_IClassFactory))
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

    if (!pluginInterface()->createDevice(DSHOW_PLUGIN_CLSID_STRING,
                                         DSHOW_PLUGIN_DESCRIPTION_L))
        return E_UNEXPECTED;

    return S_OK;
}

STDAPI DllUnregisterServer()
{
    AkLoggerLog(__FUNCTION__, "()");
    pluginInterface()->destroyDevice(DSHOW_PLUGIN_CLSID_STRING);

    return S_OK;
}

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

#include "virtualcamerasource.h"

//#define USE_DEFAULT_CATEGORY

const AMOVIESETUP_MEDIATYPE sudOpPinTypes = {
    &MEDIATYPE_Video,  // Major type
    &MEDIASUBTYPE_NULL // Minor type
};

const AMOVIESETUP_PIN pinSetup = {
    LPWSTR(OUTPUT_PIN_NAME), // Pin string name
    FALSE,                   // Is it rendered
    TRUE,                    // Is it an output
    FALSE,                   // Can we have none
    FALSE,                   // Can we have many
    &CLSID_NULL,             // Connects to filter
    nullptr,                    // Connects to pin
    1,                       // Number of types
    &sudOpPinTypes           // Pin details
};

const AMOVIESETUP_FILTER filterSetup = {
    &CLSID_VirtualCameraSource,    // Filter CLSID
    FILTER_NAME,                   // String name
    FILTER_MERIT,                  // Filter merit
    1,                             // Number pins
    &pinSetup                      // Pin details
};

CFactoryTemplate g_Templates[] = {
    {FILTER_NAME,
     &CLSID_VirtualCameraSource,
     VirtualCameraSource::CreateInstance,
     nullptr,
     &filterSetup}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
#ifdef USE_DEFAULT_CATEGORY
    HRESULT hr = AMovieDllRegisterServer2(TRUE);
#else
    HRESULT hr = RegisterFilters(TRUE);
#endif

    RegisterDevicePath();

    return hr;
}

STDAPI DllUnregisterServer()
{
#ifdef USE_DEFAULT_CATEGORY
    return AMovieDllRegisterServer2(FALSE);
#else
    return RegisterFilters(FALSE);
#endif
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  dwReason,
                      LPVOID lpReserved)
{
    return DllEntryPoint(HINSTANCE(hModule), dwReason, lpReserved);
}

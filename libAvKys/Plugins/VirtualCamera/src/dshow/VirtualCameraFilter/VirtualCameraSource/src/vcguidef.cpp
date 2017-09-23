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

#include <sstream>
#include <iomanip>
#include <list>

#include "vcguidef.h"

extern int g_cTemplates;
extern CFactoryTemplate g_Templates[];

STDAPI RegisterAllServers(LPCWSTR szFileName, BOOL bRegister);
STDAPI AMovieSetupRegisterServer(CLSID clsServer,
                                 LPCWSTR szDescription,
                                 LPCWSTR szFileName,
                                 LPCWSTR szThreadingModel = L"Both",
                                 LPCWSTR szServerType = L"InprocServer32");
STDAPI AMovieSetupUnregisterServer(CLSID clsServer);

STDAPI RegisterFilters(BOOL bRegister)
{
    WCHAR achFileName[MAX_PATH];

    {
        char achTemp[MAX_PATH];
        ASSERT(g_hInst != 0);

        if (GetModuleFileNameA(g_hInst,
                               achTemp,
                               sizeof(achTemp)) == 0) {
            return AmHresultFromWin32(GetLastError());
        }

        MultiByteToWideChar(CP_ACP,
                            0L,
                            achTemp,
                            lstrlenA(achTemp) + 1,
                            achFileName,
                            NUMELMS(achFileName));
    }

    HRESULT hr = NOERROR;

    if (bRegister)
        hr = RegisterAllServers(achFileName, TRUE);

    if (SUCCEEDED(hr)) {
        hr = CoInitialize(LPVOID(nullptr));
        ASSERT(SUCCEEDED(hr));

        IFilterMapper2 *filterMapper = nullptr;
        hr = CoCreateInstance(CLSID_FilterMapper2,
                              nullptr,
                              CLSCTX_INPROC_SERVER,
                              IID_IFilterMapper2,
                              reinterpret_cast<void **>(&filterMapper));

        if (SUCCEEDED(hr)) {
            for (int i = 0; i < g_cTemplates; i++) {
                const CFactoryTemplate *templ = &g_Templates[i];

                if (templ->m_pAMovieSetup_Filter != nullptr) {
                    hr = filterMapper->UnregisterFilter(&CLSID_VideoInputDeviceCategory,
                                                        nullptr,
                                                        *templ->m_pAMovieSetup_Filter->clsID);

                    if (bRegister) {
                        IMoniker *pMoniker = nullptr;
                        REGFILTER2 rf2;
                        rf2.dwVersion = 1;
                        rf2.dwMerit = templ->m_pAMovieSetup_Filter->dwMerit;
                        rf2.cPins = templ->m_pAMovieSetup_Filter->nPins;
                        rf2.rgPins = templ->m_pAMovieSetup_Filter->lpPin;

                        hr = filterMapper->RegisterFilter(*templ->m_pAMovieSetup_Filter->clsID,
                                                          templ->m_pAMovieSetup_Filter->strName,
                                                          &pMoniker,
                                                          &CLSID_VideoInputDeviceCategory,
                                                          nullptr,
                                                          &rf2);
                    }

                    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                        hr = NOERROR;
                }

                if (FAILED(hr))
                    break;
            }

            filterMapper->Release();
        }

        CoFreeUnusedLibraries();
        CoUninitialize();
    }

    if (SUCCEEDED(hr) && !bRegister)
        hr = RegisterAllServers(achFileName, FALSE);

    return hr;
}

std::string wstrToString(LPWSTR wstr)
{
    UINT codepage = CP_ACP;
    int strLen = WideCharToMultiByte(codepage, 0, wstr, -1, 0, 0, 0, 0) - 1;

    std::string str(size_t(strLen), '\0');
    WideCharToMultiByte(codepage, 0, wstr, -1, &str[0], strLen, nullptr, FALSE);

    return str;
}

std::string iidToString(const IID &iid)
{
    LPWSTR strIID = nullptr;
    StringFromIID(iid, &strIID);
    std::string str = wstrToString(strIID);
    CoTaskMemFree(strIID);

    return str;
}

std::string createHID(int vendorId, int productId, int revision)
{
    std::ostringstream hid;

    hid << std::uppercase
        << "USB\\VID_v"
        << std::setfill('0') << std::setw(4) << std::hex << vendorId
        << "&PID_d"
        << std::setfill('0') << std::setw(4) << std::hex << productId
        << "&REV_r"
        << std::setfill('0') << std::setw(4) << std::dec << revision;

    return hid.str();
}

std::string createHID()
{
    std::vector<int> ids = enumerateCurrentIds();
    int id = 0;

    for (; id < 10000; id++)
        if (std::find(ids.begin(), ids.end(), id) == ids.end())
            break;

    std::ostringstream hid;
    std::string iid = iidToString(KSCATEGORY_CAPTURE);
    std::transform(iid.begin(), iid.end(), iid.begin(), ::tolower);

    hid << "\\\\?\\root#image#"
        << std::setfill('0') << std::setw(4) << id
        << "#"
        << iid
        << "\\global";

    return hid.str();
}

std::vector<int> enumerateCurrentIds()
{
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                                  nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum,
                                  reinterpret_cast<void **>(&pDevEnum));

    std::list<int> ids;

    if (SUCCEEDED(hr)) {
        IEnumMoniker *pEnum = nullptr;
        // Create an enumerator for the category.
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);

        if (SUCCEEDED(hr)) {
            IMoniker *pMoniker = nullptr;

            while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
                IPropertyBag *pPropBag;
                hr = pMoniker->BindToStorage(0,
                                             0,
                                             IID_IPropertyBag,
                                             reinterpret_cast<void **>(&pPropBag));

                if (SUCCEEDED(hr)) {
                    VARIANT var;
                    VariantInit(&var);
                    hr = pPropBag->Read(L"DevicePath", &var, 0);

                    if (SUCCEEDED(hr)) {
                        std::string devicePath = wstrToString(var.bstrVal);
                        std::string startStr = "\\\\?\\root#image#";
                        size_t start = devicePath.find(startStr);

                        if (start == 0) {
                            size_t end = devicePath.find('#', startStr.size());
                            std::string id = devicePath.substr(startStr.size(), end - startStr.size());
                            ids.push_back(std::stoi(id));
                        }
                    }

                    VariantClear(&var);
                    pPropBag->Release();
                }


                pMoniker->Release();
            }

            pEnum->Release();
        }

        pDevEnum->Release();
    }

    return std::vector<int>(ids.begin(), ids.end());
}

STDAPI RegisterDevicePath()
{
    std::ostringstream captureDevicesRegkey;

    captureDevicesRegkey << "SOFTWARE\\Classes\\CLSID\\"
                         << iidToString(CLSID_VideoInputDeviceCategory)
                         << "\\Instance\\"
                         << iidToString(CLSID_VirtualCameraSource);

    std::string subKeyStr = captureDevicesRegkey.str();
    std::wstring subKey(subKeyStr.begin(), subKeyStr.end());
    HKEY hKey;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_ALL_ACCESS, &hKey);

    std::string hidStr = createHID();
    std::wstring hid(hidStr.begin(), hidStr.end());
    RegSetValueEx(hKey,
                  TEXT("DevicePath"),
                  0,
                  REG_SZ,
                  reinterpret_cast<const BYTE *>(hid.c_str()),
                  hid.size() * sizeof(wchar_t));
    RegCloseKey(hKey);

    return S_OK;
}

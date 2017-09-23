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

#include <QSettings>
#include <QFileInfo>
#include <filtercommons.h>
#include <akvideopacket.h>

#include "cameraoutdshow.h"

#define MAX_CAMERAS 1

CameraOutDShow::CameraOutDShow(QObject *parent):
    CameraOut(parent)
{
    this->m_streamIndex = -1;
    this->m_passwordTimeout = 5000;
}

CameraOutDShow::~CameraOutDShow()
{
}

QString CameraOutDShow::driverPath() const
{
    return this->m_driverPath;
}

QStringList CameraOutDShow::webcams() const
{
    IEnumMoniker *pEnum = nullptr;
    HRESULT hr = this->enumerateCameras(&pEnum);

    if (FAILED(hr))
        return QStringList();

    IMoniker *pMoniker = nullptr;
    QString devicePath;

    for (int i = 0; pEnum->Next(1, &pMoniker, nullptr) == S_OK; i++) {
        IBaseFilter *filter = nullptr;
        hr = pMoniker->BindToObject(nullptr,
                                    nullptr,
                                    IID_IBaseFilter,
                                    reinterpret_cast<void **>(&filter));

        if (FAILED(hr)) {
            pMoniker->Release();
            pMoniker = nullptr;

            continue;
        }

        CLSID clsid;

        if (FAILED(filter->GetClassID(&clsid))) {
            filter->Release();
            pMoniker->Release();
            pMoniker = nullptr;

            continue;
        }

        filter->Release();

        if (clsid != CLSID_VirtualCameraSource) {
            pMoniker->Release();
            pMoniker = nullptr;

            continue;
        }

        IPropertyBag *pPropBag = nullptr;
        hr = pMoniker->BindToStorage(nullptr,
                                     nullptr,
                                     IID_IPropertyBag,
                                     reinterpret_cast<void **>(&pPropBag));

        if (SUCCEEDED(hr)) {
            VARIANT var;
            VariantInit(&var);
            hr = pPropBag->Read(L"DevicePath", &var, 0);

            if (SUCCEEDED(hr))
                devicePath = QString::fromWCharArray(var.bstrVal);
            else
                devicePath = QString("/dev/video%1").arg(i);

            pPropBag->Release();
        }

        pMoniker->Release();
        pMoniker = nullptr;

        break;
    }

    pEnum->Release();

    QStringList webcams;

    if (!devicePath.isEmpty())
        webcams << devicePath;

    return webcams;
}

QString CameraOutDShow::device() const
{
    return this->m_device;
}

int CameraOutDShow::streamIndex() const
{
    return this->m_streamIndex;
}

AkCaps CameraOutDShow::caps() const
{
    return this->m_caps;
}

QString CameraOutDShow::description(const QString &webcam) const
{
    IEnumMoniker *pEnum = nullptr;
    HRESULT hr = this->enumerateCameras(&pEnum);

    if (FAILED(hr))
        return QString();

    IMoniker *pMoniker = nullptr;

    for (int i = 0; pEnum->Next(1, &pMoniker, nullptr) == S_OK; i++) {
        IPropertyBag *pPropBag = nullptr;
        hr = pMoniker->BindToStorage(nullptr,
                                     nullptr,
                                     IID_IPropertyBag,
                                     reinterpret_cast<void **>(&pPropBag));

        if (SUCCEEDED(hr)) {
            VARIANT var;
            VariantInit(&var);
            hr = pPropBag->Read(L"DevicePath", &var, 0);
            QString devicePath;

            if (SUCCEEDED(hr))
                devicePath = QString::fromWCharArray(var.bstrVal);
            else
                devicePath = QString("/dev/video%1").arg(i);

            if (devicePath == webcam) {
                // Get description or friendly name.
                hr = pPropBag->Read(L"Description", &var, 0);

                if (FAILED(hr))
                    hr = pPropBag->Read(L"FriendlyName", &var, 0);

                QString description;

                if (SUCCEEDED(hr))
                    description = QString::fromWCharArray(var.bstrVal);

                pPropBag->Release();
                pMoniker->Release();
                pEnum->Release();

                return description;
            }

            pPropBag->Release();
        }

        pMoniker->Release();
        pMoniker = nullptr;
    }

    pEnum->Release();

    return QString();
}
void CameraOutDShow::writeFrame(const AkPacket &frame)
{
    AkVideoPacket videoFrame = frame;

    if (this->m_ipcBridge.write(AkVideoCaps::fourCC(videoFrame.caps().format()),
                                DWORD(videoFrame.caps().width()),
                                DWORD(videoFrame.caps().height()),
                                reinterpret_cast<const BYTE *>(videoFrame.buffer().constData())) < 1)
        qDebug() << "Error writing frame";
}

int CameraOutDShow::maxCameras() const
{
    return MAX_CAMERAS;
}

bool CameraOutDShow::needRoot() const
{
    return false;
}

int CameraOutDShow::passwordTimeout() const
{
    return this->m_passwordTimeout;
}

QString CameraOutDShow::rootMethod() const
{
    return this->m_rootMethod;
}

QString CameraOutDShow::createWebcam(const QString &description,
                                     const QString &password)
{
    Q_UNUSED(password)

    if (!QFileInfo(this->m_driverPath).exists())
        return QString();

    QStringList webcams = this->webcams();

    if (!webcams.isEmpty())
        return QString();

    QString reg =
            QString("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\%1\\Instance\\%2")
            .arg(this->iidToString(CLSID_VideoInputDeviceCategory))
            .arg(this->iidToString(CLSID_VirtualCameraSource));

    QString desc = description.isEmpty()?
                       QString::fromWCharArray(FILTER_NAME):
                       description;

    QString params =
            QString("/c \"regsvr32 \"%1\" && reg add %2 /v FriendlyName /d \"%3\" /f\"")
            .arg(this->m_driverPath)
            .arg(reg)
            .arg(desc);

    if (!this->sudo("cmd", params, "", true))
        return QString();

    QStringList curWebcams = this->webcams();

    if (curWebcams != webcams)
        emit this->webcamsChanged(curWebcams);

    return curWebcams.isEmpty()? QString(): curWebcams.first();
}

bool CameraOutDShow::changeDescription(const QString &webcam,
                                       const QString &description,
                                       const QString &password) const
{
    Q_UNUSED(password)

    QStringList webcams = this->webcams();

    if (!webcams.contains(webcam))
        return false;

    QString reg =
            QString("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\%1\\Instance\\%2")
            .arg(this->iidToString(CLSID_VideoInputDeviceCategory))
            .arg(this->iidToString(CLSID_VirtualCameraSource));

    QString desc = description.isEmpty()?
                       QString::fromWCharArray(FILTER_NAME):
                       description;

    QString params =
            QString("add %1 /v FriendlyName /d \"%2\" /f")
            .arg(reg)
            .arg(desc);

    if (!this->sudo("reg", params))
        return false;

    emit this->webcamsChanged(webcams);

    return true;
}

bool CameraOutDShow::removeWebcam(const QString &webcam,
                                  const QString &password)
{
    Q_UNUSED(password)

    QStringList webcams = this->webcams();

    if (!webcams.contains(webcam))
        return false;

    QString reg =
            QString("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\%1\\InprocServer32")
            .arg(this->iidToString(CLSID_VirtualCameraSource));

    QSettings settings(reg, QSettings::NativeFormat);

    QString params =
            QString("/u \"%1\"")
            .arg(settings.value(".").toString());

    if (!this->sudo("regsvr32", params))
        return false;

    emit this->webcamsChanged(QStringList());

    return true;
}

bool CameraOutDShow::removeAllWebcams(const QString &password)
{
    Q_UNUSED(password)

    for (const QString &webcam: this->webcams())
        this->removeWebcam(webcam, password);

    return true;
}

HRESULT CameraOutDShow::enumerateCameras(IEnumMoniker **ppEnum) const
{
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                                  nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum,
                                  reinterpret_cast<void **>(&pDevEnum));

    if (SUCCEEDED(hr)) {
        // Create an enumerator for the category.
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, ppEnum, 0);

        if (hr == S_FALSE)
            hr = VFW_E_NOT_FOUND;

        pDevEnum->Release();
    }

    return hr;
}

QString CameraOutDShow::iidToString(const IID &iid) const
{
    LPWSTR strIID = nullptr;
    StringFromIID(iid, &strIID);
    QString str = QString::fromWCharArray(strIID);
    CoTaskMemFree(strIID);

    return str;
}

bool CameraOutDShow::sudo(const QString &command,
                          const QString &params,
                          const QString &dir,
                          bool hide) const
{
    const static int maxStrLen = 1024;

    wchar_t wcommand[maxStrLen];
    memset(wcommand, 0, maxStrLen * sizeof(wchar_t));
    command.toWCharArray(wcommand);

    wchar_t wparams[maxStrLen];
    memset(wparams, 0, maxStrLen * sizeof(wchar_t));
    params.toWCharArray(wparams);

    wchar_t wdir[maxStrLen];
    memset(wdir, 0, maxStrLen * sizeof(wchar_t));
    dir.toWCharArray(wdir);

    SHELLEXECUTEINFO execInfo;
    ZeroMemory(&execInfo, sizeof(SHELLEXECUTEINFO));

    execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    execInfo.hwnd = nullptr;
    execInfo.lpVerb = L"runas";
    execInfo.lpFile = wcommand;
    execInfo.lpParameters = wparams;
    execInfo.lpDirectory = wdir;
    execInfo.nShow = hide? SW_HIDE: SW_SHOWNORMAL;
    execInfo.hInstApp = nullptr;
    ShellExecuteEx(&execInfo);

    if (!execInfo.hProcess)
        return false;

    WaitForSingleObject(execInfo.hProcess, INFINITE);

    DWORD exitCode;
    BOOL ok = GetExitCodeProcess(execInfo.hProcess, &exitCode);

    CloseHandle(execInfo.hProcess);

    if (ok && FAILED(exitCode))
        return false;

    return true;
}

bool CameraOutDShow::init(int streamIndex, const AkCaps &caps)
{
    this->m_streamIndex = streamIndex;
    this->m_caps = caps;

    return this->m_ipcBridge.open(IPC_FILE_NAME, IpcBridge::Write);
}

void CameraOutDShow::uninit()
{
    this->m_ipcBridge.close();
}

void CameraOutDShow::setDriverPath(const QString &driverPath)
{
    if (this->m_driverPath == driverPath)
        return;

    this->m_driverPath = driverPath;
    emit this->driverPathChanged(driverPath);
}

void CameraOutDShow::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(device);
}

void CameraOutDShow::setPasswordTimeout(int passwordTimeout)
{
    if (this->m_passwordTimeout == passwordTimeout)
        return;

    this->m_passwordTimeout = passwordTimeout;
    emit this->passwordTimeoutChanged(passwordTimeout);
}

void CameraOutDShow::setRootMethod(const QString &rootMethod)
{
    if (this->m_rootMethod == rootMethod)
        return;

    this->m_rootMethod = rootMethod;
    emit this->rootMethodChanged(rootMethod);
}

void CameraOutDShow::resetDriverPath()
{
    this->setDriverPath("");
}

void CameraOutDShow::resetDevice()
{
    this->setDevice("");
}

void CameraOutDShow::resetPasswordTimeout()
{
    this->setPasswordTimeout(5000);
}

void CameraOutDShow::resetRootMethod()
{
    this->setRootMethod("");
}

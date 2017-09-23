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

#include <vector>
#include <windows.h>
#include <uuids.h>
#include <ks.h>

#include "ipcbridge.h"
#include "waitcondition.h"

#define MUTEX_TIMEOUT 5000


struct Frame
{
    DWORD format;
    DWORD width;
    DWORD height;
    DWORD data;
};

const int maxBufferSize = sizeof(Frame)      // Frame header
                          + 4 * 1920 * 1080; // Max. frame size

namespace IPC {
    class VideoFormat
    {
        public:
            DWORD fourcc;
            GUID guid;
            WORD bpp;

            static inline const std::vector<VideoFormat> &formats()
            {
                static const std::vector<VideoFormat> videoFormats = {
                    // RGB formats
                    {MAKEFOURCC('R', 'G', 'B', '4'), MEDIASUBTYPE_RGB32, 32},
                    {MAKEFOURCC('R', 'G', 'B', '3'), MEDIASUBTYPE_RGB24, 24},
                    {MAKEFOURCC('R', 'G', 'B', 'P'), MEDIASUBTYPE_RGB565, 16},
                    {MAKEFOURCC('R', 'G', 'B', 'O'), MEDIASUBTYPE_RGB555, 16},

                    // Luminance+Chrominance formats
                    {MAKEFOURCC('U', 'Y', 'V', 'Y'), MEDIASUBTYPE_UYVY, 16},
                    {MAKEFOURCC('Y', 'U', 'Y', '2'), MEDIASUBTYPE_YUY2, 16},
                    {MAKEFOURCC('Y', 'U', 'Y', 'V'), MEDIASUBTYPE_YUYV, 16},
                    {MAKEFOURCC('Y', 'V', '1', '2'), MEDIASUBTYPE_YV12, 12},
                    {MAKEFOURCC('I', '4', '2', '0'), MEDIASUBTYPE_IYUV, 12},
                    {MAKEFOURCC('I', 'Y', 'U', 'V'), MEDIASUBTYPE_IYUV, 12},

                    // two planes -- one Y, one Cr + Cb interleaved
                    {MAKEFOURCC('N', 'V', '1', '2'), MEDIASUBTYPE_NV12, 12},
                };

                return videoFormats;
            }

            static inline const VideoFormat *byFourCC(DWORD fourcc)
            {
                for (size_t i = 0; i < formats().size(); i++)
                    if (formats()[i].fourcc == fourcc)
                        return &formats()[i];

                return nullptr;
            }

            static inline const VideoFormat *byGuid(const GUID &guid)
            {
                for (size_t i = 0; i < formats().size(); i++)
                    if (formats()[i].guid == guid)
                        return &formats()[i];

                return nullptr;
            }

            static inline size_t frameSize(DWORD format, DWORD width, DWORD height)
            {
                const VideoFormat *vf = byFourCC(format);

                if (!vf)
                    return 0;

                return vf->bpp * width * height / 8;
            }
    };
}

class IpcBridgePrivate
{
    public:
        std::wstring m_pipeName;
        HANDLE m_fileHandle;
        Frame *m_frame;
        Mutex m_mutex;
        WaitCondition m_hasFrame;
};

IpcBridge::IpcBridge(const std::wstring &pipeName)
{
    this->d = new IpcBridgePrivate();
    this->d->m_pipeName = pipeName;
    this->d->m_fileHandle = nullptr;
    this->d->m_frame = nullptr;
}

IpcBridge::~IpcBridge()
{
    this->close();
    delete this->d;
}

std::wstring IpcBridge::pipeName() const
{
    return this->d->m_pipeName;
}

void IpcBridge::setPipeName(const std::wstring &pipeName)
{
    this->d->m_pipeName = pipeName;
}

void IpcBridge::resetPipeName()
{
    this->d->m_pipeName.clear();
}

bool IpcBridge::open(IpcBridge::OpenMode mode)
{
    if (this->d->m_pipeName.empty())
        return false;

    if (mode == Read) {
        this->d->m_fileHandle =
                OpenFileMapping(FILE_MAP_ALL_ACCESS,
                                FALSE,
                                this->d->m_pipeName.c_str());
    } else {
        this->d->m_fileHandle =
                CreateFileMapping(INVALID_HANDLE_VALUE,
                                  nullptr,
                                  PAGE_READWRITE,
                                  0,
                                  maxBufferSize,
                                  this->d->m_pipeName.c_str());
    }

    if (!this->d->m_fileHandle)
        return false;

    this->d->m_frame = reinterpret_cast<Frame *>
                       (MapViewOfFile(this->d->m_fileHandle,
                                      FILE_MAP_ALL_ACCESS,
                                      0,
                                      0,
                                      maxBufferSize));

    if (!this->d->m_frame) {
        CloseHandle(this->d->m_fileHandle);
        this->d->m_fileHandle = nullptr;

        return false;
    }

    this->d->m_mutex = Mutex(this->d->m_pipeName + L".mutex");
    this->d->m_hasFrame = WaitCondition(&this->d->m_mutex);

    return true;
}

bool IpcBridge::open(const std::wstring &pipeName, IpcBridge::OpenMode mode)
{
    this->d->m_pipeName = pipeName;

    return this->open(mode);
}

void IpcBridge::close()
{
    if (this->d->m_frame) {
        UnmapViewOfFile(this->d->m_frame);
        this->d->m_frame = nullptr;
    }

    if (this->d->m_fileHandle) {
        CloseHandle(this->d->m_fileHandle);
        this->d->m_fileHandle = nullptr;
    }
}

size_t IpcBridge::read(DWORD *format, DWORD *width, DWORD *height, BYTE **data)
{
    if (!this->d->m_frame)
        return 0;

    this->d->m_mutex.lock();

    if (!this->d->m_hasFrame.wait(&this->d->m_mutex, MUTEX_TIMEOUT)) {
        this->d->m_mutex.unlock();

        return 0;
    }

    *format = this->d->m_frame->format;
    *width = this->d->m_frame->width;
    *height = this->d->m_frame->height;
    size_t len = IPC::VideoFormat::frameSize(this->d->m_frame->format,
                                             this->d->m_frame->width,
                                             this->d->m_frame->height);

    if (len < 1 || len > maxBufferSize) {
        this->d->m_mutex.unlock();

        return 0;
    }

    *data = new BYTE[len];
    CopyMemory(*data, &this->d->m_frame->data, len);

    this->d->m_mutex.unlock();

    return len;
}

size_t IpcBridge::read(GUID *format, DWORD *width, DWORD *height, BYTE **data)
{
    DWORD fourCC;
    size_t len = this->read(&fourCC, width, height, data);

    if (len < 1)
        return 0;

    const IPC::VideoFormat *vf = IPC::VideoFormat::byFourCC(fourCC);

    if (!vf) {
        *format = GUID_NULL;

        return 0;
    }

    *format = vf->guid;

    return len;
}

size_t IpcBridge::write(DWORD format, DWORD width, DWORD height, const BYTE *data)
{
    if (!this->d->m_frame)
        return 0;

    size_t len = IPC::VideoFormat::frameSize(format, width, height);

    if (len < 1 || len > maxBufferSize)
        return 0;

    this->d->m_mutex.lock();

    this->d->m_frame->format = format;
    this->d->m_frame->width = width;
    this->d->m_frame->height = height;

    CopyMemory(&this->d->m_frame->data, data, len);
    this->d->m_hasFrame.wakeAll();
    this->d->m_mutex.unlock();

    return len;
}

size_t IpcBridge::write(const GUID &format, DWORD width, DWORD height, const BYTE *data)
{
    const IPC::VideoFormat *vf = IPC::VideoFormat::byGuid(format);

    if (!vf)
        return 0;

    return this->write(vf->fourcc, width, height, data);
}

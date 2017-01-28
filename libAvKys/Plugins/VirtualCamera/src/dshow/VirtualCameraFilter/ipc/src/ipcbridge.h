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

#ifndef IPCBRIDGE_H
#define IPCBRIDGE_H

#include <string>
#include <cstdint>
#include <windows.h>

class IpcBridgePrivate;

class IpcBridge
{
    public:
        enum OpenMode {
            Read,
            Write
        };

        IpcBridge(const std::wstring &pipeName=std::wstring());
        ~IpcBridge();

        std::wstring pipeName() const;
        void setPipeName(const std::wstring &pipeName);
        void resetPipeName();

        bool open(OpenMode mode=Read);
        bool open(const std::wstring &pipeName, OpenMode mode=Read);
        void close();

        size_t read(DWORD *format, DWORD *width, DWORD *height, BYTE **data);
        size_t read(GUID *format, DWORD *width, DWORD *height, BYTE **data);
        size_t write(DWORD format, DWORD width, DWORD height, const BYTE *data);
        size_t write(const GUID &format, DWORD width, DWORD height, const BYTE *data);

    private:
        IpcBridgePrivate *d;
};

#endif // IPCBRIDGE_H

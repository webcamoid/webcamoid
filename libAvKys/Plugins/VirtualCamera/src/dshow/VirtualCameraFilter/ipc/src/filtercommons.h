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

#ifndef FILTERCOMMONS_H
#define FILTERCOMMONS_H

#include <initguid.h>

#define FILTER_NAME L"AvKys Virtual Camera"
#define IPC_FILE_NAME L"Local\\AvKysVirtualCamera"
DEFINE_GUID(CLSID_VirtualCameraSource, 0x41764b79, 0x7320, 0x5669, 0x72, 0x74, 0x75, 0x61, 0x6c, 0x43, 0x61, 0x6d);

#endif // FILTERCOMMONS_H

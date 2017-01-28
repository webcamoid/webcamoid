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

#ifndef VCGUIDEF_H
#define VCGUIDEF_H

#include <streams.h>
#include <strmif.h>

#include "filtercommons.h"

#ifdef __out
#undef __out
#endif

#ifdef __in
#undef __in
#endif

#include <string>
#include <vector>

#define OUTPUT_PIN_NAME L"Output"
#define FILTER_MERIT MERIT_NORMAL

STDAPI RegisterServers(LPCWSTR szFileName, BOOL bRegister);
STDAPI RegisterFilters(BOOL bRegister);
STDAPI RegisterDevicePath();
std::string wstrToString(LPWSTR wstr);
std::string iidToString(const IID &iid);
std::string createHID(int vendorId, int productId, int revision);
std::string createHID();
std::vector<int> enumerateCurrentIds();
std::string bstrToString();

#endif // VCGUIDEF_H

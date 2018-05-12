/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#include "rcname.h"
#include "../cstream/cstreamread.h"

std::string AkVCam::RcName::read(const unsigned char *rcName)
{
    CStreamRead nameStream(rcName, true);
    auto size = nameStream.read<uint16_t>();
    nameStream.seek<uint32_t>();
    std::wstring wstr;

    for (decltype(size) i = 0; i < size; i++)
        wstr += nameStream.read<uint16_t>();

    return std::string(wstr.begin(), wstr.end());
}

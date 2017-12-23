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

#include <map>
#include <algorithm>

#include "utils.h"

inline const std::map<AkVCam::PixelFormat, FourCharCode> *formatsTable()
{
    static const std::map<AkVCam::PixelFormat, FourCharCode> formatsTable {
        {AkVCam::PixelFormatBGR32, kCMPixelFormat_32ARGB         },
        {AkVCam::PixelFormatRGB24, kCMPixelFormat_24RGB          },
        {AkVCam::PixelFormatRGB16, kCMPixelFormat_16LE565        },
        {AkVCam::PixelFormatRGB15, kCMPixelFormat_16LE555        },
        {AkVCam::PixelFormatUYVY , kCMPixelFormat_422YpCbCr8     },
        {AkVCam::PixelFormatYUY2 , kCMPixelFormat_422YpCbCr8_yuvs}
    };

    return &formatsTable;
}

bool AkVCam::uuidEqual(const REFIID &uuid1, const CFUUIDRef uuid2)
{
    auto iid2 = CFUUIDGetUUIDBytes(uuid2);
    auto puuid1 = reinterpret_cast<const UInt8 *>(&uuid1);
    auto puuid2 = reinterpret_cast<const UInt8 *>(&iid2);

    for (int i = 0; i < 16; i++)
        if (puuid1[i] != puuid2[i])
            return false;

    return true;
}

std::string AkVCam::enumToString(UInt32 value)
{
    auto valueChr = reinterpret_cast<char *>(&value);
    std::string valueStr(valueChr, 4);
    std::reverse(valueStr.begin(), valueStr.end());
    valueStr = "'" + valueStr + "'";

    return valueStr;
}

FourCharCode AkVCam::formatToCM(PixelFormat format)
{
    for (auto &fmt: *formatsTable())
        if (fmt.first == format)
            return fmt.second;

    return FourCharCode(0);
}

AkVCam::PixelFormat AkVCam::formatFromCM(FourCharCode format)
{
    for (auto &fmt: *formatsTable())
        if (fmt.second == format)
            return fmt.first;

    return PixelFormat(0);
}

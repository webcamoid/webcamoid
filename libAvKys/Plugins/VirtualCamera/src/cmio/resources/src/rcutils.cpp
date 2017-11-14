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

#include "rcutils.h"

uint64_t RcUtils::fromBigEndian(uint64_t num)
{
    auto num8 = reinterpret_cast<uint8_t *>(&num);

    return (uint64_t(num8[0]) << 56)
         | (uint64_t(num8[1]) << 48)
         | (uint64_t(num8[2]) << 40)
         | (uint64_t(num8[3]) << 32)
         | (uint64_t(num8[4]) << 24)
         | (uint64_t(num8[5]) << 16)
         | (uint64_t(num8[6]) << 8)
         |  uint64_t(num8[7]);
}

uint32_t RcUtils::fromBigEndian(uint32_t num)
{
    auto num8 = reinterpret_cast<uint8_t *>(&num);

    return (uint32_t(num8[0]) << 24)
         | (uint32_t(num8[1]) << 16)
         | (uint32_t(num8[2]) << 8)
         |  uint32_t(num8[3]);
}

uint16_t RcUtils::fromBigEndian(uint16_t num)
{
    auto num8 = reinterpret_cast<uint8_t *>(&num);

    return uint16_t((uint32_t(num8[0]) << 8) | uint32_t(num8[1]));
}

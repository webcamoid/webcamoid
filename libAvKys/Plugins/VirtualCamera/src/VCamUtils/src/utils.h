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

#ifndef AKVCAMUTILS_UTILS_H
#define AKVCAMUTILS_UTILS_H

#include <cstdint>

#include "logger/logger.h"

#ifndef UNUSED
    #define UNUSED(x) (void)(x);
#endif

#define GLOBAL_STATIC(type, variableName) \
    type *variableName() \
    { \
        static type _##variableName; \
        \
        return &_##variableName; \
    }

#define GLOBAL_STATIC_WITH_ARGS(type, variableName, ...) \
    type *variableName() \
    { \
        static type _##variableName {__VA_ARGS__}; \
        \
        return &_##variableName; \
    }

namespace AkVCam
{
    uint64_t id();
    std::string timeStamp();
    bool isEqualFile(const std::string &file1, const std::string &file2);
}

#endif // AKVCAMUTILS_UTILS_H

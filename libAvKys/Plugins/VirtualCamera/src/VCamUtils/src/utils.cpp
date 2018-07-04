/* Webcamoid, webcam capture application.
 * Copyright (C) 2018  Gonzalo Exequiel Pedone
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

#include <cstring>
#include <ctime>
#include <fstream>

#include "utils.h"

uint64_t AkVCam::id()
{
    static uint64_t id = 0;

    return id++;
}

std::string AkVCam::timeStamp()
{
    char ts[256];
    auto time = std::time(nullptr);
    strftime(ts, 256, "%Y%m%d%H%M%S", std::localtime(&time));

    return std::string(ts);
}

std::string AkVCam::replace(const std::string &str,
                            const std::string &from,
                            const std::string &to)
{
    auto newStr = str;

    for (auto pos = newStr.find(from);
         pos != std::string::npos;
         pos = newStr.find(from))
        newStr.replace(pos, from.size(), to);

    return newStr;
}

std::wstring AkVCam::replace(const std::wstring &str,
                             const std::wstring &from,
                             const std::wstring &to)
{
    auto newStr = str;

    for (auto pos = newStr.find(from);
         pos != std::wstring::npos;
         pos = newStr.find(from))
        newStr.replace(pos, from.size(), to);

    return newStr;
}

bool AkVCam::isEqualFile(const std::wstring &file1, const std::wstring &file2)
{
    if (file1 == file2)
        return true;

    std::fstream f1;
    std::fstream f2;
    f1.open(std::string(file1.begin(), file1.end()), std::ios_base::in);
    f2.open(std::string(file2.begin(), file2.end()), std::ios_base::in);

    if (!f1.is_open() || !f2.is_open())
        return false;

    const size_t bufferSize = 1024;
    char buffer1[bufferSize];
    char buffer2[bufferSize];
    memset(buffer1, 0, bufferSize);
    memset(buffer2, 0, bufferSize);

    while (!f1.eof() && !f2.eof()) {
        f1.read(buffer1, bufferSize);
        f2.read(buffer2, bufferSize);

        if (memcmp(buffer1, buffer2, bufferSize))
            return false;
    }

    return true;
}

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

#include <fstream>

#include "logger.h"

#ifdef QT_DEBUG

namespace Ak
{
    namespace Logger
    {
        static bool initialized = false;
        static std::string fileNamePrivate;
        static std::fstream logFilePrivate;
    }
}

void Ak::Logger::start(const std::string &fileName)
{
    fileNamePrivate = fileName;
    initialized = true;
}

std::ostream &Ak::Logger::log()
{
    if (!initialized || fileNamePrivate.empty())
        return std::cout;

    if (!logFilePrivate.is_open())
        logFilePrivate.open(fileNamePrivate, std::ios_base::out | std::ios_base::app);

    return logFilePrivate;
}

void Ak::Logger::stop()
{
    initialized = false;
    fileNamePrivate = {};

    if (logFilePrivate.is_open())
        logFilePrivate.close();
}

#endif

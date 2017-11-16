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

namespace AkVCam
{
    class LoggerPrivate
    {
        public:
            bool initialized;
            std::string fileNamePrivate;
            std::fstream logFilePrivate;

            LoggerPrivate():
                initialized(false)
            {
            }
    };

    inline LoggerPrivate *loggerPrivate()
    {
        static LoggerPrivate logger;

        return &logger;
    }
}

void AkVCam::Logger::start(const std::string &fileName)
{
    loggerPrivate()->fileNamePrivate = fileName;
    loggerPrivate()->initialized = true;
}

std::ostream &AkVCam::Logger::log()
{
    if (!loggerPrivate()->initialized
        || loggerPrivate()->fileNamePrivate.empty())
        return std::cout;

    if (!loggerPrivate()->logFilePrivate.is_open())
        loggerPrivate()->logFilePrivate.open(loggerPrivate()->fileNamePrivate,
                                             std::ios_base::out
                                             | std::ios_base::app);

    return loggerPrivate()->logFilePrivate;
}

void AkVCam::Logger::stop()
{
    loggerPrivate()->initialized = false;
    loggerPrivate()->fileNamePrivate = {};

    if (loggerPrivate()->logFilePrivate.is_open())
        loggerPrivate()->logFilePrivate.close();
}

#endif

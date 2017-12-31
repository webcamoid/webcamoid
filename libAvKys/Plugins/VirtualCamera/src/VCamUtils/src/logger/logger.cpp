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

#include <iomanip>
#include <sstream>
#include <fstream>
#include <chrono>

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

void AkVCam::Logger::start(const std::string &fileName,
                           const std::string &extension)
{
    std::stringstream ss;
    auto time = std::time(nullptr);
    ss << fileName
       << "-"
       << std::put_time(std::localtime(&time), "%Y%m%d%H%M%S")
       << "."
       << extension;

    loggerPrivate()->fileNamePrivate = ss.str();
    loggerPrivate()->initialized = true;
}

std::ostream &AkVCam::Logger::log()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto nowMSecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    auto nowSecs = std::chrono::duration_cast<std::chrono::seconds>(nowMSecs);
    std::stringstream ss;
    std::time_t time = nowSecs.count();
    ss << std::put_time(std::localtime(&time), "[%Y-%m-%d %H:%M:%S.");
    ss << nowMSecs.count() % 1000 << "] ";

    if (!loggerPrivate()->initialized
        || loggerPrivate()->fileNamePrivate.empty())
        return std::cout << ss.str();

    if (!loggerPrivate()->logFilePrivate.is_open())
        loggerPrivate()->logFilePrivate.open(loggerPrivate()->fileNamePrivate,
                                             std::ios_base::out
                                             | std::ios_base::app);

    if (!loggerPrivate()->logFilePrivate.is_open())
        return std::cout << ss.str();

    return loggerPrivate()->logFilePrivate << ss.str();
}

void AkVCam::Logger::stop()
{
    loggerPrivate()->initialized = false;
    loggerPrivate()->fileNamePrivate = {};

    if (loggerPrivate()->logFilePrivate.is_open())
        loggerPrivate()->logFilePrivate.close();
}

#endif

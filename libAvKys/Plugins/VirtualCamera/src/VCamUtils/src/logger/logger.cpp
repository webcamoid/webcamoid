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

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>

#include "logger.h"
#include "../utils.h"

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
    ss << fileName
       << "-"
       << timeStamp()
       << "."
       << extension;

    loggerPrivate()->fileNamePrivate = ss.str();
    loggerPrivate()->initialized = true;
}

std::string AkVCam::Logger::header()
{
    auto now = std::chrono::system_clock::now();
    auto nowMSecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    char timeStamp[256];
    auto time = std::chrono::system_clock::to_time_t(now);
    strftime(timeStamp, 256, "%Y-%m-%d %H:%M:%S", std::localtime(&time));
    std::stringstream ss;
    ss << "["
       << timeStamp
       << "." << nowMSecs.count() % 1000 << ", " << std::this_thread::get_id() << "] ";

    return ss.str();
}

std::ostream &AkVCam::Logger::out()
{
    if (!loggerPrivate()->initialized
        || loggerPrivate()->fileNamePrivate.empty())
        return std::cout;

    if (!loggerPrivate()->logFilePrivate.is_open())
        loggerPrivate()->logFilePrivate.open(loggerPrivate()->fileNamePrivate,
                                             std::ios_base::out
                                             | std::ios_base::app);

    if (!loggerPrivate()->logFilePrivate.is_open())
        return std::cout;

    return loggerPrivate()->logFilePrivate;
}

void AkVCam::Logger::log()
{
    tlog(header());
}

void AkVCam::Logger::tlog()
{
    out() << std::endl;
}

void AkVCam::Logger::stop()
{
    loggerPrivate()->initialized = false;
    loggerPrivate()->fileNamePrivate = {};

    if (loggerPrivate()->logFilePrivate.is_open())
        loggerPrivate()->logFilePrivate.close();
}

#endif

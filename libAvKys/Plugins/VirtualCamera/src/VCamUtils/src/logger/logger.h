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

#ifndef AKVCAMUTILS_LOGGER_H
#define AKVCAMUTILS_LOGGER_H

#include <iostream>

#ifdef QT_DEBUG
    #define AkLoggerStart(...) AkVCam::Logger::start(__VA_ARGS__)
    #define AkLoggerLog(data) AkVCam::Logger::log() << data << std::endl
    #define AkLoggerStop() AkVCam::Logger::stop(fileName)

    namespace AkVCam
    {
        namespace Logger
        {
            void start(const std::string &fileName=std::string(),
                       const std::string &extension=std::string());
            std::ostream &log();
            void stop();
        }
    }
#else
    #define AkLoggerStart(fileName)
    #define AkLoggerLog(data)
    #define AkLoggerStop(fileName)
#endif

#endif // AKVCAMUTILS_LOGGER_H

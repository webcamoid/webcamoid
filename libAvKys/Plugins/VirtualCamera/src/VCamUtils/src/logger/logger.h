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
    #define AkLoggerLog(...) AkVCam::Logger::log(__VA_ARGS__)
    #define AkLoggerStop() AkVCam::Logger::stop()

    namespace AkVCam
    {
        namespace Logger
        {
            typedef void (*LogCallBack)(const char *data,
                                        size_t size,
                                        void *userData);

            void start(const std::string &fileName=std::string(),
                       const std::string &extension=std::string());
            void start(LogCallBack callback, void *userData);
            std::string header();
            std::ostream &out();
            void log();
            void tlog();
            void stop();

            template<typename First, typename... Next>
            void tlog(const First &first, const Next &... next)
            {
                out() << first;
                tlog(next...);
            }

            template<typename... Param>
            void log(const Param &... param)
            {
                tlog(header(), " ", param...);
            }
        }
    }
#else
    #define AkLoggerStart(...)
    #define AkLoggerLog(...)
    #define AkLoggerStop()
#endif

#endif // AKVCAMUTILS_LOGGER_H

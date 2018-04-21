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

#include <string>
#include <windows.h>

#include "service.h"
#include "utils.h"
#include "VCamUtils/src/utils.h"

int main(int argc, char **argv)
{
    AkLoggerStart(AkVCam::tempPath() + "\\" DSHOW_PLUGIN_ASSISTANT_NAME, "log");

    AkVCam::Service service;

    if (argc > 1) {
        if (!strcmp(argv[1], "-i") || !strcmp(argv[1], "--install")) {
            return service.install()? EXIT_SUCCESS: EXIT_FAILURE;
        } else if (!strcmp(argv[1], "-u") || !strcmp(argv[1], "--uninstall")) {
            service.uninstall();

            return EXIT_SUCCESS;
        } else if (!strcmp(argv[1], "-d") || !strcmp(argv[1], "--debug")) {
            service.debug();

            return EXIT_SUCCESS;
        } else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
            service.showHelp(argc, argv);

            return EXIT_SUCCESS;
        }
    }

    AkLoggerLog("Setting service dispatcher");

    WCHAR serviceName[] = TEXT(DSHOW_PLUGIN_ASSISTANT_NAME);
    SERVICE_TABLE_ENTRY serviceTable[] = {
        {serviceName, serviceMain},
        {nullptr    , nullptr    }
    };

    if (!StartServiceCtrlDispatcher(serviceTable)) {
        AkLoggerLog("Service dispatcher failed");

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

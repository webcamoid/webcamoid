/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

pragma Singleton

import QtQuick 2.12

Item {
    function vcamDriverBusyMessage()
    {
        let message =
            qsTr("The virtual camera is in use by the following applications:")
        message += "<br/><br/>"

        let pids = videoLayer.clientsPids

        for (let i in pids)
            message += "<b>"
                    + pids[i]
                    + "</b>: <i>" + videoLayer.clientExe(pids[i])
                    + "</i><br/>"

        message += "<br/>"
        message += qsTr("Stop the camera in those applications or close them and try again.")

        return message
    }
}

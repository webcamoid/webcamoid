/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1

GridLayout {
    id: recCameraControls
    columns: 2

    function updateDevices()
    {
        var model = []
        var devices = VirtualCamera.medias

        for (var device in devices) {
            var deviceStr = VirtualCamera.description(devices[device])
                            + " ("
                            + devices[device]
                            + ")"

            model.push(deviceStr)
        }

        cbxDevices.model = model
        cbxDevices.currentIndex = devices.indexOf(VirtualCamera.media)
    }

    Connections {
        target: VirtualCamera

        onMediasChanged: updateDevices()
    }
    Component.onCompleted: updateDevices()

    Label {
        text: qsTr("Devices")
    }
    ComboBox {
        id: cbxDevices
        Layout.fillWidth: true

        onCurrentIndexChanged: VirtualCamera.media = VirtualCamera.medias[currentIndex]
    }
}

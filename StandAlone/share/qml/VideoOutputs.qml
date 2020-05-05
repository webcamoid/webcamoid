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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

ScrollView {
    id: view

    ColumnLayout {
        width: view.width

        Button {
            text: qsTr("Add output")
            icon.source: "image://icons/add"
            flat: true
        }
        OptionList {
            id: devicesList
            textRole: "description"
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height
            Layout.fillWidth: true
            Layout.fillHeight: true

            function updateDevices() {
            }

            Connections {
                target: videoLayer

                //onOutputsChanged: devicesList.updateDevices()
            }

            Component.onCompleted: devicesList.updateDevices()
        }
    }
}

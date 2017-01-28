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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import AkQml 1.0

ColumnLayout {
    Component.onCompleted: {
        Webcamoid.removeInterface("itmVirtualCameraControls")
        Webcamoid.embedVirtualCameraControls("itmVirtualCameraControls")
    }
    Connections {
        target: Webcamoid

        onInterfaceLoaded: {
            Webcamoid.removeInterface("itmVirtualCameraControls")
            Webcamoid.embedVirtualCameraControls("itmVirtualCameraControls")
        }
    }

    GroupBox {
        title: qsTr("Virtual camera")
        checkable: true
        checked: Webcamoid.enableVirtualCamera
        Layout.fillWidth: true

        onCheckedChanged: Webcamoid.enableVirtualCamera = checked

        GridLayout {
            id: itmVirtualCameraControls
            objectName: "itmVirtualCameraControls"
            anchors.fill: parent

            onChildrenChanged: children[0].anchors.fill = itmVirtualCameraControls
        }
    }
    Label {
        Layout.fillHeight: true
    }
}

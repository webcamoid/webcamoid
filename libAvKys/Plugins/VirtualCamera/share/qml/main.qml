/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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
import AkPluginVirtualCamera 1.0

GridLayout {
    id: recCameraControls
    columns: 2

    Label {
        text: qsTr("Horizontal mirror")
    }
    Switch {
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        checked: virtualCamera.horizontalMirrored

        onCheckedChanged: virtualCamera.horizontalMirrored = checked
    }
    Label {
        text: qsTr("Vertical mirror")
    }
    Switch {
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        checked: virtualCamera.verticalMirrored

        onCheckedChanged: virtualCamera.verticalMirrored = checked
    }
    Label {
        text: qsTr("Scaling")
    }
    ComboBox {
        textRole: "description"
        Layout.fillWidth: true
        model: ListModel {
            ListElement
            {
                option: VirtualCamera.ScalingFast
                description: "Fast"
            }
            ListElement {
                option: VirtualCamera.ScalingLinear
                description: "Linear"
            }
        }
    }
    Label {
        text: qsTr("Aspect ratio")
    }
    ComboBox {
        textRole: "description"
        Layout.fillWidth: true
        model: ListModel {
            ListElement {
                option: VirtualCamera.AspectRatioIgnore
                description: "Ignore"
            }
            ListElement
            {
                option: VirtualCamera.AspectRatioKeep
                description: "Keep"
            }
            ListElement {
                option: VirtualCamera.AspectRatioExpanding
                description: "Expanding"
            }
        }
    }
    Label {
        text: qsTr("Swap red and blue")
    }
    Switch {
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        checked: virtualCamera.swapRgb

        onCheckedChanged: virtualCamera.swapRgb = checked
    }
}

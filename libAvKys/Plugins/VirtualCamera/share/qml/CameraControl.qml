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

GridLayout {
    id: grdCameraControl
    columns: 3

    property variant controlParams: []
    property int value: 0
    property int minimumValue: 0
    property int maximumValue: 1
    property int stepSize: 1
    property variant model: []
    property int minimumLeftWidth: 0
    property int minimumRightWidth: 0
    readonly property alias leftWidth: txtControlName.width
    readonly property alias rightWidth: spbRange.width

    signal controlChanged(string controlName, int value)

    onControlParamsChanged: {
        state = controlParams.length > 1? controlParams[1]: ""
        minimumValue = controlParams.length > 2? controlParams[2]: 0
        maximumValue = controlParams.length > 3? controlParams[3]: 1
        stepSize = controlParams.length > 4? controlParams[4]: 1
        model = controlParams.length > 7? controlParams[7]: []
        value = controlParams.length > 6? controlParams[6]: 0
        spbRange.value = value
    }

    Label {
        id: txtControlName
        text: controlParams.length > 0? controlParams[0]: ""
        Layout.minimumWidth: minimumLeftWidth
    }
    Slider {
        id: sldRange
        from: grdCameraControl.minimumValue
        to: grdCameraControl.maximumValue
        stepSize: grdCameraControl.stepSize
        value: grdCameraControl.value
        Layout.fillWidth: true
        visible: false
        Accessible.name: txtControlName.text

        onValueChanged: {
            if (visible) {
                spbRange.value = value
                grdCameraControl.controlChanged(controlParams.length > 0? controlParams[0]: "", value)
            }
        }
    }
    SpinBox {
        id: spbRange
        value: sldRange.value
        from: grdCameraControl.minimumValue
        to: grdCameraControl.maximumValue
        stepSize: grdCameraControl.stepSize
        Layout.minimumWidth: minimumRightWidth
        visible: false
        editable: true
        Accessible.name: txtControlName.text

        onValueChanged: {
            if (visible)
                sldRange.value = value
        }
    }

    RowLayout {
        id: chkBoolContainer
        Layout.columnSpan: 2
        Layout.fillWidth: true
        visible: false

        Label {
            Layout.fillWidth: true
        }
        Switch {
            id: chkBool
            checked: grdCameraControl.value !== 0
            Accessible.name: txtControlName.text

            onCheckedChanged: {
                if (visible)
                    grdCameraControl.controlChanged(controlParams.length > 0? controlParams[0]: "", checked? 1: 0)
            }
        }
    }

    ComboBox {
        id: cbxMenu
        model: grdCameraControl.model
        currentIndex: grdCameraControl.value
        Layout.fillWidth: true
        Layout.columnSpan: 2
        visible: false
        //Accessible.description: lblControlName.text
        Accessible.name: txtControlName.text

        onCurrentIndexChanged: {
            if (visible)
                grdCameraControl.controlChanged(controlParams.length > 0? controlParams[0]: "", currentIndex)
        }
    }

    states: [
        State {
            name: "integer"

            PropertyChanges {
                target: sldRange
                visible: true
            }
            PropertyChanges {
                target: spbRange
                visible: true
            }
        },
        State {
            name: "integer64"

            PropertyChanges {
                target: sldRange
                visible: true
            }
            PropertyChanges {
                target: spbRange
                visible: true
            }
        },
        State {
            name: "boolean"

            PropertyChanges {
                target: chkBoolContainer
                visible: true
            }
        },
        State {
            name: "menu"

            PropertyChanges {
                target: cbxMenu
                visible: true
            }
        }
    ]
}

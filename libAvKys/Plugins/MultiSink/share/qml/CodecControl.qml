/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

GridLayout {
    id: grdCameraControl
    columns: 2
    state: controlParams.length > 2? controlParams[2]: ""

    property variant controlParams: []
    property string controlName: controlParams.length > 0? controlParams[0]: ""
    property string controlDescription: controlParams.length > 1? controlParams[1]: ""
    property real minimumValue: controlParams.length > 3? controlParams[3]: 0
    property real maximumValue: controlParams.length > 4? controlParams[4]: 0
    property real stepSize: controlParams.length > 5? controlParams[5]: 0
    property variant value: controlParams.length > 7? controlParams[7]: 0
    property int minimumLeftWidth: 0
    property int minimumRightWidth: 0
    readonly property alias leftWidth: lblControl.width
    readonly property alias rightWidth: spbRange.width
    readonly property int maxSteps: 4096
    property bool discreteRange: (maximumValue - minimumValue) <= maxSteps * stepSize

    signal controlChanged(string controlName, variant value)

    function updateMenu()
    {
        menuModel.clear()

        if (controlParams.length < 9 || controlParams[2] != "menu")
            return

        for (let i in controlParams[8]) {
            let description = controlParams[8][i][0]

            if (controlParams[8][i][1].length > 0)
                description += " - " + controlParams[8][i][1]

            menuModel.append({
                value: controlParams[8][i][0],
                description: description
            })
        }

        cbxMenu.currentIndex = currentMenuIndex(controlParams)
    }

    function currentMenuIndex(controlParams)
    {
        if (controlParams.length < 9 || controlParams[2] != "menu")
            return -1

        for (let i in controlParams[8])
            if (controlParams[8][i][0] == controlParams[7])
                return i

        return -1
    }

    function updateFlags()
    {
        // Remove old controls.
        for(let i = clyFlags.children.length - 1; i >= 0; i--)
            clyFlags.children[i].destroy()

        if (controlParams.length < 9 || controlParams[2] != "flags")
            return

        // Create new ones.
        for (let i in controlParams[8]) {
            let flag = classCheckBox.createObject(clyFlags)
            flag.children[0].text = controlParams[8][i][0]
            flag.children[1].checked =
                    controlParams[7].indexOf(flag.children[0].text) >= 0

            flag.children[1].onCheckedChanged.connect(function (checked)
            {
                var flags = []

                for (var i in clyFlags.children) {
                    if (clyFlags.children[i].checked)
                        flags += clyFlags.children[i].text
                }

                if (gbxFlags.visible)
                    grdCameraControl.controlChanged(controlName, flags)
            })
        }
    }

    onControlParamsChanged: {
        updateMenu();
        updateFlags();
    }

    Component.onCompleted: {
        updateMenu();
        updateFlags();
    }

    Label {
        id: lblControl
        text: controlName
        Layout.minimumWidth: minimumLeftWidth
    }

    TextField {
        id: txtString
        text: controlParams[2] == "string"? grdCameraControl.value: ""
        Layout.fillWidth: true
        visible: false

        onTextChanged: grdCameraControl.controlChanged(controlName, text)
    }

    TextField {
        id: txtFrac
        text: controlParams[2] == "frac"? grdCameraControl.value: ""
        Layout.fillWidth: true
        visible: false
        validator: RegExpValidator {
            regExp: /-?\d+\/\d+/
        }

        onTextChanged: grdCameraControl.controlChanged(controlName, text)
    }

    GridLayout {
        id: glyRange
        columns: 2
        visible: false

        Slider {
            id: sldRange
            from: discreteRange? grdCameraControl.minimumValue: 0
            to: discreteRange? grdCameraControl.maximumValue: 1
            stepSize: discreteRange? grdCameraControl.stepSize: 1
            value: controlParams[2] == "number"? grdCameraControl.value: 0
            Layout.fillWidth: true
            visible: discreteRange

            onValueChanged: {
                if (visible) {
                    spbRange.value = spbRange.multiplier * value
                    grdCameraControl.controlChanged(controlName, value)
                }
            }
        }
        SpinBox {
            id: spbRange
            value: multiplier * sldRange.value
            from: multiplier * (discreteRange? grdCameraControl.minimumValue: 0)
            to: multiplier * (discreteRange? grdCameraControl.maximumValue: 1)
            stepSize: multiplier * (discreteRange? grdCameraControl.stepSize: 1)
            Layout.minimumWidth: minimumRightWidth
            visible: discreteRange
            editable: true

            readonly property int decimals:
                discreteRange && grdCameraControl.stepSize < 1? 2: 0
            readonly property int multiplier: Math.pow(10, decimals)

            validator: DoubleValidator {
                bottom: Math.min(spbRange.from, spbRange.to)
                top:  Math.max(spbRange.from, spbRange.to)
            }
            textFromValue: function(value, locale) {
                return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
            }
            valueFromText: function(text, locale) {
                return Number.fromLocaleString(locale, text) * multiplier
            }
            onValueModified: {
                if (visible)
                    sldRange.value = value / multiplier
            }
        }
        TextField {
            id: txtRange
            text: controlParams[2] == "number"? grdCameraControl.value: ""
            Layout.columnSpan: 2
            Layout.fillWidth: true
            visible: !discreteRange
            validator: RegExpValidator {
                regExp: /[-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?/
            }

            onTextChanged: {
                if (visible)
                    grdCameraControl.controlChanged(controlName, Number(text))
            }
        }
    }

    RowLayout {
        id: chkBool
        Layout.fillWidth: true
        visible: false

        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: grdCameraControl.value !== 0

            onCheckedChanged: {
                if (visible)
                    grdCameraControl.controlChanged(controlName, checked)
            }
        }
    }

    ComboBox {
        id: cbxMenu
        model: ListModel {
            id: menuModel
        }
        textRole: "description"
        Layout.fillWidth: true
        visible: false

        onCurrentIndexChanged: {
            if (visible) {
                var value = menuModel.get(currentIndex).value;
                grdCameraControl.controlChanged(controlName, value);
            }
        }
    }

    GroupBox {
        id: gbxFlags
        title: controlName
        Layout.fillWidth: true
        Layout.columnSpan: 2
        visible: false

        ColumnLayout {
            id: clyFlags
            anchors.fill: parent
        }

        Component {
            id: classCheckBox

            RowLayout {
                Label {
                    Layout.fillWidth: true
                }
                Switch {
                }
            }
        }
    }

    states: [
        State {
            name: "string"

            PropertyChanges {
                target: txtString
                visible: true
            }
        },
        State {
            name: "number"

            PropertyChanges {
                target: glyRange
                visible: true
            }
        },
        State {
            name: "boolean"

            PropertyChanges {
                target: chkBool
                visible: true
            }
        },
        State {
            name: "menu"

            PropertyChanges {
                target: cbxMenu
                visible: true
            }
        },
        State {
            name: "flags"

            PropertyChanges {
                target: gbxFlags
                visible: true
            }
            PropertyChanges {
                target: lblControl
                visible: false
            }
        },
        State {
            name: "frac"

            PropertyChanges {
                target: txtFrac
                visible: true
            }
        }
    ]
}

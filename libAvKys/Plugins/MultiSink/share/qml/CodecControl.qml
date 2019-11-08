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
import AkQmlControls 1.0

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
    property bool showLabel: controlParams[2] != "string"
                             && controlParams[2] != "frac"
                             && controlParams[2] != "number"

    signal controlChanged(string controlName, variant value)

    function updateMenu()
    {
        menuModel.clear();

        if (controlParams.length < 9 || controlParams[2] != "menu")
            return;

        for (var i in controlParams[8]) {
            var description = controlParams[8][i][0];

            if (controlParams[8][i][1].length > 0)
                description += " - " + controlParams[8][i][1];

            menuModel.append({
                value: controlParams[8][i][0],
                description: description
            });
        }

        cbxMenu.currentIndex = currentMenuIndex(controlParams);
    }

    function currentMenuIndex(controlParams)
    {
        if (controlParams.length < 9 || controlParams[2] != "menu")
            return -1;

        for (var i in controlParams[8])
            if (controlParams[8][i][0] == controlParams[7])
                return i;

        return -1;
    }

    function updateFlags()
    {
        // Remove old controls.
        for(var i = clyFlags.children.length - 1; i >= 0 ; i--)
            clyFlags.children[i].destroy()

        if (controlParams.length < 9 || controlParams[2] != "flags")
            return;

        // Create new ones.
        for (var i in controlParams[8]) {
            var flag = classCheckBox.createObject(clyFlags);
            flag.text = controlParams[8][i][0];
            flag.checked = controlParams[7].indexOf(flag.text) >= 0;

            flag.onCheckedChanged.connect(function (checked)
            {
                var flags = [];

                for (var i in clyFlags.children) {
                    if (clyFlags.children[i].checked)
                        flags += clyFlags.children[i].text;
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
        Layout.column: showLabel || discreteRange? 0: 1
        Layout.minimumWidth: minimumLeftWidth
        visible: showLabel || discreteRange
    }

    TextField {
        id: txtString
        text: controlParams[2] == "string"? grdCameraControl.value: ""
        placeholderText: controlName
        Layout.column: showLabel || discreteRange? 1: 0
        Layout.columnSpan: 2
        Layout.fillWidth: true
        visible: false

        onTextChanged: grdCameraControl.controlChanged(controlName, text)
    }

    TextField {
        id: txtFrac
        text: controlParams[2] == "frac"? grdCameraControl.value: ""
        placeholderText: controlName
        Layout.column: showLabel || discreteRange? 1: 0
        Layout.columnSpan: 2
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
        Layout.column: discreteRange? 1: 0
        Layout.columnSpan: discreteRange? 1: 2
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
                    spbRange.rvalue = value
                    grdCameraControl.controlChanged(controlName, value)
                }
            }
        }
        AkSpinBox {
            id: spbRange
            minimumValue: discreteRange? grdCameraControl.minimumValue: 0
            maximumValue: discreteRange? grdCameraControl.maximumValue: 1
            step: discreteRange? grdCameraControl.stepSize: 1
            decimals: step < 1? 2: 0
            rvalue: sldRange.value
            Layout.minimumWidth: minimumRightWidth
            visible: discreteRange

            onRvalueChanged: {
                if (visible)
                    sldRange.value = rvalue
            }
        }
        TextField {
            id: txtRange
            text: controlParams[2] == "number"? grdCameraControl.value: ""
            placeholderText: controlName
            Layout.columnSpan: 2
            Layout.fillWidth: true
            visible: !discreteRange
            validator: RegExpValidator {
                regExp: /[-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?/
            }

            onTextChanged: {
                if (visible)
                    grdCameraControl.controlChanged(controlName,
                                                    parseFloat(text))
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

            SwitchDelegate {
                Layout.fillWidth: true
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

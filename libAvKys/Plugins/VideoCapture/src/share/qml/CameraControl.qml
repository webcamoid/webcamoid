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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GridLayout {
    id: grdCameraControl
    columns: 2

    property variant controlParams: []
    property real value: 0
    property real minimumValue: 0
    property real maximumValue: 1
    property real stepSize: 1
    property variant model: []
    property int minimumLeftWidth: 0
    property int minimumRightWidth: 0
    readonly property real leftWidth: lblControlName.implicitWidth
    readonly property real rightWidth: {
        if (state === "integer"
            || state === "integer64"
            || state === "float") {
            return controlLoader.item && controlLoader.item.children[1]?
                        controlLoader.item.children[1].implicitWidth:
                        0
        }

        return controlLoader.item? controlLoader.item.implicitWidth: 0
    }
    readonly property real spinBoxWidth: {
        if (state === "integer"
            || state === "integer64"
            || state === "float") {
            return controlLoader.item && controlLoader.item.children[1]?
                        controlLoader.item.children[1].implicitWidth:
                        0
        }

        return 0
    }

    signal controlChanged(string controlName, variant value)

    onControlParamsChanged: {
        if (!controlParams || controlParams.length < 2 || !controlParams[1]) {
            state = ""

            return
        }

        const type = String(controlParams[1]).toLowerCase().trim()
        state = type
        minimumValue = controlParams.length > 2? controlParams[2]: 0
        maximumValue = controlParams.length > 3? controlParams[3]: 1
        stepSize = controlParams.length > 4? controlParams[4]: 1
        model = controlParams.length > 7? controlParams[7]: []
        value = controlParams.length > 6? controlParams[6]: 0
    }

    Label {
        id: lblControlName
        text: controlParams.length > 0? controlParams[0]: ""
        Layout.minimumWidth: minimumLeftWidth
        visible: grdCameraControl.state !== ""
        Layout.alignment: Qt.AlignLeft
    }

    Loader {
        id: controlLoader
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignRight

        sourceComponent: {
            switch (grdCameraControl.state) {
                case "integer":
                case "integer64":
                    return integerComponent
                case "float":
                    return floatComponent
                case "boolean":
                    return booleanComponent
                case "menu":
                    return menuComponent
                default:
                    return null
            }
        }
    }

    Component {
        id: integerComponent

        RowLayout {
            Layout.minimumWidth: minimumRightWidth

            Slider {
                id: sldRange
                from: grdCameraControl.minimumValue
                to: grdCameraControl.maximumValue
                stepSize: grdCameraControl.stepSize
                value: grdCameraControl.value
                Layout.fillWidth: true
                Accessible.name: lblControlName.text

                onValueChanged: {
                    spbRange.value = value
                    grdCameraControl.controlChanged(controlParams.length > 0?
                                                        controlParams[0]:
                                                        "",
                                                    value)
                }
            }
            SpinBox {
                id: spbRange
                value: sldRange.value
                from: grdCameraControl.minimumValue
                to: grdCameraControl.maximumValue
                stepSize: grdCameraControl.stepSize
                editable: true
                Accessible.name: lblControlName.text

                onValueChanged: {
                    if (!sldRange.pressed) {
                        sldRange.value = value
                        grdCameraControl.controlChanged(controlParams.length > 0?
                                                            controlParams[0]:
                                                            "",
                                                        value)
                    }
                }
            }
        }
    }

    Component {
        id: floatComponent

        RowLayout {
            Layout.minimumWidth: minimumRightWidth

            Slider {
                id: sldRange
                from: grdCameraControl.minimumValue
                to: grdCameraControl.maximumValue
                stepSize: grdCameraControl.stepSize
                value: grdCameraControl.value
                Layout.fillWidth: true
                Accessible.name: lblControlName.text

                onValueChanged: {
                    spbRangeFloat.value = spbRangeFloat.multiplier * value
                    grdCameraControl.controlChanged(controlParams.length > 0?
                                                        controlParams[0]:
                                                        "",
                                                    value)
                }
            }
            SpinBox {
                id: spbRangeFloat
                value: multiplier * sldRange.value
                from: multiplier * grdCameraControl.minimumValue
                to: multiplier * grdCameraControl.maximumValue
                stepSize: Math.round(multiplier * grdCameraControl.stepSize)
                editable: true
                Accessible.name: lblControlName.text

                readonly property int decimals: 2
                readonly property int multiplier: Math.pow(10, decimals)

                validator: DoubleValidator {
                    bottom: Math.min(spbRangeFloat.from, spbRangeFloat.to)
                    top: Math.max(spbRangeFloat.from, spbRangeFloat.to)
                }
                textFromValue: function(value, locale) {
                    return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
                }
                valueFromText: function(text, locale) {
                    return Number.fromLocaleString(locale, text) * multiplier
                }

                onValueModified: {
                    if (!sldRange.pressed) {
                        sldRange.value = value / multiplier
                        grdCameraControl.controlChanged(controlParams.length > 0?
                                                            controlParams[0]:
                                                            "",
                                                        value / multiplier)
                    }
                }
            }
        }
    }

    Component {
        id: booleanComponent

        RowLayout {
            Item {
                Layout.fillWidth: true
            }
            Switch {
                id: chkBool
                checked: grdCameraControl.value !== 0
                Accessible.name: lblControlName.text

                onCheckedChanged: {
                    grdCameraControl.controlChanged(controlParams.length > 0?
                                                        controlParams[0]:
                                                        "",
                                                    checked? 1: 0);
                }
            }
        }
    }

    Component {
        id: menuComponent

        ComboBox {
            id: cbxMenu
            model: grdCameraControl.model
            currentIndex: grdCameraControl.value
            Layout.fillWidth: true
            Accessible.description: lblControlName.text

            onCurrentIndexChanged: {
                grdCameraControl.controlChanged(controlParams.length > 0?
                                                    controlParams[0]:
                                                    "",
                                                currentIndex)
            }
        }
    }
}

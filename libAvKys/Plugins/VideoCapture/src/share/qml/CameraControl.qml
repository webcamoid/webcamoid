/* Webcamoid, camera capture application.
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
import Ak
import AkControls as AK

ColumnLayout {
    id: grdCameraControl

    property variant controlParams: []
    property real value: 0
    property real defaultValue: 0
    property real minimumValue: 0
    property real maximumValue: 1
    property real stepSize: 1
    property variant model: []

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
        defaultValue = controlParams.length > 5? controlParams[5]: 0
        value = controlParams.length > 6? controlParams[6]: 0
        model = controlParams.length > 7? controlParams[7]: []
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

        ColumnLayout {
            Label {
                id: intDescription
                text: controlParams.length > 0? controlParams[0]: ""
                font.bold: true
                Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }
            AK.StickySlider {
                id: sldRange
                from: grdCameraControl.minimumValue
                to: grdCameraControl.maximumValue
                stepSize: grdCameraControl.stepSize
                value: grdCameraControl.value
                stickyPoints: [grdCameraControl.defaultValue]
                Layout.fillWidth: true
                Accessible.name: intDescription.text

                onValueChanged: {
                    grdCameraControl.controlChanged(controlParams.length > 0?
                                                        controlParams[0]:
                                                        "",
                                                    value)
                }
            }
        }
    }

    Component {
        id: floatComponent

        ColumnLayout {
            Label {
                id: floatDescription
                text: controlParams.length > 0? controlParams[0]: ""
                font.bold: true
                Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }
            AK.StickySlider {
                id: sldRange
                from: grdCameraControl.minimumValue
                to: grdCameraControl.maximumValue
                stepSize: grdCameraControl.stepSize
                value: grdCameraControl.value
                stickyPoints: [grdCameraControl.defaultValue]
                Layout.fillWidth: true
                Accessible.name: floatDescription.text

                onValueChanged: {
                    grdCameraControl.controlChanged(controlParams.length > 0?
                                                        controlParams[0]:
                                                        "",
                                                    value)
                }
            }
        }
    }

    Component {
        id: booleanComponent

        Switch {
            id: chkBool
            text: controlParams.length > 0? controlParams[0]: ""
            checked: grdCameraControl.value !== 0
            Layout.fillWidth: true
            Accessible.name: text

            onCheckedChanged: {
                grdCameraControl.controlChanged(controlParams.length > 0?
                                                    controlParams[0]:
                                                    "",
                                                checked? 1: 0);
            }
        }
    }

    Component {
        id: menuComponent

        AK.LabeledComboBox {
            id: cbxMenu
            label: controlParams.length > 0? controlParams[0]: ""
            model: grdCameraControl.model
            currentIndex: grdCameraControl.value
            Layout.fillWidth: true
            Accessible.description: label

            onCurrentIndexChanged: {
                grdCameraControl.controlChanged(controlParams.length > 0?
                                                    controlParams[0]:
                                                    "",
                                                currentIndex)
            }
        }
    }
}

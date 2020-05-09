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
import Ak 1.0

Dialog {
    id: deviceOptions
    title: qsTr("Video Device Options")
    standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Reset
    width: AkUnit.create(450 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(350 * AkTheme.controlScale, "dp").pixels
    modal: true

    function openOptions(device)
    {
        open()
    }

    ScrollView {
        id: view
        anchors.fill: parent
        contentHeight: deviceControls.height
        clip: true

        ColumnLayout {
            id: deviceControls
            width: view.width

            TextField {
                placeholderText: qsTr("Virtual camera name")
                selectByMouse: true
                Layout.fillWidth: true
            }
            Label {
            }
            TabBar {
                id: tabBar
                Layout.fillWidth: true

                TabButton {
                    text: qsTr("Formats")
                }
                TabButton {
                    text: qsTr("Advanced")
                }
            }
            StackLayout {
                id: stack
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: tabBar.currentIndex
                clip: true

                ColumnLayout {
                    Button {
                        text: qsTr("Add format")
                        icon.source: "image://icons/add"
                        flat: true
                    }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }
                GridLayout {
                    columns: 2

                    Label {
                        text: qsTr("Horizontal mirror")
                    }
                    Switch {
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    }
                    Label {
                        text: qsTr("Vertical mirror")
                    }
                    Switch {
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    }
                    Label {
                        text: qsTr("Scaling")
                    }
                    ComboBox {
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Aspect ratio")
                    }
                    ComboBox {
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Swap red and blue")
                    }
                    Switch {
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    }
                }
            }
        }
    }

    onAccepted: {}
    onRejected: {}
}

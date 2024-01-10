/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import Ak
import Webcamoid

Dialog {
    title: videoSettings?
                qsTr("Video capture settings"):
                qsTr("Image capture settings")
    standardButtons: Dialog.Close
    width: physicalWidth <= 100 || physicalHeight <= 100?
               wdgMainWidget.width: wdgMainWidget.width * 0.5
    height: physicalWidth <= 100 || physicalHeight <= 100?
                wdgMainWidget.height: wdgMainWidget.height * 0.5
    modal: true

    property bool videoSettings: false
    property real physicalWidth: wdgMainWidget.width / Screen.pixelDensity
    property real physicalHeight: wdgMainWidget.height / Screen.pixelDensity
    readonly property alias useFlash: chkFlash.checked
    readonly property alias delay: cbxTimeShot.delay

    onVisibleChanged: chkFlash.forceActiveFocus()

    ScrollView {
        id: scrollView
        anchors.fill: parent

        GridLayout {
            columns: 2
            width: scrollView.width

            Label {
                text: qsTr("Use flash")
            }
            Switch {
                id: chkFlash
                checked: true
                Accessible.name: text
                Accessible.description: qsTr("Use flash when taking a photo")
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            }
            Label {
                text: qsTr("Delay")
                enabled: chkFlash.checked
                visible: !videoSettings
            }
            ComboBox {
                id: cbxTimeShot
                textRole: "text"
                Layout.fillWidth: true
                enabled: chkFlash.checked
                visible: !videoSettings
                Accessible.name: qsTr("Photo timer")
                Accessible.description: qsTr("The time to wait before the photo is taken")
                model: ListModel {
                    id: lstTimeOptions

                    ListElement {
                        text: qsTr("Now")
                        time: 0
                    }
                }

                property real delay: 0

                Component.onCompleted: {
                    for (var i = 5; i < 35; i += 5)
                        lstTimeOptions.append({text: qsTr("%1 seconds").arg(i),
                                               time: i})
                }

                onCurrentIndexChanged:
                    if (cbxTimeShot.currentIndex >= 0) {
                        let item = lstTimeOptions.get(cbxTimeShot.currentIndex)

                        if (item)
                            delay = 1000 * item.time
                    }
            }
        }
    }

    Settings {
        category: "GeneralConfigs"

        property alias useFlash: chkFlash.checked
        property alias photoTimeout: cbxTimeShot.currentIndex
    }
}

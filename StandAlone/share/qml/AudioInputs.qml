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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: view

    Component.onCompleted: devicesList.update()
    onVisibleChanged: devicesList.forceActiveFocus()

    Connections {
        target: audioLayer

        function onInputsChanged()
        {
            devicesList.update()
        }
    }

    ColumnLayout {
        width: view.width
        clip: true

        Button {
            text: qsTr("Configure input")
            icon.source: "image://icons/settings"
            flat: true
            visible: devicesList.count > 0

            onClicked: deviceOptions.openOptions(audioLayer.audioInput[0])
        }
        OptionList {
            id: devicesList
            Layout.fillWidth: true
            Layout.minimumHeight: minHeight

            property bool updating: false
            property int minHeight: 0

            function update() {
                devicesList.minHeight = 0
                let devices = audioLayer.inputs

                for (let i = count - 1; i >= 0; i--)
                    removeItem(itemAt(i))

                let input = audioLayer.audioInput.length < 1?
                                "":
                                audioLayer.audioInput[0]
                let index = devices.indexOf(input)

                if (index < 0) {
                    if (devices.length == 1)
                        index = 0
                    else if (devices.length >= 2)
                        index = 1
                }

                updating = true

                for (let i in devices) {
                    let component = Qt.createComponent("AudioDeviceItem.qml")

                    if (component.status !== Component.Ready)
                        continue

                    let obj = component.createObject(devicesList)
                    obj.text = audioLayer.description(devices[i])
                    obj.device = devices[i]
                    obj.highlighted = i == index
                    devicesList.minHeight += obj.height

                    obj.Keys.onSpacePressed.connect(() => deviceOptions.openOptions(audioLayer.audioInput[0]))
                }

                updating = false
                setCurrentIndex(index)
            }

            onCurrentIndexChanged:
                if (!updating && itemAt(currentIndex))
                    audioLayer.audioInput = [itemAt(currentIndex).device]
        }
    }

    AudioDeviceOptions {
        id: deviceOptions
        anchors.centerIn: Overlay.overlay
    }
}

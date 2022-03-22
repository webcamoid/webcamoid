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

ScrollView {
    id: view

    Component.onCompleted: devicesList.update()
    onVisibleChanged: devicesList.forceActiveFocus()

    Connections {
        target: audioLayer

        function onOutputsChanged()
        {
            devicesList.update()
        }
    }

    OptionList {
        id: devicesList
        width: view.width
        clip: true

        function update() {
            let devices = audioLayer.outputs

            for (let i = count - 1; i >= 0; i--)
                removeItem(itemAt(i))

            let index = devices.indexOf(audioLayer.audioOutput)

            if (index < 0) {
                if (devices.length == 1)
                    index = 0
                else if (devices.length >= 2)
                    index = 1
            }

            for (let i in devices) {
                let component = Qt.createComponent("AudioDeviceItem.qml")

                if (component.status !== Component.Ready)
                    continue

                let obj = component.createObject(devicesList)
                obj.text = audioLayer.description(devices[i])
                obj.device = devices[i]
                obj.highlighted = i == index

                obj.onClicked.connect((device => function () {
                    if (audioLayer.audioOutput == device)
                        deviceOptions.openOptions(device)
                    else
                        audioLayer.audioOutput = device
                })(devices[i]))
            }

            setCurrentIndex(index)
        }

        Keys.onUpPressed:
            audioLayer.audioOutput = itemAt(currentIndex).device
        Keys.onDownPressed:
            audioLayer.audioOutput = itemAt(currentIndex).device
        Keys.onSpacePressed:
            deviceOptions.openOptions(audioLayer.audioOutput)
    }

    AudioDeviceOptions {
        id: deviceOptions
        anchors.centerIn: Overlay.overlay
    }
}

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
import Webcamoid 1.0

ScrollView {
    id: view

    signal openVideoInputAddEditDialog(string videoInput)
    signal openVideoInputOptions(string videoInput)

    Component.onCompleted: {
        lblNoWebcams.updateVisibility()
        devicesList.update()
    }
    onVisibleChanged: devicesList.forceActiveFocus()

    Connections {
        target: videoLayer

        function onInputsChanged()
        {
            devicesList.update()
        }

        function onVideoInputChanged()
        {
            lblNoWebcams.updateVisibility()
        }
    }

    ColumnLayout {
        width: view.width
        clip: true

        Button {
            text: qsTr("Configure source")
            icon.source: "image://icons/settings"
            flat: true
            visible: devicesList.count > 0

            onClicked: view.openVideoInputOptions(videoLayer.videoInput)
        }
        Button {
            text: qsTr("Add source")
            icon.source: "image://icons/add"
            flat: true

            onClicked: view.openVideoInputAddEditDialog("")
        }
        Label {
            id: lblNoWebcams
            height: visible?
                        AkUnit.create(32 * AkTheme.controlScale, "dp").pixels:
                        0
            text: qsTr("No cameras found")
            verticalAlignment: Text.AlignVCenter
            Layout.fillWidth: true
            enabled: false

            function updateVisibility()
            {
                visible = videoLayer.devicesByType(VideoLayer.InputCamera).length < 1
            }
        }
        OptionList {
            id: devicesList
            Layout.fillWidth: true

            property bool updating: false

            function update() {
                let devices = videoLayer.inputs

                for (let i = count - 1; i >= 0; i--)
                    removeItem(itemAt(i))

                let index = devices.indexOf(videoLayer.videoInput)

                if (index < 0) {
                    if (devices.length == 1)
                        index = 0
                    else if (devices.length >= 2)
                        index = 1
                }

                updating = true

                for (let i in devices) {
                    let component = Qt.createComponent("VideoDeviceItem.qml")

                    if (component.status !== Component.Ready)
                        continue

                    let obj = component.createObject(devicesList)
                    obj.text = videoLayer.description(devices[i])
                    obj.device = devices[i]
                    obj.highlighted = i == index

                    obj.Keys.onSpacePressed.connect(() => view.openVideoInputOptions(videoLayer.videoInput))
                }

                updating = false
                setCurrentIndex(index)
            }

            onCurrentIndexChanged:
                if (!updating && itemAt(currentIndex))
                    videoLayer.videoInput = itemAt(currentIndex).device
        }
    }
}

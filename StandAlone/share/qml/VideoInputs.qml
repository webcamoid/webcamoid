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
        devicesList.updateDevices()
    }

    Connections {
        target: videoLayer

        function onInputsChanged()
        {
            devicesList.updateDevices()
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
            text: qsTr("No webcams found")
            verticalAlignment: Text.AlignVCenter
            Layout.fillWidth: true
            enabled: false

            function updateVisibility()
            {
                visible = videoLayer.devicesByType(VideoLayer.InputCamera).length < 1
            }
        }
        ListView {
            id: devicesList
            model: ListModel {}
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height
            Layout.fillWidth: true
            Layout.fillHeight: true

            function updateDevices() {
                let devices = videoLayer.inputs
                model.clear()

                for (let i in devices) {
                    let device = devices[i]
                    let description = videoLayer.description(device)

                    model.append({
                        device: device,
                        description: description})
                }

                let index = devices.indexOf(videoLayer.videoInput)

                if (index < 0) {
                    if (devices.length == 1)
                        index = 0
                    else if (devices.length >= 2)
                        index = 1
                }

                currentIndex = index
            }

            delegate: ItemDelegate {
                text: index < 0 && index >= devicesList.count?
                          "":
                      devicesList.model.get(index)?
                          devicesList.model.get(index)["description"]:
                          ""
                anchors.right: parent? parent.right: undefined
                anchors.left: parent? parent.left: undefined
                height: implicitHeight
                highlighted: devicesList.currentItem == this

                onClicked: {
                    if (devicesList.currentIndex == index) {
                        if (index < 0)
                            return

                        let deviceElement = devicesList.model.get(index)

                        if (!deviceElement)
                            return

                        let device = deviceElement["device"]

                        if (!device)
                            return

                        view.openVideoInputOptions(device)
                    } else {
                        let deviceElement = devicesList.model.get(index)

                        if (!deviceElement)
                            return

                        let device = deviceElement["device"]

                        if (!device)
                            return

                        videoLayer.videoInput = device
                        devicesList.currentIndex = index
                    }
                }
            }
        }
    }
}

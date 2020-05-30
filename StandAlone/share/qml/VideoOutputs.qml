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

    signal openErrorDialog(string title, string message)
    signal openVideoOutputAddEditDialog(string videoOutput)
    signal openVideoOutputOptions(string videoOutput)

    ColumnLayout {
        width: view.width

        Button {
            text: qsTr("Add output")
            icon.source: "image://icons/add"
            flat: true

            onClicked: {
                if (videoLayer.canApply(VideoLayer.OperationCreate)) {
                    view.openVideoOutputAddEditDialog("")
                } else {
                    let title = qsTr("Error Creating Virtual Camera")
                    let message = Commons.vcamDriverBusyMessage()
                    view.openErrorDialog(title, message)
                }
            }
        }
        Button {
            text: qsTr("Remove all outputs")
            icon.source: "image://icons/no"
            flat: true

            onClicked: {
                if (videoLayer.canApply(VideoLayer.OperationDestroyAll)) {
                    videoLayer.removeAllOutputs()
                } else {
                    let title = qsTr("Error Removing Virtual Cameras")
                    let message = Commons.vcamDriverBusyMessage()
                    view.openErrorDialog(title, message)
                }
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
                let devices = videoLayer.outputs
                model.clear()

                for (let i in devices) {
                    let device = devices[i]
                    let description = videoLayer.description(device)
                    model.append({device: device,
                                  description: description})
                }

                let output = videoLayer.videoOutput.length < 1?
                                "":
                                videoLayer.videoOutput[0]
                let index = devices.indexOf(output)

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
                anchors.right: parent.right
                anchors.left: parent.left
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

                        if (!device || device == ":dummyout:")
                            return

                        view.openVideoOutputOptions(device)
                    } else {
                        let deviceElement = devicesList.model.get(index)

                        if (!deviceElement)
                            return

                        let device = deviceElement["device"]

                        if (!device)
                            return

                        videoLayer.videoOutput = [device]
                        devicesList.currentIndex = index
                    }
                }
            }

            Connections {
                target: videoLayer

                onOutputsChanged: devicesList.updateDevices()
            }

            Component.onCompleted: devicesList.updateDevices()
        }
    }
}

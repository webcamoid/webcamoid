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
import Qt.labs.settings 1.0 as LABS
import Ak 1.0
import Webcamoid 1.0

StackLayout {
    id: videoOutputsLayout
    currentIndex: !videoLayer.isVCamSupported?
                      2:
                  videoLayer.vcamInstallStatus == VideoLayer.VCamNotInstalled?
                      1:
                      0

    property int vcamStatus: updates.status("VirtualCamera")
    property string vcamVersion: videoLayer.currentVCamVersion
    property string vcamLatestVersion: updates.latestVersion("VirtualCamera")
    property bool showDialog: true

    signal openErrorDialog(string title, string message)
    signal openVideoOutputAddEditDialog(string videoOutput)
    signal openVideoOutputOptions(string videoOutput)
    signal openVideoOutputPictureDialog()
    signal openVCamDownloadDialog()

    Connections {
        target: updates

        onNewVersionAvailable: {
            if (component == "VirtualCamera") {
                videoOutputsLayout.vcamStatus = updates.status("VirtualCamera");
                videoOutputsLayout.vcamLatestVersion = latestVersion;
            }
        }
    }

    Page {
        ScrollView {
            id: videoOptionsScroll
            width: parent.width

            ColumnLayout {
                width: videoOptionsScroll.width

                ColumnLayout {
                    visible: videoLayer.vcamDriver == "VideoSink/VirtualCamera/Impl/AkVCam"
                             && videoOutputsLayout.vcamStatus == Updates.ComponentOutdated
                             && updates.notifyNewVersion
                             && videoOutputsLayout.showDialog

                    Label {
                        text: qsTr("The virtual camera is outdated (%1), install the latest version (%2)?")
                                .arg(videoOutputsLayout.vcamVersion)
                                .arg(videoOutputsLayout.vcamLatestVersion)
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                        Layout.rightMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                    }
                    Button {
                        text: qsTr("Install")
                        highlighted: true
                        Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                        Layout.topMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                        Layout.bottomMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

                        onClicked: {
                            videoLayer.downloadVCam()
                            videoOutputsLayout.openVCamDownloadDialog()
                        }
                    }
                }
                Button {
                    text: qsTr("Add output")
                    icon.source: "image://icons/add"
                    flat: true

                    onClicked: {
                        if (videoLayer.clientsPids.length < 1) {
                            videoOutputsLayout.openVideoOutputAddEditDialog("")
                        } else {
                            let title = qsTr("Error Creating Virtual Camera")
                            let message = Commons.vcamDriverBusyMessage()
                            videoOutputsLayout.openErrorDialog(title, message)
                        }
                    }
                }
                Button {
                    text: qsTr("Remove all outputs")
                    icon.source: "image://icons/no"
                    flat: true

                    onClicked: {
                        if (videoLayer.clientsPids.length < 1) {
                            videoLayer.removeAllOutputs()
                        } else {
                            let title = qsTr("Error Removing Virtual Cameras")
                            let message = Commons.vcamDriverBusyMessage()
                            videoOutputsLayout.openErrorDialog(title, message)
                        }
                    }
                }
                Button {
                    text: qsTr("Set output picture")
                    icon.source: "image://icons/picture"
                    flat: true

                    onClicked: videoOutputsLayout.openVideoOutputPictureDialog()
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

                                videoOutputsLayout.openVideoOutputOptions(device)
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
    }
    Page {
        ColumnLayout {
            width: parent.width

            Label {
                text: qsTr("The virtual camera is not installed, do you want to install it?")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Install")
                highlighted: true
                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                Layout.topMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

                onClicked: {
                    videoLayer.downloadVCam()
                    videoOutputsLayout.openVCamDownloadDialog()
                }
            }
        }
    }
    Page {
        Label {
            text: qsTr("The virtual camera is not supported in this platform")
            wrapMode: Text.WordWrap
            width: parent.width
        }
    }

    LABS.Settings {
        category: "Updates"

        property alias showDialog: videoOutputsLayout.showDialog
    }
}

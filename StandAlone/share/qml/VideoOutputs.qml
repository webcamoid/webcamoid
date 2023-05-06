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
                  (videoLayer.vcamInstallStatus == VideoLayer.VCamNotInstalled)
                  || !videoLayer.isCurrentVCamInstalled?
                      1:
                      0

    property int vcamStatus: updates.status("VirtualCamera",
                                            videoLayer.currentVCamVersion)
    property string vcamVersion: videoLayer.currentVCamVersion
    property string vcamLatestVersion: updates.latestVersion("VirtualCamera")
    property bool showDialog: true

    signal openErrorDialog(string title, string message)
    signal openVideoOutputAddEditDialog(string videoOutput)
    signal openVideoOutputOptions(string videoOutput)
    signal openVideoOutputPictureDialog()
    signal openVCamDownloadDialog()
    signal openVCamManualDownloadDialog()

    Component.onCompleted: devicesList.update()
    onVisibleChanged: devicesList.forceActiveFocus()

    Connections {
        target: videoLayer

        function onOutputsChanged()
        {
            devicesList.update()
        }
    }

    Connections {
        target: updates

        function onNewVersionAvailable(component, latestVersion)
        {
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
                    id: vcamOutdatedLayout
                    visible: videoLayer.vcamDriver == videoLayer.defaultVCamDriver
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
                        Accessible.description: qsTr("Install virtual camera")

                        onClicked: {
                            if (videoLayer.downloadVCam())
                                videoOutputsLayout.openVCamDownloadDialog()
                            else
                                videoOutputsLayout.openVCamManualDownloadDialog()
                        }
                    }
                }
                Button {
                    text: qsTr("Configure output")
                    icon.source: "image://icons/settings"
                    flat: true
                    visible: devicesList.count > 0
                    enabled: videoLayer.videoOutput[0] != ":dummyout:"

                    onClicked:
                        videoOutputsLayout.openVideoOutputOptions(videoLayer.videoOutput[0])
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
                            if (!videoLayer.removeAllOutputs()) {
                                let title = qsTr("Error removing virtual cameras")
                                videoOutputsLayout.openErrorDialog(title,
                                                                   videoLayer.outputError)
                            }
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
                    visible: videoLayer.vcamDriver == videoLayer.defaultVCamDriver
                             && videoLayer.videoOutput.length > 0

                    onClicked: videoOutputsLayout.openVideoOutputPictureDialog()
                }
                OptionList {
                    id: devicesList
                    Layout.fillWidth: true

                    property bool updating: false

                    function update() {
                        let devices = videoLayer.outputs

                        for (let i = count - 1; i >= 0; i--)
                            removeItem(itemAt(i))

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

                        updating = true

                        for (let i in devices) {
                            let component = Qt.createComponent("VideoDeviceItem.qml")

                            if (component.status !== Component.Ready)
                                continue

                            let obj = component.createObject(devicesList)
                            obj.text = videoLayer.description(devices[i])
                            obj.device = devices[i]
                            obj.highlighted = i == index

                            obj.Keys.onSpacePressed.connect(function () {
                                if (videoLayer.videoOutput[0] != ":dummyout:")
                                    videoOutputsLayout.openVideoOutputOptions(videoLayer.videoOutput[0])
                            })
                        }

                        updating = false
                        setCurrentIndex(index)
                    }

                    onCurrentIndexChanged:
                        if (!updating && itemAt(currentIndex))
                            videoLayer.videoOutput = [itemAt(currentIndex).device]
                }
            }
        }
    }
    Page {
        ColumnLayout {
            width: parent.width

            Label {
                text: videoLayer.vcamDriver == videoLayer.defaultVCamDriver?
                          qsTr("The virtual camera is not installed, do you want to install it?"):
                          qsTr("The virtual camera is not installed. Please, install <b>v4l2loopback</b>.")
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
                Accessible.description: qsTr("Install virtual camera")
                visible: videoLayer.vcamDriver == videoLayer.defaultVCamDriver

                onClicked: {
                    if (videoLayer.downloadVCam())
                        videoOutputsLayout.openVCamDownloadDialog()
                    else
                        videoOutputsLayout.openVCamManualDownloadDialog()
                }
            }
        }
    }
    Page {
        ColumnLayout {
            width: parent.width

            Label {
                text: qsTr("The virtual camera is not supported in this platform")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                Layout.rightMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            }
        }
    }

    LABS.Settings {
        category: "Updates"

        property alias showDialog: videoOutputsLayout.showDialog
    }
}

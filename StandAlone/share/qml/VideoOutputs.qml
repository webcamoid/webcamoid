/* Webcamoid, camera capture application.
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
import Ak
import Webcamoid

ScrollView {
    id: view

    property bool showDialog: true
    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    signal openErrorDialog(string title, string message)
    signal openVideoOutputAddEditDialog(string videoOutput)
    signal openVirtualCameraOptions(string videoOutput)
    signal openStreamingPlatformOptions(string videoOutput)
    signal openLocalStreamingOptions()
    signal openVCamDownloadDialog()
    signal openVCamManualDownloadDialog()

    Component.onCompleted: devicesList.update()

    onVisibleChanged: {
        devicesList.forceActiveFocus()
        let vcamStatus = updates.status("VirtualCamera",
                                        virtualCameras.currentVCamVersion)

        if (visible && showDialog) {
            if (virtualCameras.vcamDriver == virtualCameras.defaultVCamDriver
                && vcamStatus == Updates.ComponentOutdated
                && updates.notifyNewVersion) {
                vcamUpdate.openDialog()
            }

            showDialog = false
        }
    }

    Connections {
        target: virtualCameras

        function onOutputsChanged()
        {
            devicesList.update()
        }
    }

    Connections {
        target: streaming

        function onPlatformsChanged() {
            devicesList.update()
        }
    }

    // Update list when local streaming location changes
    Connections {
        target: localStreaming

        function onLocationChanged() {
            devicesList.update()
        }
    }

    ColumnLayout {
        layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight
        width: view.width

        Button {
            id: btnAddOutput
            text: qsTr("Add output")
            icon.source: "image://icons/add"
            flat: true

            onClicked: addOutput.popup()

            Menu {
                id: addOutput
                width: AkUnit.create(250 * AkTheme.controlScale, "dp").pixels
                margins: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

                MenuItem {
                    text: qsTr("Add virtual camera")
                    icon.source: "image://icons/webcam"
                    height: virtualCameras.isVCamSupported? undefined: 0
                    visible: virtualCameras.isVCamSupported

                    onClicked: {
                        if ((virtualCameras.vcamInstallStatus == virtualCameras.VCamNotInstalled)
                            || !virtualCameras.isCurrentVCamInstalled) {
                            vcamInstall.open()
                        } else {
                            if (virtualCameras.clientsPids.length < 1) {
                                view.openVideoOutputAddEditDialog("")
                            } else {
                                let title = qsTr("Error Creating Virtual Camera")
                                let message = Commons.vcamDriverBusyMessage()
                                view.openErrorDialog(title, message)
                            }
                        }
                    }
                }
                MenuItem {
                    text: qsTr("Add streaming platform")
                    icon.source: "image://icons/broadcast"
                    height: streaming.isStreamingSupported? undefined: 0
                    visible: streaming.isStreamingSupported

                    onClicked: addStreamingPlatformDialog.open()
                }
                MenuItem {
                    text: qsTr("Add local streaming")
                    icon.source: "image://icons/broadcast"
                    height: localStreaming.isLocalStreamingSupported? undefined: 0
                    visible: localStreaming.isLocalStreamingSupported

                    onClicked: {
                        localStreaming.location = localStreaming.defaultURL
                        devicesList.update()
                    }
                }
            }
        }
        OptionList {
            id: devicesList
            enableHighlight: false
            Layout.fillWidth: true
            Layout.minimumHeight: minHeight

            property bool updating: false
            property int minHeight: 0

            function update() {
                devicesList.minHeight = 0
                let devices = virtualCameras.outputs

                for (let i = count - 1; i >= 0; i--)
                    removeItem(itemAt(i))

                let output = virtualCameras.outputs.length < 1?
                                "":
                                virtualCameras.outputs[0]
                let index = devices.indexOf(output)

                if (index < 0) {
                    if (devices.length == 1)
                        index = 0
                    else if (devices.length >= 2)
                        index = 1
                }

                updating = true

                // Add virtual camera outputs
                for (let i in devices) {
                    let component = Qt.createComponent("VideoDeviceItem.qml")

                    if (component.status !== Component.Ready)
                        continue

                    let obj = component.createObject(devicesList)
                    obj.text = virtualCameras.description(devices[i])
                    obj.device = devices[i]
                    devicesList.minHeight += obj.height

                    obj.onClicked.connect((index => function () {
                        view.openVirtualCameraOptions(virtualCameras.outputs[index])
                    })(i))
                }

                // Add local streaming if location is set
                if (localStreaming.location.length > 0) {
                    let component = Qt.createComponent("LocalStreamingItem.qml")

                    if (component.status === Component.Ready) {
                        let obj = component.createObject(devicesList)
                        obj.text = qsTr("Local Streaming")
                        obj.location = localStreaming.location
                        devicesList.minHeight += obj.height

                        obj.onClicked.connect(function () {
                            view.openLocalStreamingOptions()
                        })
                    }
                }

                // Add streaming platforms
                for (let i in streaming.platforms) {
                    let platform = streaming.platforms[i]
                    let component = Qt.createComponent("StreamingPlatformItem.qml")

                    if (component.status !== Component.Ready)
                        continue

                    let obj = component.createObject(devicesList)
                    obj.text = platform
                    obj.platform = platform
                    devicesList.minHeight += obj.height

                    obj.onClicked.connect((p => function () {
                        view.openStreamingPlatformOptions(p)
                    })(platform))
                }

                updating = false
                setCurrentIndex(index)
            }

            onActiveFocusChanged:
                if (activeFocus && count > 0)
                    itemAt(currentIndex >= 0? currentIndex: 0).forceActiveFocus()
            Keys.onUpPressed:
                if (count > 0)
                    itemAt(currentIndex >= 0? currentIndex: 0).forceActiveFocus()
            Keys.onDownPressed:
                if (count > 0)
                    itemAt(currentIndex >= 0? currentIndex: 0).forceActiveFocus()
        }
    }

    AddPlatformDialog {
        id: addStreamingPlatformDialog
        anchors.centerIn: Overlay.overlay

        onPlatforAccepted: function (platform) {
            let current = streaming.platforms

            if (current.indexOf(platform) < 0) {
                streaming.platforms = [...current, platform]
                devicesList.update()
            }
        }
        onClosed: btnAddOutput.forceActiveFocus()
    }
    VirtualCameraInstallDialog {
        id: vcamInstall
        anchors.centerIn: Overlay.overlay

        onOpenVCamDownloadDialog: view.openVCamDownloadDialog()
        onOpenVCamManualDownloadDialog: view.openVCamManualDownloadDialog()
        onClosed: btnAddOutput.forceActiveFocus()
    }
    VirtualCameraUpdateDialog {
        id: vcamUpdate
        anchors.centerIn: Overlay.overlay

        onOpenVCamDownloadDialog: view.openVCamDownloadDialog()
        onOpenVCamManualDownloadDialog: view.openVCamManualDownloadDialog()
        onClosed: btnAddOutput.forceActiveFocus()
    }
}

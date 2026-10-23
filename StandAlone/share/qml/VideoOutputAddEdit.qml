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
import AkControls as AK
import Webcamoid

Dialog {
    id: addEdit
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: physicalWidth <= 100 || physicalHeight <= 100?
               wdgMainWidget.width: wdgMainWidget.width * 0.75
    height: physicalWidth <= 100 || physicalHeight <= 100?
                wdgMainWidget.height: wdgMainWidget.height * 0.75
    modal: true

    property real physicalWidth: wdgMainWidget.width / Screen.pixelDensity
    property real physicalHeight: wdgMainWidget.height / Screen.pixelDensity

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    signal edited()
    signal openErrorDialog(string title, string message)

    function fillFormats()
    {
        cbxPixelFormats.model.clear()
        let pixFormats = virtualCameras.supportedOutputPixelFormats

        for (let i in pixFormats)
            cbxPixelFormats.model.append({
                format: Number(pixFormats[i]),
                description: AkVideoCaps.pixelFormatToString(pixFormats[i])
            })
    }

    function selectCurrentCaps()
    {
        let pixFormats = virtualCameras.supportedOutputPixelFormats
        let caps = AkVideoCaps.create(virtualCameras.outputCaps)
        let format = caps.format

        if (caps.width < 1 || caps.height < 1)
            format = virtualCameras.defaultOutputPixelFormat

        let index = -1

        for (let i in pixFormats)
            if (pixFormats[i] == format)
                index = i

        if (index < 0)
            for (let i in pixFormats)
                if (pixFormats[i] == virtualCameras.defaultOutputPixelFormat)
                    index = i

        cbxPixelFormats.currentIndex = index
    }

    function openOptions(device)
    {
        title = device?
                    qsTr("Edit Virtual Camera"):
                    qsTr("Add Virtual Camera")

        if (device)
            deviceDescription.text = virtualCameras.outputDescription
        else
            deviceDescription.text = "Virtual Camera"

        deviceId.text = device
        fillFormats()
        selectCurrentCaps()
        open()
    }

    onVisibleChanged: {
        if (visible)
            deviceDescription.forceActiveFocus()
    }

    Connections {
        target: virtualCameras

        function onOutputCreated(ok, msg) {
            if (!ok) {
                let title = qsTr("Error creating the virtual camera")
                addEdit.openErrorDialog(title, msg)
            }
        }

        function onOutputEdited(ok, error) {
            addEdit.edited()

            if (!ok) {
                let title = qsTr("Error editing the virtual camera")
                addEdit.openErrorDialog(title, error)
            }
        }
    }

    ScrollView {
        id: formatsView
        anchors.fill: parent
        contentHeight: formatsControls.height
        clip: true

        ColumnLayout {
            id: formatsControls
            width: formatsView.width
            layoutDirection: addEdit.rtl? Qt.RightToLeft: Qt.LeftToRight

            TextField {
                id: deviceDescription
                placeholderText: qsTr("Virtual camera name")
                selectByMouse: true
                visible: virtualCameras.canEditVCamDescription && text.length > 0
                Layout.fillWidth: true
            }
            Label {
                id: lblDeviceDescription
                text: deviceDescription.text
                font.bold: true
                visible: !virtualCameras.canEditVCamDescription
                         && text.length > 0
                         && deviceId.text.length
                Layout.fillWidth: true
            }
            Label {
                id: deviceId
                font.italic: true
                visible: text.length > 0
                Layout.fillWidth: true
            }

            AK.LabeledComboBox {
                id: cbxPixelFormats
                label: qsTr("Format")
                Accessible.description: label
                textRole: "description"
                model: ListModel {}
                Layout.fillWidth: true
            }
        }
    }

    onAccepted: {
        let devId = deviceId.text
        let description = deviceDescription.text

        if (virtualCameras.clientsPids.length > 0) {
            let title = devId?
                    qsTr("Can't edit the virtual camera"):
                    qsTr("Can't add the virtual camera")
            let message = Commons.vcamDriverBusyMessage()
            addEdit.openErrorDialog(title, message)

            return
        }

        if (!description) {
            let title = devId?
                    qsTr("Error editing the virtual camera"):
                    qsTr("Error adding the virtual camera")
            let message = qsTr("Camera description can't be empty.")
            addEdit.openErrorDialog(title, message)

            return
        }

        let element = cbxPixelFormats.model.get(cbxPixelFormats.currentIndex)

        // The resolution and the frame rate are shared by all the outputs and
        // are configured in the composition settings, here only the format is
        // chosen. It is applied the next time the virtual camera starts, it is
        // restarted automatically if it is running.
        let outputCaps = AkVideoCaps.create(videoEffects.outputCaps)
        virtualCameras.setOutputFormat(element.format,
                                       outputCaps.width,
                                       outputCaps.height,
                                       Math.round(AkFrac.create(outputCaps.fps).value))

        if (!devId)
            virtualCameras.createOutput(description)
        else if (virtualCameras.canEditVCamDescription
                 && description !== virtualCameras.outputDescription)
            virtualCameras.editOutput(description)
        else
            addEdit.edited()
    }
}

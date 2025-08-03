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

Dialog {
    id: addEdit
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: AkUnit.create(450 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(350 * AkTheme.controlScale, "dp").pixels
    modal: true

    signal edited()
    signal openErrorDialog(string title, string message)
    signal openOutputFormatDialog(int index, variant caps)

    function addFormat(caps)
    {
        let component = Qt.createComponent("VideoFormatItem.qml")

        if (component.status !== Component.Ready)
            return

        let obj = component.createObject(vcamFormats)
        let format = AkVideoCaps.pixelFormatToString(caps.format)
        let fps = AkFrac.create(caps.fps)
        obj.text =
            format
            + " " + caps.width + "x" + caps.height
            + " " + fps.value + " FPS"
        obj.format = caps.format
        obj.formatWidth = caps.width
        obj.formatHeight = caps.height
        obj.fps = fps.value
        vcamFormats.minHeight += obj.height

        obj.onClicked.connect((index => function () {
            let element = vcamFormats.itemAt(index)
            let caps =
                AkVideoCaps.create(element.format,
                                   element.formatWidth,
                                   element.formatHeight,
                                   AkFrac.create(element.fps,
                                                 1).toVariant())
            addEdit.openOutputFormatDialog(index, caps)
        })(vcamFormats.count - 1))
    }

    function changeFormat(index, caps)
    {
        let item = vcamFormats.itemAt(index)

        if (!item)
            return

        let format = AkVideoCaps.pixelFormatToString(caps.format)
        let fps = AkFrac.create(caps.fps)
        item.text =
            format
            + " " + caps.width + "x" + caps.height
            + " " + fps.value + " FPS"
        item.format = caps.format
        item.formatWidth = caps.width
        item.formatHeight = caps.height
        item.fps = fps.value
    }

    function removeFormat(index)
    {
        vcamFormats.removeItem(vcamFormats.itemAt(index))
    }

    function isSupported(format)
    {
        let formats = videoLayer.supportedOutputPixelFormats

        for (let i in formats)
            if (formats[i] == format)
                return true

        return false
    }

    function populateFormats()
    {
        let formats = []

        if (deviceId.text) {
            let outputCaps = videoLayer.supportedOutputVideoCaps(deviceId.text)

           for (let caps in outputCaps)
               formats.push(AkVideoCaps.create(outputCaps[caps]))
        } else {
            let defaultPixelFormats = [
                AkVideoCaps.Format_yuyv422,
                AkVideoCaps.Format_uyvy422,
                AkVideoCaps.Format_xrgb,
                AkVideoCaps.Format_rgb24
            ]
            let pixelFormats = []

            for (let i in defaultPixelFormats)
                if (isSupported(defaultPixelFormats[i]))
                    pixelFormats.push(defaultPixelFormats[i])

            let resolutions = [
                Qt.size( 640,  480),
                Qt.size( 160,  120),
                Qt.size( 320,  240),
                Qt.size( 800,  600),
                Qt.size(1280,  720),
                Qt.size(1920, 1080),
            ]

            for (let format in pixelFormats)
                for (let resolution in resolutions)
                    formats.push(AkVideoCaps.create(pixelFormats[format],
                                                    resolutions[resolution].width,
                                                    resolutions[resolution].height,
                                                    AkFrac.create(30, 1).toVariant()))
        }

        vcamFormats.clear()

        for (let i in formats) {
            let component = Qt.createComponent("VideoFormatItem.qml")

            if (component.status !== Component.Ready)
                continue

            let obj = component.createObject(vcamFormats)
            let caps = formats[i]
            let format = AkVideoCaps.pixelFormatToString(caps.format)
            let fps = AkFrac.create(caps.fps)
            obj.text =
                format
                + " " + caps.width + "x" + caps.height
                + " " + fps.value + " FPS"
            obj.format = caps.format
            obj.formatWidth = caps.width
            obj.formatHeight = caps.height
            obj.fps = fps.value
            vcamFormats.minHeight += obj.height

            obj.onClicked.connect((index => function () {
                let element = vcamFormats.itemAt(index)
                let caps =
                    AkVideoCaps.create(element.format,
                                       element.formatWidth,
                                       element.formatHeight,
                                       AkFrac.create(element.fps,
                                                     1).toVariant())
                addEdit.openOutputFormatDialog(index, caps)
            })(i))
        }
    }

    function fillPTFormat(device)
    {
        cbxPixelFormatsPT.model.clear()
        let pixFormats = videoLayer.supportedOutputPixelFormats
        let index = -1

        for (let i in pixFormats) {
            if (pixFormats[i] == videoLayer.defaultOutputPixelFormat)
                index = i

            cbxPixelFormatsPT.model.append({
                format: Number(pixFormats[i]),
                description: AkVideoCaps.pixelFormatToString(pixFormats[i])
            })
        }

        if (device) {
            let outputCaps = videoLayer.supportedOutputVideoCaps(device)

            if (outputCaps.length < 1) {
                cbxPixelFormatsPT.currentIndex = index
                spbFrameWidthPT.value = 640
                spbFrameHeightPT.value = 480
                spbFrameRatePT.value = 30
            } else {
                let caps = AkVideoCaps.create(outputCaps[0])
                index = -1

                for (let i in pixFormats)
                    if (pixFormats[i] == caps.format)
                        index = i

                let fps = AkFrac.create(caps.fps)
                cbxPixelFormatsPT.currentIndex = index
                spbFrameWidthPT.value = caps.width
                spbFrameHeightPT.value = caps.height
                spbFrameRatePT.value = fps.value
            }
        } else {
            cbxPixelFormatsPT.currentIndex = index
            spbFrameWidthPT.value = 640
            spbFrameHeightPT.value = 480
            spbFrameRatePT.value = 30
        }
    }

    function openOptions(device)
    {
        title = device?
                    qsTr("Edit Virtual Camera"):
                    qsTr("Add Virtual Camera")

        if (device)
            deviceDescription.text = videoLayer.description(device)
        else
            deviceDescription.text =
                    "Virtual Camera " + mediaTools.currentTime("yyyyMMddhhmmss")

        deviceDescriptionPT.text = deviceDescription.text
        deviceId.text = device
        deviceIdPT.text = device

        if (videoLayer.isPassThroughVCam)
            fillPTFormat(device)
        else
            populateFormats()

        open()
    }

    onVisibleChanged:
        videoLayer.isPassThroughVCam?
            deviceDescriptionPT.forceActiveFocus():
            deviceDescription.forceActiveFocus()

    ScrollView {
        id: formatsView
        anchors.fill: parent
        contentHeight: videoLayer.isPassThroughVCam?
                            formatsControlsPT.height:
                            formatsControls.height
        clip: true

        ColumnLayout {
            id: formatsControls
            width: formatsView.width
            visible: !videoLayer.isPassThroughVCam

            TextField {
                id: deviceDescription
                placeholderText: qsTr("Virtual camera name")
                selectByMouse: true
                Layout.fillWidth: true
                visible: videoLayer.canEditVCamDescription && text.length > 0
            }
            Label {
                id: lblDeviceDescription
                text: deviceDescription.text
                font.bold: true
                visible: !videoLayer.canEditVCamDescription
                         && text.length > 0
                         && deviceId.text.length
            }
            Label {
                id: deviceId
                font.italic: true
                visible: text.length > 0
            }
            Button {
                text: qsTr("Add format")
                icon.source: "image://icons/add"
                flat: true

                onClicked: {
                    let caps = AkVideoCaps.create()
                    addEdit.openOutputFormatDialog(-1, caps)
                }
            }
            Button {
                text: qsTr("Clear formats")
                icon.source: "image://icons/no"
                flat: true

                onClicked: vcamFormats.clear()
            }
            OptionList {
                id: vcamFormats
                enableHighlight: false
                Layout.fillWidth: true
                Layout.minimumHeight: minHeight

                property int minHeight: 0

                function clear() {
                    minHeight = 0

                    for (let i = count - 1; i >= 0; i--)
                        removeItem(itemAt(i))
                }

                onActiveFocusChanged:
                    if (activeFocus && count > 0)
                        itemAt(currentIndex).forceActiveFocus()
                Keys.onUpPressed:
                    if (count > 0)
                        itemAt(currentIndex).forceActiveFocus()
                Keys.onDownPressed:
                    if (count > 0)
                        itemAt(currentIndex).forceActiveFocus()
            }
        }
        GridLayout {
            id: formatsControlsPT
            columns: 2
            width: formatsView.width
            visible: videoLayer.isPassThroughVCam

            TextField {
                id: deviceDescriptionPT
                placeholderText: qsTr("Virtual camera name")
                selectByMouse: true
                Layout.fillWidth: true
                visible: videoLayer.canEditVCamDescription && text.length > 0
                Layout.columnSpan: 2
            }
            Label {
                id: lblDeviceDescriptionPT
                text: deviceDescription.text
                font.bold: true
                visible: !videoLayer.canEditVCamDescription
                         && text.length > 0
                         && deviceIdPT.text.length
                         Layout.columnSpan: 2
            }
            Label {
                id: deviceIdPT
                font.italic: true
                visible: text.length > 0
                Layout.columnSpan: 2
            }
            Label {
                id: txtFormatPT
                text: qsTr("Format")
            }
            ComboBox {
                id: cbxPixelFormatsPT
                Accessible.description: txtFormatPT.text
                textRole: "description"
                model: ListModel {}
                Layout.fillWidth: true
            }
            Label {
                id: txtWidthPT
                text: qsTr("Width")
            }
            SpinBox {
                id: spbFrameWidthPT
                value: 640
                from: 1
                to: 4096
                stepSize: 1
                editable: true
                Accessible.name: txtWidthPT.text
            }
            Label {
                id: txtHeightPT
                text: qsTr("Height")
            }
            SpinBox {
                id: spbFrameHeightPT
                value: 480
                from: 1
                to: 4096
                stepSize: 1
                editable: true
                Accessible.name: txtHeightPT.text
            }
            Label {
                id: txtFrameRatePT
                text: qsTr("Frame rate")
            }
            SpinBox {
                id: spbFrameRatePT
                value: 30
                from: 1
                to: 250
                stepSize: 1
                editable: true
                Accessible.name: txtFrameRatePT.text
            }
        }
    }

    onAccepted: {
        let devId = videoLayer.isPassThroughVCam?
                        deviceId.text:
                        deviceIdPT.text
        let description = videoLayer.isPassThroughVCam?
                                deviceDescriptionPT.text:
                                deviceDescription.text

        if (videoLayer.clientsPids.length > 0) {
            let title = devId?
                    qsTr("Can't edit the virtual camera"):
                    qsTr("Can't add the virtual camera")
            let message = Commons.vcamDriverBusyMessage()
            addEdit.openErrorDialog(title, message)

            return
        }

        if (videoLayer.isPassThroughVCam) {
            if (!description) {
                let title = devId?
                        qsTr("Error editing the virtual camera"):
                        qsTr("Error adding the virtual camera")
                let message = qsTr("Camera description can't be empty.")
                addEdit.openErrorDialog(title, message)

                return
            }
        } else {
            if (vcamFormats.count < 1 || !description) {
                let title = devId?
                        qsTr("Error editing the virtual camera"):
                        qsTr("Error adding the virtual camera")
                let message = qsTr("Camera description and formats can't be empty.")
                addEdit.openErrorDialog(title, message)

                return
            }
        }

        let formats = []

        if (videoLayer.isPassThroughVCam) {
            let element = cbxPixelFormatsPT.model.get(cbxPixelFormatsPT.currentIndex)
            let fps = AkFrac.create(spbFrameRatePT.value, 1).toVariant()
            let caps = AkVideoCaps.create(element.format,
                                          spbFrameWidthPT.value,
                                          spbFrameHeightPT.value,
                                          fps)
            formats.push(caps.toVariant())
        } else {
            for (let i = 0; i < vcamFormats.count; i++) {
                let element = vcamFormats.itemAt(i)
                let caps = AkVideoCaps.create(element.format,
                                              element.formatWidth,
                                              element.formatHeight,
                                              AkFrac.create(element.fps,
                                                            1).toVariant())
                formats.push(caps.toVariant())
            }
        }

        if (devId) {
            let ok = videoLayer.editOutput(devId, description, formats)
            addEdit.edited()

            if (!ok) {
                let title = qsTr("Error editing the virtual camera")
                addEdit.openErrorDialog(title, videoLayer.outputError)
            }
        } else {
            let videoOutput =
                videoLayer.createOutput(VideoLayer.OutputVirtualCamera,
                                        description,
                                        formats)

            if (videoOutput) {
                videoLayer.videoOutput = videoOutput
            } else if (videoLayer.outputError) {
                let title = qsTr("Error creating the virtual camera")
                addEdit.openErrorDialog(title, videoLayer.outputError)
            }
        }
    }
}

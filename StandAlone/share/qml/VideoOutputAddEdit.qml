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
        let format = AkVideoCaps.pixelFormatToString(caps.format)
        let fps = AkFrac.create(caps.fps)
        let description = format
                        + " " + caps.width + "x" + caps.height
                        + " " + fps.value + " FPS"
        vcamFormats.model.append({format: caps.format,
                                  width: caps.width,
                                  height: caps.height,
                                  fps: fps.value,
                                  description: description})
    }

    function changeFormat(index, caps)
    {
        let format = AkVideoCaps.pixelFormatToString(caps.format)
        let fps = AkFrac.create(caps.fps)
        let description =
            format
            + " " + caps.width + "x" + caps.height
            + " " + fps.value + " FPS"
        vcamFormats.model.set(index,
                              {format: caps.format,
                               width: caps.width,
                               height: caps.height,
                               fps: fps.value,
                               description: description})
    }

    function removeFormat(index)
    {
        vcamFormats.model.remove(index)
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
                AkVideoCaps.Format_0rgb,
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

        vcamFormats.model.clear()

        for (let i in formats) {
            let caps = formats[i]
            let format = AkVideoCaps.pixelFormatToString(caps.format)
            let fps = AkFrac.create(caps.fps)
            let description =
                format
                + " " + caps.width + "x" + caps.height
                + " " + fps.value + " FPS"
            vcamFormats.model.append({format: caps.format,
                                      width: caps.width,
                                      height: caps.height,
                                      fps: fps.value,
                                      description: description})
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

        deviceId.text = device
        populateFormats()
        open()
    }

    ScrollView {
        id: formatsView
        anchors.fill: parent
        contentHeight: formatsControls.height
        clip: true

        ColumnLayout {
            id: formatsControls
            width: formatsView.width

            TextField {
                id: deviceDescription
                placeholderText: qsTr("Virtual camera name")
                selectByMouse: true
                Layout.fillWidth: true
            }
            Label {
                id: deviceId
                font.italic: true
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

                onClicked: vcamFormats.model.clear()
            }
            ListView {
                id: vcamFormats
                model: ListModel {}
                implicitWidth: childrenRect.width
                implicitHeight: childrenRect.height
                Layout.fillWidth: true
                Layout.fillHeight: true

                delegate: ItemDelegate {
                    text: index < 0 && index >= vcamFormats.count?
                              "":
                          vcamFormats.model.get(index)?
                              vcamFormats.model.get(index)["description"]:
                              ""
                    anchors.right: parent.right
                    anchors.left: parent.left
                    height: implicitHeight

                    onClicked: {
                        let element = vcamFormats.model.get(index)
                        let caps =
                            AkVideoCaps.create(element.format,
                                               element.width,
                                               element.height,
                                               AkFrac.create(element.fps,
                                                             1).toVariant())
                        addEdit.openOutputFormatDialog(index, caps)
                    }
                }
            }
        }
    }

    onAccepted: {
        if (videoLayer.clientsPids.length > 0) {
            let title = deviceId.text?
                    qsTr("Can't edit the virtual camera"):
                    qsTr("Can't add the virtual camera")
            let message = Commons.vcamDriverBusyMessage()
            addEdit.openErrorDialog(title, message)

            return
        }

        if (vcamFormats.count < 1 || !deviceDescription.text) {
            let title = deviceId.text?
                    qsTr("Error editing the virtual camera"):
                    qsTr("Error adding the virtual camera")
            let message = qsTr("Camera description and formats can't be empty.")
            addEdit.openErrorDialog(title, message)

            return
        }

        let formats = []

        for (let i = 0; i < vcamFormats.count; i++) {
            let element = vcamFormats.model.get(i)
            let caps = AkVideoCaps.create(element.format,
                                          element.width,
                                          element.height,
                                          AkFrac.create(element.fps,
                                                        1).toVariant())
            formats.push(caps.toVariant())
        }

        if (deviceId.text) {
            let ok = videoLayer.editOutput(deviceId.text,
                                           deviceDescription.text,
                                           formats)
            addEdit.edited()

            if (!ok) {
                let title = qsTr("Error editing the virtual camera")
                addEdit.openErrorDialog(title, videoLayer.outputError)
            }
        } else {
            let videoOutput =
                videoLayer.createOutput(VideoLayer.OutputVirtualCamera,
                                        deviceDescription.text,
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

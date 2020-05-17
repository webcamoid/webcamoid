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

Dialog {
    id: addFormat
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: AkUnit.create(450 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(350 * AkTheme.controlScale, "dp").pixels
    modal: true

    property int formatIndex: -1

    signal addFormat(variant caps)
    signal changeFormat(int index, variant caps)
    signal removeFormat(int index)

    function openOptions(formatIndex=-1, caps={})
    {
        addFormat.formatIndex = formatIndex
        title = formatIndex < 0?
                    qsTr("Add Video Format"):
                    qsTr("Change Video Format")
        pixelFormats.model.clear()
        let pixFormats = videoLayer.supportedOutputPixelFormats
        let index = -1

        for (let i in pixFormats) {
            if (pixFormats[i] == videoLayer.defaultOutputPixelFormat)
                index = i

            pixelFormats.model.append({
                format: Number(pixFormats[i]),
                description: AkVideoCaps.pixelFormatToString(pixFormats[i])
            })
        }

        removeFormat.visible = formatIndex >= 0

        if (formatIndex < 0) {
            pixelFormats.currentIndex = index
            frameWidth.value = 640
            frameHeight.value = 480
            frameRate.value = 30
        } else {
            index = -1

            for (let i in pixFormats)
                if (pixFormats[i] == caps.format)
                    index = i

            let fps = AkFrac.create(caps.fps)
            pixelFormats.currentIndex = index
            frameWidth.value = caps.width
            frameHeight.value = caps.height
            frameRate.value = fps.value
        }

        open()
    }

    ScrollView {
        id: formatView
        anchors.fill: parent
        contentHeight: formatControls.height
        clip: true

        GridLayout {
            id: formatControls
            columns: 2
            width: formatView.width

            Button {
                id: removeFormat
                text: qsTr("Remove format")
                icon.source: "image://icons/no"
                flat: true
                Layout.columnSpan: 2

                onClicked: {
                    addFormat.removeFormat(addFormat.formatIndex)
                    addFormat.reject()
                }
            }
            Label {
                text: qsTr("Format")
            }
            ComboBox {
                id: pixelFormats
                textRole: "description"
                model: ListModel {}
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Width")
            }
            SpinBox {
                id: frameWidth
                value: 640
                from: 1
                to: 4096
                stepSize: 1
                editable: true
            }
            Label {
                text: qsTr("Height")
            }
            SpinBox {
                id: frameHeight
                value: 480
                from: 1
                to: 4096
                stepSize: 1
                editable: true
            }
            Label {
                text: qsTr("Frame rate")
            }
            SpinBox {
                id: frameRate
                value: 30
                from: 1
                to: 250
                stepSize: 1
                editable: true
            }
        }
    }

    onAccepted: {
        let element = pixelFormats.model.get(pixelFormats.currentIndex)
        let fps = AkFrac.create(frameRate.value, 1).toVariant()
        let caps = AkVideoCaps.create(element.format,
                                      frameWidth.value,
                                      frameHeight.value,
                                      fps)

        if (addFormat.formatIndex < 0)
            addFormat.addFormat(caps)
        else
            addFormat.changeFormat(addFormat.formatIndex, caps)
    }
}

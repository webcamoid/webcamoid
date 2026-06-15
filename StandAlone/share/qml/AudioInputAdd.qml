/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as LABS
import Ak
import AkControls as AK

Dialog {
    id: addEdit
    title: qsTr("Select the audio source to add")
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: physicalWidth <= 100 || physicalHeight <= 100?
                wdgMainWidget.width: wdgMainWidget.width * 0.75
    height: physicalWidth <= 100 || physicalHeight <= 100?
                wdgMainWidget.height: wdgMainWidget.height * 0.75
    modal: true

    property real physicalWidth: wdgMainWidget.width / Screen.pixelDensity
    property real physicalHeight: wdgMainWidget.height / Screen.pixelDensity

    onVisibleChanged: if (visible) cbxAudioInput.update()

    ScrollView {
        id: view
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        ColumnLayout {
            id: layout
            width: view.width

            AK.LabeledComboBox {
                id: cbxAudioInput
                label: qsTr("Audio input")
                textRole: "description"
                model: ListModel {
                }
                Accessible.description: currentText
                Layout.fillWidth: true

                function update() {
                    model.clear()
                    audioInputs.updateDevices()
                    let inputs = audioInputs.inputs

                    for (let input in inputs) {
                        let description = audioInputs.description(inputs[input])

                        if (description.length < 1)
                            description = inputs[input]

                        model.append({
                            sourceId: inputs[input],
                            description: description
                        })
                    }

                    currentIndex = 0
                }
            }
        }
    }

    onAccepted: {
        if (cbxAudioInput.currentIndex >= 0) {
            let input = cbxAudioInput.model.get(cbxAudioInput.currentIndex)
            audioInputs.addInput(input.sourceId)
        }
    }
}

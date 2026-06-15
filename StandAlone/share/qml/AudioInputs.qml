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

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    signal openAudioInputAddDialog()

    Connections {
        target: audioInputs

        function onActiveInputsChanged() {
            activeSourcesList.update()
        }

        function onVumeter(input, level) {
            activeSourcesList.updateVuLevel(input, level)
        }
    }

    Component.onCompleted: activeSourcesList.update()

    onVisibleChanged: audioInputs.inputState = visible?
                            AkElement.ElementStatePlaying:
                            AkElement.ElementStateNull

    ListModel {
        id: addInputModel
    }

    ColumnLayout {
        layoutDirection: view.rtl ? Qt.RightToLeft : Qt.LeftToRight
        width: view.width
        clip: true

        Button {
            id: btnAddInput
            text: qsTr("Add audio input")
            icon.source: "image://icons/add"
            flat: true

            onClicked: addInput.popup()

            Menu {
                id: addInput
                width: AkUnit.create(250 * AkTheme.controlScale, "dp").pixels
                margins: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

                MenuItem {
                    text: qsTr("Add device input")
                    icon.source: "image://icons/mic"

                    onClicked: view.openAudioInputAddDialog()
                }
                MenuItem {
                    text: qsTr("Add video source")
                    icon.source: "image://icons/video"
                    height: visible? undefined: 0
                    visible: videoLayer.deviceType(videoLayer.videoInput) == VideoLayer.InputStream

                    onClicked: {
                        audioInputs.addInput(videoLayer.videoInput,
                                             videoLayer.description(videoLayer.videoInput))
                    }
                }
            }
        }
        Button {
            text: qsTr("Configure audio input settings")
            icon.source: "image://icons/settings"
            flat: true

            onClicked: deviceOptions.open()
        }

        ColumnLayout {
            id: activeSourcesList

            Layout.fillWidth: true
            layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight

            function update() {
                for (let i = children.length - 1; i >= 0; i--) {
                    let child = children[i]

                    if (child.isSourceItem)
                        child.destroy()
                }

                let active = audioInputs.activeInputs

                for (let i = 0; i < active.length; i++) {
                    let device = active[i]
                    sourceItemComponent.createObject(activeSourcesList, {
                        "device": device,
                        "label": audioInputs.description(device),
                        "volume": audioInputs.volume(device)
                    })
                }
            }

            function updateVuLevel(input, level) {
                for (let i = 0; i < children.length; i++) {
                    let child = children[i]

                    if (child.isSourceItem && child.device === input) {
                        child.vuLevel = level

                        break
                    }
                }
            }

            Component.onCompleted: update()
        }
    }

    Component {
        id: sourceItemComponent

        AudioInputSourceItem {
            readonly property bool isSourceItem: true

            Layout.fillWidth: true

            onRemoveClicked: audioInputs.removeInput(device)
            onAudioVolumeChanged: (v) => audioInputs.setVolume(device, v)
        }
    }

    AudioInputDeviceOptions {
        id: deviceOptions
        anchors.centerIn: Overlay.overlay
    }
}

/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1

Rectangle {
    id: recAudioConfig
    color: Qt.rgba(0, 0, 0, 0)
    clip: true
    width: 200
    height: 400

    function updateInputs(inputs)
    {
        lsvInputs.model.clear();

        for (var input in inputs) {
            if (inputs[input] === ":dummyin:")
                continue;

            lsvInputs.model.append({
                input: inputs[input],
                description: AudioLayer.description(inputs[input])})
        }

        if (AudioLayer.audioInput.indexOf(":dummyin:") >= 0) {
            lsvInputs.currentIndex = -1
            noInput.selected = true
        } else
            lsvInputs.currentIndex = inputs.indexOf(AudioLayer.audioInput[0]) - 1
    }

    function updateOutputs(outputs)
    {
        lsvOutputs.model.clear();

        for (var output in outputs) {
            if (outputs[output] === ":dummyout:")
                continue;

            lsvOutputs.model.append({
                output: outputs[output],
                description: AudioLayer.description(outputs[output])})
        }

        if (AudioLayer.audioOutput === ":dummyout:") {
            lsvOutputs.currentIndex = -1
            noOutput.selected = true
        } else
            lsvOutputs.currentIndex = outputs.indexOf(AudioLayer.audioOutput)
    }

    Component.onCompleted: {
        updateInputs(AudioLayer.inputs)
        updateOutputs(AudioLayer.outputs)
    }

    Connections {
        target: AudioLayer

        onInputsChanged: updateInputs(inputs)
        onOutputsChanged: updateOutputs(outputs)
        onAudioInputChanged: {
            lsvInputs.lock = true

            if (audioInput.length > 0)
                lsvInputs.currentIndex = AudioLayer.inputs.indexOf(audioInput[0]) - 1

            lsvInputs.lock = false
        }
        onAudioOutputChanged: {
            lsvOutputs.lock = true

            if (AudioLayer.audioOutput === ":dummyout:") {
                lsvOutputs.currentIndex = -1
                noOutput.selected = true
            } else
                lsvOutputs.currentIndex = AudioLayer.outputs.indexOf(audioOutput)

            lsvOutputs.lock = false
        }
        onInputDescriptionChanged: updateInputs(AudioLayer.inputs)
    }

    RowLayout {
        id: rlyDevices
        anchors.left: parent.left
        anchors.right: parent.right

        ToolButton {
            id: btnOutputs
            text: qsTr("Outputs")
            checked: true
            Layout.fillWidth: true
            tooltip: qsTr("Select the output device for audio playing")
            checkable: true
            iconSource: "image://icons/webcamoid-headphones"
            iconName: "webcamoid-headphones"

            onCheckedChanged: {
                if (checked) {
                    btnInputs.checked = false
                    recAudioConfig.state = ""
                }
            }
        }
        ToolButton {
            id: btnInputs
            text: qsTr("Inputs")
            Layout.fillWidth: true
            tooltip: qsTr("Select the device for audio capturing")
            checkable: true
            iconSource: "image://icons/webcamoid-mic"
            iconName: "webcamoid-mic"

            onCheckedChanged: {
                if (checked) {
                    btnOutputs.checked = false
                    recAudioConfig.state = "showInputs"
                }
            }
        }
    }

    // Output devices

    Rectangle {
        id: noOutput
        height: 32
        anchors.top: rlyDevices.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        property bool selected: false

        property color gradUp: selected?
                                   Qt.rgba(0.75, 0, 0, 1):
                                   Qt.rgba(0.25, 0, 0, 1)
        property color gradLow: selected?
                                    Qt.rgba(1, 0, 0, 1):
                                    Qt.rgba(0.5, 0, 0, 1)

        onSelectedChanged: {
            gradUp = selected?
                        Qt.rgba(0.75, 0, 0, 1):
                        Qt.rgba(0.25, 0, 0, 1)
            gradLow = selected?
                        Qt.rgba(1, 0, 0, 1):
                        Qt.rgba(0.5, 0, 0, 1)
        }

        gradient: Gradient {
            GradientStop {
                position: 0
                color: noOutput.gradUp
            }

            GradientStop {
                position: 1
                color: noOutput.gradLow
            }
        }

        Label {
            id: txtNoOutputButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Silence")
            color: noOutput.selected? Qt.rgba(1, 1, 1, 1): Qt.rgba(0.75, 0.75, 0.75, 1)
        }

        MouseArea {
            id: msaNoOutputButton
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            anchors.fill: parent

            onEntered: {
                txtNoOutputButton.font.bold = true
                noOutput.gradUp = noOutput.selected?
                                            Qt.rgba(1, 0.25, 0.25, 1):
                                            Qt.rgba(0.5, 0.25, 0.25, 1)
                noOutput.gradLow = noOutput.selected?
                                            Qt.rgba(1, 0.25, 0.25, 1):
                                            Qt.rgba(0.75, 0.25, 0.25, 1)
            }
            onExited: {
                txtNoOutputButton.font.bold = false
                txtNoOutputButton.scale = 1
                noOutput.gradUp = noOutput.selected?
                                            Qt.rgba(0.75, 0, 0, 1):
                                            Qt.rgba(0.25, 0, 0, 1)
                noOutput.gradLow = noOutput.selected?
                                            Qt.rgba(1, 0, 0, 1):
                                            Qt.rgba(0.5, 0, 0, 1)
            }
            onPressed: txtNoOutputButton.scale = 0.75
            onReleased: txtNoOutputButton.scale = 1
            onClicked: {
                AudioLayer.audioOutput = ":dummyout:"
                lsvOutputs.currentIndex = -1
                noOutput.selected = true
            }
        }
    }
    OptionList {
        id: lsvOutputs
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: noOutput.bottom
        anchors.bottom: parent.bottom
        textRole: "description"

        property bool lock: false

        onCurrentIndexChanged: {
            if (lock)
                return;

            var option = model.get(currentIndex)
            AudioLayer.audioOutput = option? option.output: ":dummyout:"
            noOutput.selected = false
        }
    }

    // Input devices

    Rectangle {
        id: noInput
        height: 0
        anchors.top: rlyDevices.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        visible: false

        property bool selected: false

        property color gradUp: selected?
                                   Qt.rgba(0.75, 0, 0, 1):
                                   Qt.rgba(0.25, 0, 0, 1)
        property color gradLow: selected?
                                    Qt.rgba(1, 0, 0, 1):
                                    Qt.rgba(0.5, 0, 0, 1)

        onSelectedChanged: {
            gradUp = selected?
                        Qt.rgba(0.75, 0, 0, 1):
                        Qt.rgba(0.25, 0, 0, 1)
            gradLow = selected?
                        Qt.rgba(1, 0, 0, 1):
                        Qt.rgba(0.5, 0, 0, 1)
        }

        gradient: Gradient {
            GradientStop {
                position: 0
                color: noInput.gradUp
            }

            GradientStop {
                position: 1
                color: noInput.gradLow
            }
        }

        Label {
            id: txtNoInputButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Silence")
            color: noInput.selected? Qt.rgba(1, 1, 1, 1): Qt.rgba(0.75, 0.75, 0.75, 1)
        }

        MouseArea {
            id: msaNoInputButton
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            anchors.fill: parent

            onEntered: {
                txtNoInputButton.font.bold = true
                noInput.gradUp = noInput.selected?
                                            Qt.rgba(1, 0.25, 0.25, 1):
                                            Qt.rgba(0.5, 0.25, 0.25, 1)
                noInput.gradLow = noInput.selected?
                                            Qt.rgba(1, 0.25, 0.25, 1):
                                            Qt.rgba(0.75, 0.25, 0.25, 1)
            }
            onExited: {
                txtNoInputButton.font.bold = false
                txtNoInputButton.scale = 1
                noInput.gradUp = noInput.selected?
                                            Qt.rgba(0.75, 0, 0, 1):
                                            Qt.rgba(0.25, 0, 0, 1)
                noInput.gradLow = noInput.selected?
                                            Qt.rgba(1, 0, 0, 1):
                                            Qt.rgba(0.5, 0, 0, 1)
            }
            onPressed: txtNoInputButton.scale = 0.75
            onReleased: txtNoInputButton.scale = 1
            onClicked: {
                AudioLayer.audioInput = [":dummyin:"]
                lsvInputs.currentIndex = -1
                noInput.selected = true
            }
        }
    }
    OptionList {
        id: lsvInputs
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: noInput.bottom
        anchors.bottom: parent.bottom
        textRole: "description"
        visible: false

        property bool lock: false

        onCurrentIndexChanged: {
            if (lock)
                return;

            var option = model.get(currentIndex)
            AudioLayer.audioInput = option? [option.input]: [":dummyin:"]
            noInput.selected = false
        }
    }

    states: [
        State {
            name: "showInputs"

            PropertyChanges {
                target: noOutput
                height: 0
                visible: false
            }
            PropertyChanges {
                target: lsvOutputs
                visible: false
            }
            PropertyChanges {
                target: noInput
                height: 32
                visible: true
            }
            PropertyChanges {
                target: lsvInputs
                visible: true
            }
        }
    ]
}

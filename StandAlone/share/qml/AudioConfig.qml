/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

ColumnLayout {
    property int currentIndex: tabBar.currentIndex

    function updateInputs(inputs)
    {
        lsvInputs.model.clear();

        for (var input in inputs) {
            if (inputs[input] === ":dummyin:")
                continue;

            lsvInputs.model.append({
                input: inputs[input],
                description: AudioLayer.description(inputs[input])
            })
        }

        if (AudioLayer.audioInput.indexOf(":dummyin:") >= 0) {
            lsvInputs.currentIndex = -1
            noInput.checked = true
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
            noOutput.checked = true
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
                noOutput.checked = true
            } else
                lsvOutputs.currentIndex = AudioLayer.outputs.indexOf(audioOutput)

            lsvOutputs.lock = false
        }

        onInputDescriptionChanged: {
            lsvInputs.lock = true
            updateInputs(AudioLayer.inputs)
            lsvInputs.lock = false
        }
    }

    TabBar {
        id: tabBar
        Layout.fillWidth: true

        TabButton {
            id: btnOutputs
            //: Output device for audio playback, for example a speaker.
            text: qsTr("Outputs")
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Select the output device for audio playing")
            icon.source: "image://icons/headphones"
            display: AbstractButton.IconOnly
        }
        TabButton {
            id: btnInputs
            //: Input device for audio capturing, for example a microphone.
            text: qsTr("Inputs")
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Select the device for audio capturing")
            icon.source: "image://icons/mic"
            display: AbstractButton.IconOnly
        }
    }

    StackLayout {
        currentIndex: tabBar.currentIndex
        Layout.fillWidth: true

        // Output devices

        ColumnLayout {
            Button {
                id: noOutput
                Layout.fillWidth: true
                text: qsTr("Silence")
                checkable: true

                onClicked: {
                    AudioLayer.audioOutput = ":dummyout:"
                    lsvOutputs.currentIndex = -1
                }
            }
            OptionList {
                id: lsvOutputs
                Layout.fillWidth: true
                textRole: "description"

                property bool lock: false

                onCurrentIndexChanged: {
                    if (lock)
                        return;

                    var option = model.get(currentIndex)
                    AudioLayer.audioOutput = option? option.output: ":dummyout:"
                    noOutput.checked = false
                }
            }
        }

        // Input devices

        ColumnLayout {
            Button {
                id: noInput
                Layout.fillWidth: true
                text: qsTr("Silence")
                checkable: true

                onClicked: {
                    AudioLayer.audioInput = ":dummyin:"
                    lsvInputs.currentIndex = -1
                }
            }
            OptionList {
                id: lsvInputs
                Layout.fillWidth: true
                textRole: "description"

                property bool lock: false

                onCurrentIndexChanged: {
                    if (lock)
                        return;

                    var option = model.get(currentIndex)
                    AudioLayer.audioInput = option? [option.input]: [":dummyin:"]
                    noInput.checked = false
                }
            }
        }
    }
}

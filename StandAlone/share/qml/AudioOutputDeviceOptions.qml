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
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import Ak
import AkControls as AK

Dialog {
    id: deviceOptions
    title: qsTr("Audio Device Options")
    standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Reset
    width: physicalWidth <= 100 || physicalHeight <= 100?
               wdgMainWidget.width: wdgMainWidget.width * 0.75
    height: physicalWidth <= 100 || physicalHeight <= 100?
                wdgMainWidget.height: wdgMainWidget.height * 0.75
    modal: true

    property real physicalWidth: wdgMainWidget.width / Screen.pixelDensity
    property real physicalHeight: wdgMainWidget.height / Screen.pixelDensity

    onVisibleChanged: {
        if (visible) {
            cbxAudioOutput.forceActiveFocus()
            updateParameters()
        }
    }

    function bound(min, value, max)
    {
        return Math.max(min, Math.min(value, max))
    }

    function updateParameters()
    {
        cbxAudioOutput.update()

        var audioCaps = AkAudioCaps.create()
        var supportedFormats = audioOutputs.supportedFormatsVariant
        cbxSampleFormats.model.clear()

        for (let i in supportedFormats) {
            let format = supportedFormats[i]
            let description = audioCaps.sampleFormatToString(format)
            cbxSampleFormats.model.append({format: format,
                                           description: description})
        }

        var supportedChannelLayouts =
                audioOutputs.supportedChannelLayoutsVariant
        cbxChannelLayouts.model.clear()

        for (let i in supportedChannelLayouts) {
            let layout = supportedChannelLayouts[i]
            let description = audioCaps.channelLayoutToString(layout)
            cbxChannelLayouts.model.append({layout: layout,
                                            description: description})
        }

        var supportedSampleRates = audioOutputs.supportedSampleRates
        cbxSampleRates.model.clear()

        for (let i in supportedSampleRates) {
            let sampleRate = supportedSampleRates[i]
            cbxSampleRates.model.append({sampleRate: sampleRate,
                                         description: sampleRate})
        }

        let caps = AkAudioCaps.create(audioOutputs.deviceCaps)

        cbxSampleFormats.currentIndex =
                bound(0,
                      supportedFormats.indexOf(caps.format),
                      cbxSampleFormats.model.count - 1)
        cbxChannelLayouts.currentIndex =
                bound(0,
                      supportedChannelLayouts.indexOf(caps.layout),
                      cbxChannelLayouts.model.count - 1)
        cbxSampleRates.currentIndex =
                bound(0,
                      supportedSampleRates.indexOf(caps.rate),
                      cbxSampleRates.model.count - 1)

        sldLatency.value = audioOutputs.latency

        open()
    }

    ScrollView {
        id: view
        anchors.fill: parent
        contentHeight: deviceControls.height
        clip: true

        GridLayout {
            id: deviceControls
            columns: 2
            width: view.width

            AK.LabeledComboBox {
                id: cbxAudioOutput
                label: qsTr("Audio input")
                textRole: "description"
                model: ListModel {
                }
                Accessible.description: currentText
                Layout.columnSpan: 2
                Layout.fillWidth: true

                function update() {
                    model.clear()
                    audioOutputs.updateDevices()
                    let outputs = audioOutputs.outputs

                    for (let output in outputs) {
                        let description = audioOutputs.description(outputs[output])

                        if (description.length < 1)
                            description = outputs[output]

                        model.append({
                            sinkId: outputs[output],
                            description: description
                        })
                    }

                    let idx = -1

                    for (let i = 0; i < model.count; ++i) {
                        if (model.get(i).sinkId === audioOutputs.audioOutnput) {
                            idx = i

                            break
                        }
                    }

                    currentIndex = idx !== -1? idx: 0
                }

                function reset()
                {
                    let idx = -1

                    for (let i = 0; i < model.count; ++i) {
                        if (model.get(i).sinkId === audioOutputs.audioOutnput) {
                            idx = i

                            break
                        }
                    }

                    currentIndex = idx !== -1? idx: 0
                }
            }
            AK.LabeledComboBox {
                id: cbxSampleFormats
                /*: An sample represents the strength of the wave at a certain
                    time.
                    A sample can be expressed as the number of bits defining it
                    (more bits better sound), the type of data representing it
                    (signed integer, unsigned integer, floating point), and the
                    endianness of the data (big endian, little endian).
                    The sample format is the representation of that information.
                    For example, 's16le' means that each sample format is
                    represented by a 16 bits signed integer arranged as little
                    endian.
                 */
                label: qsTr("Sample Format")
                model: ListModel { }
                textRole: "description"
                Accessible.description: label
                Layout.columnSpan: 2
                Layout.fillWidth: true

                function reset()
                {
                    var supportedFormats =
                            audioOutputs.supportedFormatsVariant
                    let caps =
                        AkAudioCaps.create(audioOutputs.preferredFormat)
                    currentIndex =
                            bound(0,
                                  supportedFormats.indexOf(caps.format),
                                  model.count - 1)
                }
            }
            AK.LabeledComboBox {
                id: cbxChannelLayouts
                label: qsTr("Channels")
                model: ListModel { }
                textRole: "description"
                Accessible.description: label
                Layout.columnSpan: 2
                Layout.fillWidth: true

                function reset()
                {
                    var supportedChannelLayouts =
                            audioOutputs.supportedChannelLayoutsVariant
                    let caps =
                        AkAudioCaps.create(audioOutputs.preferredFormat)
                    currentIndex =
                            bound(0,
                                  supportedChannelLayouts.indexOf(caps.layout),
                                  model.count - 1)
                }
            }
            AK.LabeledComboBox {
                id: cbxSampleRates
                //: Number of audio samples per channel to be played per second.
                label: qsTr("Sample Rate")
                model: ListModel { }
                textRole: "description"
                Accessible.description: label
                Layout.columnSpan: 2
                Layout.fillWidth: true

                function reset()
                {
                    var supportedSampleRates =
                            audioOutputs.supportedSampleRates
                    let caps =
                        AkAudioCaps.create(audioOutputs.preferredFormat)
                    currentIndex =
                            bound(0,
                                  supportedSampleRates.indexOf(caps.rate),
                                  model.count - 1)
                }
            }
            Label {
                id: txtLatency
                /*: The latency is the amount of accumulated audio ready to
                    play, measured in time.
                    Higher latency == smoother audio playback, but more
                    desynchronization with the video.
                    Lowerer latency == audio synchronization near to the video,
                    but glitchy audio playback.

                    https://en.wikipedia.org/wiki/Latency_(audio)
                 */
                text: qsTr("Latency (ms)")
                font.bold: true
                Layout.columnSpan: 2
                Layout.fillWidth: true
            }
            Slider {
                id: sldLatency
                stepSize: 1
                from: 1
                to: 2048
                Accessible.name: txtLatency.text
                Layout.fillWidth: true

                function reset()
                {
                    value = 25
                }

                onValueChanged: spbLatency.value = value
            }
            SpinBox {
                id: spbLatency
                value: sldLatency.value
                from: sldLatency.from
                to: sldLatency.to
                stepSize: sldLatency.stepSize
                editable: true
                Accessible.name: txtLatency.text

                onValueModified: sldLatency.value = value
            }
        }
    }

    onAccepted: {
        if (cbxAudioOutput.currentIndex >= 0) {
            let output = cbxAudioOutput.model.get(cbxAudioOutput.currentIndex)
            audioOutputs.audioOutnput = output.sinkId
        }

        let audioCaps = AkAudioCaps.create()

        if (cbxSampleFormats.model.count > 0
            && cbxChannelLayouts.model.count > 0
            && cbxSampleRates.model.count > 0) {
            let sampleFormatsCI =
                bound(0,
                      cbxSampleFormats.currentIndex,
                      cbxSampleFormats.model.count - 1)
            let channelLayoutsCI =
                bound(0,
                      cbxChannelLayouts.currentIndex,
                      cbxChannelLayouts.model.count - 1)
            let sampleRatesCI =
                bound(0,
                      cbxSampleRates.currentIndex,
                      cbxSampleRates.model.count - 1)
            audioCaps =
                    AkAudioCaps.create(cbxSampleFormats.model.get(sampleFormatsCI).format,
                                       cbxChannelLayouts.model.get(channelLayoutsCI).layout,
                                       false,
                                       cbxSampleRates.model.get(sampleRatesCI).sampleRate)
        }

        let state = audioOutputs.state
        audioOutputs.state = AkElement.ElementStateNull
        audioOutputs.deviceCaps = audioCaps.toVariant()
        audioOutputs.latency = sldLatency.value
        audioOutputs.state = state
    }
    onReset: {
        cbxAudioOutput.reset()
        cbxSampleFormats.reset()
        cbxChannelLayouts.reset()
        cbxSampleRates.reset()
        sldLatency.reset()
    }
}

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
    id: deviceOptions
    title: qsTr("Audio Device Options")
    standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Reset
    width: AkUnit.create(450 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(350 * AkTheme.controlScale, "dp").pixels
    modal: true

    property string device: ""

    onVisibleChanged: cbxSampleFormats.forceActiveFocus()

    function bound(min, value, max)
    {
        return Math.max(min, Math.min(value, max))
    }

    function openOptions(device)
    {
        deviceOptions.device = device
        deviceInfo.text =
                "<b>" + audioLayer.description(device) + "</b>"
                + "<br/><i>" + device + "</i>"

        var audioCaps = AkAudioCaps.create()
        var supportedFormats = audioLayer.supportedFormatsVariant(device)
        cbxSampleFormats.model.clear()

        for (let i in supportedFormats) {
            let format = supportedFormats[i]
            let description = audioCaps.sampleFormatToString(format)
            cbxSampleFormats.model.append({format: format,
                                           description: description})
        }

        var supportedChannelLayouts =
                audioLayer.supportedChannelLayoutsVariant(device)
        cbxChannelLayouts.model.clear()

        for (let i in supportedChannelLayouts) {
            let layout = supportedChannelLayouts[i]
            let description = audioCaps.channelLayoutToString(layout)
            cbxChannelLayouts.model.append({layout: layout,
                                            description: description})
        }

        var supportedSampleRates = audioLayer.supportedSampleRates(device)
        cbxSampleRates.model.clear()

        for (let i in supportedSampleRates) {
            let sampleRate = supportedSampleRates[i]
            cbxSampleRates.model.append({sampleRate: sampleRate,
                                         description: sampleRate})
        }

        let isInputDevice = audioLayer.outputs.indexOf(device) < 0
        let caps = isInputDevice?
                AkAudioCaps.create(audioLayer.inputDeviceCaps):
                AkAudioCaps.create(audioLayer.outputDeviceCaps)

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

        sldLatency.value = isInputDevice?
                    audioLayer.inputLatency:
                    audioLayer.outputLatency

        open()
    }

    ScrollView {
        id: view
        anchors.fill: parent
        contentHeight: deviceControls.height
        clip: true

        GridLayout {
            id: deviceControls
            columns: 3
            width: view.width

            Label {
                id: deviceInfo
                elide: Label.ElideRight
                Layout.columnSpan: 3
                Layout.fillWidth: true
                Layout.bottomMargin:
                    AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            }
            Label {
                id: txtSampleFormat
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
                text: qsTr("Sample Format")
            }
            ComboBox {
                id: cbxSampleFormats
                Accessible.description: txtSampleFormat.text
                model: ListModel { }
                textRole: "description"
                Layout.columnSpan: 2
                Layout.fillWidth: true

                function reset()
                {
                    var supportedFormats =
                            audioLayer.supportedFormatsVariant(deviceOptions.device)
                    let caps =
                        AkAudioCaps.create(audioLayer.preferredFormat(deviceOptions.device))
                    currentIndex =
                            bound(0,
                                  supportedFormats.indexOf(caps.format),
                                  model.count - 1)
                }
            }
            Label {
                id: txtChannelLayouts
                text: qsTr("Channels")
            }
            ComboBox {
                id: cbxChannelLayouts
                Accessible.description: txtChannelLayouts.text
                model: ListModel { }
                textRole: "description"
                Layout.columnSpan: 2
                Layout.fillWidth: true

                function reset()
                {
                    var supportedChannelLayouts =
                            audioLayer.supportedChannelLayoutsVariant(deviceOptions.device)
                    let caps =
                        AkAudioCaps.create(audioLayer.preferredFormat(deviceOptions.device))
                    currentIndex =
                            bound(0,
                                  supportedChannelLayouts.indexOf(caps.layout),
                                  model.count - 1)
                }
            }
            Label {
                id: txtSampleRate
                //: Number of audio samples per channel to be played per second.
                text: qsTr("Sample Rate")
            }
            ComboBox {
                id: cbxSampleRates
                Accessible.description: txtSampleRate.text
                model: ListModel { }
                textRole: "description"
                Layout.columnSpan: 2
                Layout.fillWidth: true

                function reset()
                {
                    var supportedSampleRates =
                            audioLayer.supportedSampleRates(deviceOptions.device)
                    let caps =
                        AkAudioCaps.create(audioLayer.preferredFormat(deviceOptions.device))
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
            }
            Slider {
                id: sldLatency
                stepSize: 1
                from: 1
                to: 2048
                Layout.fillWidth: true
                visible: true
                Accessible.name: txtLatency.text

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
                visible: true
                editable: true
                Accessible.name: txtLatency.text

                onValueModified: sldLatency.value = value
            }
        }
    }

    onAccepted: {
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

        if (audioLayer.outputs.indexOf(deviceOptions.device) < 0) {
            let state = audioLayer.inputState
            audioLayer.inputState = AkElement.ElementStateNull
            audioLayer.inputDeviceCaps = audioCaps.toVariant()
            audioLayer.inputLatency = sldLatency.value
            audioLayer.inputState = state
        } else {
            let state = audioLayer.outputState
            audioLayer.outputState = AkElement.ElementStateNull
            audioLayer.outputDeviceCaps = audioCaps.toVariant()
            audioLayer.outputLatency = sldLatency.value
            audioLayer.outputState = state
        }
    }
    onReset: {
        cbxSampleFormats.reset()
        cbxChannelLayouts.reset()
        cbxSampleRates.reset()
        sldLatency.reset()
    }
}

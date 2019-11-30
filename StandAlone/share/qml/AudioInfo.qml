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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import AkQml 1.0

GridLayout {
    columns: 2

    property string iDevice: ""
    property string iDescription: ""
    property string oDescription: ""
    property bool noUpdate: false

    function bound(min, value, max)
    {
        return Math.max(min, Math.min(value, max))
    }

    function updateInputInfo()
    {
        noUpdate = true;

        iDevice = AudioLayer.audioInput.length > 0?
                    AudioLayer.audioInput[0]: "";
        iDescription = AudioLayer.description(iDevice)

        let audioCaps = Ak.newAudioCaps();
        let supportedFormats = Ak.newList(AudioLayer.supportedFormats(iDevice))
        iSampleFormats.clear()

        for (let format in supportedFormats)
            iSampleFormats.append({format: supportedFormats[format],
                                   description: audioCaps.sampleFormatToString(supportedFormats[format])})

        let supportedChannelLayouts =
                Ak.newList(AudioLayer.supportedChannelLayouts(iDevice))
        iChannelLayouts.clear()

        for (let layout in supportedChannelLayouts)
            iChannelLayouts.append({layout: supportedChannelLayouts[layout],
                                    description: audioCaps.channelLayoutToString(supportedChannelLayouts[layout])})

        let supportedSampleRates = AudioLayer.supportedSampleRates(iDevice)
        iSampleRates.clear()

        for (let rate in supportedSampleRates)
            iSampleRates.append({sampleRate: supportedSampleRates[rate],
                                 description: supportedSampleRates[rate]})

        let preferredFormat = Ak.newAudioCaps(AudioLayer.inputDeviceCaps)

        cbxISampleFormats.currentIndex =
                bound(0,
                      supportedFormats.indexOf(preferredFormat.format),
                      cbxISampleFormats.model.count - 1)
        cbxIChannelLayouts.currentIndex =
                bound(0,
                      supportedChannelLayouts.indexOf(preferredFormat.layouts),
                      cbxIChannelLayouts.model.count - 1)
        cbxISampleRates.currentIndex =
                bound(0,
                      supportedSampleRates.indexOf(preferredFormat.rate),
                      cbxISampleRates.model.count - 1)

        noUpdate = false
    }

    function updateOutputInfo()
    {
        noUpdate = true;

        oDescription = AudioLayer.description(AudioLayer.audioOutput)

        var audioCaps = Ak.newAudioCaps()
        var supportedFormats =
                Ak.newList(AudioLayer.supportedFormats(AudioLayer.audioOutput))
        oSampleFormats.clear()

        for (let format in supportedFormats)
            oSampleFormats.append({format: supportedFormats[format],
                                   description: audioCaps.sampleFormatToString(supportedFormats[format])})

        var supportedChannelLayouts =
                Ak.newList(AudioLayer.supportedChannelLayouts(AudioLayer.audioOutput))
        oChannelLayouts.clear()

        for (let layout in supportedChannelLayouts)
            oChannelLayouts.append({layout: supportedChannelLayouts[layout],
                                    description: audioCaps.channelLayoutToString(supportedChannelLayouts[layout])})

        var supportedSampleRates =
                AudioLayer.supportedSampleRates(AudioLayer.audioOutput)
        oSampleRates.clear()

        for (let rate in supportedSampleRates)
            oSampleRates.append({sampleRate: supportedSampleRates[rate],
                                 description: supportedSampleRates[rate]})

        let preferredFormat = Ak.newAudioCaps(AudioLayer.outputDeviceCaps)

        cbxOSampleFormats.currentIndex =
                bound(0,
                      supportedFormats.indexOf(preferredFormat.format),
                      cbxOSampleFormats.model.count - 1)
        cbxOChannelLayouts.currentIndex =
                bound(0,
                      supportedChannelLayouts.indexOf(preferredFormat.layout),
                      cbxOChannelLayouts.model.count - 1)
        cbxOSampleRates.currentIndex =
                bound(0,
                      supportedSampleRates.indexOf(preferredFormat.rate),
                      cbxOSampleRates.model.count - 1)

        noUpdate = false
    }

    function updateCaps(isInput)
    {
        if (noUpdate)
            return

        let cbxSampleFormats = isInput? cbxISampleFormats: cbxOSampleFormats
        let cbxChannelLayouts = isInput? cbxIChannelLayouts: cbxOChannelLayouts
        let cbxSampleRates = isInput? cbxISampleRates: cbxOSampleRates
        let audioCaps = Ak.newAudioCaps()

        if (cbxSampleFormats.model.count > 0
            && cbxChannelLayouts.model.count > 0
            && cbxSampleRates.model.count > 0) {
            let sampleFormatsCI = bound(0, cbxSampleFormats.currentIndex, cbxSampleFormats.model.count - 1)
            let channelLayoutsCI = bound(0, cbxChannelLayouts.currentIndex, cbxChannelLayouts.model.count - 1)
            let sampleRatesCI = bound(0, cbxSampleRates.currentIndex, cbxSampleRates.model.count - 1)

            audioCaps =
                    Ak.newAudioCaps(cbxSampleFormats.model.get(sampleFormatsCI).format,
                                    cbxChannelLayouts.model.get(channelLayoutsCI).layout,
                                    cbxSampleRates.model.get(sampleRatesCI).sampleRate)
        }

        if (isInput) {
            let state = AudioLayer.inputState
            AudioLayer.inputState = AkElement.ElementStateNull
            AudioLayer.inputDeviceCaps = Ak.varAudioCaps(audioCaps)
            AudioLayer.inputState = state
        } else {
            let state = AudioLayer.outputState
            AudioLayer.outputState = AkElement.ElementStateNull
            AudioLayer.outputDeviceCaps = Ak.varAudioCaps(audioCaps)
            AudioLayer.outputState = state
        }
    }

    Component.onCompleted: {
        updateInputInfo()
        updateOutputInfo()
    }

    Connections {
        target: AudioLayer

        onAudioInputChanged: updateInputInfo()
        onAudioOutputChanged: updateOutputInfo()
    }

    Label {
        text: qsTr("Description")
        font.bold: true
        Layout.columnSpan: 2
    }
    TextField {
        id: txtODescription
        text: oDescription
        placeholderText: qsTr("Device description")
        readOnly: true
        Layout.columnSpan: 2
        Layout.fillWidth: true
    }
    TextField {
        id: txtIDescription
        text: iDescription
        placeholderText: qsTr("Device description")
        readOnly: true
        Layout.columnSpan: 2
        Layout.fillWidth: true
        visible: false
    }
    Label {
        text: qsTr("Device ID")
        font.bold: true
        Layout.columnSpan: 2
    }
    TextField {
        id: txtODevice
        text: AudioLayer.audioOutput
        placeholderText: qsTr("Device ID")
        readOnly: true
        Layout.columnSpan: 2
        Layout.fillWidth: true
    }
    TextField {
        id: txtIDevice
        text: AudioLayer.audioInput
        placeholderText: qsTr("Device ID")
        readOnly: true
        Layout.columnSpan: 2
        Layout.fillWidth: true
        visible: false
    }

    Label {
        /*: An sample represents the strength of the wave at a certain time.
            A sample can be expressed as the number of bits defining it (more
            bits better sound), the type of data representing it (signed
            integer, unsigned integer, floating point), and the endianness of
            the data (big endian, little endian).
            The sample format is the representation of that information.
            For example, 's16le' means that each sample format is represented by
            a 16 bits signed integer arranged as little endian.
         */
        text: qsTr("Sample Format")
    }
    ColumnLayout {
        ComboBox {
            id: cbxOSampleFormats
            model: ListModel {
                id: oSampleFormats
            }
            textRole: "description"
            Layout.fillWidth: true

            onCurrentIndexChanged: updateCaps(false)
        }
        ComboBox {
            id: cbxISampleFormats
            model: ListModel {
                id: iSampleFormats
            }
            textRole: "description"
            Layout.fillWidth: true
            visible: false

            onCurrentIndexChanged: updateCaps(true)
        }
    }
    Label {
        text: qsTr("Channels")
    }
    ColumnLayout {
        ComboBox {
            id: cbxOChannelLayouts
            model: ListModel {
                id: oChannelLayouts
            }
            textRole: "description"
            Layout.fillWidth: true

            onCurrentIndexChanged: updateCaps(false)
        }
        ComboBox {
            id: cbxIChannelLayouts
            model: ListModel {
                id: iChannelLayouts
            }
            textRole: "description"
            Layout.fillWidth: true
            visible: false

            onCurrentIndexChanged: updateCaps(true)
        }
    }
    Label {
        text: qsTr("Sample Rate")
    }
    ColumnLayout {
        ComboBox {
            id: cbxOSampleRates
            model: ListModel {
                id: oSampleRates
            }
            textRole: "description"
            Layout.fillWidth: true

            onCurrentIndexChanged: updateCaps(false)
        }
        ComboBox {
            id: cbxISampleRates
            model: ListModel {
                id: iSampleRates
            }
            textRole: "description"
            Layout.fillWidth: true
            visible: false

            onCurrentIndexChanged: updateCaps(true)
        }
    }
    Label {
        /*: The latency is the amount of accumulated audio ready to play,
            measured in time.
            Higher latency == smoother audio playback, but more
            desynchronization with the video.
            Lowerer latency == audio desynchronization near to the video, but
            glitchy audio playback.

            https://en.wikipedia.org/wiki/Latency_(audio)
         */
        text: qsTr("Latency (ms)")
    }
    RowLayout {
        Slider {
            id: sldOLatency
            value: AudioLayer.outputLatency
            stepSize: 1
            from: 1
            to: 2048
            Layout.fillWidth: true
            visible: true

            onValueChanged: AudioLayer.outputLatency = value
        }
        SpinBox {
            id: spbOLatency
            value: AudioLayer.outputLatency
            from: sldOLatency.from
            to: sldOLatency.to
            stepSize: sldOLatency.stepSize
            visible: true
            editable: true

            onValueChanged: AudioLayer.outputLatency = value
        }
        Slider {
            id: sldILatency
            value: AudioLayer.inputLatency
            stepSize: 1
            from: 1
            to: 2048
            Layout.fillWidth: true
            visible: false

            onValueChanged: AudioLayer.inputLatency = value
        }
        SpinBox {
            id: spbILatency
            value: AudioLayer.inputLatency
            from: sldILatency.from
            to: sldILatency.to
            stepSize: sldILatency.stepSize
            visible: false
            editable: true

            onValueChanged: AudioLayer.inputLatency = value
        }
    }
    Label {
        Layout.fillHeight: true
    }

    states: [
        State {
            name: "showInputs"

            PropertyChanges {
                target: txtODescription
                visible: false
            }
            PropertyChanges {
                target: txtIDescription
                visible: true
            }
            PropertyChanges {
                target: txtODevice
                visible: false
            }
            PropertyChanges {
                target: txtIDevice
                visible: true
            }
            PropertyChanges {
                target: cbxOSampleFormats
                visible: false
            }
            PropertyChanges {
                target: cbxISampleFormats
                visible: true
            }
            PropertyChanges {
                target: cbxOChannelLayouts
                visible: false
            }
            PropertyChanges {
                target: cbxIChannelLayouts
                visible: true
            }
            PropertyChanges {
                target: cbxOSampleRates
                visible: false
            }
            PropertyChanges {
                target: cbxISampleRates
                visible: true
            }
            PropertyChanges {
                target: sldOLatency
                visible: false
            }
            PropertyChanges {
                target: spbOLatency
                visible: false
            }
            PropertyChanges {
                target: sldILatency
                visible: true
            }
            PropertyChanges {
                target: spbILatency
                visible: true
            }
        }
    ]
}

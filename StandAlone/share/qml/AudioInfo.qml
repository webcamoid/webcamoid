/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
import AkQml 1.0

Rectangle {
    id: recAudioConfig
    color: Qt.rgba(0, 0, 0, 0)
    clip: true
    width: 200
    height: 400

    property variant iFormat: []
    property variant iChannels: []
    property variant iSampleRate: []
    property variant oFormat: []
    property variant oChannels: []
    property variant oSampleRate: []

    Component.onCompleted: {
        var inCaps = Ak.newCaps(AudioLayer.outputCaps).toMap()
        iFormat = inCaps.format? [inCaps.format]: []
        iChannels = inCaps.channels? [inCaps.channels]: []
        iSampleRate = inCaps.rate? [inCaps.rate]: []

        var outCaps = Ak.newCaps(AudioLayer.outputDeviceCaps).toMap()
        oFormat = outCaps.format? [outCaps.format]: []
        oChannels = outCaps.channels? [outCaps.channels]: []
        oSampleRate = outCaps.rate? [outCaps.rate]: []
    }

    Connections {
        target: AudioLayer

        onAudioOutputChanged: {
            if (state === "") {
                txtDescription.text = AudioLayer.description(audioOutput)
                txtDevice.text = audioOutput
            }
        }
        onOutputDeviceCapsChanged: {
            var inCaps = Ak.newCaps(outputDeviceCaps).toMap()
            oFormat = inCaps.format? [inCaps.format]: []
            oChannels = inCaps.channels? [inCaps.channels]: []
            oSampleRate = inCaps.rate? [inCaps.rate]: []
        }
        onOutputCapsChanged: {
            var outCaps = Ak.newCaps(outputCaps).toMap()
            iFormat = outCaps.format? [outCaps.format]: []
            iChannels = outCaps.channels? [outCaps.channels]: []
            iSampleRate = outCaps.rate? [outCaps.rate]: []
        }
    }

    onStateChanged: {
        if (state === "") {
            txtDescription.text = AudioLayer.description(AudioLayer.audioOutput)
            txtDevice.text = AudioLayer.audioOutput
        }
    }

    Label {
        id: lblDescription
        text: qsTr("Description")
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: parent.top
        font.bold: true
    }
    TextField {
        id: txtDescription
        text: AudioLayer.description(AudioLayer.audioOutput)
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: lblDescription.bottom
        placeholderText: qsTr("Device description")
        readOnly: true
    }
    Label {
        id: lblDevice
        text: qsTr("Device id")
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: txtDescription.bottom
        font.bold: true
    }
    TextField {
        id: txtDevice
        text: AudioLayer.audioOutput
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: lblDevice.bottom
        placeholderText: qsTr("Device id")
        readOnly: true
    }

    GridLayout {
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.top: txtDevice.bottom
        columns: 2

        Label {
            text: qsTr("Sample Format")
        }
        ComboBox {
            id: cbxSampleFormat
            model: oFormat
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Channels")
        }
        ComboBox {
            id: cbxChannels
            model: oChannels
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Sample Rate")
        }
        ComboBox {
            id: cbxSampleRate
            model: oSampleRate
            Layout.fillWidth: true
        }

        Label {
            Layout.fillHeight: true
        }
    }

    states: [
        State {
            name: "showInputs"

            PropertyChanges {
                target: txtDescription
                text: AudioLayer.audioInput.length > 0? AudioLayer.description(AudioLayer.audioInput[0]): ""
            }
            PropertyChanges {
                target: txtDevice
                text: AudioLayer.audioInput.length > 0? AudioLayer.audioInput[0]: ""
            }
            PropertyChanges {
                target: cbxSampleFormat
                model: iFormat
            }
            PropertyChanges {
                target: cbxChannels
                model: iChannels
            }
            PropertyChanges {
                target: cbxSampleRate
                model: iSampleRate
            }
        }
    ]
}

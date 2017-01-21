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

    property string iDevice: ""
    property string iDescription: ""
    property string oDescription: ""

    function updateInputInfo()
    {
        iDevice = AudioLayer.audioInput.length > 0?
                    AudioLayer.audioInput[0]: "";
        iDescription = AudioLayer.description(iDevice);

        var audioCaps = Ak.newAudioCaps();
        var supportedFormats = AudioLayer.supportedFormats(iDevice);
        iSampleFormats.clear();

        for (var format in supportedFormats)
            iSampleFormats.append({format: audioCaps.sampleFormatFromString(supportedFormats[format]),
                                   description: supportedFormats[format]});

        var supportedChannels = AudioLayer.supportedChannels(iDevice);
        iChannels.clear();

        for (var channels in supportedChannels) {
            var description =
                    supportedChannels[channels]
                    + " - "
                    + audioCaps.defaultChannelLayoutString(supportedChannels[channels]);

            iChannels.append({channels: supportedChannels[channels],
                              description: description});
        }

        var supportedSampleRates = AudioLayer.supportedSampleRates(iDevice);
        iSampleRates.clear();

        for (var rate in supportedSampleRates)
            iSampleRates.append({sampleRate: supportedSampleRates[rate],
                                 description: supportedSampleRates[rate]});

        var preferredFormat = Ak.newAudioCaps(AudioLayer.inputDeviceCaps);

        cbxISampleFormats.currentIndex = supportedFormats.indexOf(audioCaps.sampleFormatToString(preferredFormat.format));
        cbxIChannels.currentIndex = supportedChannels.indexOf(preferredFormat.channels);
        cbxISampleRates.currentIndex = supportedSampleRates.indexOf(preferredFormat.rate);
    }

    function updateOutputInfo()
    {
        oDescription = AudioLayer.description(AudioLayer.audioOutput);

        var audioCaps = Ak.newAudioCaps();
        var supportedFormats = AudioLayer.supportedFormats(AudioLayer.audioOutput);
        oSampleFormats.clear();

        for (var format in supportedFormats)
            oSampleFormats.append({format: audioCaps.sampleFormatFromString(supportedFormats[format]),
                                   description: supportedFormats[format]});

        var supportedChannels = AudioLayer.supportedChannels(AudioLayer.audioOutput);
        oChannels.clear();

        for (var channels in supportedChannels) {
            var description =
                    supportedChannels[channels]
                    + " - "
                    + audioCaps.defaultChannelLayoutString(supportedChannels[channels]);

            oChannels.append({channels: supportedChannels[channels],
                              description: description});
        }

        var supportedSampleRates = AudioLayer.supportedSampleRates(AudioLayer.audioOutput);
        oSampleRates.clear();

        for (var rate in supportedSampleRates)
            oSampleRates.append({sampleRate: supportedSampleRates[rate],
                                 description: supportedSampleRates[rate]});

        var preferredFormat = Ak.newAudioCaps(AudioLayer.outputDeviceCaps);

        cbxOSampleFormats.currentIndex = supportedFormats.indexOf(audioCaps.sampleFormatToString(preferredFormat.format));
        cbxOChannels.currentIndex = supportedChannels.indexOf(preferredFormat.channels);
        cbxOSampleRates.currentIndex = supportedSampleRates.indexOf(preferredFormat.rate);
    }

    function updateCaps(isInput)
    {
        var cbxSampleFormats = isInput? cbxISampleFormats: cbxOSampleFormats;
        var cbxChannels = isInput? cbxIChannels: cbxOChannels;
        var cbxSampleRates = isInput? cbxISampleRates: cbxOSampleRates;

        var bound = function (min, value, max) {
            return Math.max(min, Math.min(value, max));
        };

        var sampleFormatsCI = bound(0, cbxSampleFormats.currentIndex, cbxSampleFormats.model.count - 1);
        var channelsCI = bound(0, cbxChannels.currentIndex, cbxChannels.model.count - 1);
        var sampleRatesCI = bound(0, cbxSampleRates.currentIndex, cbxSampleRates.model.count - 1);

        var audioCaps =
                Ak.newAkAudioCaps(cbxSampleFormats.model.get(sampleFormatsCI).format,
                                  cbxChannels.model.get(channelsCI).channels,
                                  cbxSampleRates.model.get(sampleRatesCI).sampleRate);

        if (isInput) {
            var state = AudioLayer.inputState;
            AudioLayer.inputState = AkElement.ElementStateNull;
            AudioLayer.inputDeviceCaps = audioCaps.toCaps();
            AudioLayer.inputState = state;
        } else {
            var state = AudioLayer.outputState;
            AudioLayer.outputState = AkElement.ElementStateNull;
            AudioLayer.outputDeviceCaps = audioCaps.toCaps();
            AudioLayer.outputState = state;
        }
    }

    Component.onCompleted: {
        updateInputInfo();
        updateOutputInfo();
    }

    Connections {
        target: AudioLayer

        onAudioInputChanged: updateInputInfo()
        onAudioOutputChanged: updateOutputInfo()
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
        text: oDescription
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
        id: glyOutputs
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.top: txtDevice.bottom
        columns: 2

        Label {
            text: qsTr("Sample Format")
        }
        ComboBox {
            id: cbxOSampleFormats
            model: ListModel {
                id: oSampleFormats
            }
            textRole: "description"
            Layout.fillWidth: true

            onCurrentIndexChanged: updateCaps(false)
        }
        Label {
            text: qsTr("Channels")
        }
        ComboBox {
            id: cbxOChannels
            model: ListModel {
                id: oChannels
            }
            textRole: "description"
            Layout.fillWidth: true

            onCurrentIndexChanged: updateCaps(false)
        }
        Label {
            text: qsTr("Sample Rate")
        }
        ComboBox {
            id: cbxOSampleRates
            model: ListModel {
                id: oSampleRates
            }
            textRole: "description"
            Layout.fillWidth: true

            onCurrentIndexChanged: updateCaps(false)
        }
        Label {
            Layout.fillHeight: true
        }
    }

    GridLayout {
        id: glyInputs
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.top: txtDevice.bottom
        columns: 2
        visible: false

        Label {
            text: qsTr("Sample Format")
        }
        ComboBox {
            id: cbxISampleFormats
            model: ListModel {
                id: iSampleFormats
            }
            textRole: "description"
            Layout.fillWidth: true

            onCurrentIndexChanged: updateCaps(true)
        }
        Label {
            text: qsTr("Channels")
        }
        ComboBox {
            id: cbxIChannels
            model: ListModel {
                id: iChannels
            }
            textRole: "description"
            Layout.fillWidth: true

            onCurrentIndexChanged: updateCaps(true)
        }
        Label {
            text: qsTr("Sample Rate")
        }
        ComboBox {
            id: cbxISampleRates
            model: ListModel {
                id: iSampleRates
            }
            textRole: "description"
            Layout.fillWidth: true

            onCurrentIndexChanged: updateCaps(true)
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
                text: iDescription
            }
            PropertyChanges {
                target: txtDevice
                text: iDevice
            }
            PropertyChanges {
                target: glyOutputs
                visible: false
            }
            PropertyChanges {
                target: glyInputs
                visible: true
            }
        }
    ]
}

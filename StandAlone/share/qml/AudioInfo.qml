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

Rectangle {
    id: recAudioConfig
    color: Qt.rgba(0, 0, 0, 0)
    clip: true
    width: 200
    height: 400

    property string iDevice: ""
    property string iDescription: ""
    property string oDescription: ""

    function bound(min, value, max)
    {
        return Math.max(min, Math.min(value, max));
    }

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

        cbxISampleFormats.currentIndex =
                bound(0,
                      supportedFormats.indexOf(audioCaps.sampleFormatToString(preferredFormat.format)),
                      cbxISampleFormats.model.count - 1);
        cbxIChannels.currentIndex =
                bound(0,
                      supportedChannels.indexOf(preferredFormat.channels),
                      cbxIChannels.model.count - 1);
        cbxISampleRates.currentIndex =
                bound(0,
                      supportedSampleRates.indexOf(preferredFormat.rate),
                      cbxISampleRates.model.count - 1);
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

        cbxOSampleFormats.currentIndex =
                bound(0,
                      supportedFormats.indexOf(audioCaps.sampleFormatToString(preferredFormat.format)),
                      cbxOSampleFormats.model.count - 1);
        cbxOChannels.currentIndex =
                bound(0,
                      supportedChannels.indexOf(preferredFormat.channels),
                      cbxOChannels.model.count - 1);
        cbxOSampleRates.currentIndex =
                bound(0,
                      supportedSampleRates.indexOf(preferredFormat.rate),
                      cbxOSampleRates.model.count - 1);
    }

    function updateCaps(isInput)
    {
        var cbxSampleFormats = isInput? cbxISampleFormats: cbxOSampleFormats;
        var cbxChannels = isInput? cbxIChannels: cbxOChannels;
        var cbxSampleRates = isInput? cbxISampleRates: cbxOSampleRates;

        var audioCaps = Ak.newAudioCaps()

        if (cbxSampleFormats.model.count > 0
            && cbxChannels.model.count > 0
            && cbxSampleRates.model.count > 0) {
            var sampleFormatsCI = bound(0, cbxSampleFormats.currentIndex, cbxSampleFormats.model.count - 1);
            var channelsCI = bound(0, cbxChannels.currentIndex, cbxChannels.model.count - 1);
            var sampleRatesCI = bound(0, cbxSampleRates.currentIndex, cbxSampleRates.model.count - 1);

            audioCaps =
                    Ak.newAudioCaps(cbxSampleFormats.model.get(sampleFormatsCI).format,
                                    cbxChannels.model.get(channelsCI).channels,
                                    cbxSampleRates.model.get(sampleRatesCI).sampleRate);
        }

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
    ColumnLayout {
        id: clyDescription
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: lblDescription.bottom

        TextField {
            id: txtODescription
            text: oDescription
            placeholderText: qsTr("Device description")
            readOnly: true
            Layout.fillWidth: true
        }
        TextField {
            id: txtIDescription
            text: iDescription
            placeholderText: qsTr("Device description")
            readOnly: true
            Layout.fillWidth: true
            visible: false
        }
    }
    Label {
        id: lblDevice
        text: qsTr("Device id")
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: clyDescription.bottom
        font.bold: true
    }
    ColumnLayout {
        id: clyDevice
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: lblDevice.bottom

        TextField {
            id: txtODevice
            text: AudioLayer.audioOutput
            placeholderText: qsTr("Device id")
            readOnly: true
            Layout.fillWidth: true
        }
        TextField {
            id: txtIDevice
            text: AudioLayer.audioInput
            placeholderText: qsTr("Device id")
            readOnly: true
            Layout.fillWidth: true
            visible: false
        }
    }

    GridLayout {
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.top: clyDevice.bottom
        columns: 2

        Label {
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
                id: cbxOChannels
                model: ListModel {
                    id: oChannels
                }
                textRole: "description"
                Layout.fillWidth: true

                onCurrentIndexChanged: updateCaps(false)
            }
            ComboBox {
                id: cbxIChannels
                model: ListModel {
                    id: iChannels
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
            Layout.fillHeight: true
        }
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
                target: cbxOChannels
                visible: false
            }
            PropertyChanges {
                target: cbxIChannels
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
        }
    ]
}

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

GroupBox {
    id: gbxStreamOptions
    title: streamLabel.length > 0?
               qsTr("Stream #%1 (%2)").arg(streamIndex).arg(streamLabel):
               qsTr("Stream #%1").arg(streamIndex)

    property int outputIndex: 0
    property int streamIndex: 0
    property string streamLabel: ""
    property string codecsTextRole: ""
    property variant codecList: ListModel {}
    property string codec: ""
    property int bitrate: 0
    property int videoGOP: 0
    property variant codecOptions: ({})

    signal streamOptionsChanged(int index, variant options)

    function notifyOptions()
    {
        streamOptionsChanged(outputIndex,
                             {codec: codec,
                              bitrate: bitrate,
                              gop: videoGOP,
                              codecOptions: codecOptions})
    }

    onCodecChanged: {
        for (var i = 0; i < cbxCodec.count; i++)
            if (cbxCodec.model.get(i).codec === codec) {
                cbxCodec.currentIndex = i

                return
            }
    }

    GridLayout {
        anchors.fill: parent
        columns: 2

        Label {
            text: qsTr("Codec")
        }
        ComboBox {
            id: cbxCodec
            textRole: gbxStreamOptions.codecsTextRole
            model: gbxStreamOptions.codecList
            Layout.fillWidth: true

            onCurrentIndexChanged: {
                var option = model.get(currentIndex)

                if (option) {
                    gbxStreamOptions.codec = option.codec
                    notifyOptions()
                }
            }
        }

        Label {
            id: lblBitRate
            text: qsTr("Bitrate")
            visible: false
        }
        TextField {
            id: txtBitRate
            text: gbxStreamOptions.bitrate
            validator: RegExpValidator {
                regExp: /\d+/
            }
            visible: false
            Layout.fillWidth: true

            onTextChanged: {
                gbxStreamOptions.bitrate = text
                notifyOptions()
            }
        }

        Label {
            id: lblVideoGOP
            text: qsTr("GOP")
            visible: false
        }
        TextField {
            id: txtVideoGOP
            placeholderText: qsTr("Separation between keyframes")
            text: gbxStreamOptions.videoGOP
            validator: RegExpValidator {
                regExp: /\d+/
            }
            visible: false
            Layout.fillWidth: true

            onTextChanged: {
                gbxStreamOptions.videoGOP = text
                notifyOptions()
            }
        }

        Label {
            text: qsTr("Options")
        }
        TextField {
            text: JSON.stringify(gbxStreamOptions.codecOptions)
            placeholderText: qsTr("{\"opt1\": \"val1\", \"opt2\": \"val2\", \"opt3\": \"val3\"...}")
            Layout.fillWidth: true

            onTextChanged: {
                gbxStreamOptions.codecOptions = JSON.parse(text)
                notifyOptions()
            }
        }

        Label {
            Layout.fillHeight: true
        }
    }
    states: [
        State {
            name: "audio"

            PropertyChanges {
                target: lblBitRate
                visible: true
            }
            PropertyChanges {
                target: txtBitRate
                placeholderText: qsTr("Size of the audio (bits) / duration (seconds)")
                visible: true
            }
        },
        State {
            name: "video"

            PropertyChanges {
                target: lblVideoGOP
                visible: true
            }
            PropertyChanges {
                target: txtVideoGOP
                visible: true
            }
            PropertyChanges {
                target: lblBitRate
                visible: true
            }
            PropertyChanges {
                target: txtBitRate
                placeholderText: qsTr("Size of the video (bits) / duration (seconds)")
                visible: true
            }
        }
    ]
}

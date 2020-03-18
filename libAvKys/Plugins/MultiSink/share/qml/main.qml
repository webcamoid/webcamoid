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
import Ak 1.0

ColumnLayout {
    id: multiSink

    function updateSupportedFormats(supportedFormats)
    {
        var outputFormatIndex = -1
        lstOutputFormats.clear()

        for (var format in supportedFormats) {
            var formatId = supportedFormats[format]
            var description = formatId
                              + " - "
                              + MultiSink.formatDescription(formatId)

            if (formatId === MultiSink.outputFormat) {
                outputFormatIndex = format
                txtFileExtensions.text = MultiSink.fileExtensions(formatId).join(", ")
            }

            lstOutputFormats.append({format: formatId,
                                     description: description})
        }

        cbxOutputFormats.currentIndex = outputFormatIndex
        updateStreams()
    }

    function updateStreams()
    {
        // Clear old options
        for (var i = 0; i < clyStreamOptions.children.length; i++)
          clyStreamOptions.children[i].destroy()

        var streams = MultiSink.streams;

        for (var stream in streams) {
            var streamConfig = streams[stream]
            var streamOptions = classStreamOptions.createObject(clyStreamOptions)
            streamOptions.Layout.fillWidth = true
            var streamCaps = AkCaps.create(streamConfig.caps)

            if (streamCaps.mimeType === "audio/x-raw")
                streamOptions.state = "audio"
            else if (streamCaps.mimeType === "video/x-raw")
                streamOptions.state = "video"

            streamOptions.outputIndex = stream
            streamOptions.streamIndex = streamConfig.index
            streamOptions.codecsTextRole = "description"

            var supportedCodecs =
                    MultiSink.supportedCodecs(MultiSink.outputFormat,
                                              streamCaps.mimeType)

            for (var codec in supportedCodecs) {
                var codecName = supportedCodecs[codec];
                var codecDescription = MultiSink.codecDescription(supportedCodecs[codec]);
                var description = codecName;

                if (codecDescription.length > 0)
                    description += " - " + codecDescription;

                streamOptions.codecList.append({codec: codecName,
                                                description: description});
            }

            streamOptions.codec = streamConfig.codec

            if (streamConfig.bitrate)
                streamOptions.bitrate = streamConfig.bitrate

            if (streamConfig.gop)
                streamOptions.videoGOP = streamConfig.gop

            streamOptions.streamOptionsChanged.connect(MultiSink.updateStream)
        }
    }

    Component.onCompleted: updateSupportedFormats(MultiSink.supportedFormats)
    Component {
        id: classStreamOptions

        StreamOptions {
        }
    }

    Connections {
        target: MultiSink

        onSupportedFormatsChanged: updateSupportedFormats(supportedFormats)
        onOutputFormatChanged: {
            btnFormatOptions.enabled = MultiSink.formatOptions().length > 0;

            for (var i = 0; i < lstOutputFormats.count; i++)
                if (lstOutputFormats.get(i).format === outputFormat) {
                    cbxOutputFormats.currentIndex = i;
                    txtFileExtensions.text = MultiSink.fileExtensions(lstOutputFormats.get(i).format).join(", ");

                    break;
                }
        }
        onStreamsChanged: updateStreams()
    }

    Label {
        text: qsTr("Output format")
        font.bold: true
        Layout.fillWidth: true
    }
    ComboBox {
        id: cbxOutputFormats
        Layout.fillWidth: true
        textRole: "description"
        model: ListModel {
            id: lstOutputFormats
        }

        onCurrentIndexChanged: {
            var opt = lstOutputFormats.get(currentIndex);

            if (opt)
                MultiSink.outputFormat = opt.format
        }
    }
    Label {
        text: qsTr("File extensions")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        id: txtFileExtensions
        readOnly: true
        Layout.fillWidth: true
    }

    Button {
        id: btnFormatOptions
        text: qsTr("Advanced Format Options")
        icon.source: "image://icons/settings"
        Layout.fillWidth: true
        enabled: MultiSink.formatOptions().length > 0

        onClicked: {
            formatConfigs.isCodec = false;
            formatConfigs.codecName =
                    lstOutputFormats.get(cbxOutputFormats.currentIndex).format;
            formatConfigs.show();
        }
    }
    ColumnLayout {
        id: clyStreamOptions
        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    CodecConfigs {
        id: formatConfigs

        onFormatControlsChanged: MultiSink.setFormatOptions(controlValues);
    }
}

/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QbQml 1.0

ColumnLayout {
    Component.onCompleted: {
        var supportedFormats = MultiSink.supportedFormats()
        var outputFormatIndex = -1

        for (var format in supportedFormats) {
            var formatId = supportedFormats[format]
            var description = formatId
                              + " - "
                              + MultiSink.formatDescription(formatId)

            if (formatId === MultiSink.outputFormat) {
                outputFormatIndex = format
                txtFileExtentions.text = MultiSink.fileExtensions(formatId).join(", ")
            }

            lstOutputFormats.append({format: formatId,
                                     description: description})
        }

        cbxOutputFormats.currentIndex = outputFormatIndex
    }

    Component {
        id: classStreamOptions

        StreamOptions {
        }
    }

    Connections {
        target: MultiSink

        onOutputFormatChanged: {
            for (var i = 0; i < lstOutputFormats.count; i++)
                if (lstOutputFormats.get(i).format === outputFormat) {
                    cbxOutputFormats.currentIndex = i
                    txtFileExtentions.text = MultiSink.fileExtensions(lstOutputFormats.get(i).format).join(", ")

                    break
                }
        }
        onStreamsChanged: {
            // Clear old options
            for (var i = 0; i < clyStreamOptions.children.length; i++)
              clyStreamOptions.children[i].destroy()

            var streams = MultiSink.streams;

            for (var stream in streams) {
                var streamConfig = streams[stream]
                var streamOptions = classStreamOptions.createObject(clyStreamOptions)
                streamOptions.Layout.fillWidth = true

                var streamCaps = Qb.newCaps(streamConfig.caps)

                if (streamCaps.mimeType === "audio/x-raw")
                    streamOptions.state = "audio"
                else if (streamCaps.mimeType === "video/x-raw")
                    streamOptions.state = "video"

                streamOptions.outputIndex = stream
                streamOptions.streamIndex = streamConfig.index

                if (streamConfig.label)
                    streamOptions.streamLabel = streamConfig.label

                streamOptions.codecsTextRole = "description"

                var supportedCodecs =
                        MultiSink.supportedCodecs(MultiSink.outputFormat,
                                                  streamCaps.mimeType)
//
                for (var codec in supportedCodecs)
                    streamOptions.codecList.append({codec: supportedCodecs[codec],
                                                    description: supportedCodecs[codec]
                                                                 + " - "
                                                                 + MultiSink.codecDescription(supportedCodecs[codec])})

                streamOptions.codec = streamConfig.codec

                if (streamConfig.bitRate)
                    streamOptions.bitrate = streamConfig.bitRate

                if (streamConfig.gop)
                    streamOptions.videoGOP = streamConfig.gop

                streamOptions.codecOptions = streamConfig.codecOptions
                streamOptions.streamOptionsChanged.connect(MultiSink.updateStream)
            }
        }
    }

    Label {
        text: "Output format"
        Layout.fillWidth: true
    }
    ComboBox {
        id: cbxOutputFormats
        visible: MultiSink.showFormatOptions
        Layout.fillWidth: true
        textRole: "description"
        model: ListModel {
            id: lstOutputFormats
        }

        onCurrentIndexChanged: MultiSink.outputFormat = lstOutputFormats.get(currentIndex).format
    }
    TextField {
        visible: !MultiSink.showFormatOptions
        text: lstOutputFormats.get(cbxOutputFormats.currentIndex).description
        readOnly: true
        Layout.fillWidth: true
    }

    Label {
        text: "File extentions"
        Layout.fillWidth: true
    }
    TextField {
        id: txtFileExtentions
        readOnly: true
        placeholderText: qsTr("This output format has not specific extentions")
        Layout.fillWidth: true
    }

    ScrollView {
        id: vwScroll
        Layout.fillWidth: true
        Layout.fillHeight: true

        ColumnLayout {
            id: clyStreamOptions
            width: vwScroll.width
        }
    }
}

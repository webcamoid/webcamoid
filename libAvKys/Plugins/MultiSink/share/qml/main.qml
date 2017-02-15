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
import AkQml 1.0

ColumnLayout {
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
        createControls(MultiSink.userControls, clyUserControls)
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

            var streamCaps = Ak.newCaps(streamConfig.caps)

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

            for (var codec in supportedCodecs)
                streamOptions.codecList.append({codec: supportedCodecs[codec],
                                                description: supportedCodecs[codec]
                                                             + " - "
                                                             + MultiSink.codecDescription(supportedCodecs[codec])})

            streamOptions.codec = streamConfig.codec

            if (streamConfig.bitrate)
                streamOptions.bitrate = streamConfig.bitrate

            if (streamConfig.gop)
                streamOptions.videoGOP = streamConfig.gop

            streamOptions.streamOptionsChanged.connect(MultiSink.updateStream)
        }
    }

    function createControls(controls, where)
    {
        // Remove old controls.
        for(var i = where.children.length - 1; i >= 0 ; i--)
            where.children[i].destroy()

        // Create new ones.
        for (var control in controls) {
            var obj = classUserControl.createObject(where)
            obj.controlParams = controls[control]
            obj.onControlChanged.connect(function (controlName, value)
            {
                var ctrl = {}
                ctrl[controlName] = value
                MultiSink.setUserControlsValues(ctrl)
            })
        }
    }

    Component.onCompleted: {
        updateSupportedFormats(MultiSink.supportedFormats)
    }

    Component {
        id: classStreamOptions

        StreamOptions {
        }
    }
    Component {
        id: classUserControl

        UserControl {
        }
    }

    Connections {
        target: MultiSink

        onSupportedFormatsChanged : updateSupportedFormats(supportedFormats)
        onOutputFormatChanged: {
            for (var i = 0; i < lstOutputFormats.count; i++)
                if (lstOutputFormats.get(i).format === outputFormat) {
                    cbxOutputFormats.currentIndex = i
                    txtFileExtensions.text = MultiSink.fileExtensions(lstOutputFormats.get(i).format).join(", ")

                    break
                }
        }
        onStreamsChanged: updateStreams()
        onUserControlsChanged: createControls(userControls, clyUserControls)
        onUserControlsValuesChanged: {
        }
    }

    Label {
        text: qsTr("Output format")
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
        text: qsTr("File extensions")
        Layout.fillWidth: true
    }
    TextField {
        id: txtFileExtensions
        readOnly: true
        placeholderText: qsTr("This output format has not specific extensions")
        Layout.fillWidth: true
    }

    Button {
        text: qsTr("Advanced Format Options")
        iconName: "configure"
        iconSource: "image://icons/configure"
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
        id: clyUserControls
        Layout.fillWidth: true
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

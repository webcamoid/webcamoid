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
    }

    Label {
        text: "Output format"
        Layout.fillWidth: true
    }
    ComboBox {
        id: cbxOutputFormats
        Layout.fillWidth: true
        textRole: "description"
        model: ListModel {
            id: lstOutputFormats
        }

        onCurrentIndexChanged: MultiSink.outputFormat = lstOutputFormats.get(currentIndex).format
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
            GroupBox {
                title: qsTr("Stream #0 (Video)")
                width: vwScroll.width

                GridLayout {
                    columns: 2
                    width: parent.width - rowSpacing

                    Label {
                        text: "Codec"
                    }
                    ComboBox {
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "Bitrate"
                    }
                    TextField {
                        placeholderText: qsTr("Text Field")
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "GOP"
                    }
                    TextField {
                        placeholderText: qsTr("Text Field")
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "Options"
                    }
                    TextField {
                        placeholderText: qsTr("Text Field")
                        Layout.fillWidth: true
                    }
                }
            }
            GroupBox {
                title: qsTr("Stream #1 (Audio)")
                width: vwScroll.width

                GridLayout {
                    columns: 2
                    width: parent.width - rowSpacing

                    Label {
                        text: "Codec"
                    }
                    ComboBox {
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "Bitrate"
                    }
                    TextField {
                        placeholderText: qsTr("Text Field")
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "Options"
                    }
                    TextField {
                        placeholderText: qsTr("Text Field")
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}

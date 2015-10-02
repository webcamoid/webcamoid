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
import QtQuick.Window 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1

ApplicationWindow {
    id: recAddRecordingFormat
    title: qsTr("Add new recording format")
    color: palette.window
    flags: Qt.Dialog
    modality: Qt.ApplicationModal

    property bool editMode: false

    SystemPalette {
        id: palette
    }

    onVisibleChanged: {
        if (!visible)
            return

        txtDescription.text = recAddRecordingFormat.editMode?
                    Webcamoid.curRecordingFormat: ""
        txtSuffix.text = recAddRecordingFormat.editMode?
                  Webcamoid.recordingFormatSuffix(Webcamoid.curRecordingFormat).join(): ""
        txtParams.text = recAddRecordingFormat.editMode?
                  Webcamoid.recordingFormatParams(Webcamoid.curRecordingFormat): ""
    }

    ColumnLayout {
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.bottomMargin: 8
        anchors.topMargin: 8
        anchors.fill: parent

        Label {
            text: qsTr("Description")
            font.bold: true
            Layout.fillWidth: true
        }
        TextField {
            id: txtDescription
            placeholderText: qsTr("Insert recording format description")
            text: recAddRecordingFormat.editMode? Webcamoid.curRecordingFormat: ""
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Suffix")
            font.bold: true
            Layout.fillWidth: true
        }
        TextField {
            id: txtSuffix
            placeholderText: qsTr("Insert wanted output file suffix")
            text: recAddRecordingFormat.editMode?
                      Webcamoid.recordingFormatSuffix(Webcamoid.curRecordingFormat).join(): ""
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Parameters")
            font.bold: true
            Layout.fillWidth: true
        }
        TextField {
            id: txtParams
            placeholderText: qsTr("Insert encoding parameters")
            text: recAddRecordingFormat.editMode?
                      Webcamoid.recordingFormatParams(Webcamoid.curRecordingFormat): ""
            Layout.fillWidth: true
        }

        Label {
            Layout.fillHeight: true
        }

        RowLayout {
            Label {
                Layout.fillWidth: true
            }

            Button {
                id: btnOk
                text: qsTr("Ok")
                iconName: "ok"
                iconSource: "qrc:/icons/hicolor/scalable/ok.svg"

                onClicked: {
                    if (txtDescription.text.length > 0
                        && txtSuffix.text.length > 0
                        && txtParams.text.length > 0) {
                        Webcamoid.setRecordingFormat(txtDescription.text,
                                                     txtSuffix.text.split(/\s*,\s*/),
                                                     txtParams.text)
                    }

                    recAddRecordingFormat.visible = false
                }
            }

            Button {
                id: btnCancel
                text: qsTr("Cancel")
                iconName: "cancel"
                iconSource: "qrc:/icons/hicolor/scalable/cancel.svg"

                onClicked: recAddRecordingFormat.visible = false
            }
        }
    }
}

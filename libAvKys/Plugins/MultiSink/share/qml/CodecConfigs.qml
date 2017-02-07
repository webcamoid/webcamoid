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
import QtQuick.Window 2.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1

ApplicationWindow {
    id: recAbout
    title: qsTr("Configure %1").arg(codecName)
    color: palette.window
    flags: Qt.Dialog
    modality: Qt.ApplicationModal
    width: 400
    height: 500

    property bool isCodec: true
    property string codecName: ""

    SystemPalette {
        id: palette
    }

    Component {
        id: classCodecControl

        CodecControl {
        }
    }

    function updateOptions()
    {
        // Remove old controls.
        for(var i = clyCodecOptions.children.length - 1; i >= 0 ; i--)
            clyCodecOptions.children[i].destroy()

        var options = isCodec?
                    MultiSink.codecOptions(codecName):
                    MultiSink.formatOptions(codecName);
        var minimumLeftWidth = 0;
        var minimumRightWidth = 0;

        for (var i in options) {
            var codecOptions = classCodecControl.createObject(clyCodecOptions);
            codecOptions.controlParams = options[i];

            if (codecOptions.leftWidth > minimumLeftWidth)
                minimumLeftWidth = codecOptions.leftWidth

            if (codecOptions.rightWidth > minimumRightWidth)
                minimumRightWidth = codecOptions.rightWidth
        }

        for (var i in clyCodecOptions.children) {
            clyCodecOptions.children[i].minimumLeftWidth = minimumLeftWidth
            clyCodecOptions.children[i].minimumRightWidth = minimumRightWidth
        }
    }

    Component.onCompleted: updateOptions()
    onCodecNameChanged: updateOptions()

    ColumnLayout {
        anchors.fill: parent

        ScrollView {
            id: scrollControls
            Layout.fillHeight: true
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                id: clyCodecOptions
                width: scrollControls.viewport.width
            }
        }

        Button {
            text: qsTr("Close")
            iconName: "window-close"
            iconSource: "image://icons/window-close"
            Layout.alignment: Qt.AlignRight
            onClicked: recAbout.close()
        }
    }
}

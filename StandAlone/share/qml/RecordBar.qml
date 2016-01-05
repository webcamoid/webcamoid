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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.5
import QtQuick.Controls 1.4

Rectangle {
    id: recRecordBar
    color: Qt.rgba(0, 0, 0, 0)
    clip: true
    width: 200
    height: 400

    function updateRecordingFormatList() {
        var curRecordingFormat = Webcamoid.curRecordingFormat
        var formats = Webcamoid.recordingFormats
        lsvRecordingFormatList.model.clear()

        if (formats.length > 0)
            Webcamoid.curRecordingFormat = formats.indexOf(curRecordingFormat) < 0?
                        formats[0]: curRecordingFormat
        else
            Webcamoid.curRecordingFormat = ""

        for (var format in formats) {
            lsvRecordingFormatList.model.append({
                format: formats[format],
                description: Webcamoid.recordingFormatDescription(formats[format])})
        }

        lsvRecordingFormatList.currentIndex = formats.indexOf(Webcamoid.curRecordingFormat)
    }

    Component.onCompleted: recRecordBar.updateRecordingFormatList()

    Connections {
        target: Webcamoid
        onCurRecordingFormatChanged: lsvRecordingFormatList.currentIndex = Webcamoid.recordingFormats.indexOf(curRecordingFormat)
    }

    TextField {
        id: txtSearchFormat
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.left: parent.left
        placeholderText: qsTr("Search format...")
    }

    ScrollView {
        anchors.top: txtSearchFormat.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        OptionList {
            id: lsvRecordingFormatList
            filter: txtSearchFormat.text
            textRole: "description"

            onCurrentIndexChanged: {
                var option = model.get(currentIndex)

                if (option)
                    Webcamoid.curRecordingFormat = option.format

                txtSearchFormat.text = ""
            }
        }
    }
}

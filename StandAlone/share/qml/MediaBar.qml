/* Webcamoid, webcam capture application.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
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
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import AkQml 1.0

ColumnLayout {
    id: recMediaBar

    function updateMediaList() {
        var curStream = MediaSource.stream
        var streams = MediaSource.streams
        lsvMediaList.model.clear()

        if (streams.length > 0)
            MediaSource.stream = streams.indexOf(curStream) < 0?
                        streams[0]: curStream
        else
            MediaSource.stream = ""

        for (var stream in streams) {
            lsvMediaList.model.append({
                stream: streams[stream],
                description: MediaSource.description(streams[stream])})
        }

        lsvMediaList.currentIndex = streams.indexOf(MediaSource.stream)
    }

    Component.onCompleted: recMediaBar.updateMediaList()

    Connections {
        target: MediaSource

        onStreamsChanged: recMediaBar.updateMediaList()
    }

    Button {
        id: btnAddMedia
        text: qsTr("Add media file")
        Layout.fillWidth: true

        onClicked: dlgAddMedia.visible = true
    }
    Label {
        id: lblNoWebcams
        height: visible? 32: 0
        text: qsTr("No webcams found")
        verticalAlignment: Text.AlignVCenter
        anchors.horizontalCenter: parent.horizontalCenter
        Layout.fillWidth: true
        visible: MediaSource.cameras.length < 1
        enabled: false
    }
    OptionList {
        id: lsvMediaList
        textRole: "description"
        Layout.fillWidth: true

        onCurrentIndexChanged: {
            var option = model.get(currentIndex)
            var playing = MediaSource.state === AkElement.ElementStatePlaying
            MediaSource.state = AkElement.ElementStateNull
            MediaSource.stream = option? option.stream: ""

            if (playing)
                MediaSource.state = AkElement.ElementStatePlaying
        }
    }

    AddMedia {
        id: dlgAddMedia
        anchors.centerIn: Overlay.overlay
    }
}

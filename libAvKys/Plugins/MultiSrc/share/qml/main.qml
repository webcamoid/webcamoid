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

GridLayout {
    columns: 2
    property bool updating: false

    Component.onCompleted: updateOptions()
    Connections {
        target: MultiSrc

        function onMediaChanged() {
            updateOptions()
        }
    }

    function updateOptions()
    {
        updating = true

        lstAudioTracks.clear()
        lstVideoTracks.clear()
        lstSubtitlesTracks.clear()

        var stream
        var lang
        var description
        var streams = MultiSrc.listTracks(AkCaps.CapsAudio)
        lstAudioTracks.append({stream: -1, language: "None"})

        for (stream in streams) {
            lang = MultiSrc.streamLanguage(streams[stream])
            description = String(streams[stream])

            if (lang)
                description += " (" + lang + ")"

            lstAudioTracks.append({stream: streams[stream], language: description})
        }

        streams = MultiSrc.listTracks(AkCaps.CapsVideo)
        lstVideoTracks.append({stream: -1, language: "None"})

        for (stream in streams) {
            lang = MultiSrc.streamLanguage(streams[stream])
            description = String(streams[stream])

            if (lang)
                description += " (" + lang + ")"

            lstVideoTracks.append({stream: streams[stream], language: description})
        }

        streams = MultiSrc.listTracks(AkCaps.CapsSubtitle)
        lstSubtitlesTracks.append({stream: -1, language: "None"})

        for (stream in streams) {
            lang = MultiSrc.streamLanguage(streams[stream])
            description = String(streams[stream])

            if (lang)
                description += " (" + lang + ")"

            lstSubtitlesTracks.append({stream: streams[stream], language: description})
        }

        cbxAudioTracks.currentIndex = lstAudioTracks.count > 1? 1: 0
        cbxVideoTracks.currentIndex = lstVideoTracks.count > 1? 1: 0
        cbxSubtitlesTracks.currentIndex = lstSubtitlesTracks.count > 1? 1: 0
        MultiSrc.streams = [MultiSrc.defaultStream(AkCaps.CapsAudio),
                            MultiSrc.defaultStream(AkCaps.CapsVideo)]

        updating = false
    }

    function updateStreams()
    {
        if (updating)
            return

        var streams = []
        var item = lstAudioTracks.get(cbxAudioTracks.currentIndex)

        if (item && item.stream >= 0)
            streams.push(item.stream)

        item = lstVideoTracks.get(cbxVideoTracks.currentIndex)

        if (item && item.stream >= 0)
            streams.push(item.stream)

        item = lstSubtitlesTracks.get(cbxSubtitlesTracks.currentIndex)

        if (item && item.stream >= 0)
            streams.push(item.stream)

        MultiSrc.streams = streams.length < 1? [-1]: streams
    }

    Label {
        id: txtVideoTrack
        text: qsTr("Video track")
    }
    ComboBox {
        id: cbxVideoTracks
        textRole: "language"
        model: ListModel {
            id: lstVideoTracks
        }
        Layout.fillWidth: true
        Accessible.description: txtVideoTrack.text

        onCurrentIndexChanged: updateStreams()
    }

    Label {
        id: txtAudioTrack
        text: qsTr("Audio track")
    }
    ComboBox {
        id: cbxAudioTracks
        textRole: "language"
        model: ListModel {
            id: lstAudioTracks
        }
        Layout.fillWidth: true
        Accessible.description: txtAudioTrack.text

        onCurrentIndexChanged: updateStreams()
    }

    Label {
        id: txtSubtitlesTrack
        text: qsTr("Subtitles track")
    }
    ComboBox {
        id: cbxSubtitlesTracks
        textRole: "language"
        model: ListModel {
            id: lstSubtitlesTracks
        }
        Layout.fillWidth: true
        Accessible.description: txtSubtitlesTrack.text

        onCurrentIndexChanged: updateStreams()
    }

    CheckBox {
     id: cbSynchronise
     text: qsTr("Synchronise")
     Layout.fillWidth: true
     checked: MultiSrc.sync
     onToggled:MultiSrc.setSync(checked)
    }
}

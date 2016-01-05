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
import QtQuick.Layouts 1.1

GridLayout {
    columns: 2
    property bool updating: false

    Component.onCompleted: updateOptions()

    function updateOptions()
    {
        updating = true

        lstAudioTracks.clear()
        lstVideoTracks.clear()
        lstSubtitlesTracks.clear()

        var stream
        var lang
        var description
        var streams = MultiSrc.listTracks("audio/x-raw")
        lstAudioTracks.append({stream: -1, language: "None"})

        for (stream in streams) {
            lang = MultiSrc.streamLanguage(streams[stream])
            description = String(streams[stream])

            if (lang)
                description += " (" + lang + ")"

            lstAudioTracks.append({stream: streams[stream], language: description})
        }

        streams = MultiSrc.listTracks("video/x-raw")
        lstVideoTracks.append({stream: -1, language: "None"})

        for (stream in streams) {
            lang = MultiSrc.streamLanguage(streams[stream])
            description = String(streams[stream])

            if (lang)
                description += " (" + lang + ")"

            lstVideoTracks.append({stream: streams[stream], language: description})
        }

        streams = MultiSrc.listTracks("text/x-raw")
        lstSubtitlesTracks.append({stream: -1, language: "None"})

        for (stream in streams) {
            lang = MultiSrc.streamLanguage(streams[stream])
            description = String(streams[stream])

            if (lang)
                description += " (" + lang + ")"

            lstSubtitlesTracks.append({stream: streams[stream], language: description})
        }

        if (MultiSrc.streams.length < 1) {
            if (lstAudioTracks.count > 1)
                cbxAudioTracks.currentIndex = 1

            if (lstVideoTracks.count > 1)
                cbxVideoTracks.currentIndex = 1
        } else {
            for (stream in MultiSrc.streams) {
                var i

                for (i = 0; i < lstAudioTracks.count; i++)
                    if (lstAudioTracks.get(i).stream === MultiSrc.streams[stream])
                        cbxAudioTracks.currentIndex = i

                for (i = 0; i < lstVideoTracks.count; i++)
                    if (lstVideoTracks.get(i).stream === MultiSrc.streams[stream])
                        cbxVideoTracks.currentIndex = i

                for (i = 0; i < lstSubtitlesTracks.count; i++)
                    if (lstSubtitlesTracks.get(i).stream === MultiSrc.streams[stream])
                        cbxSubtitlesTracks.currentIndex = i
            }
        }

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
        text: qsTr("Audio track")
    }
    ComboBox {
        id: cbxAudioTracks
        textRole: "language"
        model: ListModel {
            id: lstAudioTracks
        }
        onCurrentIndexChanged: updateStreams()
    }

    Label {
        text: qsTr("Video track")
    }
    ComboBox {
        id: cbxVideoTracks
        textRole: "language"
        model: ListModel {
            id: lstVideoTracks
        }
        onCurrentIndexChanged: updateStreams()
    }

    Label {
        text: qsTr("Subtitles track")
    }
    ComboBox {
        id: cbxSubtitlesTracks
        textRole: "language"
        model: ListModel {
            id: lstSubtitlesTracks
        }
        onCurrentIndexChanged: updateStreams()
    }

    Label {
        Layout.fillHeight: true
    }
}

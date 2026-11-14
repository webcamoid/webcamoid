/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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


import QtQuick
import QtQuick.Controls
import Ak
import Webcamoid

// Warns that a video effect needs audio, and that the source of the effect is
// not associated to an audio input yet. It is shown by the application, not by
// the QML of the effect.
//
// Sources of type stream (MultiSrc) carry their own audio, so they can not be
// associated to an audio input and the warning does not apply to them.
Label {
    id: notice

    // Source the effect is applied to, -1 if it is not applied to a source.
    property int sourceId: -1

    // The effect can process audio.
    property bool effectNeedsAudio: false

    property string audioDevice: ""

    // The source provides its own audio, instead of using an associated input.
    property bool sourceHasOwnAudio: false

    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeWindow: AkTheme.palette.active.window

    signal openSourceOptions(int sourceId)

    function update()
    {
        notice.sourceHasOwnAudio =
                notice.sourceId >= 0
                && videoLayer.sourceType(notice.sourceId) === VideoLayer.InputStream
        notice.audioDevice = notice.sourceId >= 0?
                                 videoLayer.sourceAudioDevice(notice.sourceId):
                                 ""
    }

    visible: notice.effectNeedsAudio
             && notice.sourceId >= 0
             && !notice.sourceHasOwnAudio
             && notice.audioDevice === ""
    text: qsTr("This video source is not associated to an audio input, please associate the <a href=\"source\">source</a> to an audio input for this effect to work.")
    textFormat: Text.StyledText
    wrapMode: Text.WordWrap
    padding: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    background: Rectangle {
        // The colors follow the palette set by the user: the border uses the
        // highlight color and the fill is a shade of the window color, which
        // is adjusted for light and dark themes.
        color: AkTheme.shade(notice.activeWindow, -0.1)
        border.color: notice.activeHighlight
        radius: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    }

    onSourceIdChanged: notice.update()
    onLinkActivated: notice.openSourceOptions(notice.sourceId)
    Component.onCompleted: notice.update()

    Connections {
        target: videoLayer

        function onSourceAudioDeviceChanged(id, device)
        {
            if (id === notice.sourceId)
                notice.update()
        }
    }
}

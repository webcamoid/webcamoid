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
//
// The effects applied to the canvas (canvas set, sourceId is -1) receive the
// audio mix of all the audio inputs. They are warned when there is no audio
// input, or when there is no active video source, since the audio inputs are
// only captured while a video source is playing.
Label {
    id: notice

    // Source the effect is applied to, -1 if it is not applied to a source.
    property int sourceId: -1

    // The effect is applied to the canvas instead of a source.
    property bool canvas: false

    // The effect can process audio.
    property bool effectNeedsAudio: false

    // Canvas only.
    property bool canvasHasVideo: false
    property bool canvasHasAudio: false

    property string audioDevice: ""

    // The source provides its own audio, instead of using an associated input.
    property bool sourceHasOwnAudio: false

    readonly property color activeWarning: AkTheme.palette.active.warning
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

        let hasVideo = false

        if (videoLayer.state === AkElement.ElementStatePlaying) {
            let ids = videoLayer.sourceIds()

            for (let i in ids)
                if (videoLayer.sourceEnabled(ids[i])) {
                    hasVideo = true

                    break
                }
        }

        notice.canvasHasVideo = hasVideo
        notice.canvasHasAudio = audioInputs.activeInputs.length > 0
    }

    visible: notice.effectNeedsAudio
             && (notice.canvas?
                     (!notice.canvasHasVideo || !notice.canvasHasAudio):
                     (notice.sourceId >= 0
                      && !notice.sourceHasOwnAudio
                      && notice.audioDevice === ""))
    text: {
        if (notice.canvas) {
            if (!notice.canvasHasVideo)
                return qsTr("There is no active video source, this effect needs a video source playing to receive audio.")

            return qsTr("There are no audio inputs, please add an audio input for this effect to work.")
        }

        return qsTr("This video source is not associated to an audio input, please associate the <a href=\"source\">source</a> to an audio input for this effect to work.")
    }
    textFormat: Text.StyledText
    wrapMode: Text.WordWrap
    padding: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    background: Rectangle {
        // The colors follow the palette set by the user: the border uses the
        // warning color and the fill is the window color lightly tinted with
        // it, so the text keeps the contrast of the window colors.
        color: Qt.tint(notice.activeWindow,
                       Qt.rgba(notice.activeWarning.r,
                               notice.activeWarning.g,
                               notice.activeWarning.b,
                               0.15))
        border.color: notice.activeWarning
        radius: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    }

    onSourceIdChanged: notice.update()
    onCanvasChanged: notice.update()
    onLinkActivated: notice.openSourceOptions(notice.sourceId)
    Component.onCompleted: notice.update()

    Connections {
        target: videoLayer

        function onSourceAudioDeviceChanged(id, device)
        {
            if (id === notice.sourceId)
                notice.update()
        }

        function onStateChanged(state)
        {
            notice.update()
        }

        function onSourceAdded(id, device)
        {
            notice.update()
        }

        function onSourceRemoved(id)
        {
            notice.update()
        }

        function onSourceEnabledChanged(id, enabled)
        {
            notice.update()
        }
    }

    Connections {
        target: audioInputs

        function onActiveInputsChanged(activeInputs)
        {
            notice.update()
        }
    }
}

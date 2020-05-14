/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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
import Ak 1.0

Menu {
    id: settingsMenu

    signal openVideoSettings()
    signal openAudioSettings()
    signal openSettings()

    Component.onCompleted: {
        if (videoLayer.playOnStart)
            videoLayer.state = AkElement.ElementStatePlaying;
    }

    MenuItem {
        text: qsTr("Video")
        icon.source: "image://icons/video"

        onClicked: settingsMenu.openVideoSettings()
    }
    MenuItem {
        text: qsTr("Audio")
        icon.source: "image://icons/sound"

        onClicked: settingsMenu.openAudioSettings()
    }
    MenuItem {
        text: qsTr("Preferences")
        icon.source: "image://icons/settings"

        onClicked: settingsMenu.openSettings()
    }
    MenuSeparator {}
    SwitchDelegate {
        text: qsTr("Play sources")
        checked: videoLayer.state === AkElement.ElementStatePlaying

        onToggled: {
            if (checked) {
                videoLayer.state = AkElement.ElementStatePlaying;
            } else {
                recording.state = AkElement.ElementStateNull;
                videoLayer.state = AkElement.ElementStateNull;
            }
        }
    }
}

/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import Ak
import Webcamoid

Menu {
    id: settingsMenu
    margins: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    signal openVideoSettings()
    signal openAudioSettings()
    signal openVideoEffectsPanel()
    signal openSettings()
    signal openDonationsDialog()
    signal openAboutDialog()

    Component.onCompleted: {
        if (videoLayer.playOnStart) {
            if (videoLayer.cameraPermissionStatus == VideoLayer.PermissionStatus_Granted
                || videoLayer.deviceType(videoLayer.videoInput) != VideoLayer.InputCamera) {
                videoLayer.state = AkElement.ElementStatePlaying;
            }
        }
    }

    Connections {
        target: videoLayer

        function onCameraPermissionStatusChanged(status)
        {
            if (videoLayer.playOnStart) {
                if (status == VideoLayer.PermissionStatus_Granted
                    || videoLayer.deviceType(videoLayer.videoInput) != VideoLayer.InputCamera) {
                    videoLayer.state = AkElement.ElementStatePlaying;
                }
            }
        }
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
        text: qsTr("Effects")
        icon.source: "image://icons/video-effects"

        onClicked: settingsMenu.openVideoEffectsPanel()
    }
    MenuItem {
        text: qsTr("Preferences")
        icon.source: "image://icons/settings"

        onClicked: settingsMenu.openSettings()
    }
    MenuItem {
        text: qsTr("Donate")
        icon.source: "image://icons/heart"

        onClicked: settingsMenu.openDonationsDialog()
    }
    MenuItem {
        text: qsTr("Help")
        icon.source: "image://icons/help"

        onClicked: Qt.openUrlExternally(mediaTools.projectDocumentationUrl)
    }
    MenuItem {
        text: qsTr("About")
        icon.source: "image://icons/about"

        onClicked: settingsMenu.openAboutDialog()
    }
    MenuSeparator {}
    SwitchDelegate {
        text: qsTr("Play sources")
        checked: videoLayer.state == AkElement.ElementStatePlaying
        focusPolicy: Qt.StrongFocus

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

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
import QtQuick.Layouts
import Ak
import AkControls as AK

ScrollView {
    id: view

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    onVisibleChanged: volumeSlider.forceActiveFocus()

    ColumnLayout {
        layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight
        width: view.width
        clip: true

        Button {
            text: qsTr("Configure output")
            icon.source: "image://icons/settings"
            flat: true

            onClicked: deviceOptions.open()
        }
        Label {
            id: deviceInfo
            text: audioOutputs.description(audioOutputs.audioOutput)
            font.bold: true
            wrapMode: Text.WordWrap
            Layout.leftMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            Layout.rightMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            Layout.fillWidth: true
        }
        Slider {
            id: volumeSlider
            from: 0.0
            to: 1.5
            value: audioOutputs.volume
            stepSize: 0.01
            Layout.leftMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            Layout.rightMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            Layout.fillWidth: true

            onMoved: audioOutputs.volume = value
        }
    }

    AudioOutputDeviceOptions {
        id: deviceOptions
        anchors.centerIn: Overlay.overlay
    }
}

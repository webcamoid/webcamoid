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
import QtQuick.Layouts
import Ak

ColumnLayout {
    id: root

    property string device: ""
    property string label: ""
    property real   volume: 1.0
    property real   vuLevel: 0.0

    // Range (in dB below full scale) shown by the VU-meter.
    readonly property real vuRangeDb: 60.0
    // Hardware devices are listed in audioInputs.inputs, any other active
    // source is a video source.
    readonly property bool isDevice: audioInputs.inputs.indexOf(device) != -1
    // Linear peak (0..1) converted to a dB scale, so quiet signals are visible.
    readonly property real vuDisplayLevel: {
        if (vuLevel <= 0.0)
            return 0.0

        let db = 20.0 * Math.log(vuLevel) / Math.LN10

        return Math.max(0.0, Math.min(1.0, (db + vuRangeDb) / vuRangeDb))
    }

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft
    readonly property color activeWindow: AkTheme.palette.active.window

    signal removeClicked()
    signal audioVolumeChanged(real value)

    RowLayout {
        id: layout
        layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
        Layout.leftMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
        Layout.rightMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
        Layout.fillWidth: true

        Label {
            text: root.label
            font.bold: true
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
            Layout.leftMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            Layout.rightMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            Layout.fillWidth: true

            background: Item {
                // VU-meter
                Rectangle {
                    width: root.vuDisplayLevel * parent.width
                    height: parent.height
                    color: AkTheme.shade(root.activeWindow, -0.25)
                    visible: root.isDevice
                             || videoLayer.state === AkElement.ElementStatePlaying
                }
            }
        }

        Button {
            icon.source: "image://icons/no"
            implicitWidth: implicitHeight
            flat: true
            ToolTip.text: qsTr("Remove")
            ToolTip.visible: hovered

            onClicked: root.removeClicked()
        }
    }

    Slider {
        id: volumeSlider
        from: 0.0
        to: 1.5
        value: root.volume
        stepSize: 0.01
        Layout.leftMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
        Layout.rightMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
        Layout.fillWidth: true

        onMoved: root.audioVolumeChanged(value)
    }
}

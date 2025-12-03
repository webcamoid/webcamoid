/* Webcamoid, camera capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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
    layoutDirection: rtl? Qt.RightToLeft: Qt.LeftToRight

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    Connections {
        target: FpsControl

        function onFpsChanged(fps)
        {
            let fpsVal = Math.round(AkFrac.create(fps).value)
            sldFps.value = fpsVal
            spbFps.value = fpsVal
        }
    }

    Label {
        id: lblFps
        text: qsTr("Frame rate")
        font.bold: true
        Layout.fillWidth: true
    }
    RowLayout {
        layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
        Layout.fillWidth: true

        Slider {
            id: sldFps
            value: Math.round(AkFrac.create(FpsControl.fps).value)
            stepSize: 1
            to: 128
            Layout.fillWidth: true
            Accessible.name: lblFps.text

            onValueChanged: FpsControl.fps = AkFrac.create(value, 1).toVariant()
        }
        SpinBox {
            id: spbFps
            value: Math.round(AkFrac.create(FpsControl.fps).value)
            to: sldFps.to
            stepSize: sldFps.stepSize
            editable: true
            Accessible.name: lblFps.text

            onValueChanged: FpsControl.fps = AkFrac.create(Number(value), 1).toVariant()
        }
    }
    Switch {
        text: qsTr("Fill gaps")
        checked: FpsControl.fillGaps
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: FpsControl.fillGaps = checked
    }
}

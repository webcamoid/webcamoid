/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ak
import AkControls as AK

ColumnLayout {
    id: root
    layoutDirection: rtl? Qt.RightToLeft: Qt.LeftToRight

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    // Canny
    Switch {
        id: chkCanny
        //: https://en.wikipedia.org/wiki/Canny_edge_detector
        text: qsTr("Canny mode")
        checked: Edge.canny
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: Edge.canny = checked
    }

    // Equalize
    Switch {
        //: https://en.wikipedia.org/wiki/Histogram_equalization
        text: qsTr("Equalize")
        checked: Edge.equalize
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: Edge.equalize = checked
    }

    // Threshold
    Label {
        id: txtThreshold
        text: qsTr("Threshold")
        font.bold: true
        Layout.fillWidth: true
    }
    RangeSlider {
        id: sldThreshold
        first.value: Edge.thLow
        second.value: Edge.thHi
        stepSize: 0.01
        to: 1.0
        Layout.fillWidth: true
        Accessible.name: txtThreshold.text

        first.onValueChanged: Edge.thLow = first.value
        second.onValueChanged: Edge.thHi = second.value
    }

    // Line color
    AK.ColorButton {
        text: qsTr("Linecolor")
        currentColor: AkUtils.fromRgba(Edge.lineColor)
        title: qsTr("Choose the line color")
        showAlphaChannel: true
        horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
        Layout.fillWidth: true

        onCurrentColorChanged: Edge.lineColor = AkUtils.toRgba(currentColor)
    }
    AK.ColorButton {
        text: qsTr("Background color")
        currentColor: AkUtils.fromRgba(Edge.backgroundColor)
        title: qsTr("Choose the background color")
        showAlphaChannel: true
        horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
        Layout.fillWidth: true

        onCurrentColorChanged: Edge.backgroundColor = AkUtils.toRgba(currentColor)
    }
}

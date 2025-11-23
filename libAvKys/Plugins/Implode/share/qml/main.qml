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
import AkControls as AK

ColumnLayout {
    Label {
        id: txtAmount
        text: qsTr("Amount")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldAmount
        value: Implode.amount
        stepSize: 0.01
        from: -10
        to: 10
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: txtAmount.text

        onValueChanged: Implode.amount = value
    }
}

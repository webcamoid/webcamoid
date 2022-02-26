/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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
import QtQuick.Templates 2.15 as T
import Ak 1.0

T.VerticalHeaderView {
    id: control
    implicitWidth: contentWidth
    implicitHeight: syncView? syncView.height: 0

    delegate: Rectangle {
        implicitWidth: Math.max(control.width,
                                text.implicitWidth + 2 * cellPadding)
        implicitHeight: text.implicitHeight + 2 * cellPadding
        color: control.Material.backgroundColor

        readonly property real cellPadding:
            AkUnit.create(8 * AkTheme.controlScale, "dp").pixels

        Text {
            id: text
            text: model[control.textRole]
            width: parent.width
            height: parent.height
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: enabled?
                       AkTheme.palette.active.text:
                       AkTheme.palette.disabled.text
        }
    }
}

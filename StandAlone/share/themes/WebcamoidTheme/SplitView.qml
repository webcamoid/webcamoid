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

T.SplitView {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                            implicitContentHeight + topPadding + bottomPadding)

    handle: Rectangle {
        id: handle
        implicitWidth: control.orientation === Qt.Horizontal?
            thickness:
            control.width
        implicitHeight: control.orientation === Qt.Horizontal?
            control.height:
            thickness
        color: T.SplitHandle.pressed?
                    AkTheme.palette.active.light:
               T.SplitHandle.hovered?
                    AkTheme.palette.active.mid:
                    AkTheme.palette.active.dark

        readonly property int thickness:
            AkUnit.create(4 * AkTheme.controlScale, "dp").pixels

        Rectangle {
            color: AkTheme.palette.active.highlight
            width: control.orientation === Qt.Horizontal?
                thickness:
                length
            height: control.orientation === Qt.Horizontal?
                length:
                thickness
            radius: thickness
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2

            property int length: parent.T.SplitHandle.pressed?
                handle.thickness / 2:
                2 * handle.thickness
            readonly property int thickness: parent.T.SplitHandle.pressed?
                handle.thickness / 2:
                handle.thickness / 4

            Behavior on length {
                NumberAnimation {
                    duration: 100
                }
            }
        }
    }
}

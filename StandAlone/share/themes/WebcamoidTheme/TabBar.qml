/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

T.TabBar {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color disabledHighlight: AkTheme.palette.disabled.highlight
    property int orientation: ListView.Horizontal

    contentItem: ListView {
        model: control.contentModel
        currentIndex: control.currentIndex
        spacing: control.spacing
        orientation: control.orientation
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.AutoFlickIfNeeded
        snapMode: ListView.SnapToItem
        highlightMoveDuration: 250
        highlightResizeDuration: 0
        highlightFollowsCurrentItem: true
        highlightRangeMode: ListView.ApplyRange
        preferredHighlightBegin:
            AkUnit.create(36 * AkTheme.controlScale, "dp").pixels
        preferredHighlightEnd:
            control.orientation == ListView.Horizontal?
                width - AkUnit.create(36 * AkTheme.controlScale, "dp").pixels:
                height - AkUnit.create(36 * AkTheme.controlScale, "dp").pixels

        highlight: Item {
            z: 2

            Rectangle {
                x: control.orientation == ListView.Horizontal?
                       0:
                       parent.width - width
                y: control.position == T.TabBar.Footer
                   || control.orientation == ListView.Vertical?
                       0: parent.height - height
                width: control.orientation == ListView.Horizontal?
                           parent.width:
                           AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
                height: control.orientation == ListView.Horizontal?
                            AkUnit.create(2 * AkTheme.controlScale, "dp").pixels:
                            parent.height
                color: enabled?
                           control.activeHighlight:
                           control.disabledHighlight
            }
        }
    }

    background: Rectangle {
        color: "transparent"
    }
}

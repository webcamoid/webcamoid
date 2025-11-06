/* Webcamoid, camera capture application.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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
import Ak

Item {
    id: menu
    focus: true
    implicitHeight: listView.contentHeight

    property bool showDivider: false
    property variant model: {}
     property alias currentIndex: listView.currentIndex

    signal selected(int option)

    ListView {
        id: listView
        model: menu.model
        focus: true
        currentIndex: -1
        anchors.fill: parent

        // Highlight
        highlight: Rectangle {
            color: AkTheme.shade(AkTheme.palette.active.window, -0.1)

            Behavior on x { NumberAnimation { duration: 150 } }
            Behavior on width { NumberAnimation { duration: 150 } }
        }
        highlightFollowsCurrentItem: true

        delegate: OptionsMenuItem {
            width: ListView.view.width
            title: model.title
            subtitle: model.subtitle
            iconSource: model.icon
            showDivider: menu.showDivider && index < menu.model.count - 1
            highlighted: ListView.isCurrentItem

            onClicked: {
                listView.currentIndex = index
                menu.selected(index)
            }
        }

        Keys.onUpPressed: currentIndex = Math.max(currentIndex - 1, 0)
        Keys.onDownPressed: currentIndex = Math.min(currentIndex + 1, count - 1)
        Keys.onReturnPressed: if (currentIndex >= 0) selected(currentIndex)
        Keys.onSpacePressed: if (currentIndex >= 0) selected(currentIndex)
    }

    onVisibleChanged: if (visible) menu.forceActiveFocus()
    onModelChanged: if (model) implicitHeight = listView.contentHeight

    Keys.forwardTo: [listView]
}

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
import Ak 1.0

SwipeDelegate {
    id: swipeDelegate
    Accessible.name: text
    Accessible.role: Accessible.MenuItem

    signal pathRemoved(variant item)

    ListView.onRemove: SequentialAnimation {
        PropertyAction {
            target: swipeDelegate
            property: "ListView.delayRemove"
            value: true
        }
        NumberAnimation {
            target: swipeDelegate
            property: "height"
            to: 0
            easing.type: Easing.InOutQuad
        }
        PropertyAction {
            target: swipeDelegate
            property: "ListView.delayRemove"
            value: false
        }
    }

    swipe.right: Button {
        id: deleteLabel
        text: qsTr("Remove")
        flat: true
        height: parent.height
        anchors.right: parent.right

        onClicked: swipeDelegate.pathRemoved(swipeDelegate)
    }
}

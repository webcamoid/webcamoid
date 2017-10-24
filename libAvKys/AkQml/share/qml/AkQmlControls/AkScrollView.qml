/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

import QtQuick 2.7
import QtQuick.Controls 2.0

Flickable {
    id: flickable
    contentWidth: contentItem.implicitWidth || (children.length === 1 ? children[0].implicitWidth : -1)
    contentHeight: contentItem.implicitHeight || (children.length === 1 ? children[0].implicitHeight : -1)
    implicitWidth: contentWidth
    implicitHeight: contentHeight
    // NOTE: The value may depend on dpi.
//    maximumFlickVelocity: 1000
//    flickDeceleration: 5 * maximumFlickVelocity

    ScrollBar.vertical: ScrollBar {
        parent: flickable
        x: LayoutMirroring.enabled? 0: flickable.width - width
        height: flickable.availableHeight
        active: flickable.ScrollBar.horizontal.active
        // NOTE: Need Qt 5.9 or higher.
//        policy: flickable.height < flickable.contentHeight?
//                    ScrollBar.AlwaysOn: ScrollBar.AsNeeded
    }

    ScrollBar.horizontal: ScrollBar {
        parent: flickable
        y: LayoutMirroring.enabled? 0: flickable.height - height
        width: flickable.availableWidth
        active: flickable.ScrollBar.vertical.active
        // NOTE: Need Qt 5.9 or higher.
//        policy: flickable.width < flickable.contentWidth?
//                    ScrollBar.AlwaysOn: ScrollBar.AsNeeded
    }
}

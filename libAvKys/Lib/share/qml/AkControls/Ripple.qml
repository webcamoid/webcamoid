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

Rectangle {
    id: ripple
    color: parent? parent.color: AkTheme.palette.active.highlight
    opacity: 0
    scale: 0
    anchors.centerIn: parent

    property int effectDuration: 500

    function activate()
    {
        scale = 0
        opacity = 0
        scaleAnimation.start()
        opacityAnimation.start()
    }

    // Scaling animation
    PropertyAnimation {
        id: scaleAnimation
        target: ripple
        property: "scale"
        from: 0
        to: 1.5
        duration: effectDuration
        easing.type: Easing.OutQuart
    }

    // Opacity animation
    PropertyAnimation {
        id: opacityAnimation
        target: ripple
        property: "opacity"
        from: 0.3
        to: 0
        duration: effectDuration
        easing.type: Easing.OutQuart
    }
}

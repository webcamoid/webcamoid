/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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
import QtQuick.Templates 2.5 as T

T.StackView {
    id: control

    readonly property int easingType: Easing.OutCubic
    readonly property int animationTime: 200

    // Slide in left
    popEnter: Transition {
        NumberAnimation {
            property: "x"
            from: (control.mirrored? -0.5: 0.5) *  -control.width
            to: 0
            duration: control.animationTime
            easing.type: control.easingType
        }
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: control.animationTime
            easing.type: control.easingType
        }
    }

    // Slide out right
    popExit: Transition {
        NumberAnimation {
            property: "x"
            from: 0
            to: (control.mirrored? -0.5: 0.5) * control.width
            duration: control.animationTime
            easing.type: control.easingType
        }
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: control.animationTime
            easing.type: control.easingType
        }
    }

    // Slide in right
    pushEnter: Transition {
        NumberAnimation {
            property: "x"
            from: (control.mirrored? -0.5: 0.5) * control.width
            to: 0
            duration: control.animationTime
            easing.type: control.easingType
        }
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: control.animationTime
            easing.type: control.easingType
        }
    }

    // Slide out left
    pushExit: Transition {
        NumberAnimation {
            property: "x"
            from: 0
            to: (control.mirrored? -0.5: 0.5) * -control.width
            duration: control.animationTime
            easing.type: control.easingType
        }
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: control.animationTime
            easing.type: control.easingType
        }
    }

    // Slide in right
    replaceEnter: Transition {
        NumberAnimation {
            property: "x"
            from: (control.mirrored? -0.5: 0.5) * control.width
            to: 0
            duration: control.animationTime
            easing.type: control.easingType
        }
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: control.animationTime
            easing.type: control.easingType
        }
    }

    // Slide out left
    replaceExit: Transition {
        NumberAnimation {
            property: "x"
            from: 0
            to: (control.mirrored? -0.5: 0.5) * -control.width
            duration: control.animationTime
            easing.type: control.easingType
        }
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: control.animationTime
            easing.type: control.easingType
        }
    }
}

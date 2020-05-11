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
import Ak 1.0

T.PageIndicator {
    id: control
    implicitWidth:
        Math.max(implicitBackgroundWidth + leftInset + rightInset,
                 implicitContentWidth + leftPadding + rightPadding)
    implicitHeight:
        Math.max(implicitBackgroundHeight + topInset + bottomInset,
                 implicitContentHeight + topPadding + bottomPadding)
    padding: AkUnit.create(6 * AkTheme.controlScale, "dp").pixels
    spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    hoverEnabled: true

    readonly property int animationTime: 200
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color disabledHighlight: AkTheme.palette.disabled.highlight

    delegate: Rectangle {
        implicitWidth:
            AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
        implicitHeight: implicitWidth
        radius: width / 2
        color:
            !control.enabled?
                AkTheme.constShade(control.disabledHighlight, 0, 0.5):
            control.hovered?
                AkTheme.constShade(control.activeHighlight, 0.1, 0.5):
                AkTheme.constShade(control.activeHighlight, 0, 0.5)
        border.color:
            !control.enabled?
                control.disabledHighlight:
            control.hovered?
                AkTheme.constShade(control.activeHighlight, 0.1):
                control.activeHighlight
        border.width:
            AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
        opacity: index === control.currentIndex?
                     1:
                 pressed?
                     0.75:
                     0.5

        Behavior on opacity {
            OpacityAnimator {
                duration: control.animationTime
            }
        }
        Behavior on color {
            ColorAnimation {
                duration: control.animationTime
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: control.animationTime
            }
        }
    }

    contentItem: Row {
        spacing: control.spacing

        Repeater {
            model: control.count
            delegate: control.delegate
        }
    }
}

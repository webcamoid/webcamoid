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

T.ScrollIndicator {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
    leftPadding: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    rightPadding: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels

    readonly property int fadeInTime: 200
    readonly property int fadeOutTime: 450
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color disabledHighlight: AkTheme.palette.disabled.highlight

    contentItem: Rectangle {
        id: indicatorRect
        implicitWidth: AkUnit.create(6 * AkTheme.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(6 * AkTheme.controlScale, "dp").pixels
        radius: Math.min(implicitWidth, implicitHeight) / 2
        color: enabled?
                   control.activeHighlight:
                   control.disabledHighlight
        visible: control.size < 1.0
        opacity: 0.0
    }

    PropertyAnimation {
        id: fadeIn
        target: indicatorRect
        property: "opacity"
        to: 1
        duration: control.fadeInTime
    }

    SequentialAnimation {
        id: fadeOut

        PauseAnimation {
            duration: control.fadeOutTime
        }
        PropertyAnimation {
            target: indicatorRect
            property: "opacity"
            duration: control.fadeInTime
        }
    }

    onActiveChanged: {
        if (active){
            fadeOut.stop()
            fadeIn.start()
        } else {
            fadeIn.stop()
            fadeOut.start()
        }
    }
}

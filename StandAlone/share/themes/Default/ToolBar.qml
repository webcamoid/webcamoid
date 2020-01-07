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

import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Templates 2.5 as T
import QtGraphicalEffects 1.0
import AkQml 1.0

T.ToolBar {
    id: toolBar
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    spacing: 0

    background: Rectangle {
        implicitWidth: Ak.newUnit(360 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: Ak.newUnit(48 * ThemeSettings.controlScale, "dp").pixels
        color: ThemeSettings.colorPrimary
        layer.enabled: position == ToolBar.Header
        layer.effect: DropShadow {
            cached: true
            radius: Ak.newUnit(8 * ThemeSettings.controlScale, "dp").pixels
            samples: 2 * radius + 1
            color: ThemeSettings.constShade(ThemeSettings.colorBack, -0.9)
        }
    }
}

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
import QtQuick.Controls.impl 2.12
import Ak 1.0

T.ProgressBar {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    contentItem: ProgressBarImpl {
        implicitHeight: AkUnit.create(4 * ThemeSettings.controlScale, "dp").pixels
        scale: control.mirrored? -1: 1
        color:
            control.enabled?
                ThemeSettings.colorHighlight:
                ThemeSettings.shade(ThemeSettings.colorWindow, -0.3)
        progress: control.position
        indeterminate: control.visible && control.indeterminate
    }

    background: Rectangle {
        color:
            control.enabled?
                ThemeSettings.constShade(ThemeSettings.colorHighlight, 0.0, 0.5):
                ThemeSettings.shade(ThemeSettings.colorWindow, -0.1)
        implicitWidth: AkUnit.create(200 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(4 * ThemeSettings.controlScale, "dp").pixels
        y: (control.height - height) / 2
    }
}

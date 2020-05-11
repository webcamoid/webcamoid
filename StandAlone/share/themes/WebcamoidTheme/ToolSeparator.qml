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
import QtQuick.Templates 2.5 as T
import Ak 1.0

T.ToolSeparator {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    horizontalPadding: vertical?
                           AkUnit.create(12 * AkTheme.controlScale,
                                      "dp").pixels:
                           AkUnit.create(5 * AkTheme.controlScale,
                                      "dp").pixels
    verticalPadding: vertical?
                         AkUnit.create(5 * AkTheme.controlScale,
                                    "dp").pixels:
                         AkUnit.create(12 * AkTheme.controlScale,
                                    "dp").pixels

    readonly property color activeHighlightedText: AkTheme.palette.active.highlightedText
    readonly property color disabledHighlightedText: AkTheme.palette.disabled.highlightedText

    contentItem: Rectangle {
        implicitWidth: vertical?
                           AkUnit.create(1 * AkTheme.controlScale,
                                      "dp").pixels:
                           AkUnit.create(48 * AkTheme.controlScale,
                                      "dp").pixels
        implicitHeight: vertical?
                            AkUnit.create(48 * AkTheme.controlScale,
                                       "dp").pixels:
                            AkUnit.create(1 * AkTheme.controlScale,
                                       "dp").pixels
        color:
            enabled?
                control.activeHighlightedText:
                control.disabledHighlightedText
    }
}

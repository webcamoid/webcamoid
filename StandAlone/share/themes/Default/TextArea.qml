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

T.TextArea {
    id: control
    color: enabled?
               ThemeSettings.colorActiveText:
               ThemeSettings.colorDisabledText
    placeholderTextColor:
        enabled?
            ThemeSettings.shade(ThemeSettings.colorActiveText, 0, 0.5):
            ThemeSettings.shade(ThemeSettings.colorDisabledText, 0, 0.5)
    selectedTextColor:
        enabled?
            ThemeSettings.colorActiveHighlightedText:
            ThemeSettings.colorDisableHighlightedText
    selectionColor:
        enabled?
            ThemeSettings.colorActiveHighlight:
            ThemeSettings.colorDisableHighlight
    padding: AkUnit.create(12 * ThemeSettings.controlScale, "dp").pixels
    implicitWidth: Math.max(contentWidth + leftPadding + rightPadding,
                            implicitBackgroundWidth + leftInset + rightInset,
                            placeholder.implicitWidth + leftPadding + rightPadding,
                            AkUnit.create(280 * ThemeSettings.controlScale,
                                       "dp").pixels)
    implicitHeight: Math.max(contentHeight + topPadding + bottomPadding,
                             implicitBackgroundHeight + topInset + bottomInset,
                             placeholder.implicitHeight + topPadding + bottomPadding,
                             AkUnit.create(36 * ThemeSettings.controlScale,
                                        "dp").pixels)

    readonly property int animationTime: 200
    readonly property real placeHolderPadding:
        AkUnit.create(4 * ThemeSettings.controlScale, "dp").pixels

    Text {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)
        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        elide: Text.ElideRight
        renderType: control.renderType
        visible: !control.length
                 && !control.preeditText
                 && (!control.activeFocus
                     || control.horizontalAlignment !== Qt.AlignHCenter)
    }

    background: Rectangle {
        color: control.enabled?
                   ThemeSettings.colorActiveBase:
                   ThemeSettings.colorDisabledBase
    }
}

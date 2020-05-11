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
               activeText:
               disabledText
    placeholderTextColor:
        enabled?
            activePlaceholderText:
            disabledPlaceholderText
    selectedTextColor:
        enabled?
            activeHighlightedText:
            disabledHighlightedText
    selectionColor:
        enabled?
            activeHighlight:
            disabledHighlight
    padding: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
    implicitWidth: Math.max(contentWidth + leftPadding + rightPadding,
                            implicitBackgroundWidth + leftInset + rightInset,
                            placeholder.implicitWidth + leftPadding + rightPadding,
                            AkUnit.create(280 * AkTheme.controlScale,
                                       "dp").pixels)
    implicitHeight: Math.max(contentHeight + topPadding + bottomPadding,
                             implicitBackgroundHeight + topInset + bottomInset,
                             placeholder.implicitHeight + topPadding + bottomPadding,
                             AkUnit.create(36 * AkTheme.controlScale,
                                        "dp").pixels)

    readonly property int animationTime: 200
    readonly property real placeHolderPadding:
        AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    readonly property color activeBase: AkTheme.palette.active.base
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeHighlightedText: AkTheme.palette.active.highlightedText
    readonly property color activeLink: AkTheme.palette.active.link
    readonly property color activePlaceholderText: AkTheme.palette.active.placeholderText
    readonly property color activeText: AkTheme.palette.active.text
    readonly property color disabledBase: AkTheme.palette.disabled.base
    readonly property color disabledHighlight: AkTheme.palette.disabled.highlight
    readonly property color disabledHighlightedText: AkTheme.palette.disabled.highlightedText
    readonly property color disabledLink: AkTheme.palette.disabled.link
    readonly property color disabledPlaceholderText: AkTheme.palette.disabled.placeholderText
    readonly property color disabledText: AkTheme.palette.disabled.text

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
        linkColor: control.enabled?
                       control.activeLink:
                       control.disabledLink
        enabled: control.enabled
    }

    background: Rectangle {
        color: control.enabled?
                   control.activeBase:
                   control.disabledBase
    }
}

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
               AkTheme.palette.active.text:
               AkTheme.palette.disabled.text
    placeholderTextColor:
        enabled?
            AkTheme.palette.active.placeholderText:
            AkTheme.palette.disabled.placeholderText
    selectedTextColor:
        enabled?
            AkTheme.palette.active.highlightedText:
            AkTheme.palette.disabled.highlightedText
    selectionColor:
        enabled?
            AkTheme.palette.active.highlight:
            AkTheme.palette.disabled.highlight
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
                       AkTheme.palette.active.link:
                       AkTheme.palette.disabled.link
        enabled: control.enabled
    }

    background: Rectangle {
        color: control.enabled?
                   AkTheme.palette.active.base:
                   AkTheme.palette.disabled.base
    }
}

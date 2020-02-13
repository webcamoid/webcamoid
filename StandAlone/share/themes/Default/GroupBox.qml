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

T.GroupBox {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitLabelWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    spacing: AkUnit.create(6 * AkTheme.controlScale, "dp").pixels
    padding: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    topPadding: groupTitle.text?
                    groupTitle.height
                    + titleTopPadding
                    + titleBottomPadding:
                    AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    clip: true

    property int titleTopPadding:
        AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    property int titleBottomPadding:
        AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    label: Text {
        id: groupTitle
        x: control.leftPadding
        y: titleTopPadding
        width: control.availableWidth
        text: control.title
        font.bold: true
        font.capitalization: control.font.capitalization
        font.family: control.font.family
        font.hintingPreference: control.font.hintingPreference
        font.italic: control.font.italic
        font.kerning: control.font.kerning
        font.letterSpacing: control.font.letterSpacing
        font.pointSize: control.font.pointSize
        font.preferShaping: control.font.preferShaping
        font.strikeout: control.font.strikeout
        font.styleName: control.font.styleName
        font.underline: control.font.underline
        font.wordSpacing: control.font.wordSpacing
        color: control.enabled?
                   AkTheme.palette.active.windowText:
                   AkTheme.palette.disabled.windowText
        linkColor: control.enabled?
                       AkTheme.palette.active.link:
                       AkTheme.palette.disabled.link
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
        enabled: control.enabled
    }

    background: Rectangle {
        width: parent.width
        height: parent.height
        radius: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
        color: "transparent"
        border.color: enabled?
                          AkTheme.palette.active.dark:
                          AkTheme.palette.disabled.dark
        border.width: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
    }
}

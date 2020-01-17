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
import QtQuick.Controls.impl 2.12
import Ak 1.0

T.GroupBox {
    id: groupBox
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitLabelWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    spacing: AkUnit.create(6 * ThemeSettings.controlScale, "dp").pixels
    padding: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    topPadding: groupTitle.text?
                    groupTitle.height
                    + titleTopPadding
                    + titleBottomPadding:
                    AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    clip: true

    property int titleTopPadding:
        AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    property int titleBottomPadding:
        AkUnit.create(16 * ThemeSettings.controlScale, "dp").pixels

    label: Text {
        id: groupTitle
        x: groupBox.leftPadding
        y: titleTopPadding
        width: groupBox.availableWidth
        text: groupBox.title
        font.bold: true
        font.capitalization: groupBox.font.capitalization
        font.family: groupBox.font.family
        font.hintingPreference: groupBox.font.hintingPreference
        font.italic: groupBox.font.italic
        font.kerning: groupBox.font.kerning
        font.letterSpacing: groupBox.font.letterSpacing
        font.pointSize: groupBox.font.pointSize
        font.preferShaping: groupBox.font.preferShaping
        font.strikeout: groupBox.font.strikeout
        font.styleName: groupBox.font.styleName
        font.underline: groupBox.font.underline
        font.wordSpacing: groupBox.font.wordSpacing
        color: groupBox.enabled?
                   ThemeSettings.colorText:
                   ThemeSettings.shade(ThemeSettings.colorText, 0, 0.5)
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        width: parent.width
        height: parent.height
        radius: AkUnit.create(4 * ThemeSettings.controlScale, "dp").pixels
        color: "transparent"
        border.color: enabled?
                          ThemeSettings.shade(ThemeSettings.colorBack, -0.5):
                          ThemeSettings.shade(ThemeSettings.colorBack, -0.5, 0.5)
        border.width: AkUnit.create(1 * ThemeSettings.controlScale, "dp").pixels
    }
}

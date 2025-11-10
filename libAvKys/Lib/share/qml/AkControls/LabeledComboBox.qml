/* Webcamoid, camera capture application.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import Ak

ComboBox {
    id: control

    property string label: ""
    readonly property int animationTime: 200
    readonly property color activeBase: AkTheme.palette.active.base
    readonly property color activeButton: AkTheme.palette.active.button
    readonly property color activeButtonText: AkTheme.palette.active.buttonText
    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeHighlightedText: AkTheme.palette.active.highlightedText
    readonly property color activeLight: AkTheme.palette.active.light
    readonly property color activeMid: AkTheme.palette.active.mid
    readonly property color activeText: AkTheme.palette.active.text
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color disabledBase: AkTheme.palette.disabled.base
    readonly property color disabledButton: AkTheme.palette.disabled.button
    readonly property color disabledButtonText: AkTheme.palette.disabled.buttonText
    readonly property color disabledDark: AkTheme.palette.disabled.dark
    readonly property color disabledHighlight: AkTheme.palette.disabled.highlight
    readonly property color disabledMid: AkTheme.palette.disabled.mid
    readonly property color disabledText: AkTheme.palette.disabled.text
    readonly property color disabledWindow: AkTheme.palette.disabled.window

    contentItem: T.TextField {
        id: textField
        text: control.editable?
                control.editText:
              control.label.length > 0 && control.displayText.length > 0?
                control.label + " - " + control.displayText:
              control.label.length > 0?
                control.label:
                control.displayText
        enabled: control.editable
        autoScroll: control.editable
        readOnly: control.down
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        font: control.font
        color: {
            if (control.state == "Disabled") {
                if (control.editable)
                    return control.disabledText

                if (control.flat)
                    return control.disabledHighlight

                return control.disabledButtonText
            }

            if (control.editable)
                return control.activeText

            if (control.flat)
                return control.activeHighlight

            return control.activeButtonText
        }
        selectionColor: control.activeHighlight
        selectedTextColor: control.activeHighlightedText
        verticalAlignment: Text.AlignVCenter
        selectByMouse: true
    }

    transitions: Transition {
        ColorAnimation {
            target: textField
            duration: control.animationTime
        }
    }
}

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

pragma Singleton

import QtQuick 2.12

Item {
    // Palette
    readonly property color colorHighlightedText: palette.highlightedText
    readonly property color colorHighlight: palette.highlight
    readonly property color colorText: palette.text
    readonly property color colorBase: palette.base
    readonly property color colorWindowText: palette.windowText
    readonly property color colorWindow: palette.window
    readonly property color colorButtonText: palette.buttonText
    readonly property color colorLight: colorWindow.hslLightness < 0.5?
                                            palette.dark:
                                            palette.light
    readonly property color colorMidlight: colorWindow.hslLightness < 0.5?
                                               palette.mid:
                                               palette.midlight
    readonly property color colorButton: palette.button
    readonly property color colorMid: colorWindow.hslLightness < 0.5?
                                          palette.midlight:
                                          palette.mid
    readonly property color colorDark: colorWindow.hslLightness < 0.5?
                                           palette.light:
                                           palette.dark

    readonly property real controlScale: 1.75

    function contrast(color, value=0.5)
    {
        let lightness = (11 * color.r + 16 * color.g + 5 * color.b) / 32;

        if (lightness < value)
            return Qt.hsla(0, 0, 1, 1)

        return Qt.hsla(0, 0, 0, 1)
    }

    function complementary(color)
    {
        return Qt.rgba(1 - color.r, 1 - color.g, 1 - color.b, color.a)
    }

    function constShade(color, value, alpha=1)
    {
        let lightness = Math.min(Math.max(0, color.hslLightness + value), 1)

        return Qt.hsla(color.hslHue, color.hslSaturation, lightness, alpha)
    }

    function shade(color, value, alpha=1)
    {
        if (color.hslLightness < 0.5)
            value = -value

        let lightness = Math.min(Math.max(0, color.hslLightness + value), 1)

        return Qt.hsla(color.hslHue, color.hslSaturation, lightness, alpha)
    }

    SystemPalette {
        id: palette
    }
}

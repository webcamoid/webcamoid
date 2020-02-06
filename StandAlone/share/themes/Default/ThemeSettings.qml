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
    // Active Palette

    property color colorActiveHighlightedText: paletteActive.highlightedText
    property color colorActiveHighlight: paletteActive.highlight
    property color colorActiveText: paletteActive.text
    property color colorActiveBase: paletteActive.base
    property color colorActiveWindowText: paletteActive.windowText
    property color colorActiveWindow: paletteActive.window
    property color colorActiveButtonText: paletteActive.buttonText
    property color colorActiveLight: colorActiveWindow.hslLightness < 0.5?
                                         paletteActive.dark:
                                         paletteActive.light
    property color colorActiveMidlight: colorActiveWindow.hslLightness < 0.5?
                                            paletteActive.mid:
                                            paletteActive.midlight
    property color colorActiveButton: paletteActive.button
    property color colorActiveMid: colorActiveWindow.hslLightness < 0.5?
                                       paletteActive.midlight:
                                       paletteActive.mid
    property color colorActiveDark: colorActiveWindow.hslLightness < 0.5?
                                        paletteActive.light:
                                        paletteActive.dark

    // Disabled Palette

    property color colorDisabledHighlightedText: paletteDisabled.highlightedText
    property color colorDisabledHighlight: paletteDisabled.highlight
    property color colorDisabledText: paletteDisabled.text
    property color colorDisabledBase: paletteDisabled.base
    property color colorDisabledWindowText: paletteDisabled.windowText
    property color colorDisabledWindow: paletteDisabled.window
    property color colorDisabledButtonText: paletteDisabled.buttonText
    property color colorDisabledLight: colorDisabledWindow.hslLightness < 0.5?
                                           paletteDisabled.dark:
                                           paletteDisabled.light
    property color colorDisabledMidlight: colorDisabledWindow.hslLightness < 0.5?
                                              paletteDisabled.mid:
                                              paletteDisabled.midlight
    property color colorDisabledButton: paletteDisabled.button
    property color colorDisabledMid: colorDisabledWindow.hslLightness < 0.5?
                                         paletteDisabled.midlight:
                                         paletteDisabled.mid
    property color colorDisabledDark: colorDisabledWindow.hslLightness < 0.5?
                                          paletteDisabled.light:
                                          paletteDisabled.dark

    readonly property real controlScale: 1.6

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
        id: paletteActive
        colorGroup: SystemPalette.Active
    }
    SystemPalette {
        id: paletteDisabled
        colorGroup: SystemPalette.Disabled
    }
}

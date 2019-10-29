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

import QtQuick 2.0

QtObject {
    // Palette
    readonly property color colorMain:      Qt.hsla(0.71, 0.5, 0.33, 1)
    readonly property color colorSecondary: Qt.hsla(0.42, 1, 0.5, 1)
    readonly property color colorBack:      Qt.hsla(0, 0, 0.12, 1)
    readonly property color colorText:      Qt.hsla(0, 0, 1, 1);

    readonly property color colorWindow:          Qt.lighter(colorBack, 0.9);
    readonly property color colorButton:          Qt.lighter(colorBack, 2);
    readonly property color colorButtonHighlight: Qt.lighter(colorMain, 0.5);
    readonly property color colorShadow:          Qt.lighter(colorBack, 0.01);

    // Transparent palette
    readonly property color secondaryBaseSo: Qt.hsla(0.42, 1,  0.5,  0.5);
    readonly property color secondaryHiSo:   Qt.hsla(0.42, 1, 0.75,  0.5);
    readonly property color secondaryLowSo:  Qt.hsla(0.42, 1, 0.33,  0.5);
    readonly property color backBaseTs:      Qt.hsla(   0, 0, 0.12, 0.75);
    readonly property color textSo:          Qt.hsla(   0, 0,  1.0,  0.5);
    readonly property color shadow:          Qt.hsla(0, 0, 0, 0.75)
}

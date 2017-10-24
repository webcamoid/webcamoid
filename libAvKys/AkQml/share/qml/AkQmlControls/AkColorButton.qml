/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2

Rectangle {
    width: 32
    height: 32
    color: enabled? curColor: paletteDisabled.window
    border.color: contrast(color)
    border.width: 1

    property color curColor: Qt.rgba(0.0, 0.0, 0.0, 1)
    property bool enabled: true

    signal clicked(color color)

    function clamp(min, value, max)
    {
        var v = Math.min(max, Math.max(value, min));

        return v < (min + max) / 2? min: max;
    }

    function contrast(color)
    {
        var gray = (11 * color.r + 16 * color.g + 5 * color.b) / 32;
        gray = 1.0 - clamp(0.0, gray, 1.0);

        return Qt.rgba(gray, gray, gray, 1);
    }

    SystemPalette {
        id: paletteDisabled
        colorGroup: SystemPalette.Disabled
    }

    MouseArea {
        anchors.fill: parent
        enabled: parent.enabled

        onClicked: parent.clicked(parent.color)
    }
}

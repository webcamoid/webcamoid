/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

ColumnLayout {
    function fromRgba(rgba)
    {
        var r = ((rgba >> 16) & 0xff)
        var g = ((rgba >> 8) & 0xff)
        var b = (rgba & 0xff)

        return [r, g, b]
    }

    function toRgba(color)
    {
        var a = 0xff << 24
        var r = color[0] << 16
        var g = color[1] << 8
        var b = color[2]

        return a | r | g | b
    }

    function tableFromStr(str)
    {
        if (str.length < 1)
            return []

        var colorTable = JSON.parse(str)
        var table = []

        for (var color in colorTable)
            table.push(toRgba(colorTable[color]))

        return table
    }

    function tableToStr(table)
    {
        var colorTable = []

        for (var color in table)
            colorTable.push(fromRgba(table[color]))

//        return JSON.stringify(colorTable, null, 4)
        return JSON.stringify(colorTable)
    }

    // Color table.
    Label {
        text: qsTr("Color table")
    }
    TextField {
        text: tableToStr(FalseColor.table)

        onTextChanged: FalseColor.table = tableFromStr(text)
    }

    // Soft gradient.
    CheckBox {
        text: qsTr("Soft")
        checked: FalseColor.soft

        onCheckedChanged: FalseColor.soft = checked
    }
}

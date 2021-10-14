/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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
import QtQuick.Layouts 1.3
import Ak 1.0

GridLayout {
    columns: 2

    function tableFromStr(str)
    {
        if (str.length < 1)
            return []

        var colorTable = JSON.parse(str)
        var table = []

        for (var color in colorTable)
            table.push(AkUtils.toRgba(colorTable[color]))

        return table
    }

    function tableToStr(table)
    {
        var colorTable = []

        for (var color in table)
            colorTable.push(fromRgba(table[color]))

        return JSON.stringify(colorTable, null, 4)
    }

    // Soft gradient.
    Label {
        text: qsTr("Soft")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: FalseColor.soft

            onCheckedChanged: FalseColor.soft = checked
        }
    }

    // Color table.
    Label {
        text: qsTr("Color table")
        Layout.columnSpan: 2
    }
    TextArea {
        id: colorTable
        text: tableToStr(FalseColor.table)
        cursorVisible: true
        wrapMode: TextEdit.Wrap
        Layout.columnSpan: 2
        Layout.fillWidth: true

        onTextChanged: FalseColor.table = tableFromStr(text)
    }
}

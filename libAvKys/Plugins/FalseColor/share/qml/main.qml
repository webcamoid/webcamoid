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
import Qt.labs.platform 1.1 as LABS
import Ak 1.0

GridLayout {
    columns: 2

    function createColorTable()
    {
        // Remove old controls.
        for(let i = clyColorTable.children.length - 1; i >= 0 ; i--)
            clyColorTable.children[i].destroy()

        let len = FalseColor.table.length

        // Create new ones.
        for (let index = 0; index < len; index++) {
            let component = Qt.createComponent("TableColor.qml")

            if (component.status !== Component.Ready)
                continue

            let obj = component.createObject(clyColorTable)
            obj.tableColor = AkUtils.fromRgba(FalseColor.colorAt(index))
            obj.index = index
            obj.onColorChanged.connect(function (index, tableColor) {
                FalseColor.setColor(index, AkUtils.toRgba(tableColor));
            })
            obj.onColorRemoved.connect(function (index) {
                FalseColor.removeColor(index);
            })
        }
    }

    Component.onCompleted: createColorTable()

    Connections {
        target: FalseColor

        function onTableChanged()
        {
            createColorTable()
        }
    }

    // Soft gradient.
    Label {
        id: txtSoft
        text: qsTr("Soft")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: FalseColor.soft
            Accessible.name: txtSoft.text

            onCheckedChanged: FalseColor.soft = checked
        }
    }

    Button {
        text: qsTr("Add color")
        icon.source: "image://icons/add"
        flat: true
        Layout.columnSpan: 2

        onClicked: colorDialog.open()
    }
    Button {
        text: qsTr("Clear all colors")
        icon.source: "image://icons/no"
        flat: true
        Layout.columnSpan: 2

        onClicked: FalseColor.clearTable()
    }
    ColumnLayout {
        id: clyColorTable
        Layout.fillWidth: true
        Layout.columnSpan: 2
    }

    LABS.ColorDialog {
        id: colorDialog
        //: Select the color to add to the color table
        title: qsTr("Select the color to add")

        onAccepted: FalseColor.addColor(AkUtils.toRgba(color))
    }
}

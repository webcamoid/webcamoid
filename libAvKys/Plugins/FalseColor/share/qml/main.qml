/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as LABS
import Ak

ColumnLayout {
    id: root
    layoutDirection: rtl? Qt.RightToLeft: Qt.LeftToRight

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

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
    Switch {
        text: qsTr("Soft")
        checked: FalseColor.soft
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: FalseColor.soft = checked
    }

    Button {
        text: qsTr("Add color")
        icon.source: "image://icons/add"
        flat: true

        onClicked: colorDialog.open()
    }
    Button {
        text: qsTr("Clear all colors")
        icon.source: "image://icons/no"
        flat: true

        onClicked: FalseColor.clearTable()
    }
    ColumnLayout {
        id: clyColorTable
        Layout.fillWidth: true
    }

    LABS.ColorDialog {
        id: colorDialog
        //: Select the color to add to the color table
        title: qsTr("Select the color to add")

        onAccepted: FalseColor.addColor(AkUtils.toRgba(color))
    }
}

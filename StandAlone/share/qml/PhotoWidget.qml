/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1

Rectangle {
    id: recPhotoWidget
    width: 480
    height: 80
    radius: 4
    color: "black"

    Component.onCompleted: {
        for (var i = 5; i < 35; i += 5)
            lstTimeOptions.append({text: qsTr("%1 seconds").arg(i),
                                   time: i})
    }

    GridLayout {
        id: glyControls
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.bottomMargin: 8
        anchors.topMargin: 8
        anchors.fill: parent
        columns: 3

        ComboBox {
            id: cbxTimeShot
            textRole: "text"
            model: ListModel {
                id: lstTimeOptions

                ListElement {
                    text: qsTr("Now")
                    time: 0
                }
            }
        }
        ProgressBar {
            id: pgbPhotoShot
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.rowSpan: 2
        }
        Button {
            id: btnPhotoShot
            text: qsTr("Shot!")
            isDefault: true
            Layout.fillHeight: true
            Layout.rowSpan: 2
        }
        CheckBox {
            id: chkFlash
            text: qsTr("Use flash")
            checked: true
        }
    }
}

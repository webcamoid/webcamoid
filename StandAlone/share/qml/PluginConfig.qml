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
import AkQml 1.0

ColumnLayout {
    width: 400
    height: 450

    AkElement {
        id: globalElement
    }

    GroupBox {
        title: qsTr("Search paths")
        Layout.fillWidth: true

        GridLayout {
            columns: 2
            anchors.fill: parent

            CheckBox {
                text: qsTr("Search plugins in subfolders.")
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Add")
            }
            TableView {
                Layout.columnSpan: 2
                Layout.fillWidth: true

                TableViewColumn {
                    role: "path"
                    title: qsTr("Search path")
                }
                TableViewColumn {
                    role: "action"
                    title: qsTr("Action")
                }
            }
        }
    }

    GroupBox {
        title: qsTr("Plugins list")
        Layout.fillWidth: true

        ColumnLayout {
            anchors.fill: parent

            Button {
                text: qsTr("Refresh")
            }
            TableView {
                Layout.fillWidth: true

                TableViewColumn {
                    role: "pluginPath"
                    title: qsTr("Search path")
                }
                TableViewColumn {
                    role: "action"
                    title: qsTr("Enable/Disable")
                }
            }
        }
    }

    Label {
        Layout.fillHeight: true
    }
}

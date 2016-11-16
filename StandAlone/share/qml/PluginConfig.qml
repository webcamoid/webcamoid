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
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import AkQml 1.0

ColumnLayout {
    width: 400
    height: 450

    function fillSearchPaths()
    {
        searchPathsTable.model.clear()
        var searchPaths = globalElement.searchPaths()

        for (var path in searchPaths) {
            searchPathsTable.model.append({
                path: searchPaths[path]
            })
        }
    }

    function fillPluginList()
    {
        pluginsTable.model.clear()
        var pluginsPaths = globalElement.listPluginPaths()

        for (var path in pluginsPaths) {
            pluginsTable.model.append({
                path: pluginsPaths[path]
            })
        }
    }

    function refreshCache()
    {
        globalElement.clearCache()
        fillPluginList()
        PluginConfigs.saveProperties()
    }

    AkElement {
        id: globalElement
    }

    Component.onCompleted: {
        fillSearchPaths()
        fillPluginList()
    }

    Label {
        text: qsTr("Use this page for configuring the plugins search paths.<br /><b>Don't touch nothing unless you know what you are doing</b>.")
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    GroupBox {
        title: qsTr("Search paths")
        Layout.fillWidth: true

        GridLayout {
            columns: 3
            anchors.fill: parent

            CheckBox {
                text: qsTr("Search plugins in subfolders.")
                checked: globalElement.recursiveSearch()
                Layout.fillWidth: true

                onCheckedChanged: {
                    globalElement.setRecursiveSearch(checked)
                    refreshCache()
                }
            }
            Button {
                text: qsTr("Add")
                iconName: "add"
                iconSource: "image://icons/add"

                onClicked: fileDialog.open()
            }
            Button {
                text: qsTr("Remove")
                iconName: "remove"
                iconSource: "image://icons/remove"
            }
            TableView {
                id: searchPathsTable
                headerVisible: false
                Layout.columnSpan: 3
                Layout.fillWidth: true
                model: ListModel {
                }

                TableViewColumn {
                    role: "path"
                    title: qsTr("Search path")
                }
            }
        }
    }
    GroupBox {
        title: qsTr("Plugins list")
        Layout.fillWidth: true

        GridLayout {
            columns: 3
            anchors.fill: parent

            Button {
                text: qsTr("Refresh")
                iconName: "reset"
                iconSource: "image://icons/reset"

                onClicked: refreshCache()
            }
            Label {
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Disable")
            }
            TableView {
                id: pluginsTable
                headerVisible: false
                Layout.columnSpan: 3
                Layout.fillWidth: true
                model: ListModel {
                }

                TableViewColumn {
                    role: "path"
                    title: qsTr("Plugin path")
                }
            }
        }
    }
    Label {
        Layout.fillHeight: true
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Add plugins search path")
        selectFolder: true

        onAccepted: {
            var path = Webcamoid.urlToLocalFile(folder)
            globalElement.addSearchPath(path)
            fillSearchPaths()
            refreshCache()
        }
    }
}

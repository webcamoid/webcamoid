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
                path: pluginsPaths[path],
                enabled: true
            })
        }

        var blackList = globalElement.pluginsBlackList()

        for (var path in blackList) {
            pluginsTable.model.append({
                path: blackList[path],
                enabled: false
            })
        }
    }

    function refreshCache()
    {
        globalElement.clearCache()
        fillPluginList()
        PluginConfigs.saveProperties()
    }

    function refreshAll()
    {
        fillSearchPaths()
        refreshCache()
    }

    AkElement {
        id: globalElement
    }
    SystemPalette {
        id: systemPalette
        colorGroup: SystemPalette.Disabled
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
                id: btnRemove
                text: qsTr("Remove")
                iconName: "remove"
                iconSource: "image://icons/remove"
                enabled: searchPathsTable.currentRow >= 0

                onClicked: {
                    var removeIndex = []

                    searchPathsTable.selection.forEach(function (rowIndex) {
                        removeIndex.push(rowIndex)
                    })

                    var searchPaths = globalElement.searchPaths();
                    var sp = []

                    for (var path in searchPaths)
                        if (removeIndex.indexOf(parseInt(path)) < 0)
                            sp.push(searchPaths[path])

                    globalElement.setSearchPaths(sp);
                    refreshAll()
                    searchPathsTable.selection.clear()
                    searchPathsTable.currentRow = -1
                }
            }
            TableView {
                id: searchPathsTable
                headerVisible: false
                selectionMode: SelectionMode.SingleSelection
                Layout.columnSpan: 3
                Layout.fillWidth: true
                model: ListModel {
                    id: searchPathsModel
                }

                TableViewColumn {
                    role: "path"
                    title: qsTr("Search path")
                    elideMode: Text.ElideLeft
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
                id: btnEnableDisable
                text: pluginIsEnabled? qsTr("Disable"): qsTr("Enable")
                enabled: pluginsTable.currentRow >= 0

                property bool pluginIsEnabled: false

                onClicked: {
                    var blackList = globalElement.pluginsBlackList()

                    if (pluginIsEnabled) {
                        pluginsTable.selection.forEach(function (rowIndex) {
                            var path = pluginsTable.model.get(rowIndex).path

                            if (blackList.indexOf(path) < 0)
                                blackList.push(path)
                        })
                    } else {
                        pluginsTable.selection.forEach(function (rowIndex) {
                            var path = pluginsTable.model.get(rowIndex).path
                            var index = blackList.indexOf(path)

                            if (index >= 0)
                                blackList.splice(index, 1)
                        })
                    }

                    globalElement.setPluginsBlackList(blackList)
                    refreshCache()
                    pluginsTable.selection.clear()
                    pluginsTable.currentRow = -1
                }
            }
            TableView {
                id: pluginsTable
                headerVisible: false
                selectionMode: SelectionMode.SingleSelection
                Layout.columnSpan: 3
                Layout.fillWidth: true
                model: ListModel {
                    id: pluginsModel
                }
                itemDelegate: Item {
                    height: Math.max(16, label.implicitHeight)
                    property int implicitWidth: label.implicitWidth + 20

                    Text {
                        id: label
                        enabled: false
                        objectName: "label"
                        width: parent.width
                        anchors.leftMargin: 12
                        anchors.left: parent.left
                        anchors.right: parent.right
                        horizontalAlignment: styleData.textAlignment
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: 1
                        elide: styleData.elideMode
                        text: styleData.value !== undefined? styleData.value: ""
                        color: styleData.row >= 0 && pluginsTable.model.get(styleData.row).enabled?
                                   styleData.textColor: systemPalette.text
                        renderType: Text.NativeRendering
                    }
                }

                onCurrentRowChanged: {
                    btnEnableDisable.pluginIsEnabled =
                        currentRow < 0? false: pluginsTable.model.get(currentRow).enabled
                }

                TableViewColumn {
                    role: "path"
                    title: qsTr("Plugin path")
                    elideMode: Text.ElideLeft
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
            refreshAll()
        }
    }
}

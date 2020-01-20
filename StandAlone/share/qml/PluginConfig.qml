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
import Qt.labs.platform 1.1 as LABS
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Ak 1.0

ColumnLayout {
    id: pluginConfigs

    function fillSearchPaths()
    {
        searchPathsTable.model.clear()
        let searchPaths = AkElement.searchPaths()

        for (let path in searchPaths) {
            searchPathsTable.model.append({
                path: searchPaths[path]
            })
        }
    }

    function fillPluginList()
    {
        pluginsTable.model.clear()
        let pluginsPaths = AkElement.listPluginPaths()

        for (let path in pluginsPaths) {
            pluginsTable.model.append({
                path: pluginsPaths[path],
                pluginEnabled: true
            })
        }

        let blackList = AkElement.pluginsBlackList()

        for (let path in blackList) {
            pluginsTable.model.append({
                path: blackList[path],
                pluginEnabled: false
            })
        }
    }

    function refreshCache()
    {
        AkElement.clearCache()
        fillPluginList()
        PluginConfigs.saveProperties()
    }

    function refreshAll()
    {
        fillSearchPaths()
        refreshCache()
    }

    function colorForIndex(index, pluginEnabled, table) {
        let pal = pluginEnabled? palette: paletteDisabled;

        return index === table.currentIndex?
                           pal.highlight: index & 1?
                               pal.alternateBase: pal.base
    }

    SystemPalette {
        id: palette
    }
    SystemPalette {
        id: paletteDisabled
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
        title: qsTr("Extra search paths")
        Layout.fillWidth: true
        clip: true

        GridLayout {
            columns: 4
            anchors.fill: parent

            Label {
                text: qsTr("Search plugins in subfolders.")
                Layout.fillWidth: true
            }
            Switch {
                checked: AkElement.recursiveSearch()

                onCheckedChanged: {
                    AkElement.setRecursiveSearch(checked)
                    refreshCache()
                }
            }
            Button {
                text: qsTr("Add")
                icon.source: "image://icons/add"

                onClicked: fileDialog.open()
            }
            Button {
                id: btnRemove
                text: qsTr("Remove")
                icon.source: "image://icons/no"
                enabled: searchPathsTable.currentIndex >= 0

                onClicked: {
                    let searchPaths = AkElement.searchPaths();
                    let sp = []

                    for (let path in searchPaths)
                        if (path != searchPathsTable.currentIndex)
                            sp.push(searchPaths[path])

                    AkElement.setSearchPaths(sp);
                    refreshAll()
                    searchPathsTable.currentIndex = -1
                }
            }
            ScrollView {
                id: searchPathsTableScroll
                height: 150
                Layout.columnSpan: 4
                Layout.fillWidth: true
                contentHeight: searchPathsTable.height
                ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                ListView {
                    id: searchPathsTable
                    height: contentHeight
                    width: searchPathsTableScroll.width
                           - (searchPathsTableScroll.ScrollBar.vertical.visible?
                                  searchPathsTableScroll.ScrollBar.vertical.width: 0)
                    clip: true

                    model: ListModel {
                        id: searchPathsModel
                    }
                    delegate: ItemDelegate {
                        text: path
                        highlighted: searchPathsTable.currentItem == this
                        anchors.right: parent.right
                        anchors.left: parent.left

                        onClicked: {
                            searchPathsTable.currentIndex = index
                            searchPathsTable.positionViewAtIndex(index, ListView.Contain)
                        }
                    }
                }
            }
        }
    }
    GroupBox {
        title: qsTr("Plugins list")
        Layout.fillWidth: true
        clip: true

        GridLayout {
            columns: 3
            anchors.fill: parent

            Button {
                text: qsTr("Refresh")
                icon.source: "image://icons/reset"

                onClicked: refreshCache()
            }
            Label {
                Layout.fillWidth: true
            }
            Button {
                id: btnEnableDisable
                text: pluginIsEnabled? qsTr("Disable"): qsTr("Enable")
                enabled: pluginsTable.currentIndex >= 0

                property bool pluginIsEnabled: false

                onClicked: {
                    let blackList = AkElement.pluginsBlackList()
                    let path = pluginsTable.model.get(pluginsTable.currentIndex).path
                    let index = blackList.indexOf(path)

                    if (pluginIsEnabled) {
                        if (index < 0)
                            blackList.push(path)
                    } else {
                        if (index >= 0)
                            blackList.splice(index, 1)
                    }

                    AkElement.setPluginsBlackList(blackList)
                    refreshCache()
                    pluginsTable.currentIndex = -1
                }
            }
            ScrollView {
                id: pluginsTableScroll
                height: 150
                Layout.columnSpan: 3
                Layout.fillWidth: true
                contentHeight: pluginsTable.height
                ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                ListView {
                    id: pluginsTable
                    height: contentHeight
                    width: pluginsTableScroll.width
                           - (pluginsTableScroll.ScrollBar.vertical.visible?
                                  pluginsTableScroll.ScrollBar.vertical.width: 0)
                    clip: true

                    model: ListModel {
                        id: pluginsModel
                    }

                    delegate: ItemDelegate {
                        text: path
                        highlighted: pluginsTable.currentItem == this
                        opacity: pluginEnabled? 1.0: 0.5
                        anchors.right: parent.right
                        anchors.left: parent.left

                        onClicked: {
                            pluginsTable.currentIndex = index
                            pluginsTable.positionViewAtIndex(index, ListView.Contain)
                        }
                    }

                    onCurrentIndexChanged: {
                        btnEnableDisable.pluginIsEnabled =
                            currentIndex < 0? false: pluginsTable.model.get(currentIndex).pluginEnabled
                    }
                }
            }
        }
    }

    LABS.FolderDialog {
        id: fileDialog
        title: qsTr("Add plugins search path")

        onAccepted: {
            let path = Webcamoid.urlToLocalFile(folder)
            AkElement.addSearchPath(path)
            refreshAll()
        }
    }
}

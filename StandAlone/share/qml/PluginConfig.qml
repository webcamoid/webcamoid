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

import QtQuick
import Qt.labs.platform as LABS
import QtQuick.Controls
import QtQuick.Layouts
import Ak

Page {
    ColumnLayout {
        anchors.fill: parent

        TabBar {
            id: tabBar
            Layout.fillWidth: true

            TabButton {
                text: qsTr("Paths")
            }
            TabButton {
                text: qsTr("Plugins")
            }
        }
        StackLayout {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            function fillSearchPaths()
            {
                searchPathsTable.model.clear()
                let searchPaths = AkPluginManager.searchPaths

                for (let path in searchPaths) {
                    searchPathsTable.model.append({
                        path: searchPaths[path]
                    })
                }
            }

            function fillPluginList()
            {
                pluginsTable.model.clear()
                let plugins = AkPluginManager.listPlugins()
                plugins.sort(function(a, b) {
                    if (a < b)
                        return -1
                    else if (a > b)
                        return 1

                    return 0
                })

                for (let plugin in plugins) {
                    pluginsTable.model.append({
                        pluginId: plugins[plugin],
                        pluginEnabled: AkPluginManager.pluginStatus(plugins[plugin]) == AkPluginManager.Enabled
                    })
                }
            }

            function refreshCache()
            {
                AkPluginManager.setCachedPlugins([])
                AkPluginManager.scanPlugins()
                fillPluginList()
                pluginConfigs.saveProperties()
            }

            function refreshAll()
            {
                fillSearchPaths()
                refreshCache()
            }

            Component.onCompleted: {
                fillSearchPaths()
                fillPluginList()
            }

            // Paths tab
            ScrollView {
                id: pathsScrollView
                contentHeight: pathsConfigs.height
                clip: true

                ColumnLayout {
                    id: pathsConfigs
                    width: pathsScrollView.width

                    Switch {
                        text: qsTr("Search plugins in subfolders")
                        checked: AkPluginManager.recursiveSearch

                        onCheckedChanged: {
                            AkPluginManager.recursiveSearch = checked
                            stack.refreshCache()
                        }
                    }
                    Button {
                        text: qsTr("Add path")
                        icon.source: "image://icons/add"
                        flat: true

                        onClicked: fileDialog.open()
                    }
                    ListView {
                        id: searchPathsTable
                        Layout.fillWidth: true
                        implicitWidth: childrenRect.width
                        implicitHeight: childrenRect.height
                        clip: true

                        model: ListModel {
                            id: searchPathsModel
                        }
                        delegate: SwipeDelegate {
                            id: swipeDelegate
                            text: path
                            anchors.right: parent.right
                            anchors.left: parent.left

                            ListView.onRemove: SequentialAnimation {
                                PropertyAction {
                                    target: swipeDelegate
                                    property: "ListView.delayRemove"
                                    value: true
                                }
                                NumberAnimation {
                                    target: swipeDelegate
                                    property: "height"
                                    to: 0
                                    easing.type: Easing.InOutQuad
                                }
                                PropertyAction {
                                    target: swipeDelegate
                                    property: "ListView.delayRemove"
                                    value: false
                                }
                            }

                            swipe.right: Button {
                                id: deleteLabel
                                text: qsTr("Remove")
                                flat: true
                                height: parent.height
                                anchors.right: parent.right

                                onClicked: {
                                    let searchPaths = AkPluginManager.searchPaths();
                                    let sp = []

                                    for (let path in searchPaths)
                                        if (path != index)
                                            sp.push(searchPaths[path])

                                    AkPluginManager.setSearchPaths(sp)
                                    searchPathsModel.remove(index)
                                    stack.refreshCache()
                                }
                            }
                        }
                    }
                }
            }

            // Plugins tabs
            ScrollView {
                id: pluginsScrollView
                contentHeight: pluginConfigsLayout.height
                clip: true

                ColumnLayout {
                    id: pluginConfigsLayout
                    width: pluginsScrollView.width

                    Button {
                        text: qsTr("Update")
                        icon.source: "image://icons/reset"
                        flat: true

                        onClicked: stack.refreshCache()
                    }
                    ListView {
                        id: pluginsTable
                        Layout.fillWidth: true
                        implicitWidth: childrenRect.width
                        implicitHeight: childrenRect.height
                        clip: true

                        model: ListModel {
                            id: pluginsModel
                        }

                        delegate: CheckDelegate {
                            text: pluginId
                            width: pluginsScrollView.width
                            checked: pluginEnabled

                            onToggled: {
                                let disabledPlugins =
                                    AkPluginManager.listPlugins("",
                                                                [],
                                                                AkPluginManager.FilterDisabled)

                                if (checked) {
                                    let index = disabledPlugins.indexOf(pluginId)

                                    if (index >= 0)
                                        disabledPlugins.splice(index, 1)
                                } else {
                                    disabledPlugins.push(pluginId)
                                }

                                AkPluginManager.setPluginStatus(disabledPlugins,
                                                                   AkPluginManager.Disabled)
                                pluginConfigs.saveProperties()
                            }
                        }
                    }
                }
            }
        }
    }

    LABS.FolderDialog {
        id: fileDialog
        title: qsTr("Add plugins search path")

        onAccepted: {
            let path = mediaTools.urlToLocalFile(folder)
            AkPluginManager.addSearchPath(path)
            stack.refreshAll()
        }
    }
}

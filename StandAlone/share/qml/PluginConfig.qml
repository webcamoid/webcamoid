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
                let blackList = AkElement.pluginsBlackList()
                let pluginsPaths = AkElement.listPluginPaths()
                pluginsPaths = pluginsPaths.concat(blackList)
                pluginsPaths.sort(function(a, b) {
                    a = AkElement.pluginIdFromPath(a)
                    b = AkElement.pluginIdFromPath(b)

                    if (a < b)
                        return -1
                    else if (a > b)
                        return 1

                    return 0
                })

                for (let path in pluginsPaths) {
                    pluginsTable.model.append({
                        path: pluginsPaths[path],
                        pluginId: AkElement.pluginIdFromPath(pluginsPaths[path]),
                        pluginEnabled: !blackList.includes(pluginsPaths[path])
                    })
                }
            }

            function refreshCache()
            {
                AkElement.clearCache()
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
                        checked: AkElement.recursiveSearch()

                        onCheckedChanged: {
                            AkElement.setRecursiveSearch(checked)
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
                                    let searchPaths = AkElement.searchPaths();
                                    let sp = []

                                    for (let path in searchPaths)
                                        if (path != index)
                                            sp.push(searchPaths[path])

                                    AkElement.setSearchPaths(sp)
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
                            anchors.right: parent.right
                            anchors.left: parent.left
                            checked: pluginEnabled

                            onToggled: {
                                let blackList = AkElement.pluginsBlackList()

                                if (checked) {
                                    let index = blackList.indexOf(path)

                                    if (index >= 0)
                                        blackList.splice(index, 1)
                                } else {
                                    blackList.push(path)
                                }

                                AkElement.setPluginsBlackList(blackList)
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
            AkElement.addSearchPath(path)
            stack.refreshAll()
        }
    }
}

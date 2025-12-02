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
import Qt.labs.platform as LABS
import QtQuick.Controls
import QtQuick.Layouts
import Ak
import AkControls as AK

AK.MenuOption {
    id: root
    title: qsTr("Plugins")
    subtitle: qsTr("Enable and disable %1 plugins").arg(mediaTools.applicationName)
    icon: "image://icons/plugin"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

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
                searchPathsTable.minHeight = 0

                for (let i = searchPathsTable.count - 1; i >= 0; i--)
                    searchPathsTable.removeItem(searchPathsTable.itemAt(i))

                let searchPaths = AkPluginManager.searchPaths

                for (let path in searchPaths) {
                    let component = Qt.createComponent("PluginsPathItem.qml")

                    if (component.status !== Component.Ready)
                        continue

                    let obj = component.createObject(searchPathsTable)
                    obj.text = searchPaths[path]
                    searchPathsTable.minHeight += obj.height
                    obj.onPathRemoved.connect(function (item) {
                        let index = -1

                        for (let i in searchPathsTable.contentChildren)
                            if (searchPathsTable.contentChildren[i] == item) {
                                index = i

                                break
                            }

                        let searchPaths = AkPluginManager.searchPaths
                        let sp = []

                        for (let i in searchPaths)
                            if (i != index)
                                sp.push(searchPaths[i])

                        AkPluginManager.setSearchPaths(sp)
                        searchPathsTable.removeItem(item)
                        stack.refreshCache()
                    })
                }
            }

            function fillPluginList()
            {
                pluginsTable.minHeight = 0

                for (let i = pluginsTable.count - 1; i >= 0; i--)
                    pluginsTable.removeItem(pluginsTable.itemAt(i))

                let plugins = AkPluginManager.listPlugins()
                plugins.sort(function(a, b) {
                    if (a < b)
                        return -1
                    else if (a > b)
                        return 1

                    return 0
                })

                for (let plugin in plugins) {
                    let component = Qt.createComponent("PluginItem.qml")

                    if (component.status !== Component.Ready)
                        continue

                    let obj = component.createObject(pluginsTable)
                    obj.text = plugins[plugin]
                    obj.pluginId = plugins[plugin]
                    obj.checked = AkPluginManager.pluginStatus(plugins[plugin]) == AkPluginManager.Enabled
                    pluginsTable.minHeight += obj.height

                    obj.onToggled.connect((item => function () {
                        let enabledPlugins =
                            AkPluginManager.listPlugins("",
                                                        [],
                                                        AkPluginManager.FilterEnabled)
                        let disabledPlugins =
                            AkPluginManager.listPlugins("",
                                                        [],
                                                        AkPluginManager.FilterDisabled)

                        let enabledIndex = enabledPlugins.indexOf(item.pluginId)
                        let disabledIndex = disabledPlugins.indexOf(item.pluginId)

                        if (item.checked) {
                            if (enabledIndex < 0)
                                enabledPlugins.push(item.pluginId)

                            if (disabledIndex >= 0)
                                disabledPlugins.splice(disabledIndex, 1)
                        } else {
                            if (enabledIndex >= 0)
                                enabledPlugins.splice(enabledIndex, 1)

                            if (disabledIndex < 0)
                                disabledPlugins.push(item.pluginId)
                        }

                        AkPluginManager.setPluginsStatus(enabledPlugins,
                                                         AkPluginManager.Enabled)
                        AkPluginManager.setPluginsStatus(disabledPlugins,
                                                         AkPluginManager.Disabled)

                        pluginConfigs.saveProperties()
                    })(obj))
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
                    layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
                    width: pathsScrollView.width

                    Switch {
                        text: qsTr("Search plugins in subfolders")
                        checked: AkPluginManager.recursiveSearch
                        Layout.leftMargin: root.leftMargin
                        Layout.rightMargin: root.rightMargin
                        Layout.fillWidth: true

                        onCheckedChanged: {
                            AkPluginManager.recursiveSearch = checked
                            stack.refreshCache()
                        }
                    }
                    Button {
                        text: qsTr("Add path")
                        icon.source: "image://icons/add"
                        flat: true
                        Layout.leftMargin: root.leftMargin
                        Layout.rightMargin: root.rightMargin

                        onClicked: fileDialog.open()
                    }
                    OptionList {
                        id: searchPathsTable
                        enableHighlight: false
                        Layout.fillWidth: true
                        Layout.minimumHeight: minHeight
                        clip: true

                        property int minHeight: 0

                        onActiveFocusChanged:
                            if (activeFocus && count > 0)
                                itemAt(currentIndex).forceActiveFocus()
                        Keys.onUpPressed:
                            itemAt(currentIndex).forceActiveFocus()
                        Keys.onDownPressed:
                            itemAt(currentIndex).forceActiveFocus()
                        Keys.onLeftPressed:
                            itemAt(currentIndex).swipe.open(SwipeDelegate.Right)
                        Keys.onRightPressed:
                            itemAt(currentIndex).swipe.close()
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
                        Layout.leftMargin: root.leftMargin
                        Layout.rightMargin: root.rightMargin
                        Accessible.description: qsTr("Update plugins list")
                        icon.source: "image://icons/reset"
                        flat: true

                        onClicked: stack.refreshCache()
                    }
                    OptionList {
                        id: pluginsTable
                        enableHighlight: false
                        Layout.fillWidth: true
                        Layout.minimumHeight: minHeight
                        Layout.leftMargin: root.leftMargin
                        Layout.rightMargin: root.rightMargin
                        clip: true

                        property int minHeight: 0

                        onActiveFocusChanged:
                            if (activeFocus && count > 0)
                                itemAt(currentIndex).forceActiveFocus()
                        Keys.onUpPressed:
                            itemAt(currentIndex).forceActiveFocus()
                        Keys.onDownPressed:
                            itemAt(currentIndex).forceActiveFocus()
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
}

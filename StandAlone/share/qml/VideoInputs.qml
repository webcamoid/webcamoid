/* Webcamoid, camera capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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
import QtQml.Models
import Ak
import Webcamoid

ScrollView {
    id: view

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    signal openVideoInputAddCameraDialog()
    signal openVideoInputAddScreenDialog()
    signal openVideoInputAddWindowDialog()
    signal openVideoInputAddFileDialog()
    signal openVideoInputAddUrlDialog()
    signal openVideoInputOptions(int sourceId)
    signal openCanvasVideoEffectsDialog()
    signal enterEditLayoutMode()

    // Internal properties
    property var sourcesModel: ListModel {}
    property bool isDragging: false

    readonly property int itemHeight: AkUnit.create(56 * AkTheme.controlScale, "dp").pixels
    readonly property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    readonly property color activeWindowText: AkTheme.palette.active.windowText
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeHighlightedText: AkTheme.palette.active.highlightedText

    function buildSourcesModel() {
        sourcesModel.clear()
        let ids = videoLayer.sourceIds()
        let sourceItems = []

        for (let i = 0; i < ids.length; i++) {
            let id = ids[i]
            let type = videoLayer.deviceType(id)
            let label = videoLayer.sourceLabel(id)
            let enabled = videoLayer.sourceEnabled(id)
            let zOrder = videoLayer.sourceZOrder(id)

            sourceItems.push({
                id: id,
                type: type,
                label: label,
                enabled: enabled,
                zOrder: zOrder
            })
        }

        sourceItems.sort(function(a, b) { return b.zOrder - a.zOrder })

        for (let i = 0; i < sourceItems.length; i++)
            sourcesModel.append(sourceItems[i])
    }

    function sourceTypeIcon(type) {
        switch (type) {
            case VideoLayer.InputCamera:
                return "webcam"
            case VideoLayer.InputScreen:
                return "screen"
            case VideoLayer.InputImage:
                return "picture"
            default:
                return "video"
        }
    }

    function sourceTypeName(type) {
        switch (type) {
            case VideoLayer.InputCamera:
                return qsTr("Camera")
            case VideoLayer.InputScreen:
                return qsTr("Screen")
            case VideoLayer.InputImage:
                return qsTr("Image")
            case VideoLayer.InputStream:
                return qsTr("Video")
            default:
                return qsTr("Unknown")
        }
    }

    Component.onCompleted: buildSourcesModel()

    Connections {
        target: videoLayer
        function onSourceAdded(id, device) { view.buildSourcesModel() }
        function onSourceRemoved(id) { view.buildSourcesModel() }
        function onSourceLabelChanged(id, label) { view.buildSourcesModel() }
    }

    ColumnLayout {
        layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight
        width: view.width
        clip: true

        Button {
            text: qsTr("Add source")
            icon.source: "image://icons/add"
            flat: true

            onClicked: addSourceMenu.popup()

            Menu {
                id: addSourceMenu
                width: AkUnit.create(250 * AkTheme.controlScale, "dp").pixels
                margins: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

                MenuItem {
                    text: qsTr("Add camera")
                    icon.source: "image://icons/webcam"
                    enabled: videoLayer.cameras.length > 0

                    onClicked: view.openVideoInputAddCameraDialog()
                }
                MenuItem {
                    text: videoLayer.canCaptureWindows?
                            qsTr("Add screen"):
                            qsTr("Add screen source")
                    icon.source: "image://icons/screen"
                    enabled: videoLayer.screens.length > 0

                    onClicked: view.openVideoInputAddScreenDialog()
                }
                MenuItem {
                    text: qsTr("Add window")
                    icon.source: "image://icons/window"
                    height: videoLayer.canCaptureWindows? undefined: 0
                    visible: videoLayer.canCaptureWindows
                    enabled: videoLayer.windows.length > 0

                    onClicked: view.openVideoInputAddWindowDialog()
                }
                MenuItem {
                    text: qsTr("Add media file")
                    icon.source: "image://icons/file"

                    onClicked: view.openVideoInputAddFileDialog()
                }
                MenuItem {
                    text: qsTr("Add media URL")
                    icon.source: "image://icons/link"

                    onClicked: view.openVideoInputAddUrlDialog()
                }
            }
        }
        Button {
            text: qsTr("Edit layout")
            icon.source: "image://icons/edit"
            flat: true
            visible: sourcesList.count > 0

            onClicked: view.enterEditLayoutMode()
        }
        Button {
            text: qsTr("Manage canvas effects")
            icon.source: "image://icons/video-effects"
            flat: true

            onClicked: view.openCanvasVideoEffectsDialog()
        }

        ListView {
            id: sourcesList
            Layout.fillWidth: true
            Layout.minimumHeight: count > 0 ? count * (view.itemHeight + spacing) : 0
            clip: true
            spacing: AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
            cacheBuffer: view.itemHeight * 4
            interactive: !view.isDragging

            // Smooth displacement animation
            displaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 250; easing.type: Easing.OutQuad }
            }

            // Using DelegateModel to wrap the ListModel
            model: DelegateModel {
                id: visualModel
                model: view.sourcesModel

                delegate: Item {
                    id: delegateRoot
                    width: sourcesList.width
                    height: view.itemHeight

                    // Static drop area that stays in place in the list
                    DropArea {
                        anchors.fill: parent
                        keys: ["source"]

                        onEntered: drag => {
                            let fromIndex = drag.source.DelegateModel.itemsIndex
                            let toIndex = delegateRoot.DelegateModel.itemsIndex

                            if (fromIndex !== undefined && toIndex !== undefined && fromIndex !== toIndex) {
                                visualModel.items.move(fromIndex, toIndex)
                            }
                        }
                    }

                    // Visual item container that will physically move
                    Rectangle {
                        id: sourceItem
                        width: parent.width
                        height: parent.height
                        color: dragMouseArea.drag.active || sourceItemMouseArea.pressed ?
                                    AkTheme.shade(view.activeHighlight, 0.2) :
                               rowHover.hovered ?
                                    AkTheme.shade(view.activeWindow, -0.1) :
                                    Qt.rgba(0, 0, 0, 0)
                        z: dragMouseArea.drag.active? 10: 1

                        // Attached Drag properties
                        Drag.active: dragMouseArea.drag.active
                        Drag.source: delegateRoot
                        Drag.keys: ["source"]
                        Drag.hotSpot.x: width / 2
                        Drag.hotSpot.y: height / 2

                        HoverHandler {
                            id: rowHover
                        }

                        // Clean state transition to detach coordinates during drag
                        states: [
                            State {
                                when: dragMouseArea.drag.active

                                PropertyChanges {
                                    target: sourceItem
                                    x: sourceItem.x
                                    y: sourceItem.y
                                }
                            }
                        ]

                        // Drag handle
                        Item {
                            id: dragHandle
                            width: parent.height / 2
                            anchors.left: view.rtl? undefined: parent.left
                            anchors.right: view.rtl? parent.right: undefined
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom

                            AkColorizedImage {
                                width: 0.8 * Math.min(parent.width, parent.height)
                                height: width
                                source: "image://icons/drag"
                                sourceSize: Qt.size(width, height)
                                color: dragMouseArea.drag.active?
                                            view.activeHighlightedText:
                                            view.activeWindowText
                                asynchronous: true
                                mipmap: true
                                fillMode: Image.PreserveAspectFit
                                anchors.centerIn: parent
                            }

                            MouseArea {
                                id: dragMouseArea
                                anchors.fill: parent
                                cursorShape: Qt.OpenHandCursor
                                drag.target: sourceItem
                                drag.axis: Drag.YAxis

                                onPressed: view.isDragging = true

                                onReleased: {
                                    view.isDragging = false
                                    let n = visualModel.items.count
                                    let currentIndex = delegateRoot.DelegateModel.itemsIndex
                                    let item = visualModel.items.get(currentIndex)
                                    let sourceId = item.model.id
                                    videoLayer.setSourceZOrder(sourceId, n - 1 - currentIndex)
                                }
                            }
                        }

                        // Visibility toggle
                        Item {
                            id: btnVisibility
                            width: parent.height / 2
                            anchors.left: view.rtl? undefined: dragHandle.right
                            anchors.right: view.rtl? dragHandle.left: undefined
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom

                            property bool checked: model.enabled

                            AkColorizedImage {
                                width: 0.8 * Math.min(parent.width, parent.height)
                                height: width
                                source: btnVisibility.checked?
                                            "image://icons/open-eye":
                                            "image://icons/closed-eye"
                                sourceSize: Qt.size(width, height)
                                color: dragMouseArea.drag.active?
                                            view.activeHighlightedText:
                                            view.activeWindowText
                                asynchronous: true
                                mipmap: true
                                fillMode: Image.PreserveAspectFit
                                anchors.centerIn: parent
                            }

                            MouseArea {
                                id: visivilityMouseArea
                                anchors.fill: parent

                                onClicked: {
                                    btnVisibility.checked = !btnVisibility.checked
                                    videoLayer.setSourceEnabled(model.id, btnVisibility.checked)
                                }
                            }
                        }

                        Item {
                            id: typeIcon
                            width: parent.height / 2
                            anchors.left: view.rtl? undefined: btnVisibility.right
                            anchors.right: view.rtl? btnVisibility.left: undefined
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom

                            AkColorizedImage {
                                width: 0.8 * Math.min(parent.width, parent.height)
                                height: width
                                source: "image://icons/" + view.sourceTypeIcon(model.type)
                                sourceSize: Qt.size(width, height)
                                color: dragMouseArea.drag.active?
                                            view.activeHighlightedText:
                                            view.activeWindowText
                                asynchronous: true
                                mipmap: true
                                fillMode: Image.PreserveAspectFit
                                anchors.centerIn: parent
                            }
                        }

                        // Source label + type
                        ColumnLayout {
                            anchors.left: view.rtl? undefined: typeIcon.right
                            anchors.right: view.rtl? typeIcon.left: undefined
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                            anchors.rightMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                            spacing: 0

                            Label {
                                text: model.label || qsTr("Source %1").arg(model.id)
                                elide: Text.ElideRight
                                color: dragMouseArea.drag.active?
                                            view.activeHighlightedText:
                                            view.activeWindowText
                                Layout.fillWidth: true
                            }

                            Label {
                                text: view.sourceTypeName(model.type)
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                font: AkTheme.fontSettings.subtitle2
                                color: dragMouseArea.drag.active?
                                            view.activeHighlightedText:
                                            view.activeWindowText
                                opacity: 0.5
                            }
                        }

                        // Click to open options
                        MouseArea {
                            id: sourceItemMouseArea
                            anchors.left: view.rtl? parent.left: typeIcon.right
                            anchors.right: view.rtl? typeIcon.left: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            acceptedButtons: Qt.LeftButton

                            onClicked: view.openVideoInputOptions(model.id)
                        }
                    }
                }
            }
        }
    }
}

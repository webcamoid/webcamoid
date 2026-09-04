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

ScrollView {
    id: effectsView

    property int sourceId: -1

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft
    readonly property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property bool isSource: sourceId >= 0

    // Internal properties
    property var effectsModel: ListModel {}
    property bool isDragging: false

    readonly property int itemHeight: AkUnit.create(56 * AkTheme.controlScale, "dp").pixels

    readonly property color activeWindowText: AkTheme.palette.active.windowText
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeHighlightedText: AkTheme.palette.active.highlightedText

    signal openVideoEffectsDialog(int sourceId)
    signal openVideoEffectOptions(int sourceId, int effectIndex)

    function activeEffects() {
        if (effectsView.isSource)
            return videoEffects.sourceEffects(effectsView.sourceId)

        return videoEffects.effects
    }

    function effectEnabledAt(effectIndex) {
        if (effectsView.isSource)
            return videoEffects.sourceEffectEnabled(effectsView.sourceId, effectIndex)

        return videoEffects.effectEnabled(effectIndex)
    }

    function setEffectEnabledAt(effectIndex, enabled) {
        if (effectsView.isSource)
            videoEffects.setSourceEffectEnabled(effectsView.sourceId, effectIndex, enabled)
        else
            videoEffects.setEffectEnabled(effectIndex, enabled)
    }

    function moveEffectAt(from, to) {
        if (effectsView.isSource)
            videoEffects.moveSourceEffect(effectsView.sourceId, from, to)
        else
            videoEffects.moveEffect(from, to)
    }

    function buildEffectsModel() {
        effectsModel.clear()
        let ids = effectsView.activeEffects()

        for (let i = 0; i < ids.length; i++) {
            let info = AkPluginInfo.create(videoEffects.effectInfo(ids[i]))

            effectsModel.append({
                effectIndex: i,
                label: info.description,
                enabled: effectsView.effectEnabledAt(i)
            })
        }
    }

    Component.onCompleted: buildEffectsModel()
    onVisibleChanged: {
        if (visible)
            buildEffectsModel()

        effectsList.forceActiveFocus()
    }
    onSourceIdChanged: buildEffectsModel()

    Connections {
        target: videoEffects

        function onEffectsChanged()
        {
            if (!effectsView.isSource)
                effectsView.buildEffectsModel()
        }

        function onSourceEffectsChanged(id)
        {
            if (effectsView.isSource && id === effectsView.sourceId)
                effectsView.buildEffectsModel()
        }

        function onEffectEnabledChanged(index, enabled)
        {
            if (!effectsView.isSource)
                effectsView.buildEffectsModel()
        }

        function onSourceEffectEnabledChanged(id, index, enabled)
        {
            if (effectsView.isSource && id === effectsView.sourceId)
                effectsView.buildEffectsModel()
        }
    }

    ColumnLayout {
        layoutDirection: effectsView.rtl? Qt.RightToLeft: Qt.LeftToRight
        width: effectsView.width
        clip: true

        Label {
            id: deviceInfo
            text: {
                if (!effectsView.isSource)
                    return ""

                return "<b>" + videoLayer.sourceLabel(effectsView.sourceId) + "</b>"
                       + "<br/><i>" + videoLayer.sourceDevice(effectsView.sourceId) + "</i>"
            }
            elide: Label.ElideRight
            height: effectsView.isSource? undefined: 0
            visible: effectsView.isSource
            Layout.fillWidth: true
            Layout.leftMargin: effectsView.leftMargin
            Layout.rightMargin: effectsView.rightMargin
            Layout.bottomMargin:
                AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        }

        Button {
            text: qsTr("Add effect")
            icon.source: "image://icons/add"
            flat: true

            onClicked: effectsView.openVideoEffectsDialog(effectsView.sourceId)
        }
        Button {
            text: qsTr("Remove all effects")
            icon.source: "image://icons/no"
            flat: true
            visible: effectsList.count > 0

            onClicked: {
                if (effectsView.isSource)
                    videoEffects.removeAllSourceEffects(effectsView.sourceId)
                else
                    videoEffects.removeAllEffects()
            }
        }

        ListView {
            id: effectsList
            Layout.fillWidth: true
            Layout.minimumHeight: count > 0 ? count * (effectsView.itemHeight + spacing) : 0
            clip: true
            spacing: AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
            cacheBuffer: effectsView.itemHeight * 4
            interactive: !effectsView.isDragging

            // Smooth displacement animation
            displaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 250; easing.type: Easing.OutQuad }
            }

            // Using DelegateModel to wrap the ListModel
            model: DelegateModel {
                id: visualModel
                model: effectsView.effectsModel

                delegate: Item {
                    id: delegateRoot
                    width: effectsList.width
                    height: effectsView.itemHeight

                    property int dragStartIndex: -1

                    // Static drop area that stays in place in the list
                    DropArea {
                        anchors.fill: parent
                        keys: ["effect"]

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
                        id: effectItem
                        width: parent.width
                        height: parent.height
                        color: dragMouseArea.drag.active || effectItemMouseArea.pressed ?
                                    AkTheme.shade(effectsView.activeHighlight, 0.2) :
                               rowHover.hovered ?
                                    AkTheme.shade(effectsView.activeWindow, -0.1) :
                                    Qt.rgba(0, 0, 0, 0)
                        z: dragMouseArea.drag.active? 10: 1

                        // Attached Drag properties
                        Drag.active: dragMouseArea.drag.active
                        Drag.source: delegateRoot
                        Drag.keys: ["effect"]
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
                                    target: effectItem
                                    x: effectItem.x
                                    y: effectItem.y
                                }
                            }
                        ]

                        // Drag handle
                        Item {
                            id: dragHandle
                            width: parent.height / 2
                            anchors.left: effectsView.rtl? undefined: parent.left
                            anchors.right: effectsView.rtl? parent.right: undefined
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom

                            AkColorizedImage {
                                width: 0.8 * Math.min(parent.width, parent.height)
                                height: width
                                source: "image://icons/drag"
                                sourceSize: Qt.size(width, height)
                                color: dragMouseArea.drag.active?
                                            effectsView.activeHighlightedText:
                                            effectsView.activeWindowText
                                asynchronous: true
                                mipmap: true
                                fillMode: Image.PreserveAspectFit
                                anchors.centerIn: parent
                            }

                            MouseArea {
                                id: dragMouseArea
                                anchors.fill: parent
                                cursorShape: Qt.OpenHandCursor
                                drag.target: effectItem
                                drag.axis: Drag.YAxis

                                onPressed: {
                                    effectsView.isDragging = true
                                    delegateRoot.dragStartIndex =
                                            delegateRoot.DelegateModel.itemsIndex
                                }

                                onReleased: {
                                    effectsView.isDragging = false
                                    let finalIndex = delegateRoot.DelegateModel.itemsIndex

                                    if (delegateRoot.dragStartIndex >= 0
                                        && finalIndex !== delegateRoot.dragStartIndex) {
                                        effectsView.moveEffectAt(delegateRoot.dragStartIndex,
                                                                 finalIndex)
                                    }

                                    delegateRoot.dragStartIndex = -1
                                }
                            }
                        }

                        // Visibility toggle (enable/disable the effect)
                        Item {
                            id: btnVisibility
                            width: parent.height / 2
                            anchors.left: effectsView.rtl? undefined: dragHandle.right
                            anchors.right: effectsView.rtl? dragHandle.left: undefined
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
                                            effectsView.activeHighlightedText:
                                            effectsView.activeWindowText
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
                                    effectsView.setEffectEnabledAt(model.effectIndex,
                                                                   btnVisibility.checked)
                                }
                            }
                        }

                        // Effect label
                        ColumnLayout {
                            anchors.left: effectsView.rtl? undefined: btnVisibility.right
                            anchors.right: effectsView.rtl? btnVisibility.left: undefined
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                            anchors.rightMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                            spacing: 0

                            Label {
                                text: model.label
                                elide: Text.ElideRight
                                color: dragMouseArea.drag.active?
                                            effectsView.activeHighlightedText:
                                            effectsView.activeWindowText
                                Layout.fillWidth: true
                            }
                        }

                        // Click to open options
                        MouseArea {
                            id: effectItemMouseArea
                            anchors.left: effectsView.rtl? parent.left: btnVisibility.right
                            anchors.right: effectsView.rtl? btnVisibility.left: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            acceptedButtons: Qt.LeftButton

                            onClicked: effectsView.openVideoEffectOptions(effectsView.sourceId,
                                                                          model.effectIndex)
                        }
                    }
                }
            }
        }
    }
}

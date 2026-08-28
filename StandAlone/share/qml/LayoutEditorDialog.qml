/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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
import Ak
import AkControls as AK
import Webcamoid

Dialog {
    id: layoutEditor
    anchors.centerIn: Overlay.overlay
    width: Overlay.overlay? Overlay.overlay.width: 0
    height: Overlay.overlay? Overlay.overlay.height: 0
    topPadding: 0
    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0
    modal: true
    closePolicy: Popup.CloseOnEscape

    readonly property real snapPositionThreshold: 0.03
    readonly property real snapAngleThreshold: 8.0
    readonly property var snapAngles: [0, 45, 90, 135, 180, 225, 270, 315]

    property int selectedSourceId: -1
    property var sourcesModel: ListModel {}

    readonly property rect canvasRect: {
        let caps = AkVideoCaps.create(videoEffects.outputCaps)
        let vW = caps ? caps.width : 16
        let vH = caps ? caps.height : 9

        if (vW <= 0) vW = 16

        if (vH <= 0)
            vH = 9

        let aW = canvasArea.width
        let aH = canvasArea.height

        if (aW <= 0)
            aW = 1

        if (aH <= 0)
            aH = 1

        let cW, cH, cX, cY

        if (aW / aH > vW / vH) {
            cH = aH
            cW = aH * (vW / vH)
            cX = (aW - cW) / 2
            cY = 0
        } else {
            cW = aW
            cH = aW * (vH / vW)
            cX = 0
            cY = (aH - cH) / 2
        }

        return Qt.rect(cX, cY, cW, cH)
    }

    function buildSourcesModel() {
        sourcesModel.clear()
        let ids = videoLayer.sourceIds()

        for (let i = 0; i < ids.length; i++) {
            let id = ids[i]

            if (!videoLayer.sourceEnabled(id))
                continue

            sourcesModel.append({
                sourceId: id,
                label: videoLayer.sourceLabel(id) || qsTr("Source %1").arg(id)
            })
        }

        if (sourcesModel.count > 0) {
            let stillPresent = false

            for (let i = 0; i < sourcesModel.count; i++)
                if (sourcesModel.get(i).sourceId === layoutEditor.selectedSourceId) {
                    stillPresent = true
                    break
                }

            if (!stillPresent)
                layoutEditor.selectedSourceId = sourcesModel.get(0).sourceId
        } else {
            layoutEditor.selectedSourceId = -1
        }
    }

    function snapRect(id, rect) {
        let x = rect.x
        let y = rect.y
        let w = rect.width
        let h = rect.height
        let th = layoutEditor.snapPositionThreshold
        let hAnchors = [0, 0.25, 0.5, 0.75, 1.0]
        let vAnchors = [0, 0.25, 0.5, 0.75, 1.0]
        let ids = videoLayer.sourceIds()

        for (let i = 0; i < ids.length; i++) {
            let peerId = ids[i]

            if (peerId === id || !videoLayer.sourceEnabled(peerId))
                continue

            let pr = videoLayer.sourceRect(peerId)
            hAnchors.push(pr.x, pr.x + pr.width / 2, pr.x + pr.width)
            vAnchors.push(pr.y, pr.y + pr.height / 2, pr.y + pr.height)
        }

        let srcHPoints = [x, x + w / 2, x + w]
        let srcVPoints = [y, y + h / 2, y + h]
        let bestDx = th
        let snapX = x

        for (let si = 0; si < srcHPoints.length; si++) {
            for (let ai = 0; ai < hAnchors.length; ai++) {
                let d = Math.abs(srcHPoints[si] - hAnchors[ai])

                if (d < bestDx) {
                    bestDx = d
                    snapX = x + (hAnchors[ai] - srcHPoints[si])
                }
            }
        }

        let bestDy = th
        let snapY = y

        for (let si = 0; si < srcVPoints.length; si++)
            for (let ai = 0; ai < vAnchors.length; ai++) {
                let d = Math.abs(srcVPoints[si] - vAnchors[ai])

                if (d < bestDy) {
                    bestDy = d
                    snapY = y + (vAnchors[ai] - srcVPoints[si])
                }
            }

        return Qt.rect(snapX, snapY, w, h)
    }

    function snapAngle(angle) {
        let normalized = ((angle % 360) + 360) % 360
        let best = normalized
        let bestDist = layoutEditor.snapAngleThreshold

        for (let i = 0; i < layoutEditor.snapAngles.length; i++) {
            let candidate = layoutEditor.snapAngles[i]
            let d = Math.abs(normalized - candidate)

            if (d > 180)
                d = 360 - d

            if (d < bestDist) {
                bestDist = d
                best = candidate
            }
        }

        return best
    }

    Connections {
        target: videoLayer

        function onSourceAdded(id, device) {
            layoutEditor.buildSourcesModel()
        }

        function onSourceRemoved(id) {
            layoutEditor.buildSourcesModel()
        }

        function onSourceEnabledChanged(id, enabled) {
            layoutEditor.buildSourcesModel()
        }

        function onSourceLabelChanged(id, label) {
            layoutEditor.buildSourcesModel()
        }
    }

    onOpened: {
        videoEffects.linkLayoutEditor()
        buildSourcesModel()

        if (sourcesModel.count > 0 && selectedSourceId < 0)
            selectedSourceId = sourcesModel.get(0).sourceId
    }

    onClosed: {
        videoEffects.unlinkLayoutEditor()
    }

    contentItem: ColumnLayout {
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            z: 100

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                anchors.rightMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels

                AK.LabeledComboBox {
                    id: sourceCombo
                    label: qsTr("Source")
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    Accessible.name: label
                    model: layoutEditor.sourcesModel
                    textRole: "label"

                    onCurrentIndexChanged: {
                        if (currentIndex >= 0
                            && currentIndex < layoutEditor.sourcesModel.count) {
                            layoutEditor.selectedSourceId =
                                layoutEditor.sourcesModel.get(currentIndex).sourceId
                        }
                    }

                    Connections {
                        target: layoutEditor

                        function onSelectedSourceIdChanged() {
                            for (let i = 0; i < layoutEditor.sourcesModel.count; i++)
                                if (layoutEditor.sourcesModel.get(i).sourceId === layoutEditor.selectedSourceId) {
                                    sourceCombo.currentIndex = i

                                    return
                                }
                        }
                    }
                }

                ToolButton {
                    text: qsTr("Close")
                    icon.source: "image://icons/no"
                    display: AbstractButton.IconOnly
                    ToolTip.visible: hovered
                    ToolTip.text: text
                    onClicked: layoutEditor.close()
                }
            }
        }

        Item {
            id: canvasArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            VideoDisplay {
                id: editorVideoDisplay
                objectName: "editorVideoDisplay"
                visible: videoLayer.state == AkElement.ElementStatePlaying
                anchors.fill: parent
                smooth: true
            }

            Repeater {
                id: overlayRepeater
                model: layoutEditor.sourcesModel

                delegate: Item {
                    id: delegateRoot
                    required property int index
                    required property var model

                    readonly property int sourceId: model.sourceId
                    property rect normRect: videoLayer.sourceRect(sourceId)
                    property real normRotation: videoLayer.sourceRotation(sourceId)

                    readonly property real pxX: layoutEditor.canvasRect.x + normRect.x * layoutEditor.canvasRect.width
                    readonly property real pxY: layoutEditor.canvasRect.y + normRect.y * layoutEditor.canvasRect.height
                    readonly property real pxW: normRect.width * layoutEditor.canvasRect.width
                    readonly property real pxH: normRect.height * layoutEditor.canvasRect.height
                    readonly property real pxCx: pxX + pxW / 2
                    readonly property real pxCy: pxY + pxH / 2

                    readonly property bool isSelected: layoutEditor.selectedSourceId === sourceId
                    readonly property real handleSize: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels

                    property bool isResizeMode: true

                    Connections {
                        target: videoEffects

                        function onSourceRectChanged(id, rect) {
                            if (id === delegateRoot.sourceId)
                                delegateRoot.normRect = rect
                        }

                        function onSourceRotationChanged(id, rotation) {
                            if (id === delegateRoot.sourceId)
                                delegateRoot.normRotation = rotation
                        }
                    }

                    Rectangle {
                        id: sourceBorder
                        x: delegateRoot.pxX
                        y: delegateRoot.pxY
                        width: delegateRoot.pxW
                        height: delegateRoot.pxH
                        color: "transparent"
                        border.color: delegateRoot.isSelected ? AkTheme.palette.active.highlight : AkTheme.palette.active.windowText
                        border.width: delegateRoot.isSelected ? AkUnit.create(2 * AkTheme.controlScale, "dp").pixels : AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
                        opacity: delegateRoot.isSelected ? 1 : 0.5
                        rotation: delegateRoot.normRotation
                        transformOrigin: Item.Center

                        MouseArea {
                            id: moveArea
                            anchors.fill: parent
                            anchors.margins: delegateRoot.handleSize / 2
                            cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                            drag.target: moveProxy
                            drag.threshold: 0
                            enabled: delegateRoot.isSelected

                            Item {
                                id: moveProxy
                                parent: canvasArea
                                x: delegateRoot.pxCx - delegateRoot.pxW / 2
                                y: delegateRoot.pxCy - delegateRoot.pxH / 2
                                width: delegateRoot.pxW
                                height: delegateRoot.pxH

                                onXChanged: if (moveArea.drag.active) applyMove()
                                onYChanged: if (moveArea.drag.active) applyMove()

                                function applyMove() {
                                    let nx = (moveProxy.x - layoutEditor.canvasRect.x) / layoutEditor.canvasRect.width
                                    let ny = (moveProxy.y - layoutEditor.canvasRect.y) / layoutEditor.canvasRect.height
                                    let nw = delegateRoot.normRect.width
                                    let nh = delegateRoot.normRect.height
                                    let snapped = layoutEditor.snapRect(delegateRoot.sourceId, Qt.rect(nx, ny, nw, nh))
                                    videoLayer.setSourceRect(delegateRoot.sourceId, snapped)
                                }
                            }

                            onClicked: (mouse) => {
                                let mapped = moveArea.mapToItem(canvasArea, mouse.x, mouse.y)
                                let mouseX = mapped.x
                                let mouseY = mapped.y
                                let ids = videoLayer.sourceIds()
                                let candidates = []

                                for (let i = 0; i < ids.length; i++) {
                                    let id = ids[i]

                                    if (!videoLayer.sourceEnabled(id))
                                        continue

                                    let r = videoLayer.sourceRect(id)
                                    let pxX = layoutEditor.canvasRect.x + r.x * layoutEditor.canvasRect.width
                                    let pxY = layoutEditor.canvasRect.y + r.y * layoutEditor.canvasRect.height
                                    let pxW = r.width * layoutEditor.canvasRect.width
                                    let pxH = r.height * layoutEditor.canvasRect.height

                                    if (mouseX >= pxX
                                        && mouseX <= pxX + pxW
                                        && mouseY >= pxY
                                        && mouseY <= pxY + pxH) {
                                        candidates.push({ id: id, z: videoLayer.sourceZOrder(id) })
                                    }
                                }

                                if (candidates.length > 0) {
                                    candidates.sort(function(a, b) { return b.z - a.z })
                                    layoutEditor.selectedSourceId = candidates[0].id
                                }
                            }

                            onDoubleClicked: (mouse) => {
                                if (delegateRoot.isSelected)
                                    delegateRoot.isResizeMode = !delegateRoot.isResizeMode
                            }
                        }
                    }

                    // Resize handles (default)
                    Repeater {
                        id: resizeHandles
                        model: [
                            { corner: "tl", x: 0, y: 0 },
                            { corner: "tr", x: 1, y: 0 },
                            { corner: "bl", x: 0, y: 1 },
                            { corner: "br", x: 1, y: 1 }
                        ]
                        delegate: Rectangle {
                            id: resizeHandle
                            width: delegateRoot.handleSize
                            height: delegateRoot.handleSize
                            color: AkTheme.palette.active.highlight
                            border.color: AkTheme.palette.active.highlightedText
                            border.width: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
                            visible: delegateRoot.isSelected && delegateRoot.isResizeMode
                            z: 10

                            rotation: delegateRoot.normRotation
                            transformOrigin: Item.Center

                            readonly property real rx: modelData.x === 0 ? -1 : 1
                            readonly property real ry: modelData.y === 0 ? -1 : 1
                            readonly property real rel_x: rx * (delegateRoot.pxW / 2)
                            readonly property real rel_y: ry * (delegateRoot.pxH / 2)
                            readonly property real a: delegateRoot.normRotation * Math.PI / 180
                            readonly property real rot_x: rel_x * Math.cos(a) - rel_y * Math.sin(a)
                            readonly property real rot_y: rel_x * Math.sin(a) + rel_y * Math.cos(a)

                            x: delegateRoot.pxCx + rot_x - width / 2
                            y: delegateRoot.pxCy + rot_y - height / 2

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: modelData.x === modelData.y ? Qt.SizeFDiagCursor : Qt.SizeBDiagCursor
                                drag.target: proxy
                                drag.axis: Drag.XAndYAxis
                                drag.threshold: 0

                                Item {
                                    id: proxy
                                    parent: canvasArea
                                    x: resizeHandle.x + resizeHandle.width / 2
                                    y: resizeHandle.y + resizeHandle.height / 2
                                    width: 1
                                    height: 1

                                    property var corner: modelData
                                    property real startPxW: 0
                                    property real startPxH: 0
                                    property real startPxCx: 0
                                    property real startPxCy: 0
                                    property real startRotation: 0
                                }

                                onPressed: (mouse) => {
                                    proxy.x = resizeHandle.x + resizeHandle.width / 2
                                    proxy.y = resizeHandle.y + resizeHandle.height / 2
                                    proxy.startPxW = delegateRoot.pxW
                                    proxy.startPxH = delegateRoot.pxH
                                    proxy.startPxCx = delegateRoot.pxCx
                                    proxy.startPxCy = delegateRoot.pxCy
                                    proxy.startRotation = delegateRoot.normRotation
                                }

                                onPositionChanged: (mouse) => {
                                    if (!pressed)
                                        return

                                    let rx = proxy.corner.x === 0 ? -1 : 1
                                    let ry = proxy.corner.y === 0 ? -1 : 1

                                    let F_rel_x = -rx * (proxy.startPxW / 2)
                                    let F_rel_y = -ry * (proxy.startPxH / 2)

                                    let a = proxy.startRotation * Math.PI / 180
                                    let cos_a = Math.cos(a)
                                    let sin_a = Math.sin(a)

                                    let F_rot_x = F_rel_x * cos_a - F_rel_y * sin_a
                                    let F_rot_y = F_rel_x * sin_a + F_rel_y * cos_a

                                    let F_screen_x = proxy.startPxCx + F_rot_x
                                    let F_screen_y = proxy.startPxCy + F_rot_y

                                    let D_screen_x = proxy.x
                                    let D_screen_y = proxy.y

                                    let dx = D_screen_x - F_screen_x
                                    let dy = D_screen_y - F_screen_y

                                    let v_x = dx * cos_a + dy * sin_a
                                    let v_y = -dx * sin_a + dy * cos_a

                                    let minW = 0.02 * layoutEditor.canvasRect.width
                                    let minH = 0.02 * layoutEditor.canvasRect.height

                                    let newW_px = Math.max(minW, v_x * rx)
                                    let newH_px = Math.max(minH, v_y * ry)

                                    let C_rel_F_local_x = rx * newW_px / 2
                                    let C_rel_F_local_y = ry * newH_px / 2

                                    let C_rel_F_screen_x = C_rel_F_local_x * cos_a - C_rel_F_local_y * sin_a
                                    let C_rel_F_screen_y = C_rel_F_local_x * sin_a + C_rel_F_local_y * cos_a

                                    let new_C_screen_x = F_screen_x + C_rel_F_screen_x
                                    let new_C_screen_y = F_screen_y + C_rel_F_screen_y

                                    let new_cx_n = (new_C_screen_x - layoutEditor.canvasRect.x) / layoutEditor.canvasRect.width
                                    let new_cy_n = (new_C_screen_y - layoutEditor.canvasRect.y) / layoutEditor.canvasRect.height
                                    let new_w_n = newW_px / layoutEditor.canvasRect.width
                                    let new_h_n = newH_px / layoutEditor.canvasRect.height

                                    let new_x_n = new_cx_n - new_w_n / 2
                                    let new_y_n = new_cy_n - new_h_n / 2

                                    videoLayer.setSourceRect(delegateRoot.sourceId, Qt.rect(new_x_n, new_y_n, new_w_n, new_h_n))
                                }

                                onReleased: {
                                    let currentRect = videoLayer.sourceRect(delegateRoot.sourceId)
                                    let snappedRect = layoutEditor.snapRect(delegateRoot.sourceId, currentRect)
                                    videoLayer.setSourceRect(delegateRoot.sourceId, snappedRect)
                                }
                            }
                        }
                    }

                    // Rotation handles
                    Repeater {
                        id: rotateHandles
                        model: [
                            { corner: "tl", x: 0, y: 0 },
                            { corner: "tr", x: 1, y: 0 },
                            { corner: "bl", x: 0, y: 1 },
                            { corner: "br", x: 1, y: 1 }
                        ]
                        delegate: Rectangle {
                            id: rotateHandle
                            width: delegateRoot.handleSize
                            height: delegateRoot.handleSize
                            radius: delegateRoot.handleSize / 2
                            color: AkTheme.palette.active.highlight
                            border.color: AkTheme.palette.active.highlightedText
                            border.width: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
                            visible: delegateRoot.isSelected && !delegateRoot.isResizeMode
                            z: 10

                            rotation: delegateRoot.normRotation
                            transformOrigin: Item.Center

                            readonly property real rx: modelData.x === 0 ? -1 : 1
                            readonly property real ry: modelData.y === 0 ? -1 : 1
                            readonly property real rel_x: rx * (delegateRoot.pxW / 2)
                            readonly property real rel_y: ry * (delegateRoot.pxH / 2)
                            readonly property real a: delegateRoot.normRotation * Math.PI / 180
                            readonly property real rot_x: rel_x * Math.cos(a) - rel_y * Math.sin(a)
                            readonly property real rot_y: rel_x * Math.sin(a) + rel_y * Math.cos(a)

                            x: delegateRoot.pxCx + rot_x - width / 2
                            y: delegateRoot.pxCy + rot_y - height / 2

                            Item {
                                id: dragProxy
                                parent: canvasArea
                                width: 1
                                height: 1

                                property real startCenterX: 0
                                property real startCenterY: 0
                                property real startNormW: 1
                                property real startNormH: 1
                                property var corner: modelData
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.SizeAllCursor
                                drag.target: dragProxy
                                drag.threshold: 0

                                onPressed: (mouse) => {
                                    dragProxy.x = rotateHandle.x + rotateHandle.width / 2
                                    dragProxy.y = rotateHandle.y + rotateHandle.height / 2
                                    dragProxy.startCenterX  = delegateRoot.pxCx
                                    dragProxy.startCenterY  = delegateRoot.pxCy
                                    dragProxy.startNormW    = delegateRoot.normRect.width
                                    dragProxy.startNormH    = delegateRoot.normRect.height
                                    dragProxy.corner        = modelData
                                }

                                onPositionChanged: (mouse) => {
                                    if (!pressed)
                                        return

                                    let dx_px = dragProxy.x - dragProxy.startCenterX
                                    let dy_px = dragProxy.y - dragProxy.startCenterY
                                    let dist_px = Math.sqrt(dx_px * dx_px + dy_px * dy_px)

                                    let startPxW = dragProxy.startNormW * layoutEditor.canvasRect.width
                                    let startPxH = dragProxy.startNormH * layoutEditor.canvasRect.height
                                    let startDist_px = Math.sqrt(Math.pow(startPxW / 2, 2) + Math.pow(startPxH / 2, 2))

                                    let scale = startDist_px > 0 ? dist_px / startDist_px : 1.0
                                    let newNormW = Math.max(0.02, dragProxy.startNormW * scale)
                                    let newNormH = Math.max(0.02, dragProxy.startNormH * scale)

                                    let cornerX = dragProxy.corner.x === 0 ? -startPxW / 2 : startPxW / 2
                                    let cornerY = dragProxy.corner.y === 0 ? -startPxH / 2 : startPxH / 2
                                    let alpha = Math.atan2(cornerY, cornerX)

                                    let theta_mouse = Math.atan2(dy_px, dx_px)
                                    let theta_rad = theta_mouse - alpha
                                    let theta_deg = theta_rad * 180 / Math.PI
                                    theta_deg = ((theta_deg % 360) + 360) % 360

                                    let cx_n = (dragProxy.startCenterX - layoutEditor.canvasRect.x) / layoutEditor.canvasRect.width
                                    let cy_n = (dragProxy.startCenterY - layoutEditor.canvasRect.y) / layoutEditor.canvasRect.height

                                    let nr = Qt.rect(
                                        cx_n - newNormW / 2,
                                        cy_n - newNormH / 2,
                                        newNormW,
                                        newNormH
                                    )

                                    videoLayer.setSourceRect(delegateRoot.sourceId, nr)
                                    videoLayer.setSourceRotation(delegateRoot.sourceId, theta_deg)
                                }

                                onReleased: {
                                    let currentRect = videoLayer.sourceRect(delegateRoot.sourceId)
                                    let snappedRect = layoutEditor.snapRect(delegateRoot.sourceId, currentRect)
                                    videoLayer.setSourceRect(delegateRoot.sourceId, snappedRect)

                                    let currentRotation = videoLayer.sourceRotation(delegateRoot.sourceId)
                                    let snappedRotation = layoutEditor.snapAngle(currentRotation)
                                    videoLayer.setSourceRotation(delegateRoot.sourceId, snappedRotation)
                                }
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: !delegateRoot.isSelected
                        z: -1
                        onClicked: layoutEditor.selectedSourceId = delegateRoot.sourceId
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                z: -2
                onClicked: (mouse) => {
                    let ids = videoLayer.sourceIds()
                    let candidates = []

                    for (let i = 0; i < ids.length; i++) {
                        let id = ids[i]

                        if (!videoLayer.sourceEnabled(id))
                            continue

                        let r = videoLayer.sourceRect(id)
                        let pxX = layoutEditor.canvasRect.x + r.x * layoutEditor.canvasRect.width
                        let pxY = layoutEditor.canvasRect.y + r.y * layoutEditor.canvasRect.height
                        let pxW = r.width * layoutEditor.canvasRect.width
                        let pxH = r.height * layoutEditor.canvasRect.height

                        if (mouse.x >= pxX
                            && mouse.x <= pxX + pxW
                            && mouse.y >= pxY
                            && mouse.y <= pxY + pxH) {
                            candidates.push({ id: id, z: videoLayer.sourceZOrder(id) })
                        }
                    }

                    if (candidates.length > 0) {
                        candidates.sort(function(a, b) { return b.z - a.z })
                        layoutEditor.selectedSourceId = candidates[0].id
                    }
                }
            }
        }
    }
}

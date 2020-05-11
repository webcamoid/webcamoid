/* Webcamoid, webcam capture application.
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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Ak 1.0

Pane {
    id: optionsPanel
    implicitWidth: AkUnit.create(250 * AkTheme.controlScale, "dp").pixels
    implicitHeight: AkUnit.create(250 * AkTheme.controlScale, "dp").pixels
    x: edge == Qt.LeftEdge?
           (k - 1) * width:
       edge == Qt.RightEdge?
           parent.width - k * width:
           0
    y: edge == Qt.TopEdge?
           (k - 1) * height:
       edge == Qt.BottomEdge?
           parent.height - k * height:
           0
    width: edge == Qt.LeftEdge || edge == Qt.RightEdge?
               implicitWidth:
               parent.width
    height: edge == Qt.TopEdge || edge == Qt.BottomEdge?
                implicitHeight:
                parent.height
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0
    visible: false
    clip: true

    property string title: ""
    property int edge: Qt.LeftEdge
    property bool opened: false
    property int panelBorder: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
    property int dragBorder: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    property int minimumWidth: AkUnit.create(100 * AkTheme.controlScale, "dp").pixels
    property int minimumHeight: AkUnit.create(100 * AkTheme.controlScale, "dp").pixels
    property variant contents: Item {}
    property real k: 0
    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color disabledDark: AkTheme.palette.disabled.dark

    signal actionClicked()
    signal closed()

    function open()
    {
        openAnimation.start()
        focus = true
        opened = true
    }

    function close()
    {
        closeAnimation.start()
        opened = false
        closed()
    }

    function closeAndOpen()
    {
        closeOpenAnimation.start()
    }

    function reparentChilds()
    {
        contents.parent = panelContents
        contents.width = panelContents.width
        contents.height = panelContents.height
    }

    Component.onCompleted: {
        panelHandler.createObject(optionsPanel.parent)
        reparentChilds()
    }
    onContentsChanged: reparentChilds()

    Connections {
        target: parent

        onWidthChanged: {
            switch (optionsPanel.edge) {
            case Qt.LeftEdge:
                optionsPanel.width = Math.max(optionsPanel.width,
                                       optionsPanel.minimumWidth)
                optionsPanel.width = Math.min(optionsPanel.width,
                                       optionsPanel.parent.width
                                       - optionsPanel.panelBorder
                                       - optionsPanel.dragBorder)

                break

            case Qt.RightEdge:
                optionsPanel.width = Math.max(optionsPanel.width,
                                       optionsPanel.minimumWidth)
                optionsPanel.width = Math.min(optionsPanel.width,
                                       optionsPanel.parent.width
                                       - optionsPanel.panelBorder
                                       - optionsPanel.dragBorder)

                break

            default:
                break
            }
        }
        onHeightChanged: {
            switch (optionsPanel.edge) {
            case Qt.TopEdge:
                optionsPanel.height = Math.max(optionsPanel.height,
                                        optionsPanel.minimumHeight)
                optionsPanel.height = Math.min(optionsPanel.height,
                                        optionsPanel.parent.height
                                        - optionsPanel.panelBorder
                                        - optionsPanel.dragBorder)

                break

            case Qt.BottomEdge:
                optionsPanel.height = Math.max(optionsPanel.height,
                                        optionsPanel.minimumHeight)
                optionsPanel.height = Math.min(optionsPanel.height,
                                        optionsPanel.parent.height
                                        - optionsPanel.panelBorder
                                        - optionsPanel.dragBorder)

                break

            default:
                break
            }
        }
    }

    GridLayout {
        anchors.fill: parent
        columns: 2
        rowSpacing: 0

        Label {
            text: optionsPanel.title
            elide: Label.ElideRight
            font.bold: true
            font.pointSize: 14
            enabled: optionsPanel.enabled
            Layout.fillWidth: true
            Layout.leftMargin:
                AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.topMargin:
                edge == Qt.TopEdge?
                    AkUnit.create(16 * AkTheme.controlScale, "dp").pixels:
                    AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            Layout.bottomMargin:
                edge == Qt.TopEdge?
                    AkUnit.create(8 * AkTheme.controlScale, "dp").pixels:
                    AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.row: edge == Qt.TopEdge? 1: 0
        }
        Button {
            icon.source: "image://icons/no"
            flat: true
            enabled: optionsPanel.enabled
            Layout.rightMargin:
                AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.topMargin:
                edge == Qt.TopEdge?
                    AkUnit.create(16 * AkTheme.controlScale, "dp").pixels:
                    AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            Layout.bottomMargin:
                edge == Qt.TopEdge?
                    AkUnit.create(8 * AkTheme.controlScale, "dp").pixels:
                    AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.column: 1
            Layout.row: edge == Qt.TopEdge? 1: 0

            onClicked: optionsPanel.actionClicked()
        }
        Item {
            id: panelContents
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: edge == Qt.TopEdge? 0: 1

            onWidthChanged: contents.width = width
            onHeightChanged: contents.height = height
        }
    }

    Component {
        id: panelHandler

        Item {
            id: element
            x: edge == Qt.LeftEdge?
                   k * optionsPanel.width - width / 2:
               edge == Qt.RightEdge?
                   optionsPanel.parent.width - k * optionsPanel.width - width / 2:
                   0
            y: edge == Qt.TopEdge?
                   k * optionsPanel.height - height / 2:
               edge == Qt.BottomEdge?
                   optionsPanel.parent.height - k * optionsPanel.height - height / 2:
                   0
            width: optionsPanel.edge == Qt.LeftEdge || optionsPanel.edge == Qt.RightEdge?
                       optionsPanel.panelBorder + 2 * optionsPanel.dragBorder:
                       optionsPanel.width
            height: optionsPanel.edge == Qt.TopEdge || optionsPanel.edge == Qt.BottomEdge?
                        optionsPanel.panelBorder + 2 * optionsPanel.dragBorder:
                        optionsPanel.height
            visible: optionsPanel.visible

            Rectangle {
                id: rectangleLeft
                color: optionsPanel.enabled?
                           optionsPanel.activeDark:
                           optionsPanel.disabledDark
                x: optionsPanel.edge == Qt.LeftEdge || optionsPanel.edge == Qt.RightEdge?
                       (parent.width - optionsPanel.panelBorder) / 2:
                       0
                y: optionsPanel.edge == Qt.TopEdge || optionsPanel.edge == Qt.BottomEdge?
                       (parent.height - optionsPanel.panelBorder) / 2:
                       0
                width: optionsPanel.edge == Qt.LeftEdge || optionsPanel.edge == Qt.RightEdge?
                           optionsPanel.panelBorder:
                           parent.width
                height: optionsPanel.edge == Qt.TopEdge || optionsPanel.edge == Qt.BottomEdge?
                            optionsPanel.panelBorder:
                            parent.height
            }
            MouseArea {
                cursorShape: optionsPanel.edge == Qt.LeftEdge || optionsPanel.edge == Qt.RightEdge?
                                 Qt.SizeHorCursor:
                                 Qt.SizeVerCursor
                drag.axis: optionsPanel.edge == Qt.LeftEdge || optionsPanel.edge == Qt.RightEdge?
                               Drag.XAxis:
                               Drag.YAxis
                anchors.fill: parent

                onPositionChanged: {
                    switch (optionsPanel.edge) {
                    case Qt.LeftEdge:
                        optionsPanel.width += mouse.x
                        optionsPanel.width = Math.max(optionsPanel.width,
                                               optionsPanel.minimumWidth)
                        optionsPanel.width = Math.min(optionsPanel.width,
                                               optionsPanel.parent.width
                                               - optionsPanel.panelBorder
                                               - optionsPanel.dragBorder)

                        break

                    case Qt.RightEdge:
                        optionsPanel.width -= mouse.x
                        optionsPanel.width = Math.max(optionsPanel.width,
                                               optionsPanel.minimumWidth)
                        optionsPanel.width = Math.min(optionsPanel.width,
                                               optionsPanel.parent.width
                                               - optionsPanel.panelBorder
                                               - optionsPanel.dragBorder)

                        break

                    case Qt.TopEdge:
                        optionsPanel.height += mouse.y
                        optionsPanel.height = Math.max(optionsPanel.height,
                                                optionsPanel.minimumHeight)
                        optionsPanel.height = Math.min(optionsPanel.height,
                                                optionsPanel.parent.height
                                                - optionsPanel.panelBorder
                                                - optionsPanel.dragBorder)

                        break

                    case Qt.BottomEdge:
                        optionsPanel.height -= mouse.y
                        optionsPanel.height = Math.max(optionsPanel.height,
                                                optionsPanel.minimumHeight)
                        optionsPanel.height = Math.min(optionsPanel.height,
                                                optionsPanel.parent.height
                                                - optionsPanel.panelBorder
                                                - optionsPanel.dragBorder)

                        break
                    }
                }
            }
        }
    }

    SequentialAnimation {
        id: openAnimation

        PropertyAnimation {
            target: optionsPanel
            property: "visible"
            to: true
            duration: 0
        }
        PropertyAnimation {
            target: optionsPanel
            property: "k"
            to: 1
            duration: 200
        }
    }
    SequentialAnimation {
        id: closeAnimation

        PropertyAnimation {
            target: optionsPanel
            property: "k"
            to: 0
            duration: 200
        }
        PropertyAnimation {
            target: optionsPanel
            property: "visible"
            to: false
            duration: 0
        }
    }
    SequentialAnimation {
        id: closeOpenAnimation

        PropertyAnimation {
            target: optionsPanel
            property: "k"
            to: 0
            duration: 200
        }
        PropertyAnimation {
            target: optionsPanel
            property: "visible"
            to: false
            duration: 0
        }
        PropertyAnimation {
            target: optionsPanel
            property: "visible"
            to: true
            duration: 0
        }
        PropertyAnimation {
            target: optionsPanel
            property: "k"
            to: 1
            duration: 200
        }
    }
}

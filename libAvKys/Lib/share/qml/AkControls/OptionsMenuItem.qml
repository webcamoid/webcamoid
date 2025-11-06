/* Webcamoid, camera capture application.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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
import Ak

Item {
    id: root
    width: ListView.view?
                ListView.view.width:
                AkUnit.create(300 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(72 * AkTheme.controlScale, "dp").pixels

    property string title: ""
    property string subtitle: ""
    property string iconSource: ""
    property bool showDivider: false
    property bool highlighted: false

    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeHighlightedText: AkTheme.palette.active.highlightedText
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color activeWindowText: AkTheme.palette.active.windowText

    signal clicked()

    Item {
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            color:
                root.highlighted?
                   root.activeHighlight:
                   AkTheme.shade(root.activeWindow, 0, 0)
            opacity: root.highlighted? 1.0: 0.0
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        Row {
            spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            anchors.left: parent.left
            anchors.leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            anchors.verticalCenter: parent.verticalCenter

            // Icon
            AkColorizedImage {
                id: icon
                width: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
                height: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
                source: root.iconSource
                color: root.highlighted?
                           root.activeHighlightedText:
                           root.activeWindowText
                colorize: true
                visible: root.iconSource !== "" && status == Image.Ready
                asynchronous: true
                fillMode: AkColorizedImage.PreserveAspectFit
                mipmap: true
                smooth: true
                anchors.verticalCenter: parent.verticalCenter
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: AkUnit.create(2 * AkTheme.controlScale, "dp").pixels

                Label {
                    text: root.title
                    color: root.highlighted?
                               root.activeHighlightedText:
                               root.activeWindowText
                    font: AkTheme.fontSettings.body1
                }

                Label {
                    text: root.subtitle
                    color: root.highlighted?
                               root.activeHighlightedText:
                               root.activeWindowText
                    font: AkTheme.fontSettings.subtitle1
                    visible: root.subtitle !== ""
                    opacity: 0.5
                }
            }
        }

        Ripple {
            id: rippleEffect
            color: root.highlighted?
                       AkTheme.constShade(root.activeHighlight, 0.2):
                       AkTheme.shade(root.activeWindow, -0.2)
            anchors.fill: parent
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true

            onEntered: root.highlighted = true
            onExited: root.highlighted = false
            onPressed: rippleEffect.activate()
            onClicked: root.clicked()
        }

        Rectangle {
            width: parent.width
            height: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
            color: root.activeDark
            visible: root.showDivider
            anchors.bottom: parent.bottom
        }
    }
}

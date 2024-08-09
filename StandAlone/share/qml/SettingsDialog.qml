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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ak

Dialog {
    id: settingsDialog
    standardButtons: Dialog.Close
    width: AkUnit.create(420 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(320 * AkTheme.controlScale, "dp").pixels
    modal: true
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    property int panelBorder: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
    property int dragBorder: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    property int minimumWidth: AkUnit.create(100 * AkTheme.controlScale, "dp").pixels
    property int maximumWidth:
        width - AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color disabledDark: AkTheme.palette.disabled.dark

    function openAtIndex(index) {
        options.currentIndex = index
        open()
    }

    onWidthChanged: {
        if (settingsDialog.visible)
            optionsItem.implicitWidth =
                    Math.min(Math.max(settingsDialog.minimumWidth,
                                      optionsItem.implicitWidth),
                             settingsDialog.maximumWidth)
    }
    onVisibleChanged: options.forceActiveFocus()

    RowLayout {
        anchors.fill: parent

        Item {
            id: optionsItem
            Layout.fillHeight: true
            implicitWidth:
                AkUnit.create(200 * AkTheme.controlScale, "dp").pixels

            ScrollView {
                id: optionsView
                anchors.fill: parent
                contentHeight: options.height
                clip: true

                OptionList {
                    id: options
                    width: optionsView.width

                    ItemDelegate {
                        text: qsTr("Image Capture")
                    }
                    ItemDelegate {
                        text: qsTr("Video Recording")
                    }
                    ItemDelegate {
                        text: qsTr("General Options")
                    }
                    ItemDelegate {
                        text: qsTr("Plugins")
                    }
                    ItemDelegate {
                        text: qsTr("Updates")
                    }
                    ItemDelegate {
                        text: qsTr("Debug Log")
                    }
                }
            }
            Rectangle {
                id: rectangleRight
                width: settingsDialog.panelBorder
                color: settingsDialog.activeDark
                anchors.leftMargin: -width / 2
                anchors.left: optionsView.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
            }
            MouseArea {
                cursorShape: Qt.SizeHorCursor
                drag.axis: Drag.XAxis
                width: settingsDialog.panelBorder + 2 * settingsDialog.dragBorder
                anchors.leftMargin: -width / 2
                anchors.left: optionsView.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                onPositionChanged: function (mouse) {
                    optionsItem.implicitWidth =
                            Math.min(Math.max(settingsDialog.minimumWidth,
                                              optionsItem.implicitWidth
                                              + mouse.x),
                                     settingsDialog.maximumWidth)
                }
            }
        }
        StackLayout {
            id: stackLayout
            currentIndex: options.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true

            ImageCapture { }
            VideoRecording { }
            GeneralConfig { }
            PluginConfig { }
            UpdatesConfig { }
            DebugLog { }
        }
    }
}

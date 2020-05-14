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
import Webcamoid 1.0

Dialog {
    id: videoEffectsDialog
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: AkUnit.create(420 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(320 * AkTheme.controlScale, "dp").pixels
    modal: true
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0
    title: qsTr("Add video effect")

    property int panelBorder: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
    property int dragBorder: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    property int minimumWidth: AkUnit.create(100 * AkTheme.controlScale, "dp").pixels
    property int maximumWidth:
        width - AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color disabledDark: AkTheme.palette.disabled.dark

    onWidthChanged: {
        if (videoEffectsDialog.visible)
            optionsItem.implicitWidth =
                    Math.min(Math.max(videoEffectsDialog.minimumWidth,
                                      optionsItem.implicitWidth),
                             videoEffectsDialog.maximumWidth)
    }

    RowLayout {
        anchors.fill: parent

        Item {
            id: optionsItem
            Layout.fillHeight: true
            implicitWidth:
                AkUnit.create(200 * AkTheme.controlScale, "dp").pixels

            ColumnLayout {
                id: optionsLayout
                anchors.fill: parent

                TextField {
                    id: searchEffect
                    placeholderText: qsTr("Search effect")
                    Layout.fillWidth: true
                }
                ScrollView {
                    id: optionsView
                    contentHeight: options.height
                    clip: true
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    OptionList {
                        id: options
                        width: optionsView.width
                        textRole: "description"
                        filter: searchEffect.text

                        function update() {
                            var effects = videoEffects.availableEffects
                            model.clear()

                            for (let effect in effects) {
                                let effectInfo =
                                    videoEffects.effectInfo(effects[effect])
                                model.append({
                                    effect: effects[effect],
                                    description:
                                        effectInfo["MetaData"]["description"]})
                            }

                            currentIndex = count > 0? 0: -1
                        }

                        function updatePreview() {
                            if (count < 1) {
                                videoEffects.setPreview("")

                                return
                            }

                            let index =
                                Math.min(Math.max(0, currentIndex), count - 1)

                            var option = model.get(currentIndex)

                            if (option)
                                videoEffects.setPreview(option.effect)
                            else
                                videoEffects.setPreview("")
                        }

                        Connections {
                            target: mediaTools

                            onInterfaceLoaded: videoEffects.setPreview("")
                        }

                        Connections {
                            target: videoEffects

                            onAvailableEffectsChanged: options.update()
                        }

                        onCurrentIndexChanged: {
                            updatePreview()
                            searchEffect.text = ""
                        }
                        Component.onCompleted: update()
                    }
                }
            }
            Rectangle {
                id: rectangleRight
                width: videoEffectsDialog.panelBorder
                color: videoEffectsDialog.activeDark
                anchors.leftMargin: -width / 2
                anchors.left: optionsLayout.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
            }
            MouseArea {
                cursorShape: Qt.SizeHorCursor
                drag.axis: Drag.XAxis
                width: videoEffectsDialog.panelBorder
                       + 2 * videoEffectsDialog.dragBorder
                anchors.leftMargin: -width / 2
                anchors.left: optionsLayout.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                onPositionChanged: {
                    optionsItem.implicitWidth =
                            Math.min(Math.max(videoEffectsDialog.minimumWidth,
                                              optionsItem.implicitWidth
                                              + mouse.x),
                                     videoEffectsDialog.maximumWidth)
                }
            }
        }
        ColumnLayout {
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                VideoDisplay {
                    id: effectPreview
                    objectName: "effectPreview"
                    visible: videoLayer.state === AkElement.ElementStatePlaying
                    smooth: true
                    anchors.fill: parent
                }
            }
            Switch {
                //: Apply the effect over the other effects.
                text: qsTr("Chain effect")
                Layout.fillWidth: true
                checked: videoEffects.chainEffects

                onCheckedChanged: videoEffects.chainEffects = checked
            }
        }
    }

    onVisibleChanged: {
        if (visible)
            options.updatePreview()
        else
            videoEffects.setPreview("")
    }
    onAccepted: videoEffects.applyPreview()
    onRejected: videoEffects.setPreview("")

    header: Item {
        id: rectangle
        clip: true
        visible: videoEffectsDialog.title
        height: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels

        Label {
            text: videoEffectsDialog.title
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin:
                AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
            elide: Label.ElideRight
            font.bold: true
            font.pointSize: 16
            enabled: videoEffectsDialog.enabled
        }

        Rectangle {
            color: videoEffectsDialog.enabled?
                       videoEffectsDialog.activeDark:
                       videoEffectsDialog.disabledDark
            height: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
            anchors.left: rectangle.left
            anchors.right: rectangle.right
            anchors.bottom: rectangle.bottom
        }
    }
}

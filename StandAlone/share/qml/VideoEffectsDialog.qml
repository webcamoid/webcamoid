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
import Ak
import AkControls as AK
import Webcamoid

Dialog {
    id: videoEffectsDialog
    title: qsTr("Add video effect")
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: AkUnit.create(420 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(320 * AkTheme.controlScale, "dp").pixels
    modal: true
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    property int sourceId: -1

    readonly property bool isSource: sourceId >= 0

    function activeChainEffects() {
        if (videoEffectsDialog.isSource)
            return videoEffects.sourceChainEffects(videoEffectsDialog.sourceId)

        return videoEffects.chainEffects
    }

    function setActiveChainEffects(value) {
        if (videoEffectsDialog.isSource)
            videoEffects.setSourceChainEffects(videoEffectsDialog.sourceId,
                                               value)
        else
            videoEffects.chainEffects = value
    }

    function setActivePreview(effect) {
        if (videoEffectsDialog.isSource)
            videoEffects.setSourcePreview(videoEffectsDialog.sourceId, effect)
        else
            videoEffects.preview = effect
    }

    function clearActivePreview() {
        videoEffectsDialog.setActivePreview("")
    }

    function embedActivePreviewControls(where) {
        if (videoEffectsDialog.isSource)
            videoEffects.embedPreviewControls(where,
                                              videoEffectsDialog.sourceId)
        else
            videoEffects.embedPreviewControls(where)
    }

    function applyActivePreview() {
        if (videoEffectsDialog.isSource)
            videoEffects.applySourcePreview(videoEffectsDialog.sourceId)
        else
            videoEffects.applyPreview()
    }

    Connections {
        target: mediaTools

        function onInterfaceLoaded()
        {
            videoEffectsDialog.clearActivePreview()
        }
    }

    Connections {
        target: videoEffects

        function onAvailableEffectsChanged()
        {
            cbkEffects.update()
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: glyEffects.height
        clip: true

        ColumnLayout {
            id: glyEffects
            width: scrollView.width

            AK.LabeledComboBox {
                id: cbkEffects
                label: qsTr("Effect")
                textRole: "description"
                model: ListModel {
                }
                Accessible.description: currentText
                Layout.fillWidth: true

                function update() {
                    model.clear()
                    var effects = videoEffects.availableEffects

                    for (let effect in effects) {
                        let effectInfo =
                            AkPluginInfo.create(videoEffects.effectInfo(effects[effect]))

                        model.append({
                            effect: effects[effect],
                            description: effectInfo.description
                        })
                    }

                    currentIndex = 0
                }

                function updatePreview() {
                    if (count < 1) {
                        videoEffectsDialog.clearActivePreview()

                        return
                    }

                    let index =
                        Math.min(Math.max(0, currentIndex), count - 1)

                    var option = model.get(currentIndex)
                    videoEffectsDialog.setActivePreview(option.effect)
                    videoEffects.removeInterface("itmEffectPreviewControls")

                    if (videoEffectsDialog.visible)
                        videoEffectsDialog.embedActivePreviewControls("itmEffectPreviewControls")
                }

                Component.onCompleted: update()
                onCurrentIndexChanged: updatePreview()
            }
            Item {
                id: preview
                Layout.minimumHeight: effectPreview.visible?
                                        AkUnit.create(240 * AkTheme.controlScale, "dp").pixels:
                                        0
                Layout.topMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true

                VideoDisplay {
                    id: effectPreview
                    objectName: "effectPreview"
                    visible: videoLayer.state == AkElement.ElementStatePlaying
                    smooth: true
                    anchors.fill: parent
                }
            }
            Switch {
                //: Apply the effect over the other effects.
                text: qsTr("Chain effect")
                checked: videoEffectsDialog.activeChainEffects()
                Layout.fillWidth: true

                onCheckedChanged: videoEffectsDialog.setActiveChainEffects(checked)
            }
            ColumnLayout {
                id: itmEffectControls
                objectName: "itmEffectPreviewControls"
                Layout.leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                Layout.rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            cbkEffects.updatePreview()
            cbkEffects.forceActiveFocus()
        } else {
            videoEffectsDialog.clearActivePreview()
        }
    }
    onAccepted: videoEffectsDialog.applyActivePreview()
    onRejected: videoEffectsDialog.clearActivePreview()

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
            font: AkTheme.fontSettings.h6
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

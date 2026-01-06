/* Webcamoid, camera capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

Dialog {
    id: addEdit
    standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Reset
    width: AkUnit.create(450 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(350 * AkTheme.controlScale, "dp").pixels
    modal: true

    property string basePaletteName: ""

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    signal paletteUpdated(string palName)

    function initializePalette()
    {
        let pal = AkPalette.create(basePaletteName)
        btnHighlightedText.currentColor = pal.active.highlightedText
        btnHighlight.currentColor = pal.active.highlight
        btnText.currentColor = pal.active.text
        btnPlaceholderText.currentColor = pal.active.placeholderText
        btnBase.currentColor = pal.active.base
        btnAlternateBase.currentColor = pal.active.alternateBase
        btnWindowText.currentColor = pal.active.windowText
        btnWindow.currentColor = pal.active.window
        btnButtonText.currentColor = pal.active.buttonText
        btnLight.currentColor = pal.active.light
        btnMidlight.currentColor = pal.active.midlight
        btnButton.currentColor = pal.active.button
        btnMid.currentColor = pal.active.mid
        btnDark.currentColor = pal.active.dark
        btnShadow.currentColor = pal.active.shadow
        btnToolTipText.currentColor = pal.active.toolTipText
        btnToolTipBase.currentColor = pal.active.toolTipBase
        btnLink.currentColor = pal.active.link
        btnLinkVisited.currentColor = pal.active.linkVisited
    }

    function openOptions(edit, basePalName)
    {
        title = edit?
                    qsTr("Edit color scheme"):
                    qsTr("Create a new color scheme")
        basePaletteName = basePalName
        paletteName.text = edit?
            basePalName:
            qsTr("Color Palette %1").arg(mediaTools.currentTime("yyyyMMddhhmmss"))
        paletteName.enabled = !edit
        initializePalette()
        open()
    }

    onVisibleChanged: {
        paletteName.forceActiveFocus()
        tabBar.currentIndex = 0
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        ColumnLayout {
            id: layout
            width: scrollView.width

            TextField {
                id: paletteName
                placeholderText: qsTr("Palette name")
                selectByMouse: true
                Layout.fillWidth: true
            }
            TabBar {
                id: tabBar
                Layout.fillWidth: true

                TabButton {
                    text: qsTr("Enabled")
                }
                TabButton {
                    text: qsTr("Disabled")
                }
            }
            GroupBox {
                title: qsTr("Preview")
                Layout.fillWidth: true

                Frame {
                    anchors.fill: parent
                    enabled: glyControls.controlEnabled

                    AkTheme.palette.active.highlightedText: btnHighlightedText.currentColor
                    AkTheme.palette.active.highlight: btnHighlight.currentColor
                    AkTheme.palette.active.text: btnText.currentColor
                    AkTheme.palette.active.placeholderText: btnPlaceholderText.currentColor
                    AkTheme.palette.active.base: btnBase.currentColor
                    AkTheme.palette.active.alternateBase: btnAlternateBase.currentColor
                    AkTheme.palette.active.windowText: btnWindowText.currentColor
                    AkTheme.palette.active.window: btnWindow.currentColor
                    AkTheme.palette.active.buttonText: btnButtonText.currentColor
                    AkTheme.palette.active.light: btnLight.currentColor
                    AkTheme.palette.active.midlight: btnMidlight.currentColor
                    AkTheme.palette.active.button: btnButton.currentColor
                    AkTheme.palette.active.mid: btnMid.currentColor
                    AkTheme.palette.active.dark: btnDark.currentColor
                    AkTheme.palette.active.shadow: btnShadow.currentColor
                    AkTheme.palette.active.toolTipText: btnToolTipText.currentColor
                    AkTheme.palette.active.toolTipBase: btnToolTipBase.currentColor
                    AkTheme.palette.active.link: btnLink.currentColor
                    AkTheme.palette.active.linkVisited: btnLinkVisited.currentColor

                    AkTheme.palette.disabled.highlightedText: disabledColors.highlightedText
                    AkTheme.palette.disabled.highlight: disabledColors.highlight
                    AkTheme.palette.disabled.text: disabledColors.text
                    AkTheme.palette.disabled.placeholderText: disabledColors.placeholderText
                    AkTheme.palette.disabled.base: disabledColors.base
                    AkTheme.palette.disabled.alternateBase: disabledColors.alternateBase
                    AkTheme.palette.disabled.windowText: disabledColors.windowText
                    AkTheme.palette.disabled.window: disabledColors.window
                    AkTheme.palette.disabled.buttonText: disabledColors.buttonText
                    AkTheme.palette.disabled.light: disabledColors.light
                    AkTheme.palette.disabled.midlight: disabledColors.midlight
                    AkTheme.palette.disabled.button: disabledColors.button
                    AkTheme.palette.disabled.mid: disabledColors.mid
                    AkTheme.palette.disabled.dark: disabledColors.dark
                    AkTheme.palette.disabled.shadow: disabledColors.shadow
                    AkTheme.palette.disabled.toolTipText: disabledColors.toolTipText
                    AkTheme.palette.disabled.toolTipBase: disabledColors.toolTipBase
                    AkTheme.palette.disabled.link: disabledColors.link
                    AkTheme.palette.disabled.linkVisited: disabledColors.linkVisited

                    ColumnLayout {
                        id: glyControls
                        anchors.fill: parent

                        property bool controlEnabled: tabBar.currentIndex < 1
                        property int controlValue: 50
                        property int controlMinValue: 0
                        property int controlMaxValue: 100
                        property int controlStep: 1

                        RowLayout {
                            Label {
                                text: qsTr("Window text <a href=\"#\">link</a>")
                                enabled: glyControls.controlEnabled
                                Layout.fillWidth: true

                                AkTheme.palette.active.highlightedText: btnHighlightedText.currentColor
                                AkTheme.palette.active.highlight: btnHighlight.currentColor
                                AkTheme.palette.active.text: btnText.currentColor
                                AkTheme.palette.active.placeholderText: btnPlaceholderText.currentColor
                                AkTheme.palette.active.base: btnBase.currentColor
                                AkTheme.palette.active.alternateBase: btnAlternateBase.currentColor
                                AkTheme.palette.active.windowText: btnWindowText.currentColor
                                AkTheme.palette.active.window: btnWindow.currentColor
                                AkTheme.palette.active.buttonText: btnButtonText.currentColor
                                AkTheme.palette.active.light: btnLight.currentColor
                                AkTheme.palette.active.midlight: btnMidlight.currentColor
                                AkTheme.palette.active.button: btnButton.currentColor
                                AkTheme.palette.active.mid: btnMid.currentColor
                                AkTheme.palette.active.dark: btnDark.currentColor
                                AkTheme.palette.active.shadow: btnShadow.currentColor
                                AkTheme.palette.active.toolTipText: btnToolTipText.currentColor
                                AkTheme.palette.active.toolTipBase: btnToolTipBase.currentColor
                                AkTheme.palette.active.link: btnLink.currentColor
                                AkTheme.palette.active.linkVisited: btnLinkVisited.currentColor

                                AkTheme.palette.disabled.highlightedText: disabledColors.highlightedText
                                AkTheme.palette.disabled.highlight: disabledColors.highlight
                                AkTheme.palette.disabled.text: disabledColors.text
                                AkTheme.palette.disabled.placeholderText: disabledColors.placeholderText
                                AkTheme.palette.disabled.base: disabledColors.base
                                AkTheme.palette.disabled.alternateBase: disabledColors.alternateBase
                                AkTheme.palette.disabled.windowText: disabledColors.windowText
                                AkTheme.palette.disabled.window: disabledColors.window
                                AkTheme.palette.disabled.buttonText: disabledColors.buttonText
                                AkTheme.palette.disabled.light: disabledColors.light
                                AkTheme.palette.disabled.midlight: disabledColors.midlight
                                AkTheme.palette.disabled.button: disabledColors.button
                                AkTheme.palette.disabled.mid: disabledColors.mid
                                AkTheme.palette.disabled.dark: disabledColors.dark
                                AkTheme.palette.disabled.shadow: disabledColors.shadow
                                AkTheme.palette.disabled.toolTipText: disabledColors.toolTipText
                                AkTheme.palette.disabled.toolTipBase: disabledColors.toolTipBase
                                AkTheme.palette.disabled.link: disabledColors.link
                                AkTheme.palette.disabled.linkVisited: disabledColors.linkVisited
                            }
                            Button {
                                id: btnExampleButton
                                text: qsTr("Button text")
                                enabled: glyControls.controlEnabled

                                AkTheme.palette.active.highlightedText: btnHighlightedText.currentColor
                                AkTheme.palette.active.highlight: btnHighlight.currentColor
                                AkTheme.palette.active.text: btnText.currentColor
                                AkTheme.palette.active.placeholderText: btnPlaceholderText.currentColor
                                AkTheme.palette.active.base: btnBase.currentColor
                                AkTheme.palette.active.alternateBase: btnAlternateBase.currentColor
                                AkTheme.palette.active.windowText: btnWindowText.currentColor
                                AkTheme.palette.active.window: btnWindow.currentColor
                                AkTheme.palette.active.buttonText: btnButtonText.currentColor
                                AkTheme.palette.active.light: btnLight.currentColor
                                AkTheme.palette.active.midlight: btnMidlight.currentColor
                                AkTheme.palette.active.button: btnButton.currentColor
                                AkTheme.palette.active.mid: btnMid.currentColor
                                AkTheme.palette.active.dark: btnDark.currentColor
                                AkTheme.palette.active.shadow: btnShadow.currentColor
                                AkTheme.palette.active.toolTipText: btnToolTipText.currentColor
                                AkTheme.palette.active.toolTipBase: btnToolTipBase.currentColor
                                AkTheme.palette.active.link: btnLink.currentColor
                                AkTheme.palette.active.linkVisited: btnLinkVisited.currentColor

                                AkTheme.palette.disabled.highlightedText: disabledColors.highlightedText
                                AkTheme.palette.disabled.highlight: disabledColors.highlight
                                AkTheme.palette.disabled.text: disabledColors.text
                                AkTheme.palette.disabled.placeholderText: disabledColors.placeholderText
                                AkTheme.palette.disabled.base: disabledColors.base
                                AkTheme.palette.disabled.alternateBase: disabledColors.alternateBase
                                AkTheme.palette.disabled.windowText: disabledColors.windowText
                                AkTheme.palette.disabled.window: disabledColors.window
                                AkTheme.palette.disabled.buttonText: disabledColors.buttonText
                                AkTheme.palette.disabled.light: disabledColors.light
                                AkTheme.palette.disabled.midlight: disabledColors.midlight
                                AkTheme.palette.disabled.button: disabledColors.button
                                AkTheme.palette.disabled.mid: disabledColors.mid
                                AkTheme.palette.disabled.dark: disabledColors.dark
                                AkTheme.palette.disabled.shadow: disabledColors.shadow
                                AkTheme.palette.disabled.toolTipText: disabledColors.toolTipText
                                AkTheme.palette.disabled.toolTipBase: disabledColors.toolTipBase
                                AkTheme.palette.disabled.link: disabledColors.link
                                AkTheme.palette.disabled.linkVisited: disabledColors.linkVisited
                            }
                            ToolTip {
                                id: toolTip
                                text: qsTr("Tooltip text")
                                visible: btnExampleButton.hovered && glyControls.controlEnabled

                                AkTheme.palette.active.highlightedText: btnHighlightedText.currentColor
                                AkTheme.palette.active.highlight: btnHighlight.currentColor
                                AkTheme.palette.active.text: btnText.currentColor
                                AkTheme.palette.active.placeholderText: btnPlaceholderText.currentColor
                                AkTheme.palette.active.base: btnBase.currentColor
                                AkTheme.palette.active.alternateBase: btnAlternateBase.currentColor
                                AkTheme.palette.active.windowText: btnWindowText.currentColor
                                AkTheme.palette.active.window: btnWindow.currentColor
                                AkTheme.palette.active.buttonText: btnButtonText.currentColor
                                AkTheme.palette.active.light: btnLight.currentColor
                                AkTheme.palette.active.midlight: btnMidlight.currentColor
                                AkTheme.palette.active.button: btnButton.currentColor
                                AkTheme.palette.active.mid: btnMid.currentColor
                                AkTheme.palette.active.dark: btnDark.currentColor
                                AkTheme.palette.active.shadow: btnShadow.currentColor
                                AkTheme.palette.active.toolTipText: btnToolTipText.currentColor
                                AkTheme.palette.active.toolTipBase: btnToolTipBase.currentColor
                                AkTheme.palette.active.link: btnLink.currentColor
                                AkTheme.palette.active.linkVisited: btnLinkVisited.currentColor

                                AkTheme.palette.disabled.highlightedText: disabledColors.highlightedText
                                AkTheme.palette.disabled.highlight: disabledColors.highlight
                                AkTheme.palette.disabled.text: disabledColors.text
                                AkTheme.palette.disabled.placeholderText: disabledColors.placeholderText
                                AkTheme.palette.disabled.base: disabledColors.base
                                AkTheme.palette.disabled.alternateBase: disabledColors.alternateBase
                                AkTheme.palette.disabled.windowText: disabledColors.windowText
                                AkTheme.palette.disabled.window: disabledColors.window
                                AkTheme.palette.disabled.buttonText: disabledColors.buttonText
                                AkTheme.palette.disabled.light: disabledColors.light
                                AkTheme.palette.disabled.midlight: disabledColors.midlight
                                AkTheme.palette.disabled.button: disabledColors.button
                                AkTheme.palette.disabled.mid: disabledColors.mid
                                AkTheme.palette.disabled.dark: disabledColors.dark
                                AkTheme.palette.disabled.shadow: disabledColors.shadow
                                AkTheme.palette.disabled.toolTipText: disabledColors.toolTipText
                                AkTheme.palette.disabled.toolTipBase: disabledColors.toolTipBase
                                AkTheme.palette.disabled.link: disabledColors.link
                                AkTheme.palette.disabled.linkVisited: disabledColors.linkVisited
                            }
                        }
                        RowLayout {
                            CheckBox {
                                enabled: glyControls.controlEnabled

                                AkTheme.palette.active.highlightedText: btnHighlightedText.currentColor
                                AkTheme.palette.active.highlight: btnHighlight.currentColor
                                AkTheme.palette.active.text: btnText.currentColor
                                AkTheme.palette.active.placeholderText: btnPlaceholderText.currentColor
                                AkTheme.palette.active.base: btnBase.currentColor
                                AkTheme.palette.active.alternateBase: btnAlternateBase.currentColor
                                AkTheme.palette.active.windowText: btnWindowText.currentColor
                                AkTheme.palette.active.window: btnWindow.currentColor
                                AkTheme.palette.active.buttonText: btnButtonText.currentColor
                                AkTheme.palette.active.light: btnLight.currentColor
                                AkTheme.palette.active.midlight: btnMidlight.currentColor
                                AkTheme.palette.active.button: btnButton.currentColor
                                AkTheme.palette.active.mid: btnMid.currentColor
                                AkTheme.palette.active.dark: btnDark.currentColor
                                AkTheme.palette.active.shadow: btnShadow.currentColor
                                AkTheme.palette.active.toolTipText: btnToolTipText.currentColor
                                AkTheme.palette.active.toolTipBase: btnToolTipBase.currentColor
                                AkTheme.palette.active.link: btnLink.currentColor
                                AkTheme.palette.active.linkVisited: btnLinkVisited.currentColor

                                AkTheme.palette.disabled.highlightedText: disabledColors.highlightedText
                                AkTheme.palette.disabled.highlight: disabledColors.highlight
                                AkTheme.palette.disabled.text: disabledColors.text
                                AkTheme.palette.disabled.placeholderText: disabledColors.placeholderText
                                AkTheme.palette.disabled.base: disabledColors.base
                                AkTheme.palette.disabled.alternateBase: disabledColors.alternateBase
                                AkTheme.palette.disabled.windowText: disabledColors.windowText
                                AkTheme.palette.disabled.window: disabledColors.window
                                AkTheme.palette.disabled.buttonText: disabledColors.buttonText
                                AkTheme.palette.disabled.light: disabledColors.light
                                AkTheme.palette.disabled.midlight: disabledColors.midlight
                                AkTheme.palette.disabled.button: disabledColors.button
                                AkTheme.palette.disabled.mid: disabledColors.mid
                                AkTheme.palette.disabled.dark: disabledColors.dark
                                AkTheme.palette.disabled.shadow: disabledColors.shadow
                                AkTheme.palette.disabled.toolTipText: disabledColors.toolTipText
                                AkTheme.palette.disabled.toolTipBase: disabledColors.toolTipBase
                                AkTheme.palette.disabled.link: disabledColors.link
                                AkTheme.palette.disabled.linkVisited: disabledColors.linkVisited
                            }
                            TextField {
                                placeholderText: qsTr("Placeholder text")
                                text: qsTr("Text")
                                selectByMouse: true
                                enabled: glyControls.controlEnabled
                                Layout.fillWidth: true

                                AkTheme.palette.active.highlightedText: btnHighlightedText.currentColor
                                AkTheme.palette.active.highlight: btnHighlight.currentColor
                                AkTheme.palette.active.text: btnText.currentColor
                                AkTheme.palette.active.placeholderText: btnPlaceholderText.currentColor
                                AkTheme.palette.active.base: btnBase.currentColor
                                AkTheme.palette.active.alternateBase: btnAlternateBase.currentColor
                                AkTheme.palette.active.windowText: btnWindowText.currentColor
                                AkTheme.palette.active.window: btnWindow.currentColor
                                AkTheme.palette.active.buttonText: btnButtonText.currentColor
                                AkTheme.palette.active.light: btnLight.currentColor
                                AkTheme.palette.active.midlight: btnMidlight.currentColor
                                AkTheme.palette.active.button: btnButton.currentColor
                                AkTheme.palette.active.mid: btnMid.currentColor
                                AkTheme.palette.active.dark: btnDark.currentColor
                                AkTheme.palette.active.shadow: btnShadow.currentColor
                                AkTheme.palette.active.toolTipText: btnToolTipText.currentColor
                                AkTheme.palette.active.toolTipBase: btnToolTipBase.currentColor
                                AkTheme.palette.active.link: btnLink.currentColor
                                AkTheme.palette.active.linkVisited: btnLinkVisited.currentColor

                                AkTheme.palette.disabled.highlightedText: disabledColors.highlightedText
                                AkTheme.palette.disabled.highlight: disabledColors.highlight
                                AkTheme.palette.disabled.text: disabledColors.text
                                AkTheme.palette.disabled.placeholderText: disabledColors.placeholderText
                                AkTheme.palette.disabled.base: disabledColors.base
                                AkTheme.palette.disabled.alternateBase: disabledColors.alternateBase
                                AkTheme.palette.disabled.windowText: disabledColors.windowText
                                AkTheme.palette.disabled.window: disabledColors.window
                                AkTheme.palette.disabled.buttonText: disabledColors.buttonText
                                AkTheme.palette.disabled.light: disabledColors.light
                                AkTheme.palette.disabled.midlight: disabledColors.midlight
                                AkTheme.palette.disabled.button: disabledColors.button
                                AkTheme.palette.disabled.mid: disabledColors.mid
                                AkTheme.palette.disabled.dark: disabledColors.dark
                                AkTheme.palette.disabled.shadow: disabledColors.shadow
                                AkTheme.palette.disabled.toolTipText: disabledColors.toolTipText
                                AkTheme.palette.disabled.toolTipBase: disabledColors.toolTipBase
                                AkTheme.palette.disabled.link: disabledColors.link
                                AkTheme.palette.disabled.linkVisited: disabledColors.linkVisited
                            }
                            Switch {
                                enabled: glyControls.controlEnabled

                                AkTheme.palette.active.highlightedText: btnHighlightedText.currentColor
                                AkTheme.palette.active.highlight: btnHighlight.currentColor
                                AkTheme.palette.active.text: btnText.currentColor
                                AkTheme.palette.active.placeholderText: btnPlaceholderText.currentColor
                                AkTheme.palette.active.base: btnBase.currentColor
                                AkTheme.palette.active.alternateBase: btnAlternateBase.currentColor
                                AkTheme.palette.active.windowText: btnWindowText.currentColor
                                AkTheme.palette.active.window: btnWindow.currentColor
                                AkTheme.palette.active.buttonText: btnButtonText.currentColor
                                AkTheme.palette.active.light: btnLight.currentColor
                                AkTheme.palette.active.midlight: btnMidlight.currentColor
                                AkTheme.palette.active.button: btnButton.currentColor
                                AkTheme.palette.active.mid: btnMid.currentColor
                                AkTheme.palette.active.dark: btnDark.currentColor
                                AkTheme.palette.active.shadow: btnShadow.currentColor
                                AkTheme.palette.active.toolTipText: btnToolTipText.currentColor
                                AkTheme.palette.active.toolTipBase: btnToolTipBase.currentColor
                                AkTheme.palette.active.link: btnLink.currentColor
                                AkTheme.palette.active.linkVisited: btnLinkVisited.currentColor

                                AkTheme.palette.disabled.highlightedText: disabledColors.highlightedText
                                AkTheme.palette.disabled.highlight: disabledColors.highlight
                                AkTheme.palette.disabled.text: disabledColors.text
                                AkTheme.palette.disabled.placeholderText: disabledColors.placeholderText
                                AkTheme.palette.disabled.base: disabledColors.base
                                AkTheme.palette.disabled.alternateBase: disabledColors.alternateBase
                                AkTheme.palette.disabled.windowText: disabledColors.windowText
                                AkTheme.palette.disabled.window: disabledColors.window
                                AkTheme.palette.disabled.buttonText: disabledColors.buttonText
                                AkTheme.palette.disabled.light: disabledColors.light
                                AkTheme.palette.disabled.midlight: disabledColors.midlight
                                AkTheme.palette.disabled.button: disabledColors.button
                                AkTheme.palette.disabled.mid: disabledColors.mid
                                AkTheme.palette.disabled.dark: disabledColors.dark
                                AkTheme.palette.disabled.shadow: disabledColors.shadow
                                AkTheme.palette.disabled.toolTipText: disabledColors.toolTipText
                                AkTheme.palette.disabled.toolTipBase: disabledColors.toolTipBase
                                AkTheme.palette.disabled.link: disabledColors.link
                                AkTheme.palette.disabled.linkVisited: disabledColors.linkVisited
                            }
                        }
                        RowLayout {
                            Slider {
                                value: glyControls.controlValue
                                from: glyControls.controlMinValue
                                to: glyControls.controlMaxValue
                                stepSize: glyControls.controlStep
                                enabled: glyControls.controlEnabled
                                Layout.fillWidth: true

                                AkTheme.palette.active.highlightedText: btnHighlightedText.currentColor
                                AkTheme.palette.active.highlight: btnHighlight.currentColor
                                AkTheme.palette.active.text: btnText.currentColor
                                AkTheme.palette.active.placeholderText: btnPlaceholderText.currentColor
                                AkTheme.palette.active.base: btnBase.currentColor
                                AkTheme.palette.active.alternateBase: btnAlternateBase.currentColor
                                AkTheme.palette.active.windowText: btnWindowText.currentColor
                                AkTheme.palette.active.window: btnWindow.currentColor
                                AkTheme.palette.active.buttonText: btnButtonText.currentColor
                                AkTheme.palette.active.light: btnLight.currentColor
                                AkTheme.palette.active.midlight: btnMidlight.currentColor
                                AkTheme.palette.active.button: btnButton.currentColor
                                AkTheme.palette.active.mid: btnMid.currentColor
                                AkTheme.palette.active.dark: btnDark.currentColor
                                AkTheme.palette.active.shadow: btnShadow.currentColor
                                AkTheme.palette.active.toolTipText: btnToolTipText.currentColor
                                AkTheme.palette.active.toolTipBase: btnToolTipBase.currentColor
                                AkTheme.palette.active.link: btnLink.currentColor
                                AkTheme.palette.active.linkVisited: btnLinkVisited.currentColor

                                AkTheme.palette.disabled.highlightedText: disabledColors.highlightedText
                                AkTheme.palette.disabled.highlight: disabledColors.highlight
                                AkTheme.palette.disabled.text: disabledColors.text
                                AkTheme.palette.disabled.placeholderText: disabledColors.placeholderText
                                AkTheme.palette.disabled.base: disabledColors.base
                                AkTheme.palette.disabled.alternateBase: disabledColors.alternateBase
                                AkTheme.palette.disabled.windowText: disabledColors.windowText
                                AkTheme.palette.disabled.window: disabledColors.window
                                AkTheme.palette.disabled.buttonText: disabledColors.buttonText
                                AkTheme.palette.disabled.light: disabledColors.light
                                AkTheme.palette.disabled.midlight: disabledColors.midlight
                                AkTheme.palette.disabled.button: disabledColors.button
                                AkTheme.palette.disabled.mid: disabledColors.mid
                                AkTheme.palette.disabled.dark: disabledColors.dark
                                AkTheme.palette.disabled.shadow: disabledColors.shadow
                                AkTheme.palette.disabled.toolTipText: disabledColors.toolTipText
                                AkTheme.palette.disabled.toolTipBase: disabledColors.toolTipBase
                                AkTheme.palette.disabled.link: disabledColors.link
                                AkTheme.palette.disabled.linkVisited: disabledColors.linkVisited

                                onValueChanged: glyControls.controlValue = value
                            }
                            SpinBox {
                                value: glyControls.controlValue
                                from: glyControls.controlMinValue
                                to: glyControls.controlMaxValue
                                stepSize: glyControls.controlStep
                                enabled: glyControls.controlEnabled

                                AkTheme.palette.active.highlightedText: btnHighlightedText.currentColor
                                AkTheme.palette.active.highlight: btnHighlight.currentColor
                                AkTheme.palette.active.text: btnText.currentColor
                                AkTheme.palette.active.placeholderText: btnPlaceholderText.currentColor
                                AkTheme.palette.active.base: btnBase.currentColor
                                AkTheme.palette.active.alternateBase: btnAlternateBase.currentColor
                                AkTheme.palette.active.windowText: btnWindowText.currentColor
                                AkTheme.palette.active.window: btnWindow.currentColor
                                AkTheme.palette.active.buttonText: btnButtonText.currentColor
                                AkTheme.palette.active.light: btnLight.currentColor
                                AkTheme.palette.active.midlight: btnMidlight.currentColor
                                AkTheme.palette.active.button: btnButton.currentColor
                                AkTheme.palette.active.mid: btnMid.currentColor
                                AkTheme.palette.active.dark: btnDark.currentColor
                                AkTheme.palette.active.shadow: btnShadow.currentColor
                                AkTheme.palette.active.toolTipText: btnToolTipText.currentColor
                                AkTheme.palette.active.toolTipBase: btnToolTipBase.currentColor
                                AkTheme.palette.active.link: btnLink.currentColor
                                AkTheme.palette.active.linkVisited: btnLinkVisited.currentColor

                                AkTheme.palette.disabled.highlightedText: disabledColors.highlightedText
                                AkTheme.palette.disabled.highlight: disabledColors.highlight
                                AkTheme.palette.disabled.text: disabledColors.text
                                AkTheme.palette.disabled.placeholderText: disabledColors.placeholderText
                                AkTheme.palette.disabled.base: disabledColors.base
                                AkTheme.palette.disabled.alternateBase: disabledColors.alternateBase
                                AkTheme.palette.disabled.windowText: disabledColors.windowText
                                AkTheme.palette.disabled.window: disabledColors.window
                                AkTheme.palette.disabled.buttonText: disabledColors.buttonText
                                AkTheme.palette.disabled.light: disabledColors.light
                                AkTheme.palette.disabled.midlight: disabledColors.midlight
                                AkTheme.palette.disabled.button: disabledColors.button
                                AkTheme.palette.disabled.mid: disabledColors.mid
                                AkTheme.palette.disabled.dark: disabledColors.dark
                                AkTheme.palette.disabled.shadow: disabledColors.shadow
                                AkTheme.palette.disabled.toolTipText: disabledColors.toolTipText
                                AkTheme.palette.disabled.toolTipBase: disabledColors.toolTipBase
                                AkTheme.palette.disabled.link: disabledColors.link
                                AkTheme.palette.disabled.linkVisited: disabledColors.linkVisited

                                onValueChanged: glyControls.controlValue = value
                            }
                        }
                        Label {
                            text: qsTr("Progress %1%").arg(glyControls.controlValue)
                            enabled: glyControls.controlEnabled
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            AkTheme.palette.active.highlightedText: btnHighlightedText.currentColor
                            AkTheme.palette.active.highlight: btnHighlight.currentColor
                            AkTheme.palette.active.text: btnText.currentColor
                            AkTheme.palette.active.placeholderText: btnPlaceholderText.currentColor
                            AkTheme.palette.active.base: btnBase.currentColor
                            AkTheme.palette.active.alternateBase: btnAlternateBase.currentColor
                            AkTheme.palette.active.windowText: btnWindowText.currentColor
                            AkTheme.palette.active.window: btnWindow.currentColor
                            AkTheme.palette.active.buttonText: btnButtonText.currentColor
                            AkTheme.palette.active.light: btnLight.currentColor
                            AkTheme.palette.active.midlight: btnMidlight.currentColor
                            AkTheme.palette.active.button: btnButton.currentColor
                            AkTheme.palette.active.mid: btnMid.currentColor
                            AkTheme.palette.active.dark: btnDark.currentColor
                            AkTheme.palette.active.shadow: btnShadow.currentColor
                            AkTheme.palette.active.toolTipText: btnToolTipText.currentColor
                            AkTheme.palette.active.toolTipBase: btnToolTipBase.currentColor
                            AkTheme.palette.active.link: btnLink.currentColor
                            AkTheme.palette.active.linkVisited: btnLinkVisited.currentColor

                            AkTheme.palette.disabled.highlightedText: disabledColors.highlightedText
                            AkTheme.palette.disabled.highlight: disabledColors.highlight
                            AkTheme.palette.disabled.text: disabledColors.text
                            AkTheme.palette.disabled.placeholderText: disabledColors.placeholderText
                            AkTheme.palette.disabled.base: disabledColors.base
                            AkTheme.palette.disabled.alternateBase: disabledColors.alternateBase
                            AkTheme.palette.disabled.windowText: disabledColors.windowText
                            AkTheme.palette.disabled.window: disabledColors.window
                            AkTheme.palette.disabled.buttonText: disabledColors.buttonText
                            AkTheme.palette.disabled.light: disabledColors.light
                            AkTheme.palette.disabled.midlight: disabledColors.midlight
                            AkTheme.palette.disabled.button: disabledColors.button
                            AkTheme.palette.disabled.mid: disabledColors.mid
                            AkTheme.palette.disabled.dark: disabledColors.dark
                            AkTheme.palette.disabled.shadow: disabledColors.shadow
                            AkTheme.palette.disabled.toolTipText: disabledColors.toolTipText
                            AkTheme.palette.disabled.toolTipBase: disabledColors.toolTipBase
                            AkTheme.palette.disabled.link: disabledColors.link
                            AkTheme.palette.disabled.linkVisited: disabledColors.linkVisited
                        }
                        ProgressBar {
                            value: glyControls.controlValue
                            from: glyControls.controlMinValue
                            to: glyControls.controlMaxValue
                            enabled: glyControls.controlEnabled
                            Layout.fillWidth: true

                            AkTheme.palette.active.highlightedText: btnHighlightedText.currentColor
                            AkTheme.palette.active.highlight: btnHighlight.currentColor
                            AkTheme.palette.active.text: btnText.currentColor
                            AkTheme.palette.active.placeholderText: btnPlaceholderText.currentColor
                            AkTheme.palette.active.base: btnBase.currentColor
                            AkTheme.palette.active.alternateBase: btnAlternateBase.currentColor
                            AkTheme.palette.active.windowText: btnWindowText.currentColor
                            AkTheme.palette.active.window: btnWindow.currentColor
                            AkTheme.palette.active.buttonText: btnButtonText.currentColor
                            AkTheme.palette.active.light: btnLight.currentColor
                            AkTheme.palette.active.midlight: btnMidlight.currentColor
                            AkTheme.palette.active.button: btnButton.currentColor
                            AkTheme.palette.active.mid: btnMid.currentColor
                            AkTheme.palette.active.dark: btnDark.currentColor
                            AkTheme.palette.active.shadow: btnShadow.currentColor
                            AkTheme.palette.active.toolTipText: btnToolTipText.currentColor
                            AkTheme.palette.active.toolTipBase: btnToolTipBase.currentColor
                            AkTheme.palette.active.link: btnLink.currentColor
                            AkTheme.palette.active.linkVisited: btnLinkVisited.currentColor

                            AkTheme.palette.disabled.highlightedText: disabledColors.highlightedText
                            AkTheme.palette.disabled.highlight: disabledColors.highlight
                            AkTheme.palette.disabled.text: disabledColors.text
                            AkTheme.palette.disabled.placeholderText: disabledColors.placeholderText
                            AkTheme.palette.disabled.base: disabledColors.base
                            AkTheme.palette.disabled.alternateBase: disabledColors.alternateBase
                            AkTheme.palette.disabled.windowText: disabledColors.windowText
                            AkTheme.palette.disabled.window: disabledColors.window
                            AkTheme.palette.disabled.buttonText: disabledColors.buttonText
                            AkTheme.palette.disabled.light: disabledColors.light
                            AkTheme.palette.disabled.midlight: disabledColors.midlight
                            AkTheme.palette.disabled.button: disabledColors.button
                            AkTheme.palette.disabled.mid: disabledColors.mid
                            AkTheme.palette.disabled.dark: disabledColors.dark
                            AkTheme.palette.disabled.shadow: disabledColors.shadow
                            AkTheme.palette.disabled.toolTipText: disabledColors.toolTipText
                            AkTheme.palette.disabled.toolTipBase: disabledColors.toolTipBase
                            AkTheme.palette.disabled.link: disabledColors.link
                            AkTheme.palette.disabled.linkVisited: disabledColors.linkVisited
                        }
                    }
                }
            }
            StackLayout {
                currentIndex: tabBar.currentIndex
                clip: true
                Layout.fillWidth: true

                Page {
                    id: activeColors
                    Layout.fillWidth: true

                    ColumnLayout {
                        width: parent.width

                        AK.LabeledComboBox {
                            id: cbxColorGroups
                            label: qsTr("Color group")
                            model: [qsTr("Window"),
                                    qsTr("Buttons"),
                                    qsTr("Input Controls"),
                                    qsTr("Control accents"),
                                    qsTr("Highlight"),
                                    qsTr("Tooltips"),
                                    qsTr("Hyperlinks")]
                            Layout.fillWidth: true
                        }
                        StackLayout {
                            currentIndex: cbxColorGroups.currentIndex
                            Layout.fillWidth: true

                            Page {
                                width: parent.width

                                ColumnLayout {
                                    width: parent.width
                                    layoutDirection: addEdit.rtl? Qt.RightToLeft: Qt.LeftToRight

                                    AK.ColorButton {
                                        id: btnWindowText
                                        text: qsTr("Window text")
                                        currentColor: AkTheme.palette.active.windowText
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnWindow
                                        text: qsTr("Window")
                                        currentColor: AkTheme.palette.active.window
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                            Page {
                                width: parent.width

                                ColumnLayout {
                                    width: parent.width
                                    layoutDirection: addEdit.rtl? Qt.RightToLeft: Qt.LeftToRight

                                    AK.ColorButton {
                                        id: btnButtonText
                                        text: qsTr("Button text")
                                        currentColor: AkTheme.palette.active.buttonText
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnButton
                                        text: qsTr("Button")
                                        currentColor: AkTheme.palette.active.button
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                            Page {
                                width: parent.width

                                ColumnLayout {
                                    width: parent.width
                                    layoutDirection: addEdit.rtl? Qt.RightToLeft: Qt.LeftToRight

                                    AK.ColorButton {
                                        id: btnText
                                        text: qsTr("Text")
                                        currentColor: AkTheme.palette.active.text
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnPlaceholderText
                                        text: qsTr("Placeholder text")
                                        currentColor: AkTheme.palette.active.placeholderText
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnBase
                                        text: qsTr("Base")
                                        currentColor: AkTheme.palette.active.base
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnAlternateBase
                                        text: qsTr("Alternate base")
                                        currentColor: AkTheme.palette.active.alternateBase
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                            Page {
                                width: parent.width

                                ColumnLayout {
                                    width: parent.width
                                    layoutDirection: addEdit.rtl? Qt.RightToLeft: Qt.LeftToRight

                                    AK.ColorButton {
                                        id: btnLight
                                        text: qsTr("Light")
                                        currentColor: AkTheme.palette.active.light
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnMidlight
                                        text: qsTr("Mid light")
                                        currentColor: AkTheme.palette.active.midlight
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnMid
                                        text: qsTr("Mid")
                                        currentColor: AkTheme.palette.active.mid
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnDark
                                        text: qsTr("Dark")
                                        currentColor: AkTheme.palette.active.dark
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnShadow
                                        text: qsTr("Shadow")
                                        currentColor: AkTheme.palette.active.shadow
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                            Page {
                                width: parent.width

                                ColumnLayout {
                                    width: parent.width
                                    layoutDirection: addEdit.rtl? Qt.RightToLeft: Qt.LeftToRight

                                    AK.ColorButton {
                                        id: btnHighlightedText
                                        text: qsTr("Highlighted text")
                                        currentColor: AkTheme.palette.active.highlightedText
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnHighlight
                                        text: qsTr("Highlight")
                                        currentColor: AkTheme.palette.active.highlight
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                            Page {
                                width: parent.width

                                ColumnLayout {
                                    width: parent.width
                                    layoutDirection: addEdit.rtl? Qt.RightToLeft: Qt.LeftToRight

                                    AK.ColorButton {
                                        id: btnToolTipText
                                        text: qsTr("Tooltip text")
                                        currentColor: AkTheme.palette.active.toolTipText
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnToolTipBase
                                        text: qsTr("ToolTip base")
                                        currentColor: AkTheme.palette.active.toolTipBase
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                            Page {
                                width: parent.width

                                ColumnLayout {
                                    width: parent.width
                                    layoutDirection: addEdit.rtl? Qt.RightToLeft: Qt.LeftToRight

                                    AK.ColorButton {
                                        id: btnLink
                                        text: qsTr("Link")
                                        currentColor: AkTheme.palette.active.link
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnLinkVisited
                                        text: qsTr("Visited link")
                                        currentColor: AkTheme.palette.active.linkVisited
                                        title: qsTr("Choose the color for %1").arg(text)
                                        horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }
                }
                Page {
                    id: disabledColors
                    Layout.fillWidth: true

                    property color highlightedText: applyEffect(btnHighlightedText.currentColor)
                    property color highlight: applyEffect(btnHighlight.currentColor)
                    property color text: applyEffect(btnText.currentColor)
                    property color placeholderText: applyEffect(btnPlaceholderText.currentColor)
                    property color base: applyEffect(btnBase.currentColor)
                    property color alternateBase: applyEffect(btnAlternateBase.currentColor)
                    property color windowText: applyEffect(btnWindowText.currentColor)
                    property color window: applyEffect(btnWindow.currentColor)
                    property color buttonText: applyEffect(btnButtonText.currentColor)
                    property color light: applyEffect(btnLight.currentColor)
                    property color midlight: applyEffect(btnMidlight.currentColor)
                    property color button: applyEffect(btnButton.currentColor)
                    property color mid: applyEffect(btnMid.currentColor)
                    property color dark: applyEffect(btnDark.currentColor)
                    property color shadow: applyEffect(btnShadow.currentColor)
                    property color toolTipText: applyEffect(btnToolTipText.currentColor)
                    property color toolTipBase: applyEffect(btnToolTipBase.currentColor)
                    property color link: applyEffect(btnLink.currentColor)
                    property color linkVisited: applyEffect(btnLinkVisited.currentColor)

                    function brightness(c)
                    {
                        if (glyDisabled.brightness < 0)
                            return Math.min(Math.max(0, c * (glyDisabled.brightness + 1)), 1)

                        return Math.min(Math.max(0, glyDisabled.brightness * (1 - c) + c), 1)
                    }

                    function contrast(c, q)
                    {
                        if (glyDisabled.contrast < 0)
                            return Math.min(Math.max(0, (glyDisabled.contrast + 1) * (c - q) + q), 1)

                        let r = c < 0.5? 0: 1

                        return Math.min(Math.max(0, glyDisabled.contrast * (r - c) + c), 1)
                    }

                    function saturation(c, l)
                    {
                        return Math.min(Math.max(0, (glyDisabled.saturation + 1) * (c - l) + l), 1)
                    }

                    function applyEffect(color)
                    {
                        let c = Qt.rgba(color.r, color.g, color.b, color.a);

                        let l = c.hslLightness
                        c.r = saturation(c.r, l)
                        c.g = saturation(c.g, l)
                        c.b = saturation(c.b, l)

                        c.r = contrast(c.r, btnContrastColor.currentColor.r)
                        c.g = contrast(c.g, btnContrastColor.currentColor.g)
                        c.b = contrast(c.b, btnContrastColor.currentColor.b)

                        c.r = brightness(c.r)
                        c.g = brightness(c.g)
                        c.b = brightness(c.b)

                        return c;
                    }

                    ColumnLayout {
                        id: glyDisabled
                        width: parent.width
                        layoutDirection: addEdit.rtl? Qt.RightToLeft: Qt.LeftToRight

                        property real brightness: -0.25
                        property real contrast: -0.5
                        property real saturation: -1
                        property real controlMinValue: -1
                        property real controlMaxValue: 1
                        property real controlStep: 0.1

                        Label {
                            text: qsTr("Brightness")
                            font.bold: true
                            Layout.fillWidth: true
                        }
                        AK.StickySlider {
                            id: sldBrightness
                            value: glyDisabled.brightness
                            from: glyDisabled.controlMinValue
                            to: glyDisabled.controlMaxValue
                            stepSize: glyDisabled.controlStep
                            stickyPoints: [0]
                            Layout.fillWidth: true

                            onValueChanged: glyDisabled.brightness = value
                        }
                        Label {
                            text: qsTr("Contrast")
                            font.bold: true
                            Layout.fillWidth: true
                        }
                        AK.StickySlider {
                            id: sldContrast
                            value: glyDisabled.contrast
                            from: glyDisabled.controlMinValue
                            to: glyDisabled.controlMaxValue
                            stepSize: glyDisabled.controlStep
                            stickyPoints: [0]
                            Layout.fillWidth: true

                            onValueChanged: glyDisabled.contrast = value
                        }
                        AK.ColorButton {
                            id: btnContrastColor
                            text: qsTr("Contrast color")
                            currentColor: Qt.rgba(0.5, 0.5, 0.5)
                            title: qsTr("Choose the contrast color")
                            horizontalAlignment: addEdit.rtl? Text.AlignRight: Text.AlignLeft
                            Layout.fillWidth: true
                        }
                        Label {
                            text: qsTr("Saturation")
                            font.bold: true
                            Layout.fillWidth: true
                        }
                        AK.StickySlider {
                            id: sldSaturation
                            value: glyDisabled.saturation
                            from: glyDisabled.controlMinValue
                            to: glyDisabled.controlMaxValue
                            stepSize: glyDisabled.controlStep
                            stickyPoints: [0]
                            Layout.fillWidth: true

                            onValueChanged: glyDisabled.saturation = value
                        }
                    }
                }
            }
        }
    }

    onAccepted: {
        let pal = AkPalette.create(paletteName.text)
        pal.active.highlightedText = btnHighlightedText.currentColor
        pal.active.highlight = btnHighlight.currentColor
        pal.active.text = btnText.currentColor
        pal.active.placeholderText = btnPlaceholderText.currentColor
        pal.active.base = btnBase.currentColor
        pal.active.alternateBase = btnAlternateBase.currentColor
        pal.active.windowText = btnWindowText.currentColor
        pal.active.window = btnWindow.currentColor
        pal.active.buttonText = btnButtonText.currentColor
        pal.active.light = btnLight.currentColor
        pal.active.midlight = btnMidlight.currentColor
        pal.active.button = btnButton.currentColor
        pal.active.mid = btnMid.currentColor
        pal.active.dark = btnDark.currentColor
        pal.active.shadow = btnShadow.currentColor
        pal.active.toolTipText = btnToolTipText.currentColor
        pal.active.toolTipBase = btnToolTipBase.currentColor
        pal.active.link = btnLink.currentColor
        pal.active.linkVisited = btnLinkVisited.currentColor

        pal.disabled.highlightedText = disabledColors.highlightedText
        pal.disabled.highlight = disabledColors.highlight
        pal.disabled.text = disabledColors.text
        pal.disabled.placeholderText = disabledColors.placeholderText
        pal.disabled.base = disabledColors.base
        pal.disabled.alternateBase = disabledColors.alternateBase
        pal.disabled.windowText = disabledColors.windowText
        pal.disabled.window = disabledColors.window
        pal.disabled.buttonText = disabledColors.buttonText
        pal.disabled.light = disabledColors.light
        pal.disabled.midlight = disabledColors.midlight
        pal.disabled.button = disabledColors.button
        pal.disabled.mid = disabledColors.mid
        pal.disabled.dark = disabledColors.dark
        pal.disabled.shadow = disabledColors.shadow
        pal.disabled.toolTipText = disabledColors.toolTipText
        pal.disabled.toolTipBase = disabledColors.toolTipBase
        pal.disabled.link = disabledColors.link
        pal.disabled.linkVisited = disabledColors.linkVisited

        pal.save(paletteName.text);
        addEdit.paletteUpdated(paletteName.text)
    }
    onReset: initializePalette()
}

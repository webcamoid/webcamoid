/* Webcamoid, webcam capture application.
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

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: qsTr("Color group")
                            }
                            ComboBox {
                                id: cbxColorGroups
                                model: [qsTr("Window"),
                                        qsTr("Buttons"),
                                        qsTr("Input Controls"),
                                        qsTr("Control accents"),
                                        qsTr("Highlight"),
                                        qsTr("Tooltips"),
                                        qsTr("Hyperlinks")]
                                Layout.fillWidth: true
                            }
                        }
                        StackLayout {
                            currentIndex: cbxColorGroups.currentIndex
                            Layout.fillWidth: true

                            Page {
                                Layout.fillWidth: true

                                GridLayout {
                                    columns: 2
                                    width: parent.width

                                    Label {
                                        id: lblWindowText
                                        text: qsTr("Window text")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnWindowText
                                        currentColor: AkTheme.palette.active.windowText
                                        title: qsTr("Choose the color for %1").arg(lblWindowText.text)
                                        Accessible.description: lblWindowText.text
                                    }
                                    Label {
                                        id: lblWindow
                                        text: qsTr("Window")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnWindow
                                        currentColor: AkTheme.palette.active.window
                                        title: qsTr("Choose the color for %1").arg(lblWindow.text)
                                        Accessible.description: lblWindow.text
                                    }
                                }
                            }
                            Page {
                                Layout.fillWidth: true

                                GridLayout {
                                    columns: 2
                                    width: parent.width

                                    Label {
                                        id: lblButtonText
                                        text: qsTr("Button text")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnButtonText
                                        currentColor: AkTheme.palette.active.buttonText
                                        title: qsTr("Choose the color for %1").arg(lblButtonText.text)
                                        Accessible.description: lblButtonText.text
                                    }
                                    Label {
                                        id: lblButton
                                        text: qsTr("Button")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnButton
                                        currentColor: AkTheme.palette.active.button
                                        title: qsTr("Choose the color for %1").arg(lblButton.text)
                                        Accessible.description: lblButton.text
                                    }
                                }
                            }
                            Page {
                                Layout.fillWidth: true

                                GridLayout {
                                    columns: 2
                                    width: parent.width

                                    Label {
                                        id: lblText
                                        text: qsTr("Text")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnText
                                        currentColor: AkTheme.palette.active.text
                                        title: qsTr("Choose the color for %1").arg(lblText.text)
                                        Accessible.description: lblText.text
                                    }
                                    Label {
                                        id: lblPlaceholderText
                                        text: qsTr("Placeholder text")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnPlaceholderText
                                        currentColor: AkTheme.palette.active.placeholderText
                                        title: qsTr("Choose the color for %1").arg(lblPlaceholderText.text)
                                        Accessible.description: lblPlaceholderText.text
                                    }
                                    Label {
                                        id: lblBase
                                        text: qsTr("Base")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnBase
                                        currentColor: AkTheme.palette.active.base
                                        title: qsTr("Choose the color for %1").arg(lblBase.text)
                                        Accessible.description: lblBase.text
                                    }
                                    Label {
                                        id: lblAlternateBase
                                        text: qsTr("Alternate base")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnAlternateBase
                                        currentColor: AkTheme.palette.active.alternateBase
                                        title: qsTr("Choose the color for %1").arg(lblAlternateBase.text)
                                        Accessible.description: lblAlternateBase.text
                                    }
                                }
                            }
                            Page {
                                Layout.fillWidth: true

                                GridLayout {
                                    columns: 2
                                    width: parent.width

                                    Label {
                                        id: lblLight
                                        text: qsTr("Light")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnLight
                                        currentColor: AkTheme.palette.active.light
                                        title: qsTr("Choose the color for %1").arg(lblLight.text)
                                        Accessible.description: lblLight.text
                                    }
                                    Label {
                                        id: lblMidLight
                                        text: qsTr("Mid light")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnMidlight
                                        currentColor: AkTheme.palette.active.midlight
                                        title: qsTr("Choose the color for %1").arg(lblMidLight.text)
                                        Accessible.description: lblMidLight.text
                                    }
                                    Label {
                                        id: lblMid
                                        text: qsTr("Mid")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnMid
                                        currentColor: AkTheme.palette.active.mid
                                        title: qsTr("Choose the color for %1").arg(lblMid.text)
                                        Accessible.description: lblMid.text
                                    }
                                    Label {
                                        id: lblDark
                                        text: qsTr("Dark")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnDark
                                        currentColor: AkTheme.palette.active.dark
                                        title: qsTr("Choose the color for %1").arg(lblDark.text)
                                        Accessible.description: lblDark.text
                                    }
                                    Label {
                                        id: lblShadow
                                        text: qsTr("Shadow")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnShadow
                                        currentColor: AkTheme.palette.active.shadow
                                        title: qsTr("Choose the color for %1").arg(lblShadow.text)
                                        Accessible.description: lblShadow.text
                                    }
                                }
                            }
                            Page {
                                Layout.fillWidth: true

                                GridLayout {
                                    columns: 2
                                    width: parent.width

                                    Label {
                                        id: lblHighlightedText
                                        text: qsTr("Highlighted text")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnHighlightedText
                                        currentColor: AkTheme.palette.active.highlightedText
                                        title: qsTr("Choose the color for %1").arg(lblHighlightedText.text)
                                        Accessible.description: lblHighlightedText.text
                                    }
                                    Label {
                                        id: lblHighlight
                                        text: qsTr("Highlight")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnHighlight
                                        currentColor: AkTheme.palette.active.highlight
                                        title: qsTr("Choose the color for %1").arg(lblHighlight.text)
                                        Accessible.description: lblHighlight.text
                                    }
                                }
                            }
                            Page {
                                Layout.fillWidth: true

                                GridLayout {
                                    columns: 2
                                    width: parent.width

                                    Label {
                                        id: lblTooltipText
                                        text: qsTr("Tooltip text")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnToolTipText
                                        currentColor: AkTheme.palette.active.toolTipText
                                        title: qsTr("Choose the color for %1").arg(lblTooltipText.text)
                                        Accessible.description: lblTooltipText.text
                                    }
                                    Label {
                                        id: lblTooltipBase
                                        text: qsTr("ToolTip base")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnToolTipBase
                                        currentColor: AkTheme.palette.active.toolTipBase
                                        title: qsTr("Choose the color for %1").arg(lblTooltipBase.text)
                                        Accessible.description: lblTooltipBase.text
                                    }
                                }
                            }
                            Page {
                                Layout.fillWidth: true

                                GridLayout {
                                    columns: 2
                                    width: parent.width

                                    Label {
                                        id: lblLink
                                        text: qsTr("Link")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnLink
                                        currentColor: AkTheme.palette.active.link
                                        title: qsTr("Choose the color for %1").arg(lblLink.text)
                                        Accessible.description: lblLink.text
                                    }
                                    Label {
                                        id: lblVisitedLink
                                        text: qsTr("Visited link")
                                        Layout.fillWidth: true
                                    }
                                    AK.ColorButton {
                                        id: btnLinkVisited
                                        currentColor: AkTheme.palette.active.linkVisited
                                        title: qsTr("Choose the color for %1").arg(lblVisitedLink.text)
                                        Accessible.description: lblVisitedLink.text
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

                    GridLayout {
                        id: glyDisabled
                        columns: 4
                        width: parent.width

                        property real brightness: -0.25
                        property real contrast: -0.5
                        property real saturation: -1
                        property real controlMinValue: -1
                        property real controlMaxValue: 1
                        property real controlStep: 0.1

                        Label {
                            text: qsTr("Brightness")
                        }
                        Slider {
                            id: sldBrightness
                            value: glyDisabled.brightness
                            from: glyDisabled.controlMinValue
                            to: glyDisabled.controlMaxValue
                            stepSize: glyDisabled.controlStep
                            Layout.fillWidth: true

                            onValueChanged: glyDisabled.brightness = value
                        }
                        SpinBox {
                            id: spbBrightness
                            value: multiplier * glyDisabled.brightness
                            from: multiplier * glyDisabled.controlMinValue
                            to: multiplier * glyDisabled.controlMaxValue
                            stepSize: multiplier * glyDisabled.controlStep
                            editable: true

                            readonly property int decimals: 2
                            readonly property int multiplier: Math.pow(10, decimals)

                            validator: DoubleValidator {
                                bottom: Math.min(spbBrightness.from, spbBrightness.to)
                                top:  Math.max(spbBrightness.from, spbBrightness.to)
                            }
                            textFromValue: function(value, locale) {
                                return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
                            }
                            valueFromText: function(text, locale) {
                                return Number.fromLocaleString(locale, text) * multiplier
                            }
                            onValueModified: glyDisabled.brightness = value / multiplier
                        }
                        Label {
                        }
                        Label {
                            text: qsTr("Contrast")
                        }
                        Slider {
                            id: sldContrast
                            value: glyDisabled.contrast
                            from: glyDisabled.controlMinValue
                            to: glyDisabled.controlMaxValue
                            stepSize: glyDisabled.controlStep
                            Layout.fillWidth: true

                            onValueChanged: glyDisabled.contrast = value
                        }
                        SpinBox {
                            id: spbContrast
                            value: multiplier * glyDisabled.contrast
                            from: multiplier * glyDisabled.controlMinValue
                            to: multiplier * glyDisabled.controlMaxValue
                            stepSize: multiplier * glyDisabled.controlStep
                            editable: true

                            readonly property int decimals: 2
                            readonly property int multiplier: Math.pow(10, decimals)

                            validator: DoubleValidator {
                                bottom: Math.min(spbContrast.from, spbContrast.to)
                                top:  Math.max(spbContrast.from, spbContrast.to)
                            }
                            textFromValue: function(value, locale) {
                                return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
                            }
                            valueFromText: function(text, locale) {
                                return Number.fromLocaleString(locale, text) * multiplier
                            }
                            onValueModified: glyDisabled.contrast = value / multiplier
                        }
                        AK.ColorButton {
                            id: btnContrastColor
                            currentColor: Qt.rgba(0.5, 0.5, 0.5)
                            title: qsTr("Choose the contrast color")
                        }
                        Label {
                            text: qsTr("Saturation")
                        }
                        Slider {
                            id: sldSaturation
                            value: glyDisabled.saturation
                            from: glyDisabled.controlMinValue
                            to: glyDisabled.controlMaxValue
                            stepSize: glyDisabled.controlStep
                            Layout.fillWidth: true

                            onValueChanged: glyDisabled.saturation = value
                        }
                        SpinBox {
                            id: spbSaturation
                            value: multiplier * glyDisabled.saturation
                            from: multiplier * glyDisabled.controlMinValue
                            to: multiplier * glyDisabled.controlMaxValue
                            stepSize: multiplier * glyDisabled.controlStep
                            editable: true

                            readonly property int decimals: 2
                            readonly property int multiplier: Math.pow(10, decimals)

                            validator: DoubleValidator {
                                bottom: Math.min(spbSaturation.from, spbSaturation.to)
                                top:  Math.max(spbSaturation.from, spbSaturation.to)
                            }
                            textFromValue: function(value, locale) {
                                return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
                            }
                            valueFromText: function(text, locale) {
                                return Number.fromLocaleString(locale, text) * multiplier
                            }
                            onValueModified: glyDisabled.saturation = value / multiplier
                        }
                        Label {
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

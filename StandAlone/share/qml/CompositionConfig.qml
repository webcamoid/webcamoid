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

AK.MenuOption {
    id: root
    title: qsTr("Composition")
    subtitle: qsTr("Configure the canvas, the output resolution, and the frame rate.")
    icon: "image://icons/screen"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    // Canvas presets (landscape). Portrait simply swaps w/h.
    readonly property var canvasPresets: [
        { width: 640,  height: 360,  fps: 30},
        { width: 854,  height: 480,  fps: 30},
        { width: 1280, height: 720,  fps: 30},
        { width: 1280, height: 720,  fps: 60},
        { width: 1920, height: 1080, fps: 30},
        { width: 1920, height: 1080, fps: 60},
        { width: 2560, height: 1440, fps: 30},
        { width: 2560, height: 1440, fps: 60},
        { width: 3840, height: 2160, fps: 30},
        { width: 3840, height: 2160, fps: 60},
    ]

    // Standard output resolutions, expressed as the short side of the frame
    // (720 means 1280x720 in landscape, and 720x1280 in portrait). The
    // output always keeps the orientation and aspect ratio of the canvas, so
    // only its scale can be changed.
    readonly property var standardOutputResolutions:
        [240, 360, 480, 720, 1080, 1440, 2160]

    // Current canvas caps from the compositor.
    readonly property var currentCaps: AkVideoCaps.create(videoEffects.canvasCaps)
    readonly property int currentFps: Math.round(AkFrac.create(currentCaps.fps).value)
    readonly property bool isPortrait: cbxOrientation.currentIndex != 0

    // Effective dimensions (swapped for portrait display).
    readonly property int effectiveWidth: isPortrait? currentCaps.height: currentCaps.width
    readonly property int effectiveHeight: isPortrait? currentCaps.width: currentCaps.height

    // Find the preset index that matches current effective dimensions + FPS, or -1.
    readonly property int matchedPresetIndex: {
        for (let i = 0; i < canvasPresets.length; i++) {
            let p = canvasPresets[i]
            let pw = isPortrait? p.height: p.width
            let ph = isPortrait? p.width: p.height

            if (effectiveWidth === pw
                && effectiveHeight === ph
                && currentFps === p.fps)
                return i
        }

        return -1
    }

    // Guard to prevent re-entrant loops when applying presets.
    property bool updatingFromPreset: false

    // The output can't be bigger than the canvas, so only the standard
    // resolutions smaller than the canvas are offered, plus the last stop
    // (0) that means "same as the canvas".
    readonly property int canvasShortSide:
        Math.min(currentCaps.width, currentCaps.height)
    readonly property var outputStops: {
        let stops = []

        for (let i = 0; i < standardOutputResolutions.length; i++)
            if (standardOutputResolutions[i] < canvasShortSide)
                stops.push(standardOutputResolutions[i])

        stops.push(0)

        return stops
    }
    readonly property bool outputIsNative:
        outputStopIndex === outputStops.length - 1
    readonly property int outputStopIndex: {
        let res = videoEffects.outputResolution

        if (res <= 0 || res >= canvasShortSide)
            return outputStops.length - 1

        // Use the biggest stop that doesn't exceed the saved value.
        let index = 0

        for (let i = 0; i < outputStops.length - 1; i++)
            if (outputStops[i] <= res)
                index = i

        return index
    }
    readonly property var outputVideoCaps: AkVideoCaps.create(videoEffects.outputCaps)

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        ColumnLayout {
            id: layout
            width: scrollView.width
            layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight

            AK.LabeledComboBox {
                id: cbxOrientation
                label: qsTr("Orientation")
                model: [qsTr("Landscape"), qsTr("Portrait")]
                currentIndex: {
                    let caps = AkVideoCaps.create(videoEffects.canvasCaps)

                    return caps.width >= caps.height? 0: 1
                }
                onCurrentIndexChanged: {
                    let caps = AkVideoCaps.create(videoEffects.canvasCaps)
                    let targetIsLandscape = (currentIndex === 0)
                    let currentIsLandscape = (caps.width >= caps.height)

                    if (targetIsLandscape !== currentIsLandscape) {
                        let temp = caps.width
                        caps.width = caps.height
                        caps.height = temp
                        videoEffects.canvasCaps = caps.toVariant()
                    }
                }
                Accessible.description: label
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }

            // Canvas resolution preset slider

            Label {
                text: qsTr("Canvas resolution")
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }

            Slider {
                id: qualitySlider
                from: -1
                to: root.canvasPresets.length - 1
                stepSize: 1
                value: root.matchedPresetIndex
                snapMode: Slider.SnapAlways
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                Accessible.name: qsTr("Canvas resolution")

                onMoved: {
                    if (value < 0)
                        return

                    let p = root.canvasPresets[value]
                    root.updatingFromPreset = true

                    let newCaps = AkVideoCaps.create(videoEffects.canvasCaps)
                    newCaps.width = root.isPortrait? p.height: p.width
                    newCaps.height = root.isPortrait? p.width: p.height
                    newCaps.fps = AkFrac.create(p.fps, 1).toVariant()
                    videoEffects.canvasCaps = newCaps.toVariant()

                    root.updatingFromPreset = false
                }

                Connections {
                    target: root

                    function onMatchedPresetIndexChanged() {
                        if (!root.updatingFromPreset)
                            qualitySlider.value = root.matchedPresetIndex
                    }
                }
            }

            // Summary label showing current resolution + FPS
            Label {
                id: qualitySummary
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.bottomMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
                wrapMode: Text.WordWrap

                text: {
                    let header = qualitySlider.value < 0?
                                    "<b>" + qsTr("Custom") + "</b><br/>":
                                    ""

                    return header
                        + qsTr("%1×%2 @ %3 FPS")
                          .arg(root.effectiveWidth)
                          .arg(root.effectiveHeight)
                          .arg(root.currentFps)
                }
            }

            // Output resolution. It follows the canvas orientation and aspect
            // ratio, and it can't exceed the canvas resolution. It is shared
            // by all the outputs (recording, virtual camera, streaming and
            // local streaming).

            Label {
                text: qsTr("Output resolution")
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }

            Slider {
                id: outputSlider
                from: 0
                to: root.outputStops.length - 1
                stepSize: 1
                value: root.outputStopIndex
                snapMode: Slider.SnapAlways
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                Accessible.name: qsTr("Output resolution")

                onMoved: videoEffects.outputResolution = root.outputStops[value]
            }

            Label {
                id: outputSummary
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.bottomMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
                wrapMode: Text.WordWrap

                text: {
                    let info = qsTr("%1×%2 @ %3 FPS")
                                .arg(root.outputVideoCaps.width)
                                .arg(root.outputVideoCaps.height)
                                .arg(root.currentFps)

                    if (root.outputIsNative)
                        info += " (" + qsTr("same as the canvas") + ")"

                    return info
                }
            }

            AK.ColorButton {
                text: qsTr("Canvas color")
                currentColor: AkUtils.fromRgba(videoEffects.canvasColor)
                title: qsTr("Choose the canvas color")
                showAlphaChannel: true
                horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true

                onCurrentColorChanged: videoEffects.canvasColor = AkUtils.toRgba(currentColor)
            }

            // Advanced settings (collapsible)

            Switch {
                id: chkAdvanced
                text: qsTr("Advanced settings")
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                checked: false
            }

            ColumnLayout {
                visible: chkAdvanced.checked
                Layout.fillWidth: true

                Label {
                    text: qsTr("Canvas size")
                    font: AkTheme.fontSettings.h6
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                    Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                    Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                    Layout.fillWidth: true
                }

                GridLayout {
                    columns: 2
                    layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                    Layout.fillWidth: true

                    Label {
                        id: txtCanvasWidth
                        text: qsTr("Width")
                    }
                    SpinBox {
                        id: spbCanvasWidth
                        value: root.effectiveWidth
                        from: 160
                        to: 32768
                        stepSize: 1
                        editable: true
                        Accessible.name: txtCanvasWidth.text
                        Layout.rightMargin: root.rightMargin

                        onValueModified: {
                            if (root.updatingFromPreset)
                                return

                            let newCaps = AkVideoCaps.create(videoEffects.canvasCaps)

                            if (root.isPortrait)
                                newCaps.height = value
                            else
                                newCaps.width = value

                            videoEffects.canvasCaps = newCaps.toVariant()
                        }
                    }

                    Label {
                        id: txtCanvasHeight
                        text: qsTr("Height")
                    }
                    SpinBox {
                        id: spbCanvasHeight
                        value: root.effectiveHeight
                        from: 90
                        to: 32768
                        stepSize: 1
                        editable: true
                        Accessible.name: txtCanvasHeight.text
                        Layout.rightMargin: root.rightMargin

                        onValueModified: {
                            if (root.updatingFromPreset)
                                return

                            let newCaps = AkVideoCaps.create(videoEffects.canvasCaps)

                            if (root.isPortrait)
                                newCaps.width = value
                            else
                                newCaps.height = value

                            videoEffects.canvasCaps = newCaps.toVariant()
                        }
                    }

                    Label {
                        id: txtCanvasFps
                        text: qsTr("Frame rate")
                    }
                    SpinBox {
                        id: spbCanvasFps
                        value: root.currentFps
                        from: 1
                        to: 256
                        stepSize: 1
                        editable: true
                        Accessible.name: txtCanvasFps.text
                        Layout.rightMargin: root.rightMargin

                        onValueModified: {
                            if (root.updatingFromPreset)
                                return

                            let newCaps = AkVideoCaps.create(videoEffects.canvasCaps)
                            newCaps.fps = AkFrac.create(value, 1).toVariant()
                            videoEffects.canvasCaps = newCaps.toVariant()
                        }
                    }
                }

                Label {
                    text: qsTr("Canvas output buffers")
                    font: AkTheme.fontSettings.h6
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                    Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                    Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                    Layout.fillWidth: true
                }

                Switch {
                    id: chkAsyncRead
                    text: qsTr("Enable asynchronous read")
                    checked: videoEffects.asyncRead
                    Layout.fillWidth: true

                    onCheckedChanged: videoEffects.asyncRead = checked
                }

                GridLayout {
                    columns: 2
                    layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                    Layout.fillWidth: true

                    Label {
                        id: txtCanvasOutputBuffers
                        text: qsTr("Output buffers")
                        enabled: chkAsyncRead.checked
                    }
                    SpinBox {
                        id: spbCanvasOutputBuffers
                        value: videoEffects.outputBufferSize
                        from: 2
                        to: 16
                        stepSize: 1
                        editable: true
                        enabled: chkAsyncRead.checked
                        Accessible.name: txtCanvasOutputBuffers.text
                        Layout.rightMargin: root.rightMargin

                        onValueModified: videoEffects.outputBufferSize = value
                    }
                }
            }

        }
    }
}

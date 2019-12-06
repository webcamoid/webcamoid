/* Webcamoid, webcam capture application.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
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

import QtQuick 2.7
import QtQuick.Window 2.2
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import AkQml 1.0
import Webcamoid 1.0
import WebcamoidUpdates 1.0

ApplicationWindow {
    id: wdgMainWidget
    title: Webcamoid.applicationName()
           + " "
           + Webcamoid.applicationVersion()
           + " - "
           + MediaSource.description(MediaSource.stream)
    visible: true
    x: (Screen.desktopAvailableWidth - width) / 2
    y: (Screen.desktopAvailableHeight - height) / 2
    width: Webcamoid.windowWidth
    height: Webcamoid.windowHeight

    function togglePlay() {
        if (MediaSource.state === AkElement.ElementStatePlaying) {
            Webcamoid.virtualCameraState = AkElement.ElementStateNull;
            Recording.state = AkElement.ElementStateNull;
            MediaSource.state = AkElement.ElementStateNull;

            if (buttonGroup.currentOption == optionRecording) {
                buttonGroup.currentOption = null
                optionRecording.checked = false
            }
        } else {
            MediaSource.state = AkElement.ElementStatePlaying;

            if (Webcamoid.enableVirtualCamera)
                Webcamoid.virtualCameraState = AkElement.ElementStatePlaying;
        }
    }

    function notifyUpdate(versionType)
    {
        if (Updates.notifyNewVersion
            && versionType == UpdatesT.VersionTypeOld) {
            trayIcon.show();
            trayIcon.showMessage(qsTr("New version available!"),
                                 qsTr("Download %1 %2 NOW!")
                                    .arg(Webcamoid.applicationName())
                                    .arg(Updates.latestVersion));
            notifyTimer.start();
        }
    }

    function showPane(pane, widget)
    {
        for (let i in pane.children)
            pane.children[i].destroy()

        let component = Qt.createComponent(widget + ".qml")

        if (component.status === Component.Ready) {
            let object = component.createObject(pane)
            object.Layout.fillWidth = true

            return object
        }

        return null
    }

    Timer {
        id: notifyTimer
        repeat: false
        triggeredOnStart: false
        interval: 10000

        onTriggered: trayIcon.hide()
    }

    onWidthChanged: Webcamoid.windowWidth = width
    onHeightChanged: Webcamoid.windowHeight = height
    onClosing: trayIcon.hide()

    Component.onCompleted: {
        if (MediaSource.playOnStart)
            togglePlay();

        notifyUpdate(Updates.versionType);
    }

    Connections {
        target: MediaSource

        onStateChanged: {
            if (state === AkElement.ElementStatePlaying) {
                itmPlayStopButton.checked = true
                itmPlayStopButton.ToolTip.text = qsTr("Stop")
                itmPlayStopButton.icon.source = "image://icons/stop"
                videoDisplay.visible = true
            } else {
                itmPlayStopButton.checked = false
                itmPlayStopButton.ToolTip.text = qsTr("Play")
                itmPlayStopButton.icon.source = "image://icons/play"
                videoDisplay.visible = false
            }
        }
    }
    Connections {
        target: Recording

        onStateChanged: recordingNotice.visible = state === AkElement.ElementStatePlaying
    }
    Connections {
        target: Updates

        onVersionTypeChanged: notifyUpdate(versionType);
    }
    Connections {
        target: trayIcon

        onMessageClicked: Qt.openUrlExternally(Webcamoid.projectDownloadsUrl())
    }

    VideoDisplay {
        id: videoDisplay
        objectName: "videoDisplay"
        visible: false
        smooth: true
        anchors.fill: parent
    }
    RecordingNotice {
        id: recordingNotice
        anchors.top: parent.top
        anchors.topMargin: 16
        anchors.horizontalCenter: parent.horizontalCenter
        visible: false
    }
    PhotoWidget {
        id: photoWidget
        anchors.bottom: footer.top
        anchors.bottomMargin: 16
        anchors.horizontalCenter: parent.horizontalCenter
        visible: optionPhoto.checked
    }
    Item {
        id: splitView
        anchors.fill: parent

        property int panelBorder: 2
        property int dragBorder: 4
        property int minimumWidth: 100

        onWidthChanged: {
            paneLeft.width = Math.max(paneLeft.width,
                                      splitView.minimumWidth)
            paneLeft.width = Math.min(paneLeft.width,
                                      splitView.width
                                      - paneRight.width
                                      - splitView.panelBorder
                                      - splitView.dragBorder)
            paneRight.width = Math.max(paneRight.width,
                                       splitView.minimumWidth)
            paneRight.width = Math.min(paneRight.width,
                                       splitView.width
                                       - paneLeft.width
                                       - splitView.panelBorder
                                       - splitView.dragBorder)
        }

        ScrollView {
            id: paneLeft
            implicitWidth: 250
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            contentHeight: paneLeftLayout.height
            clip: true
            visible: optionWebcam.checked
                     || optionSound.checked
                     || optionRecording.checked
                     || optionEffects.checked
                     || optionSettings.checked

            ColumnLayout {
                id: paneLeftLayout
                width: paneLeft.width
            }
        }
        Rectangle {
            id: rectangleLeft
            width: splitView.panelBorder
            color: "#000000"
            anchors.leftMargin: -splitView.panelBorder / 2
            anchors.left: paneLeft.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            visible: paneLeft.visible
        }
        MouseArea {
            cursorShape: Qt.SizeHorCursor
            width: splitView.panelBorder + 2 * splitView.dragBorder
            anchors.leftMargin: -width / 2
            anchors.left: paneLeft.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            drag.axis: Drag.XAxis
            visible: paneLeft.visible

            onPositionChanged: {
                paneLeft.width += mouse.x
                paneLeft.width = Math.max(paneLeft.width,
                                          splitView.minimumWidth)
                paneLeft.width = Math.min(paneLeft.width,
                                          splitView.width
                                          - paneRight.width
                                          - splitView.panelBorder
                                          - splitView.dragBorder)
            }
        }

        ScrollView {
            id: paneRight
            implicitWidth: 450
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            clip: true
            contentHeight: paneRightLayout.height
            visible: optionWebcam.checked
                     || optionSound.checked
                     || optionRecording.checked
                     || optionEffects.checked
                     || optionSettings.checked

            ColumnLayout {
                id: paneRightLayout
                width: paneRight.width
            }
        }
        Rectangle {
            id: rectangleRight
            width: splitView.panelBorder
            color: "#000000"
            anchors.rightMargin: -splitView.panelBorder / 2
            anchors.right: paneRight.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            visible: paneRight.visible
        }
        MouseArea {
            cursorShape: Qt.SizeHorCursor
            width: splitView.panelBorder + 2 * splitView.dragBorder
            anchors.rightMargin: -width / 2
            anchors.right: rectangleRight.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            drag.axis: Drag.XAxis
            visible: paneRight.visible

            onPositionChanged: {
                paneRight.width -= mouse.x
                paneRight.width = Math.max(paneRight.width,
                                           splitView.minimumWidth)
                paneRight.width = Math.min(paneRight.width,
                                           splitView.width
                                           - paneLeft.width
                                           - splitView.panelBorder
                                           - splitView.dragBorder)
            }
        }
    }

    footer: ToolBar {
        id: toolBar

        ButtonGroup {
            id: buttonGroup

            property Item currentOption: null

            onClicked: {
                if (currentOption == button) {
                    currentOption = null
                    button.checked = false
                } else {
                    currentOption = button
                }
            }
        }
        RowLayout {
            id: iconBar
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.top: parent.top
            spacing: 0

            ToolButton {
                id: itmPlayStopButton
                width: toolBar.height
                height: toolBar.height
                icon.source: "image://icons/play"
                checkable: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Play")
                display: AbstractButton.IconOnly

                onClicked: togglePlay()
            }
            ToolButton {
                id: optionWebcam
                width: toolBar.height
                height: toolBar.height
                icon.source: "image://icons/webcam"
                checkable: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Configure sources")
                display: AbstractButton.IconOnly
                ButtonGroup.group: buttonGroup

                onClicked: {
                    showPane(paneLeftLayout, "MediaBar")
                    showPane(paneRightLayout, "MediaConfig")
                }
            }
            ToolButton {
                id: optionSound
                width: toolBar.height
                height: toolBar.height
                icon.source: "image://icons/sound"
                checkable: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Configure audio")
                display: AbstractButton.IconOnly
                ButtonGroup.group: buttonGroup

                onClicked: {
                    let audioConfig = showPane(paneLeftLayout, "AudioConfig")
                    let audioInfo = showPane(paneRightLayout, "AudioInfo")

                    audioInfo.currentIndex = audioConfig.currentIndex
                    audioConfig.onCurrentIndexChanged.connect(function () {
                        audioInfo.currentIndex = audioConfig.currentIndex
                    })
                }
            }
            ToolButton {
                id: optionPhoto
                width: toolBar.height
                height: toolBar.height
                icon.source: "image://icons/photo"
                checkable: true
                enabled: MediaSource.state === AkElement.ElementStatePlaying
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Take a photo")
                display: AbstractButton.IconOnly
                ButtonGroup.group: buttonGroup
            }
            ToolButton {
                id: optionRecording
                width: toolBar.height
                height: toolBar.height
                icon.source: "image://icons/video"
                checkable: true
                enabled: MediaSource.state === AkElement.ElementStatePlaying
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Record video")
                display: AbstractButton.IconOnly
                ButtonGroup.group: buttonGroup

                onClicked: {
                    showPane(paneLeftLayout, "RecordBar")
                    showPane(paneRightLayout, "RecordConfig")
                }
            }
            ToolButton {
                id: optionEffects
                width: toolBar.height
                height: toolBar.height
                icon.source: "image://icons/video-effects"
                checkable: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Configure Effects")
                display: AbstractButton.IconOnly
                ButtonGroup.group: buttonGroup

                onClicked: {
                    let effectBar = showPane(paneLeftLayout, "EffectBar")
                    let effectConfig = showPane(paneRightLayout, "EffectConfig")

                    effectConfig.curEffect = effectBar.curEffect
                    effectConfig.curEffectIndex = effectBar.curEffectIndex
                    effectConfig.editMode = !effectBar.editMode
                    effectBar.onCurEffectChanged.connect(function () {
                        effectConfig.curEffect = effectBar.curEffect
                    })
                    effectBar.onCurEffectIndexChanged.connect(function () {
                        effectConfig.curEffectIndex = effectBar.curEffectIndex
                    })
                    effectBar.onEditModeChanged.connect(function () {
                        effectConfig.editMode = !effectBar.editMode
                    })
                }
            }
            ToolButton {
                id: optionSettings
                width: toolBar.height
                height: toolBar.height
                icon.source: "image://icons/settings"
                checkable: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Preferences")
                display: AbstractButton.IconOnly
                ButtonGroup.group: buttonGroup

                onClicked: {
                    let options = {
                        "output": "OutputConfig",
                        "general": "GeneralConfig",
                        "plugins": "PluginConfig",
                        "updates": "UpdatesConfig",
                        "about": "About",
                        "contributors": "Contributors",
                        "license": "License",
                        "3rdpartylicenses": "ThirdPartyLicenses"
                    }
                    let configBar = showPane(paneLeftLayout, "ConfigBar")

                    if (options[configBar.option])
                        showPane(paneRightLayout, options[configBar.option])

                    configBar.onOptionChanged.connect(function () {
                        if (options[configBar.option])
                            showPane(paneRightLayout, options[configBar.option])
                    })
                }
            }
        }
    }
}

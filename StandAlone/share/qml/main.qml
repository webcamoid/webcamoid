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

    property string currentOption: ""

    function rgbChangeAlpha(color, alpha)
    {
        return Qt.rgba(color.r, color.g, color.b, alpha);
    }

    function togglePlay() {
        if (MediaSource.state === AkElement.ElementStatePlaying) {
            Webcamoid.virtualCameraState = AkElement.ElementStateNull;
            Recording.state = AkElement.ElementStateNull;
            MediaSource.state = AkElement.ElementStateNull;

            if (splitView.state == "showPanels")
                splitView.state = "";
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
                itmPlayStopButton.text = qsTr("Stop")
                itmPlayStopButton.icon = "image://icons/webcamoid-stop"
                videoDisplay.visible = true
            } else {
                itmPlayStopButton.text = qsTr("Play")
                itmPlayStopButton.icon = "image://icons/webcamoid-play"
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
        anchors.bottom: iconBarRect.top
        anchors.bottomMargin: 16
        anchors.horizontalCenter: parent.horizontalCenter
        visible: false
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
            visible: false

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
            visible: false

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

        states: [
            State {
                name: "showPanels"

                PropertyChanges {
                    target: paneLeft
                    visible: true
                }
                PropertyChanges {
                    target: paneRight
                    visible: true
                }
            },
            State {
                name: "showPhotoWidget"
                PropertyChanges {
                    target: photoWidget
                    visible: true
                }
            }
        ]
    }

    Rectangle {
        id: iconBarRect
        width: height * nIcons
        height: 48
        radius: height / 2
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        opacity: 0.5

        property real nIcons: 7

        gradient: Gradient {
            GradientStop {
                position: 0
                color: Qt.rgba(0.25, 0.25, 0.25, 1)
            }
            GradientStop {
                position: 1
                color: Qt.rgba(0, 0, 0, 1)
            }
        }

        MouseArea {
            id: mouseArea
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.top: parent.top
            hoverEnabled: true
            onEntered: iconBarRect.opacity = 1
            onExited: iconBarRect.opacity = 0.5

            Row {
                id: iconBar
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.top: parent.top
                objectName: "IconBar"

                IconBarItem {
                    id: itmPlayStopButton
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Play")
                    icon: "image://icons/webcamoid-play"

                    onClicked: togglePlay()
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Configure sources")
                    icon: "image://icons/webcamoid-camera-web"

                    onClicked: {
                        showPane(paneLeftLayout, "MediaBar")
                        showPane(paneRightLayout, "MediaConfig")
                        let option = "sources"

                        if (wdgMainWidget.currentOption == option) {
                            splitView.state = ""
                            wdgMainWidget.currentOption = ""
                        } else {
                            splitView.state = "showPanels"
                            wdgMainWidget.currentOption = option
                        }
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Configure audio")
                    icon: "image://icons/webcamoid-sound"

                    onClicked: {
                        let audioConfig = showPane(paneLeftLayout, "AudioConfig")
                        let audioInfo = showPane(paneRightLayout, "AudioInfo")
                        let option = "audio"

                        if (wdgMainWidget.currentOption == option) {
                            splitView.state = ""
                            wdgMainWidget.currentOption = ""
                        } else {
                            splitView.state = "showPanels"
                            wdgMainWidget.currentOption = option
                        }

                        audioInfo.state = audioConfig.state
                        audioConfig.onStateChanged.connect(function (state) {
                            audioInfo.state = state
                        })
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Take a photo")
                    icon: "image://icons/webcamoid-picture"
                    enabled: MediaSource.state === AkElement.ElementStatePlaying

                    onClicked: {
                        let option = "photo"

                        if (wdgMainWidget.currentOption == option) {
                            splitView.state = ""
                            wdgMainWidget.currentOption = ""
                        } else {
                            splitView.state = "showPhotoWidget"
                            wdgMainWidget.currentOption = option
                        }
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Record video")
                    icon: "image://icons/webcamoid-video"
                    enabled: MediaSource.state === AkElement.ElementStatePlaying

                    onClicked: {
                        showPane(paneLeftLayout, "RecordBar")
                        showPane(paneRightLayout, "RecordConfig")
                        let option = "record"

                        if (wdgMainWidget.currentOption == option) {
                            splitView.state = ""
                            wdgMainWidget.currentOption = ""
                        } else {
                            splitView.state = "showPanels"
                            wdgMainWidget.currentOption = option
                        }
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Configure Effects")
                    icon: "image://icons/webcamoid-effects"

                    onClicked: {
                        let effectBar = showPane(paneLeftLayout, "EffectBar")
                        let effectConfig = showPane(paneRightLayout, "EffectConfig")
                        let option = "effects"

                        if (wdgMainWidget.currentOption == option) {
                            splitView.state = ""
                            wdgMainWidget.currentOption = ""
                        } else {
                            splitView.state = "showPanels"
                            wdgMainWidget.currentOption = option
                        }

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
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Preferences")
                    icon: "image://icons/webcamoid-setup"

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

                        let option = "preferences"

                        if (wdgMainWidget.currentOption == option) {
                            splitView.state = ""
                            wdgMainWidget.currentOption = ""
                        } else {
                            splitView.state = "showPanels"
                            wdgMainWidget.currentOption = option
                        }
                    }
                }
            }
        }
    }
}

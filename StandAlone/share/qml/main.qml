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
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import AkQml 1.0
import Webcamoid 1.0
import WebcamoidUpdates 1.0
import AkQmlControls 1.0

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
    color: palette.window

    property bool showEffectBar: false

    function rgbChangeAlpha(color, alpha)
    {
        return Qt.rgba(color.r, color.g, color.b, alpha);
    }

    function togglePlay() {
        if (MediaSource.state === AkElement.ElementStatePlaying) {
            Webcamoid.virtualCameraState = AkElement.ElementStateNull;
            Recording.state = AkElement.ElementStateNull;
            MediaSource.state = AkElement.ElementStateNull;

            if (splitView.state == "showRecordPanels")
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

    SystemPalette {
        id: palette
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

    Rectangle {
        id: recordingNotice
        color: "black"
        width: 128
        height: 60
        radius: 4
        anchors.top: parent.top
        anchors.topMargin: 16
        anchors.horizontalCenter: parent.horizontalCenter
        visible: false

        onVisibleChanged: {
            if (visible) {
                recordingTimer.startTime = new Date().getTime()
                recordingTimer.start()
            } else
                recordingTimer.stop()
        }

        Image {
            id: recordingIcon
            source: "image://icons/webcamoid-recording"
            sourceSize: Qt.size(width, height)
            width: height
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.bottom: recordingTime.top
        }
        Label {
            text: qsTr("Recording")
            color: "white"
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.left: recordingIcon.right
            anchors.leftMargin: 8
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.bottom: recordingTime.top
        }
        Label {
            id: recordingTime
            color: "white"
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
        }

        Timer {
            id: recordingTimer
            interval: 500
            repeat: true

            property double startTime: new Date().getTime()

            function pad(x)
            {
                return x < 10? "0" + x: x
            }

            onTriggered: {
                var diffTime = (new Date().getTime() - startTime) / 1000
                diffTime = parseInt(diffTime, 10)

                var ss = diffTime % 60;
                diffTime = (diffTime - ss) / 60;
                var mm = diffTime % 60;
                var hh = (diffTime - mm) / 60;

                recordingTime.text = pad(hh) + ":" + pad(mm) + ":" + pad(ss)
            }
        }

        SequentialAnimation on opacity {
            loops: Animation.Infinite
            running: recordingNotice.visible

            PropertyAnimation {
                easing.type: Easing.OutSine
                to: 0
                duration: 1000
            }
            PropertyAnimation {
                easing.type: Easing.InSine
                to: 1
                duration: 1000
            }
        }
    }

    PhotoWidget {
        id: photoWidget
        anchors.bottom: iconBarRect.top
        anchors.bottomMargin: 16
        anchors.horizontalCenter: parent.horizontalCenter
        visible: false

        function savePhoto()
        {
            Recording.takePhoto()
            var suffix = "png";
            var fileName = qsTr("Picture %1.%2")
                                .arg(Webcamoid.currentTime())
                                .arg(suffix)

            var filters = ["PNG file (*.png)",
                           "JPEG file (*.jpg)",
                           "BMP file (*.bmp)",
                           "GIF file (*.gif)"]

            var fileUrl = Webcamoid.saveFileDialog(qsTr("Save photo as..."),
                                     fileName,
                                     Webcamoid.standardLocations("pictures")[0],
                                     "." + suffix,
                                     filters.join(";;"))

            if (fileUrl !== "")
                Recording.savePhoto(fileUrl)
        }

        onTakePhoto: {
            if (useFlash)
                flash.show()
            else
                savePhoto()
        }
    }

    AkSplitView {
        id: splitView
        anchors.fill: parent
        orientation: Qt.Horizontal
        Layout.minimumWidth: 600

        RowLayout {
            id: leftPanel
            width: 200
            visible: false

            Rectangle {
                color: rgbChangeAlpha(palette.window, 0.9)
                Layout.fillWidth: true
                Layout.fillHeight: true

                MediaBar {
                    id: mdbMediaBar
                    visible: false
                    anchors.fill: parent
                }
                AudioConfig {
                    id: audioConfig
                    visible: false
                    anchors.fill: parent
                }
                EffectBar {
                    id: effectBar
                    visible: false
                    anchors.fill: parent
                    onCurEffectChanged: effectConfig.curEffect = curEffect
                    onCurEffectIndexChanged: effectConfig.curEffectIndex = curEffectIndex
                }
                RecordBar {
                    id: recordBar
                    visible: false
                    anchors.fill: parent
                }
                ConfigBar {
                    id: configBar
                    visible: false
                    anchors.fill: parent

                    onOptionChanged: {
                        if (generalConfig.children[0])
                            generalConfig.children[0].destroy()

                        var options = {
                            "output": "OutputConfig.qml",
                            "general": "GeneralConfig.qml",
                            "plugins": "PluginConfig.qml",
                            "updates": "UpdatesConfig.qml",
                        }

                        if (options[option]) {
                            var component = Qt.createComponent(options[option]);

                            if (component.status === Component.Ready) {
                                var object = component.createObject(generalConfig);
                                object.anchors.fill = generalConfig;
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        RowLayout {
            id: rightPanel
            visible: false
            width: 400

            Rectangle {
                color: rgbChangeAlpha(palette.window, 0.9)
                Layout.fillWidth: true
                Layout.fillHeight: true

                MediaConfig {
                    id: mediaConfig
                    visible: false
                    anchors.fill: parent
                }
                AudioInfo {
                    id: audioInfo
                    visible: false
                    anchors.fill: parent
                    state: audioConfig.state
                }
                EffectConfig {
                    id: effectConfig
                    curEffect: effectBar.curEffect
                    curEffectIndex: effectBar.curEffectIndex
                    editMode: !effectBar.editMode
                    visible: false
                    anchors.fill: parent
                }
                RecordConfig {
                    id: recordConfig
                    anchors.fill: parent
                    visible: false
                }
                ColumnLayout {
                    id: generalConfig
                    anchors.fill: parent
                    visible: false
                }
            }
        }

        states: [
            State {
                name: "showMediaPanels"
                PropertyChanges {
                    target: leftPanel
                    visible: true
                }
                PropertyChanges {
                    target: rightPanel
                    visible: true
                }
                PropertyChanges {
                    target: mdbMediaBar
                    visible: true
                }
                PropertyChanges {
                    target: mediaConfig
                    visible: true
                }
            },
            State {
                name: "showAudioPanels"
                PropertyChanges {
                    target: leftPanel
                    visible: true
                }
                PropertyChanges {
                    target: rightPanel
                    visible: true
                }
                PropertyChanges {
                    target: audioConfig
                    visible: true
                }
                PropertyChanges {
                    target: audioInfo
                    visible: true
                }
            },
            State {
                name: "showPhotoWidget"
                PropertyChanges {
                    target: photoWidget
                    visible: true
                }
            },
            State {
                name: "showEffectPanels"
                PropertyChanges {
                    target: leftPanel
                    visible: true
                }
                PropertyChanges {
                    target: rightPanel
                    visible: true
                }
                PropertyChanges {
                    target: effectBar
                    visible: true
                }
                PropertyChanges {
                    target: effectConfig
                    visible: true
                }
            },
            State {
                name: "showRecordPanels"
                PropertyChanges {
                    target: leftPanel
                    visible: true
                }
                PropertyChanges {
                    target: rightPanel
                    visible: true
                }
                PropertyChanges {
                    target: recordBar
                    visible: true
                }
                PropertyChanges {
                    target: recordConfig
                    visible: true
                }
            },
            State {
                name: "showConfigPanels"
                PropertyChanges {
                    target: leftPanel
                    visible: true
                }
                PropertyChanges {
                    target: rightPanel
                    visible: true
                }
                PropertyChanges {
                    target: configBar
                    visible: true
                }
                PropertyChanges {
                    target: generalConfig
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

        property real nIcons: 8

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

            Rectangle {
                id: btnGoBack
                height: parent.height
                width: height
                color: "#00000000"
                visible: false

                property int margins: 8

                Component.onCompleted: {
                    btnGoBack.width = imgGoBack.width + txtGoBack.width + 3 * btnGoBack.margins
                }

                Rectangle {
                    id: highlighter
                    radius: iconBarRect.radius
                    anchors.fill: parent
                    visible: false
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: Qt.rgba(0.67, 0.5, 1, 0.5)
                        }

                        GradientStop {
                            position: 1
                            color: Qt.rgba(0.5, 0.25, 1, 1)
                        }
                    }
                }
                Image {
                    id: imgGoBack
                    height: parent.height - 2 * btnGoBack.margins
                    anchors.left: parent.left
                    anchors.leftMargin: btnGoBack.margins
                    anchors.verticalCenter: parent.verticalCenter
                    width: height
                    source: "image://icons/webcamoid-go-back"
                    sourceSize: Qt.size(width, height)
                }
                Label {
                    id: txtGoBack
                    text: qsTr("Go back")
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: imgGoBack.right
                    anchors.leftMargin: btnGoBack.margins
                    color: "white"
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor

                    onClicked: {
                        iconBarRect.state = ""
                        splitView.state = ""
                    }
                    onPressed: {
                        imgGoBack.scale = 0.75
                        txtGoBack.scale = 0.75
                    }
                    onReleased: {
                        imgGoBack.scale = 1
                        txtGoBack.scale = 1
                    }
                    onEntered: {
                        highlighter.visible = true
                    }
                    onExited: {
                        imgGoBack.scale = 1
                        txtGoBack.scale = 1
                        highlighter.visible = false
                    }
                }
            }

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
                        if (splitView.state == "showMediaPanels")
                            splitView.state = ""
                        else
                            splitView.state = "showMediaPanels"
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Configure audio")
                    icon: "image://icons/webcamoid-sound"

                    onClicked: {
                        if (splitView.state == "showAudioPanels")
                            splitView.state = ""
                        else
                            splitView.state = "showAudioPanels"
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Take a photo")
                    icon: "image://icons/webcamoid-picture"
                    enabled: MediaSource.state === AkElement.ElementStatePlaying

                    onClicked: {
                        if (splitView.state == "showPhotoWidget")
                            splitView.state = ""
                        else
                            splitView.state = "showPhotoWidget"
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Record video")
                    icon: "image://icons/webcamoid-video"
                    enabled: MediaSource.state === AkElement.ElementStatePlaying

                    onClicked: {
                        if (splitView.state == "showRecordPanels")
                            splitView.state = ""
                        else
                            splitView.state = "showRecordPanels"
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Configure Effects")
                    icon: "image://icons/webcamoid-effects"

                    onClicked: {
                        if (splitView.state == "showEffectPanels")
                            splitView.state = ""
                        else
                            splitView.state = "showEffectPanels"
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Preferences")
                    icon: "image://icons/webcamoid-setup"

                    onClicked: {
                        if (splitView.state == "showConfigPanels")
                            splitView.state = ""
                        else
                            splitView.state = "showConfigPanels"
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("About")
                    icon: "image://icons/webcamoid-help-about"

                    onClicked: about.show()
                }
            }
        }

        states: [
            State {
                name: "showOptions"
                PropertyChanges {
                    target: iconBarRect
                    width: btnGoBack.width
                    radius: 4
                }
                PropertyChanges {
                    target: iconBar
                    visible: false
                }
                PropertyChanges {
                    target: btnGoBack
                    visible: true
                }
            }
        ]
    }

    Flash {
        id: flash

        onTriggered: photoWidget.savePhoto()
    }
    About {
        id: about
    }
}

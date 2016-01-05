/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.5
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import Webcamoid 1.0

ApplicationWindow {
    id: wdgMainWidget
    title: Webcamoid.applicationName()
           +
           " "
           + Webcamoid.applicationVersion()
           + " - "
           + Webcamoid.streamDescription(Webcamoid.curStream)
    visible: true
    x: (Screen.desktopAvailableWidth - width) / 2
    y: (Screen.desktopAvailableHeight - height) / 2
    width: Webcamoid.windowWidth
    height: Webcamoid.windowHeight
    color: palette.window

    property bool showEffectBar: false

    onWidthChanged: Webcamoid.windowWidth = width
    onHeightChanged: Webcamoid.windowHeight = height

    function rgbChangeAlpha(color, alpha)
    {
        return Qt.rgba(color.r, color.g, color.b, alpha)
    }

    SystemPalette {
        id: palette
    }

    Connections {
        target: Webcamoid
        onStateChanged: {
            if (Webcamoid.isPlaying) {
                itmPlayStopButton.icon = "qrc:/icons/hicolor/scalable/stop.svg"
                videoDisplay.visible = true
            }
            else {
                itmPlayStopButton.icon = "qrc:/icons/hicolor/scalable/play.svg"
                videoDisplay.visible = false
            }
        }
        onRecordingChanged: recordingNotice.visible = recording
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
            source: "qrc:/icons/hicolor/scalable/recording.svg"
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

    SplitView {
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
                EffectBar {
                    id: effectBar
                    visible: false
                    anchors.fill: parent
                    onCurEffectChanged: effectConfig.curEffect = curEffect
                }
                RecordBar {
                    id: recordBar
                    visible: false
                    anchors.fill: parent
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
                EffectConfig {
                    id: effectConfig
                    curEffect: effectBar.curEffect
                    inUse: !effectBar.editMode
                    visible: false
                    anchors.fill: parent
                }
                RecordConfig {
                    id: recordConfig
                    anchors.fill: parent
                    visible: false
                }
                GeneralConfig {
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
                    target: rightPanel
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
                    icon: "qrc:/icons/hicolor/scalable/play.svg"

                    onClicked: {
                        if (Webcamoid.isPlaying) {
                            Webcamoid.stopRecording();
                            Webcamoid.stop();
                        } else
                            Webcamoid.start();
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Configure streams")
                    icon: "qrc:/icons/hicolor/scalable/camera-web.svg"

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
                    text: qsTr("Take a photo")
                    icon: "qrc:/icons/hicolor/scalable/picture.svg"
                    enabled: Webcamoid.isPlaying

                    onClicked: {
                        Webcamoid.takePhoto()
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
                            Webcamoid.savePhoto(fileUrl)
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Record video")
                    icon: "qrc:/icons/hicolor/scalable/video.svg"
                    enabled: Webcamoid.isPlaying

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
                    icon: "qrc:/icons/hicolor/scalable/effects.svg"

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
                    icon: "qrc:/icons/hicolor/scalable/setup.svg"

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
                    icon: "qrc:/icons/hicolor/scalable/help-about.svg"

                    onClicked: about.show()
                }
            }
        }
    }

    About {
        id: about
    }
}

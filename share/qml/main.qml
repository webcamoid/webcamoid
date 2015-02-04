/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

import QtQuick 2.3
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.2
import QtQuick.Controls 1.2
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

    SystemPalette {
        id: palette
    }

    Connections {
        target: Webcamoid
        onStateChanged: {
            if (Webcamoid.isPlaying) {
                itmPlayStopButton.icon = "qrc:/Webcamoid/share/icons/stop.svg"
                videoDisplay.visible = true
            }
            else {
                itmPlayStopButton.icon = "qrc:/Webcamoid/share/icons/play.svg"
                videoDisplay.visible = false
            }
        }
    }

    VideoDisplay {
        id: videoDisplay
        objectName: "videoDisplay"
        visible: false
        smooth: true
        anchors.fill: splitView
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
                color: palette.window
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
                color: palette.window
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
                    icon: "qrc:/Webcamoid/share/icons/play.svg"

                    onClicked: {
                        if (Webcamoid.isPlaying)
                            Webcamoid.stop();
                        else
                            Webcamoid.start();
                    }
                }
                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Configure streams")
                    icon: "qrc:/Webcamoid/share/icons/webcam.svg"

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
                    icon: "qrc:/Webcamoid/share/icons/picture.svg"
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
                    icon: "qrc:/Webcamoid/share/icons/video.svg"
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
                    icon: "qrc:/Webcamoid/share/icons/effects.svg"

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
                    icon: "qrc:/Webcamoid/share/icons/setup.svg"

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
                    icon: "qrc:/Webcamoid/share/icons/about.svg"

                    onClicked: about.show()
                }
            }
        }
    }

    About {
        id: about
    }
}

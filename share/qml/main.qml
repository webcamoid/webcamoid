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
import QtQuick.Window 2.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import Webcamoid 1.0

ApplicationWindow {
    id: wdgMainWidget
    visible: true
    x: (Screen.desktopAvailableWidth - width) / 2
    y: (Screen.desktopAvailableHeight - height) / 2
    width: 1024
    height: 600
    color: Qt.rgba(0, 0, 0, 1)

    property bool showEffectBar: false

    Connections {
        target: Webcamoid
        onStateChanged: {
            if (Webcamoid.isPlaying()) {
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

        property bool showBars: false

        onShowBarsChanged: {
            leftPanel.visible = showBars
            rightPanel.visible = showBars
        }

        RowLayout {
            id: leftPanel
            width: 200
            visible: false

            MediaBar {
                id: mdbMediaBar
                visible: false
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            EffectBar {
                id: effectBar
                visible: false
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        Item {
            Layout.fillWidth: true

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
                                if (Webcamoid.isPlaying())
                                    Webcamoid.stop();
                                else
                                    Webcamoid.start();
                            }
                        }

                        IconBarItem {
                            width: iconBarRect.height
                            height: iconBarRect.height
                            text: qsTr("Manage Streams")
                            icon: "qrc:/Webcamoid/share/icons/webcam.svg"

                            onClicked: {
                                if (splitView.state == "showMediaPanels"
                                    && splitView.showBars)
                                    splitView.showBars = false
                                else
                                    splitView.showBars = true

                                splitView.state = "showMediaPanels"
                            }
                        }

                        IconBarItem {
                            width: iconBarRect.height
                            height: iconBarRect.height
                            text: qsTr("Take a Picture")
                            icon: "qrc:/Webcamoid/share/icons/picture.svg"
                        }

                        IconBarItem {
                            width: iconBarRect.height
                            height: iconBarRect.height
                            text: qsTr("Record Video")
                            icon: "qrc:/Webcamoid/share/icons/video.svg"
                        }

                        IconBarItem {
                            width: iconBarRect.height
                            height: iconBarRect.height
                            text: qsTr("Apply Effects")
                            icon: "qrc:/Webcamoid/share/icons/effects.svg"

                            onClicked: {
                                if (splitView.state == "showEffectPanels"
                                    && splitView.showBars)
                                    splitView.showBars = false
                                else
                                    splitView.showBars = true

                                splitView.state = "showEffectPanels"
                            }
                        }

                        IconBarItem {
                            width: iconBarRect.height
                            height: iconBarRect.height
                            text: qsTr("Preferences")
                            icon: "qrc:/Webcamoid/share/icons/setup.svg"
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
        }

        Rectangle {
            id: rightPanel
            width: 200
            visible: false
            color: Qt.rgba(0, 0, 0, 1)

            MediaConfig {
                id: mediaConfig
                visible: false
                onWidthChanged: rightPanel.width = width
            }

            Rectangle {
                id: effectConfig
                color: Qt.rgba(0, 0, 1, 1)
                anchors.fill: parent
                visible: false
            }
        }

        states: [
            State {
                name: "showMediaPanels"
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
                    target: effectBar
                    visible: true
                }
                PropertyChanges {
                    target: effectConfig
                    visible: true
                }
            }
        ]
    }

    About {
        id: about
    }
}

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
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QbQml 1.0

ApplicationWindow {
    id: wdgMainWidget
    visible: true
    x: (Screen.desktopAvailableWidth - width) / 2
    y: (Screen.desktopAvailableHeight - height) / 2
    width: 640
    height: 480
    color: Qt.rgba(0, 0, 0, 1)

    property bool showEffectBar: false

    Image {
        id: imgBackground
        objectName: "WindowBackground"
        cache: false
        smooth: true
        fillMode: Image.PreserveAspectFit
        anchors.fill: parent

//        source: "image://webcam/image"
    }

/*
    onMousePressed:
    {
        webcams.visible = false
        about.visible = false
    }

    Effects
    {
        id: cdbEffects
        opacity: 0.95
        anchors.bottom: iconbar.top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: parent.top
        objectName: "Effects"
        visible: wdgMainWidget.showEffectBar
    }
*/
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
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Play")
                    icon: "qrc:/Webcamoid/share/icons/play.svg"
                }

                IconBarItem {
                    width: iconBarRect.height
                    height: iconBarRect.height
                    text: qsTr("Set a Device")
                    icon: "qrc:/Webcamoid/share/icons/webcam.svg"
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
                }
            }
        }
    }
/*
    Devices
    {
        id: webcams
        anchors.bottomMargin: 5
        anchors.rightMargin: -(webcams.width + iconbar.iconSize) / 2
        anchors.right: iconbar.left
        objectName: "Devices"
        opacity: 0.95
        visible: false
        anchors.bottom: iconbar.top
        onEscapePressed: visible = false
        onActiveDevicesChanged: cdbEffects.activeDevices = webcams.activeDevices
        onDevicesChanged: cdbEffects.devices = webcams.devices
    }

    About
    {
        id: about
        visible: false
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }*/
}

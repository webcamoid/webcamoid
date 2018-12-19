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
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import AkQmlControls 1.0

ApplicationWindow {
    id: recAbout
    title: qsTr("About %1").arg(Webcamoid.applicationName())
    color: palette.window
    flags: Qt.Dialog
    modality: Qt.ApplicationModal
    width: 600
    height: 300

    SystemPalette {
        id: palette
    }

    ColumnLayout {
        anchors.fill: parent

        TabBar {
            id: aboutTabs
            Layout.fillWidth: true

            TabButton {
                text: qsTr("Information")
            }
            TabButton {
                text: qsTr("Thanks!")
            }
            TabButton {
                text: qsTr("License")
            }
        }

        StackLayout {
            currentIndex: aboutTabs.currentIndex
            Layout.fillWidth: true

            AkScrollView {
                clip: true
                contentWidth: clyProgramInfo.childrenRect.width
                contentHeight: clyProgramInfo.childrenRect.height

                ColumnLayout {
                    id: clyProgramInfo

                    RowLayout {
                        Image {
                            fillMode: Image.PreserveAspectFit
                            Layout.minimumWidth: 128
                            Layout.minimumHeight: 128
                            Layout.maximumWidth: 128
                            Layout.maximumHeight: 128
                            source: "image://icons/webcamoid"
                            sourceSize: Qt.size(width, height)
                        }

                        ColumnLayout {
                            Label {
                                text: Webcamoid.applicationName()
                                font.bold: true
                                font.pointSize: 12
                            }
                            Label {
                                text: qsTr("Version %1").arg(Webcamoid.applicationVersion())
                                font.bold: true
                            }
                            Label {
                                text: qsTr("Using Qt %1")
                                        .arg(Webcamoid.qtVersion())
                            }
                            AkButton {
                                label: qsTr("Website")
                                iconRc: "image://icons/applications-internet"

                                onClicked: Qt.openUrlExternally(Webcamoid.projectUrl())
                            }
                        }
                    }

                    Label {
                        text: qsTr("Webcam capture application.")
                    }
                    Label {
                        text: qsTr("A simple webcam application for picture and video capture.")
                    }
                    Label {
                        text: Webcamoid.copyrightNotice()
                    }
                }
            }
            AkScrollView {
                clip: true
                contentWidth: clyThanks.childrenRect.width
                contentHeight: clyThanks.childrenRect.height

                ColumnLayout {
                    id: clyThanks

                    Label {
                        text: qsTr("Thanks to all these cool people that helped contributing to Webcamoid all these years.")
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    TextArea {
                        id: contributorsText
                        text: Webcamoid.readFile(":/Webcamoid/share/contributors.txt")
                        font.family: "Courier"
                        readOnly: true
                    }
                }
            }
            AkScrollView {
                clip: true
                contentWidth: licenseText.width
                contentHeight: licenseText.height

                TextArea {
                    id: licenseText
                    text: Webcamoid.readFile(":/Webcamoid/COPYING")
                    font.family: "Courier"
                    readOnly: true
                }
            }
        }

        AkButton {
            label: qsTr("Close")
            iconRc: "image://icons/window-close"
            Layout.alignment: Qt.AlignRight
            onClicked: recAbout.close()
        }
    }
}

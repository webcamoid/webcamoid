/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.0
import QtQuick.Window 2.0
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

ApplicationWindow {
    id: recAbout
    title: qsTr("Add new media")
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

        TabView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Tab {
                title: qsTr("Information")
                clip: true

                ScrollView {
                    ColumnLayout {
                        RowLayout {
                            Image {
                                fillMode: Image.PreserveAspectFit
                                Layout.minimumWidth: 128
                                Layout.minimumHeight: 128
                                Layout.maximumWidth: 128
                                Layout.maximumHeight: 128
                                source: "qrc:/icons/hicolor/128x128/webcamoid.png"
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
                                    text: qsTr("Using Qt %1").arg(Webcamoid.qtVersion())
                                }

                                Button {
                                    iconName: "applications-internet"
                                    iconSource: "qrc:/icons/hicolor/scalable/applications-internet.svg"
                                    text: qsTr("Website")

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

                        Label {
                            Layout.fillHeight: true
                        }
                    }
                }
            }

            Tab {
                title: qsTr("License")

                TextArea {
                    text: Webcamoid.readFile(":/Webcamoid/COPYING")
                    font.family: "Courier"
                    readOnly: true
                }
            }
        }

        Button {
            text: qsTr("Close")
            iconName: "window-close"
            iconSource: "qrc:/icons/hicolor/scalable/window-close.svg"
            Layout.alignment: Qt.AlignRight
            onClicked: recAbout.close()
        }
    }
}

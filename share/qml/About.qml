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

    SystemPalette {
        id: palette
    }

    ColumnLayout {
        id: recLicense
        anchors.fill: parent

        RowLayout {
            Image {
                fillMode: Image.PreserveAspectFit
                Layout.minimumWidth: 48
                Layout.minimumHeight: 48
                Layout.maximumWidth: 48
                Layout.maximumHeight: 48
                source: "qrc:/Webcamoid/share/icons/webcam.svg"
            }

            ColumnLayout {
                Label {
                    text: Webcamoid.applicationName()
                    font.bold: true
                    font.pointSize: 12
                }

                Label {
                    text: "Version " + Webcamoid.applicationVersion()
                    font.bold: true
                }

                Label {
                    text: "Using Qt version " + Webcamoid.qtVersion()
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

        RowLayout {
            Layout.alignment: Qt.AlignHCenter

            Button {
                iconName: "go-next"
                text: qsTr("Website")

                onClicked: Qt.openUrlExternally(Webcamoid.projectUrl())
            }

            Button {
                iconName: "go-next"
                text: qsTr("License")

                onClicked: Qt.openUrlExternally(Webcamoid.projectLicenseUrl())
            }
        }

        Label {
            Layout.fillHeight: true
        }

        Button {
            text: qsTr("Close")
            Layout.alignment: Qt.AlignRight
            onClicked: recAbout.close()
        }
    }
}

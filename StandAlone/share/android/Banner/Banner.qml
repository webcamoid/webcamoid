/* Webcamoid, webcam capture application.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 320
    height: 180
    color: "#3f2a7e"
    visible: true

    ColumnLayout {
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter

        GridLayout {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            columns: 2

            // Info
            Image {
                id: icon
                width: 96
                height: width
                sourceSize.width: width
                sourceSize.height: height
                source: "../../themes/WebcamoidTheme/icons/hicolor/128x128/webcamoid.png"
            }

            ColumnLayout {
                Text {
                    id: programName
                    color: "#ffffff"
                    text: "Webcamoid"
                    font.weight: Font.Bold
                    font.pixelSize: 24
                }

                Text {
                    color: "#ffffff"
                    text: "The ultimate video capture suite!"
                    leftPadding: 12
                    font.weight: Font.Bold
                    font.pixelSize: 0.5 * programName.font.pixelSize
                    font.italic: true
                }
            }
        }
    }
}

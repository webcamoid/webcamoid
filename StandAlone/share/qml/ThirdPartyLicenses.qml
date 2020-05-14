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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

Page {
    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        ColumnLayout {
            id: layout
            spacing: 16
            width: scrollView.width

            Label {
                text: qsTr("The followings are the Licenses for 3rd-party work incorporated into Webcamoid. <b>These Licenses DOES NOT applies to Webcamoid itself.</b>")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("License for resources taken from openclipart.org:")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextArea {
                text: mediaTools.readFile(":/Webcamoid/share/3rd-party/licenses/openclipart.txt")
                readOnly: true
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("License for code taken from OpenCV:")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextArea {
                text: mediaTools.readFile(":/Webcamoid/share/3rd-party/licenses/OpenCV.txt")
                readOnly: true
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("License for code and algorithms used in Temperature plugin:")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextArea {
                text: mediaTools.readFile(":/Webcamoid/share/3rd-party/licenses/TemperatureAlgorithm.txt")
                readOnly: true
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("License for the usb.ids file:")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextArea {
                text: mediaTools.readFile(":/Webcamoid/share/3rd-party/licenses/UsbIds.txt")
                readOnly: true
                Layout.fillWidth: true
            }
        }
    }
}

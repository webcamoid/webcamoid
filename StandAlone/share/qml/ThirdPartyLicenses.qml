/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ak
import AkControls as AK

AK.MenuOption {
    id: root

    //: License for 3rd party components used in Webcamoid, like
    //  libraries and code snippets.
    title: qsTr("3rd Party Licenses")
    subtitle: qsTr("Licenses for the code incorporated into %1").arg(mediaTools.applicationName)
    icon: "image://icons/notes"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

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
                text: "The followings are the Licenses for 3rd-party work incorporated into Webcamoid. <b>These Licenses DOES NOT applies to Webcamoid itself.</b>"
                wrapMode: Text.WordWrap
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.maximumWidth: parent.width
                Layout.fillWidth: true
            }
            Label {
                text: "License for resources taken from openclipart.org:"
                wrapMode: Text.WordWrap
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.maximumWidth: parent.width
                Layout.fillWidth: true
            }
            TextArea {
                text: mediaTools.readFile(":/Webcamoid/share/3rd-party/licenses/openclipart.txt")
                wrapMode: Text.WordWrap
                readOnly: true
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
            }
            Label {
                text: "License for code taken from OpenCV:"
                wrapMode: Text.WordWrap
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.maximumWidth: parent.width
                Layout.fillWidth: true
            }
            TextArea {
                text: mediaTools.readFile(":/Webcamoid/share/3rd-party/licenses/OpenCV.txt")
                wrapMode: Text.WordWrap
                readOnly: true
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
            }
            Label {
                text: "License for code and algorithms used in Temperature plugin:"
                wrapMode: Text.WordWrap
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.maximumWidth: parent.width
                Layout.fillWidth: true
            }
            TextArea {
                text: mediaTools.readFile(":/Webcamoid/share/3rd-party/licenses/TemperatureAlgorithm.txt")
                wrapMode: Text.WordWrap
                readOnly: true
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
            }
            Label {
                text: "License for the usb.ids file:"
                wrapMode: Text.WordWrap
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.maximumWidth: parent.width
                Layout.fillWidth: true
            }
            TextArea {
                text: mediaTools.readFile(":/Webcamoid/share/3rd-party/licenses/UsbIds.txt")
                wrapMode: Text.WordWrap
                readOnly: true
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
            }
        }
    }
}

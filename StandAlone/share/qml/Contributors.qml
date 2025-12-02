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

    //: List of people contributing to the project: software
    //  developers, translators, designers, etc..
    title: qsTr("Contributors")
    subtitle: qsTr("Great people who collaborated to make %1 better").arg(mediaTools.applicationName)
    icon: "image://icons/people"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: clyThanks.height
        clip: true

        ColumnLayout {
            id: clyThanks
            width: scrollView.width

            Label {
                text: qsTr("Thanks to all these cool people that helped contributing to Webcamoid all these years.")
                wrapMode: Text.WordWrap
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }
            TextArea {
                id: contributorsText
                text: mediaTools.readFile(":/Webcamoid/share/contributors.txt")
                wrapMode: Text.WordWrap
                readOnly: true
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
            }
        }
    }
}

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

    //: Information of the program, like name, description, version,
    //  etc..
    title: qsTr("About %1").arg(mediaTools.applicationName)
    subtitle: qsTr("%1 version, copyleft, and build information.").arg(mediaTools.applicationName)
    icon: "image://icons/about"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: clyProgramInfo.height
        clip: true

        ColumnLayout {
            id: clyProgramInfo
            width: scrollView.width
            layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight

            Image {
                fillMode: Image.PreserveAspectFit
                source: "image://icons/webcamoid"
                sourceSize: Qt.size(width, height)
                Layout.minimumWidth:
                    AkUnit.create(96 * AkTheme.controlScale, "dp").pixels
                Layout.minimumHeight:
                    AkUnit.create(96 * AkTheme.controlScale, "dp").pixels
                Layout.maximumWidth:
                    AkUnit.create(96 * AkTheme.controlScale, "dp").pixels
                Layout.maximumHeight:
                    AkUnit.create(96 * AkTheme.controlScale, "dp").pixels
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
            }
            Label {
                text: mediaTools.applicationName
                      + " "
                      + mediaTools.applicationVersion
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
            }
            RowLayout {
                visible: mediaTools.projectGitShortCommit.length > 0
                layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin

                Label {
                    //: Built from "short commit hash"
                    text: qsTr("Built from")
                }
                Button {
                    text: mediaTools.projectGitShortCommit
                    flat: true
                    Accessible.name:
                        qsTr("Built from %1").arg(mediaTools.projectGitShortCommit)
                    Accessible.description:
                        qsTr("Open the commit in your web browser")

                    onClicked: Qt.openUrlExternally(mediaTools.projectGitCommitUrl)
                }
            }
            Label {
                text: qsTr("Using Qt %1")
                        .arg(mediaTools.qtVersion)
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
            }
            Label {
                text: qsTr("Webcam capture application.")
                wrapMode: Text.WordWrap
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("A multi-platform camera application for picture and video capture.")
                wrapMode: Text.WordWrap
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }
            Label {
                text: mediaTools.copyrightNotice
                wrapMode: Text.WordWrap
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Website")
                icon.source: "image://icons/internet"
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Accessible.name: text
                Accessible.description:
                    qsTr("Go to %1 website").arg(mediaTools.applicationName)

                onClicked: Qt.openUrlExternally(mediaTools.projectUrl)
            }
        }
    }
}

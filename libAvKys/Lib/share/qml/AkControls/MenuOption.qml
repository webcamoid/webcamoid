/* Webcamoid, camera capture application.
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
import QtQuick.Controls
import QtQuick.Layouts
import Ak

Page {
    id: rootOption

    //property string title: ""
    property string subtitle: ""
    property string icon: ""
    property bool showDivider: false
    default property alias content: contentItem.children

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft
    readonly property color activeDark: AkTheme.palette.active.dark

    signal goBack()

    // Main layout
    ColumnLayout {
        anchors.fill: parent

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: AkUnit.create(32 * AkTheme.controlScale, "dp").pixels
            Layout.leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.topMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

            // Go back button
            Button {
                id: backButton
                icon.source: "image://icons/left-arrow"
                implicitWidth: implicitHeight
                flat: true
                Layout.alignment: Qt.AlignVCenter

                onClicked: rootOption.goBack()
            }

            RowLayout {
                layoutDirection: rootOption.rtl? Qt.RightToLeft: Qt.LeftToRight
                Layout.fillWidth: true

                // Page icon
                AkColorizedImage {
                    width: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
                    height: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
                    source: rootOption.icon
                    color: AkTheme.palette.active.windowText
                    colorize: true
                    visible: rootOption.icon !== "" && status == Image.Ready
                    asynchronous: true
                    fillMode: AkColorizedImage.PreserveAspectFit
                    mipmap: true
                    smooth: true
                    Layout.alignment: Qt.AlignVCenter
                }

                // Page title
                Label {
                    text: rootOption.title
                    font: AkTheme.fontSettings.h6
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }

        // Horizontal header divider
        Rectangle {
            height: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
            color: rootOption.activeDark
            visible: rootOption.showDivider
            Layout.fillWidth: true
        }

        // Content area
        Item {
            id: contentItem
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}

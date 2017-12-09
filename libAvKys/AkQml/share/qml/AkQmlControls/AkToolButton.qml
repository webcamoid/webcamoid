/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

ToolButton {
    id: btnButton
    Layout.minimumWidth: buttonContents.minWidth < 90? 90: buttonContents.minWidth
    Layout.minimumHeight: buttonContents.minHeight

    readonly property int showText: 1
    readonly property int showIcon: 2
    readonly property int showTextAndIcon: 3

    property string label: ""
    property int buttonStyle: showIcon
    property string iconRc: ""
    property int iconSize: 16
    property int iconSpacing: 4
    property int contentsPadding: 8

    contentItem: Item {
        id: buttonContents

        property int minWidth: childrenRect.width + 2 * btnButton.contentsPadding
        property int minHeight: childrenRect.height + 2 * btnButton.contentsPadding

        RowLayout {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: btnButton.iconSpacing

            Image {
                visible: btnButton.buttonStyle & btnButton.showIcon
                         && btnButton.iconRc.length > 0
                fillMode: Image.PreserveAspectFit
                width: btnButton.buttonStyle & btnButton.showIcon
                       && btnButton.iconRc.length > 0? btnButton.iconSize: 0
                height: width
                sourceSize.width: width
                sourceSize.height: width
                source: btnButton.iconRc
            }
            Label {
                visible: btnButton.buttonStyle & btnButton.showText
                         && btnButton.label.length > 0
                text: btnButton.buttonStyle & btnButton.showText? btnButton.label: ""
                font: btnButton.font
            }
        }
    }
}

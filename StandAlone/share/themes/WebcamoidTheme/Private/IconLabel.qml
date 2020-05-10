/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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
import Ak 1.0

RowLayout {
    id: iconLabel

    property int leftPadding: 0
    property int rightPadding: 0
    property string iconName: ""
    property alias iconSource: icon.source
    property alias iconWidth: icon.width
    property alias iconHeight: icon.height
    property alias iconColor: icon.color
    property int display: AbstractButton.TextBesideIcon
    property alias spacing: mainLayout.columnSpacing
    property bool mirrored: false
    property alias text: label.text
    property alias font: label.font
    property alias color: label.color
    property int alignment: Qt.AlignHCenter | Qt.AlignVCenter
    property int elide: Text.ElideNone

    Item {
        width: iconLabel.leftPadding
    }
    GridLayout {
        id: mainLayout
        rowSpacing: columnSpacing
        layoutDirection: iconLabel.mirrored?
                                Qt.RightToLeft:
                                Qt.LeftToRight
        columns: iconLabel.display == AbstractButton.TextUnderIcon? 1: 2
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

        AkColorizedImage {
            id: icon
            color: label.color
            visible: status == Image.Ready
                        && iconLabel.display != AbstractButton.TextOnly
            Layout.alignment: iconLabel.alignment
            asynchronous: true
            mipmap: true
        }
        Text {
            id: label
            visible: text && iconLabel.display != AbstractButton.IconOnly
            Layout.alignment: iconLabel.alignment
            Layout.fillWidth: true
            elide: iconLabel.elide
            linkColor: iconLabel.enabled?
                           AkTheme.palette.active.link:
                           AkTheme.palette.disabled.link
        }
    }
    Item {
        width: iconLabel.rightPadding
    }
}

/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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
import Webcamoid

Dialog {
    id: root
    standardButtons: Dialog.Yes | Dialog.No
    width: AkUnit.create(320 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(200 * AkTheme.controlScale, "dp").pixels
    modal: true
    title: !stopStreaming?
                qsTr("Start streaming"):
                qsTr("Stop streaming")

    property bool stopStreaming: false

    onVisibleChanged: forceActiveFocus()

    ScrollView {
        id: view
        anchors.fill: parent

        ColumnLayout {
            width: view.width
            clip: true

            Label {
                text: !root.stopStreaming?
                            qsTr("Thousands of people will start watching you around the world on internet.<br/>Are you sure that you want to start streaming?"):
                            qsTr("Are you sure that you want to stop streaming?")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

    onAccepted: streaming.state = stopStreaming?
                        AkElement.ElementStateNull:
                        AkElement.ElementStatePlaying
}

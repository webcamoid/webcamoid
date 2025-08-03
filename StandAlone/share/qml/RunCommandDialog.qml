/* Webcamoid, camera capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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
import Ak
import Webcamoid

Dialog {
    id: commandDialog
    standardButtons: Dialog.NoButton
    closePolicy: Popup.NoAutoClose
    width: physicalWidth <= 100 || physicalHeight <= 100?
               wdgMainWidget.width: wdgMainWidget.width * 0.75
    height: physicalWidth <= 100 || physicalHeight <= 100?
                wdgMainWidget.height: wdgMainWidget.height * 0.75
    modal: true

    property real physicalWidth: wdgMainWidget.width / Screen.pixelDensity
    property real physicalHeight: wdgMainWidget.height / Screen.pixelDensity
    property string message: ""

    function start()
    {
        commandOutput.text = ""
        closeButton.enabled = false
    }

    function writeLine(str)
    {
        commandOutput.text += str
        scrollView.scrollPosition = commandOutput.contentHeight
    }

    function stop()
    {
        closeButton.enabled = true
    }

    ScrollView {
        id: scrollView
        ScrollBar.vertical.position: scrollPosition
        anchors.fill: parent
        contentHeight: cmdLayout.height
        clip: true

        property double scrollPosition: 0

        ColumnLayout {
            id: cmdLayout
            width: scrollView.width

            Label {
                text: commandDialog.message
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextArea {
                id: commandOutput
                wrapMode: Text.WordWrap
                readOnly: true
                selectByMouse: true
                width: scrollView.width
                Layout.fillWidth: true
                color: "white"
                font.family: "monospace"

                background: Rectangle {
                    color: "black"
                }
            }
            Button {
                id: closeButton
                text: qsTr("Close")
                enabled: true
                flat: true
                Layout.alignment: Qt.AlignRight

                onClicked: commandDialog.close()
            }
        }
    }
}

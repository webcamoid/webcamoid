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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.1

Rectangle {
    id: recPhotoWidget
    width: 480
    height: 80
    color: "#e6000000"
    radius: 4

    signal takePhoto(bool useFlash)

    Component.onCompleted: {
        for (var i = 5; i < 35; i += 5)
            lstTimeOptions.append({text: qsTr("%1 seconds").arg(i),
                                   time: i})
    }

    GridLayout {
        id: glyControls
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.bottomMargin: 8
        anchors.topMargin: 8
        anchors.fill: parent
        columns: 3

        ComboBox {
            id: cbxTimeShot
            textRole: "text"
            Layout.fillWidth: true
            model: ListModel {
                id: lstTimeOptions

                ListElement {
                    text: qsTr("Now")
                    time: 0
                }
            }
        }
        ProgressBar {
            id: pgbPhotoShot
            Layout.fillWidth: true
            Layout.rowSpan: 2

            property double start: 0

            onValueChanged: {
                if (value >= 1) {
                    updateProgress.stop()
                    value = 0
                    cbxTimeShot.enabled = true
                    chkFlash.enabled = true
                    recPhotoWidget.takePhoto(chkFlash.checked)
                }
            }
        }
        Button {
            id: btnPhotoShot
            text: updateProgress.running? qsTr("Cancel"): qsTr("Shot!")
            isDefault: true
            Layout.fillHeight: true
            Layout.rowSpan: 2

            onClicked: {
                if (cbxTimeShot.currentIndex == 0) {
                    recPhotoWidget.takePhoto(chkFlash.checked)

                    return
                }

                if (updateProgress.running) {
                    updateProgress.stop()
                    pgbPhotoShot.value = 0
                    cbxTimeShot.enabled = true
                    chkFlash.enabled = true
                } else {
                    cbxTimeShot.enabled = false
                    chkFlash.enabled = false
                    pgbPhotoShot.start = new Date().getTime()
                    updateProgress.start()
                }
            }
        }
        RowLayout {
            CheckBox {
                id: chkFlash
                checked: true
            }
            Label {
                text: qsTr("Use flash")
                color: "white"
                Layout.fillWidth: true
            }
        }
    }

    Timer {
        id: updateProgress
        interval: 100
        repeat: true

        onTriggered: {
            var timeout = 1000 * lstTimeOptions.get(cbxTimeShot.currentIndex).time
            pgbPhotoShot.value = (new Date().getTime() - pgbPhotoShot.start) / timeout
        }
    }
}

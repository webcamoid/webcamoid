/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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
import Qt.labs.platform 1.1 as LABS
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

Pane {
    id: recPhotoWidget
    width: 480
    height: 120

    function savePhoto()
    {
        Recording.takePhoto()
        fileDialog.open()
    }

    Component.onCompleted: {
        for (var i = 5; i < 35; i += 5)
            lstTimeOptions.append({text: qsTr("%1 seconds").arg(i),
                                   time: i})
    }

    GridLayout {
        id: glyControls
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
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

                    if (chkFlash.checked)
                        flash.show()
                    else
                        savePhoto()
                }
            }
        }
        Button {
            id: btnPhotoShot
            text: updateProgress.running? qsTr("Cancel"): qsTr("Shot!")
            focus: true
            Layout.fillHeight: true
            Layout.rowSpan: 2

            onClicked: {
                if (cbxTimeShot.currentIndex == 0) {
                    if (chkFlash.checked)
                        flash.show()
                    else
                        savePhoto()

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
            Label {
                text: qsTr("Use flash")
                Layout.fillWidth: true
            }
            Switch {
                id: chkFlash
                checked: true
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

    Flash {
        id: flash

        onTriggered: savePhoto()
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Save photo as…")
        folder: "file://" + Webcamoid.standardLocations("pictures")[0]
        currentFile: folder + "/" + qsTr("Picture %1.png").arg(Webcamoid.currentTime())
        defaultSuffix: "png"
        fileMode: LABS.FileDialog.SaveFile
        selectedNameFilter.index: 0
        nameFilters: ["All Picture Files (*.png *.jpg *.bmp *.gif)",
                      "PNG file (*.png)",
                      "JPEG file (*.jpg)",
                      "BMP file (*.bmp)",
                      "GIF file (*.gif)",
                      "All Files (*)"]

        onAccepted: Recording.savePhoto(currentFile)
    }
}

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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

Rectangle {
    id: recPhotoWidget
    width: 480
    height: 120
    color: "transparent"
    radius: 4
    clip: true

    function savePhoto()
    {
        Recording.takePhoto()
        var suffix = "png";
        var fileName = qsTr("Picture %1.%2")
                            .arg(Webcamoid.currentTime())
                            .arg(suffix)

        var filters = ["PNG file (*.png)",
                       "JPEG file (*.jpg)",
                       "BMP file (*.bmp)",
                       "GIF file (*.gif)"]

        var fileUrl = Webcamoid.saveFileDialog(qsTr("Save photo as..."),
                                 fileName,
                                 Webcamoid.standardLocations("pictures")[0],
                                 "." + suffix,
                                 filters.join(";;"))

        if (fileUrl !== "")
            Recording.savePhoto(fileUrl)
    }

    Component.onCompleted: {
        for (var i = 5; i < 35; i += 5)
            lstTimeOptions.append({text: qsTr("%1 seconds").arg(i),
                                   time: i})
    }

    Rectangle{
        id: paneMask
        anchors.fill: parent
        radius: parent.radius
        color: Qt.hsla(0, 0, 0, 1)
        visible: false
    }
    Pane {
        anchors.fill: parent
        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: paneMask
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
}

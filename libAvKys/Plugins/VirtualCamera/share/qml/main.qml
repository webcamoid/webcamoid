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
import QtQuick.Layouts 1.1

GridLayout {
    id: recCameraControls
    columns: 2

    function updateDevices()
    {
        var model = []
        var devices = VirtualCamera.medias

        for (var device in devices) {
            var deviceStr = VirtualCamera.description(devices[device])
                    + " ("
                    + devices[device]
                    + ")"

            model.push(deviceStr)
        }

        cbxDevices.model = model
        cbxDevices.currentIndex = devices.indexOf(VirtualCamera.media)
    }

    Connections {
        target: VirtualCamera

        onMediasChanged: updateDevices()
    }
    Component.onCompleted: {
        if (OsName == "linux")
            recCameraControls.state = VirtualCamera.maxCameras > 0? "": "missing"
        else
            recCameraControls.state = VirtualCamera.maxCameras > 0? "": "unsupported"

        if (recCameraControls.state == "")
            updateDevices()
    }

    Label {
        id: txtDevices
        text: qsTr("Devices")
    }
    ComboBox {
        id: cbxDevices
        Layout.fillWidth: true

        onCurrentIndexChanged: VirtualCamera.media = VirtualCamera.medias[currentIndex]
    }
    GridLayout {
        id: glyOptions
        columns: 3
        Layout.columnSpan: 2

        Label {
            Layout.fillWidth: true
        }
        Button {
            text: qsTr("Add")
            iconName: "add"
            enabled: cbxDevices.count < VirtualCamera.maxCameras

            onClicked: {
                glyCommitChanges.operation = "add"
                glyCommitChanges.state = "createChange"
                recCameraControls.state = "commitChanges"
            }
        }
        Button {
            text: qsTr("Edit")
            iconName: "edit"
            enabled: cbxDevices.count > 0

            onClicked: {
                glyCommitChanges.operation = "edit"
                glyCommitChanges.state = "createChange"
                recCameraControls.state = "commitChanges"
            }
        }
        Label {
        }
        Button {
            text: qsTr("Remove")
            iconName: "remove"
            enabled: cbxDevices.count > 0

            onClicked: {
                glyCommitChanges.operation = "remove"
                glyCommitChanges.state = ""
                recCameraControls.state = "commitChanges"
            }
        }
        Button {
            text: qsTr("Remove All")
            iconName: "remove"
            enabled: cbxDevices.count > 0

            onClicked: {
                glyCommitChanges.operation = "removeAll"
                glyCommitChanges.state = ""
                recCameraControls.state = "commitChanges"
            }
        }
    }
    GridLayout {
        id: glyCommitChanges
        columns: 2
        Layout.columnSpan: 2
        visible: false

        property string operation: ""

        Label {
            id: lblDescription
            text: qsTr("Description")
            visible: false
        }
        TextField {
            id: txtDescription
            Layout.fillWidth: true
            placeholderText: qsTr("Camera name (optional)")
            visible: false
        }
        Label {
            text: qsTr("Password")
            visible: VirtualCamera.needRoot
        }
        TextField {
            id: txtPassword
            echoMode: 2
            Layout.fillWidth: true
            placeholderText: qsTr("Write root password")
            visible: VirtualCamera.needRoot
        }

        RowLayout {
            Layout.columnSpan: 2

            Label {
                Layout.fillWidth: true
            }
            Button {
                id: btnOk
                text: qsTr("Ok")
                iconName: "ok"

                function commitChanges()
                {
                    var result = false
                    var webcam = ""
                    var newWebcam = ""

                    if (VirtualCamera.medias.length > 0) {
                        if (cbxDevices.currentIndex >= 0)
                            webcam = VirtualCamera.medias[cbxDevices.currentIndex]
                        else
                            webcam = VirtualCamera.medias[0]
                    }

                    if (glyCommitChanges.operation == "add") {
                        newWebcam = VirtualCamera.createWebcam(txtDescription.text,
                                                               txtPassword.text)
                        result = newWebcam != ""
                    } else if (glyCommitChanges.operation == "edit") {
                        result = VirtualCamera.changeDescription(webcam,
                                                                 txtDescription.text,
                                                                 txtPassword.text)
                    } else if (glyCommitChanges.operation == "remove") {
                        result = VirtualCamera.removeWebcam(webcam,
                                                            txtPassword.text)
                    } else if (glyCommitChanges.operation == "removeAll") {
                        result = VirtualCamera.removeAllWebcams(txtPassword.text)
                    } else
                        return

                    if (result) {
                        recCameraControls.state = ""
                        txtDescription.text = ""
                        txtPassword.text = ""

                        if (newWebcam != "")
                            cbxDevices.currentIndex = VirtualCamera.medias.indexOf(newWebcam)
                    } else {
                        recCameraControls.state = "passwordError"
                        txtPassword.text = ""
                    }
                }

                onClicked: commitChanges()
            }
            Button {
                text: qsTr("Cancel")
                iconName: "cancel"

                onClicked: {
                    recCameraControls.state = ""
                    txtDescription.text = ""
                    txtPassword.text = ""
                }
            }
        }

        states: [
            State {
                name: "createChange"

                PropertyChanges {
                    target: lblDescription
                    visible: true
                }
                PropertyChanges {
                    target: txtDescription
                    visible: true
                }
            }
        ]
    }
    Label {
        id: message
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        Layout.columnSpan: 2
        visible: false
    }
    Label {
        Layout.fillHeight: true
    }

    states: [
        State {
            name: "unsupported"

            PropertyChanges {
                target: txtDevices
                visible: false
            }
            PropertyChanges {
                target: cbxDevices
                visible: false
            }
            PropertyChanges {
                target: glyOptions
                visible: false
            }
            PropertyChanges {
                target: message
                visible: true
                text: qsTr("This system is not supported yet")
                enabled: false
            }
        },
        State {
            name: "missing"

            PropertyChanges {
                target: txtDevices
                visible: false
            }
            PropertyChanges {
                target: cbxDevices
                visible: false
            }
            PropertyChanges {
                target: glyOptions
                visible: false
            }
            PropertyChanges {
                target: message
                visible: true
                text: qsTr("Please, install <b>v4l2loopback</b> for using this option")
                enabled: false
            }
        },
        State {
            name: "passwordError"

            PropertyChanges {
                target: glyCommitChanges
                visible: true
            }
            PropertyChanges {
                target: message
                visible: true
                text: qsTr("Wrong password")
                color: "#ff0000"
                style: Text.Raised
            }
        },
        State {
            name: "commitChanges"

            PropertyChanges {
                target: glyCommitChanges
                visible: true
            }
        }
    ]
}

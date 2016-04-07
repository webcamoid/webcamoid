/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
    columns: 3

    function controlsUpdated(controls)
    {
        var controlsCont = [clyImageControls, clyCameraControls]

        for (var where in controlsCont)
            for (var child in controlsCont[where].children) {
                var controlName = controlsCont[where].children[child].controlParams[0]

                if (controlName in controls)
                    controlsCont[where].children[child].value = controls[controlName]
            }
    }

    function createControls(controls, where)
    {
        var minimumLeftWidth = lblFormat.width
        var minimumRightWidth = btnReset.width

        for (var control in controls) {
            var component = Qt.createComponent("CameraControl.qml");

            if (component.status !== Component.Ready)
                continue

            var obj = component.createObject(where);
            obj.controlParams = controls[control]
            obj.onControlChanged.connect(function (controlName, value)
            {
                var ctrl = {}
                ctrl[controlName] = value
                VideoCapture.setImageControls(ctrl)
                VideoCapture.setCameraControls(ctrl)
            })

            if (obj.leftWidth > minimumLeftWidth)
                minimumLeftWidth = obj.leftWidth

            if (obj.rightWidth > minimumRightWidth)
                minimumRightWidth = obj.rightWidth
        }

        return [minimumLeftWidth, minimumRightWidth]
    }

    function createInterface()
    {
        var minimumImageWidth = createControls(VideoCapture.imageControls(), clyImageControls)
        var minimumCameraWidth = createControls(VideoCapture.cameraControls(), clyCameraControls)

        var minimumLeftWidth = Math.max(minimumImageWidth[0], minimumCameraWidth[0])
        var minimumRightWidth = Math.max(minimumImageWidth[1], minimumCameraWidth[1])

        var controls = [clyImageControls, clyCameraControls]

        for (var where in controls)
            for (var child in controls[where].children) {
                controls[where].children[child].minimumLeftWidth = minimumLeftWidth
                controls[where].children[child].minimumRightWidth = minimumRightWidth
            }

        lblFormat.minimumWidth = minimumLeftWidth
        btnReset.minimumWidth = minimumRightWidth
    }

    Component.onCompleted: createInterface()

    Connections {
        id: conVideoCapture
        target: VideoCapture

        onImageControlsChanged: controlsUpdated(imageControls)
        onCameraControlsChanged: controlsUpdated(cameraControls)
        onStreamsChanged: cbxFormat.currentIndex = streams.length > 0? streams[0]: -1
    }

    Label {
        id: lblFormat
        text: qsTr("Video format")
        Layout.minimumWidth: minimumWidth

        property int minimumWidth: 0
    }
    ComboBox {
        id: cbxFormat
        model: VideoCapture.listCapsDescription()
        currentIndex: VideoCapture.defaultStream("video/x-raw")
        Layout.fillWidth: true

        onCurrentIndexChanged: VideoCapture.streams = [currentIndex]
    }
    Button {
        id: btnReset
        text: qsTr("Reset")
        iconName: "reset"
        Layout.minimumWidth: minimumWidth

        property int minimumWidth: 0

        onClicked: VideoCapture.reset()
    }

    ColumnLayout {
        id: clyImageControls
        Layout.fillWidth: true
        Layout.columnSpan: 3
    }
    ColumnLayout {
        id: clyCameraControls
        Layout.fillWidth: true
        Layout.columnSpan: 3
    }
    Label {
        Layout.fillHeight: true
    }
}

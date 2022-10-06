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
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

ColumnLayout {
    id: recCameraControls

    function createControls()
    {
        let controls = virtualCamera.controls()

        for (let i = clyControls.children.length - 1; i >= 0 ; i--)
            clyControls.children[i].destroy()

        let minimumLeftWidth = 0
        let minimumRightWidth = 0

        // Create new ones.
        for (let control in controls) {
            let component = Qt.createComponent("CameraControl.qml")

            if (component.status !== Component.Ready)
                continue

            let obj = component.createObject(clyControls)
            obj.controlParams = controls[control]

            obj.onControlChanged.connect(function (controlName, value)
            {
                let ctrl = {}
                ctrl[controlName] = value
                virtualCamera.setControls(ctrl)
            })

            if (obj.leftWidth > minimumLeftWidth)
                minimumLeftWidth = obj.leftWidth

            if (obj.rightWidth > minimumRightWidth)
                minimumRightWidth = obj.rightWidth
        }

        for (let i in clyControls.children) {
            let ctrl = clyControls.children[i];
            ctrl.minimumLeftWidth = minimumLeftWidth
            ctrl.minimumRightWidth = minimumRightWidth
        }
    }

    Component.onCompleted: createControls()

    Connections {
        target: virtualCamera

        function onMediaChanged() {
            recCameraControls.createControls()
        }
    }

    ColumnLayout {
        id: clyControls
        Layout.fillWidth: true
    }
    Label {
        Layout.fillHeight: true
    }
}

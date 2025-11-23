/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: recCameraControls

    property int streamIndex: virtualCamera.streams.length < 1?
                                0: virtualCamera.streams[0]
    property var cachedComponent: null

    function createControlsInBatches(controls, where)
    {
        for (const child of where.children)
            child.destroy()

        where.children = []

        if (!cachedComponent)
            cachedComponent = Qt.createComponent("CameraControl.qml")

        if (cachedComponent.status !== Component.Ready) {
            console.warn("Component not ready:", cachedComponent.errorString())

            return
        }

        let index = 0
        const batchSize = 3
        const leftWidths = []
        const supportedTypes = ["integer",
                                "integer64",
                                "float",
                                "boolean",
                                "menu"]

        function createBatch() {
            const limit = Math.min(index + batchSize, controls.length)

            while (index < limit) {
                const controlParams = controls[index]

                if (!controlParams
                    || controlParams.length < 2
                    || !controlParams[1]) {
                    index++

                    continue
                }

                const type = controlParams[1]

                if (!supportedTypes.includes(controlParams[1])) {
                    index++

                    continue
                }

                const obj = cachedComponent.createObject(where, { controlParams: controlParams })

                if (!obj) {
                    index++

                    continue
                }

                obj.onControlChanged.connect(function(controlName, value) {
                    const ctrl = {}
                    ctrl[controlName] = value
                    virtualCamera.setControls(ctrl)
                })

                index++
            }

            if (index < controls.length)
                Qt.callLater(createBatch)
        }

        createBatch()
    }

    function createCameraControls()
    {
        createControlsInBatches(virtualCamera.controls(), clyControls)
    }

    Component.onCompleted: {
        createCameraControls()
    }

    Connections {
        target: virtualCamera

        function onMediaChanged()
        {
            recCameraControls.createCameraControls()
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

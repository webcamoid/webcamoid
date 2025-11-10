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
import Ak
import AkControls as AK

ColumnLayout {
    id: recCameraControls

    property int streamIndex: VideoCapture.streams.length < 1?
                                0: VideoCapture.streams[0]
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
                    VideoCapture.setImageControls(ctrl)
                    VideoCapture.setCameraControls(ctrl)
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
        createControlsInBatches(VideoCapture.imageControls(),
                                clyImageControls)
        createControlsInBatches(VideoCapture.cameraControls(),
                                clyCameraControls)
    }

    Component.onCompleted: {
        createCameraControls()
    }

    Connections {
        target: VideoCapture

        function onMediaChanged()
        {
            recCameraControls.createCameraControls()
        }
    }

    AK.LabeledComboBox {
        id: cbxFormat
        label: qsTr("Video format")
        Layout.fillWidth: true
        Accessible.description: label
        model: VideoCapture.listFormats(VideoCapture.media)
        currentIndex: VideoCapture.formatIndex(VideoCapture.media,
                                               recCameraControls.streamIndex)

        onCurrentIndexChanged: {
            VideoCapture.streams =
                [VideoCapture.streamIndex(VideoCapture.media,
                                          cbxFormat.currentIndex,
                                          cbxResolution.currentIndex,
                                          cbxFps.currentIndex)]
        }
    }
    AK.LabeledComboBox {
        id: cbxResolution
        label: qsTr("Resolution")
        Layout.fillWidth: true
        Accessible.description: label
        model: VideoCapture.listResolutions(VideoCapture.media,
                                            cbxFormat.currentIndex)
        currentIndex: VideoCapture.resolutionIndex(VideoCapture.media,
                                                   recCameraControls.streamIndex)

        onCurrentIndexChanged: {
            VideoCapture.streams =
                [VideoCapture.streamIndex(VideoCapture.media,
                                          cbxFormat.currentIndex,
                                          cbxResolution.currentIndex,
                                          cbxFps.currentIndex)]
        }
    }
    AK.LabeledComboBox {
        id: cbxFps
        label: qsTr("FPS")
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.description: label
        model: VideoCapture.listFps(VideoCapture.media,
                                    cbxFormat.currentIndex,
                                    cbxResolution.currentIndex)
        currentIndex: VideoCapture.fpsIndex(VideoCapture.media,
                                            recCameraControls.streamIndex)

        onCurrentIndexChanged: {
            VideoCapture.streams =
                [VideoCapture.streamIndex(VideoCapture.media,
                                          cbxFormat.currentIndex,
                                          cbxResolution.currentIndex,
                                          cbxFps.currentIndex)]
        }
    }
    Button {
        id: btnReset
        text: qsTr("Reset")
        highlighted: true
        icon.source: "image://icons/reset"
        Layout.fillWidth: true
        Accessible.description: qsTr("Reset to default values")

        onClicked: {
            VideoCapture.reset()
            createCameraControls()
            cbxFormat.currentIndex =
                VideoCapture.formatIndex(VideoCapture.media, 0)
            cbxResolution.currentIndex =
                VideoCapture.resolutionIndex(VideoCapture.media, 0)
            cbxFps.currentIndex =
                VideoCapture.fpsIndex(VideoCapture.media, 0)
        }
    }

    ColumnLayout {
        id: clyImageControls
        Layout.fillWidth: true
    }
    ColumnLayout {
        id: clyCameraControls
        Layout.fillWidth: true
    }
    Label {
        Layout.fillHeight: true
    }
}

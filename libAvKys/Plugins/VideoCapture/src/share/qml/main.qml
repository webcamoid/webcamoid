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

GridLayout {
    id: recCameraControls
    columns: 3

    property int streamIndex: VideoCapture.streams.length < 1?
                                0: VideoCapture.streams[0]
    property var cachedComponent: null

    function createControlsInBatches(controls, where, doneCallback)
    {
        for (const child of where.children)
            child.destroy()

        where.children = []

        if (!cachedComponent)
            cachedComponent = Qt.createComponent("CameraControl.qml")

        if (cachedComponent.status !== Component.Ready) {
            console.warn("Component not ready:", cachedComponent.errorString())
            doneCallback([0, 0])

            return
        }

        let index = 0
        const batchSize = 3
        const leftWidths = []
        const spinBoxWidths = []
        let minimumLeftWidth = lblFormat.width
        let minimumRightWidth = btnReset.width

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

                leftWidths.push(obj.leftWidth)

                if (type === "integer"
                    || type === "integer64"
                    || type === "float") {
                    spinBoxWidths.push(obj.spinBoxWidth)
                }

                index++
            }

            if (leftWidths.length > 0) {
                minimumLeftWidth = Math.max(minimumLeftWidth, ...leftWidths)
                minimumRightWidth = spinBoxWidths.length > 0? Math.max(...spinBoxWidths): btnReset.width
            }

            if (index < controls.length)
                Qt.callLater(createBatch)
            else
                doneCallback([minimumLeftWidth, minimumRightWidth, spinBoxWidths])
        }

        createBatch()
    }

    function createCameraControls()
    {
        let completed = 0
        let minimumImageWidth, minimumCameraWidth
        let imageSpinBoxWidths = [], cameraSpinBoxWidths = []

        function checkCompletion()
        {
            if (++completed < 2)
                return

            const minimumLeftWidth = Math.max(minimumImageWidth[0], minimumCameraWidth[0])
            const minimumRightWidth = Math.max(minimumImageWidth[1], minimumCameraWidth[1])
            const allSpinBoxWidths = [...imageSpinBoxWidths, ...cameraSpinBoxWidths]
            const maxSpinBoxWidth = allSpinBoxWidths.length > 0? Math.max(...allSpinBoxWidths): btnReset.width
            const controls = [clyImageControls, clyCameraControls]

            for (const where of controls)
                for (const child of where.children) {
                    child.minimumLeftWidth = minimumLeftWidth
                    child.minimumRightWidth = minimumRightWidth
                }

            lblFormat.minimumWidth = minimumLeftWidth
            btnReset.minimumWidth = maxSpinBoxWidth
        }

        createControlsInBatches(VideoCapture.imageControls(),
                                clyImageControls,
                                function(widths) {
            minimumImageWidth = widths
            imageSpinBoxWidths = widths[2]
            checkCompletion()
        })
        createControlsInBatches(VideoCapture.cameraControls(),
                                clyCameraControls,
                                function(widths) {
            minimumCameraWidth = widths
            cameraSpinBoxWidths = widths[2]
            checkCompletion()
        })
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

    Label {
        id: lblFormat
        text: qsTr("Video format")
        Layout.minimumWidth: minimumWidth

        property int minimumWidth: 0
    }
    ComboBox {
        id: cbxFormat
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.description: lblFormat.text
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
    Label {
        id: lblResolution
        text: qsTr("Resolution")
        Layout.minimumWidth: minimumWidth

        property int minimumWidth: 0
    }
    ComboBox {
        id: cbxResolution
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.description: lblResolution.text
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
    Label {
        id: lblFps
        text: qsTr("FPS")
        Layout.minimumWidth: minimumWidth

        property int minimumWidth: 0
    }
    ComboBox {
        id: cbxFps
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.description: lblFps.text
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
    Label {
        Layout.fillWidth: true
        Layout.columnSpan: 2
    }
    Button {
        id: btnReset
        text: qsTr("Reset")
        icon.source: "image://icons/reset"
        Layout.minimumWidth: minimumWidth
        Accessible.description: qsTr("Reset to default values")

        property int minimumWidth: 75

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

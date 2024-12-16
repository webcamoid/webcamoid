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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ak

GridLayout {
    id: recCameraControls
    columns: 3

    property variant capsCache: []
    property variant currentCaps: {}
    property int paramLevel: 0

    function updateCurrentCaps()
    {
        let streamIndex = VideoCapture.streams.length < 1?
                            0: VideoCapture.streams[0]
        let ncaps = VideoCapture.listTracks().length

        if (streamIndex >= ncaps)
            streamIndex = 0;

        let caps = VideoCapture.rawCaps(streamIndex)
        let format = 0
        let resolution = Qt.size(0, 0)
        let fps = AkFrac.create()
        let description = ""

        switch (AkCaps.create(caps).type) {
        case AkCaps.CapsVideo: {
            let videoCaps = AkVideoCaps.create(caps)
            format = videoCaps.format
            resolution = Qt.size(videoCaps.width, videoCaps.height)
            fps = AkFrac.create(videoCaps.fps)
            description = AkVideoCaps.pixelFormatToString(videoCaps.format).toUpperCase()

            break
        }

        case AkCaps.CapsVideoCompressed: {
            let videoCaps = AkCompressedVideoCaps.create(caps)
            format = videoCaps.codec
            let rawCaps = AkVideoCaps.create(videoCaps.rawCaps)
            resolution = Qt.size(rawCaps.width, rawCaps.height)
            fps = AkFrac.create(rawCaps.fps)
            description = AkCompressedVideoCaps.videoCodecIDToString(videoCaps.codec).toUpperCase()

            break
        }

        default:
            break
        }

        currentCaps = {format: format,
                       resolution: resolution,
                       fps: fps}
    }

    function updateFormatControls()
    {
        updateCurrentCaps()
        cbxFormat.updateModel()
    }

    function updateStream()
    {
        paramLevel--

        if (paramLevel > 0)
            return

        if (cbxFormat.currentIndex < 0
            || cbxResolution.currentIndex < 0
            || cbxFps.currentIndex < 0) {
            return
        }

        let format = cbxFormat.model.get(cbxFormat.currentIndex).format
        let resolution = cbxResolution.model.get(cbxResolution.currentIndex).resolution
        resolution = Qt.size(resolution.width, resolution.height)
        let fps = cbxFps.model.get(cbxFps.currentIndex).fps

        for (let i in recCameraControls.capsCache) {
            let caps = recCameraControls.capsCache[i]
            let cFormat = caps.format
            let cResolution = Qt.size(caps.resolution.width,
                                     caps.resolution.height)
            let cFps = caps.fps

            if (cFormat != format
                || cResolution != resolution
                || cFps != fps)
                continue

            VideoCapture.streams = [i]
            updateCurrentCaps()

            break
        }
    }

    function createControls(controls, where)
    {
        // Remove old controls.
        for(let i = where.children.length - 1; i >= 0 ; i--)
            where.children[i].destroy()

        let minimumLeftWidth = lblFormat.width
        let minimumRightWidth = btnReset.width

        // Create new ones.
        for (let control in controls) {
            let component = Qt.createComponent("CameraControl.qml")

            if (component.status !== Component.Ready)
                continue

            let obj = component.createObject(where)
            obj.controlParams = controls[control]

            obj.onControlChanged.connect(function (controlName, value)
            {
                let ctrl = {}
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

    function createCameraControls()
    {
        let minimumImageWidth =
                recCameraControls.createControls(VideoCapture.imageControls(),
                                                 clyImageControls)
        let minimumCameraWidth =
                recCameraControls.createControls(VideoCapture.cameraControls(),
                                                 clyCameraControls)

        let minimumLeftWidth = Math.max(minimumImageWidth[0],
                                        minimumCameraWidth[0])
        let minimumRightWidth = Math.max(minimumImageWidth[1],
                                         minimumCameraWidth[1])

        let controls = [clyImageControls, clyCameraControls]

        for (let where in controls)
            for (let child in controls[where].children) {
                let ctrl = controls[where].children[child];
                ctrl.minimumLeftWidth = minimumLeftWidth
                ctrl.minimumRightWidth = minimumRightWidth
            }

        lblFormat.minimumWidth = minimumLeftWidth
        btnReset.minimumWidth = minimumRightWidth
    }

    Component.onCompleted: {
        createCameraControls()
        updateFormatControls()
    }

    Connections {
        target: VideoCapture

        function onMediaChanged()
        {
            recCameraControls.createCameraControls()
            recCameraControls.updateFormatControls()
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
        textRole: "description"
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.description: lblFormat.text
        model: ListModel { }

        function updateIndex()
        {
            for (let i = 0; i < model.count; i++) {
                if (model.get(i).format == recCameraControls.currentCaps.format) {
                    currentIndex = i;

                    return;
                }
            }

            currentIndex = model.count > 0? 0: -1;
        }

        function updateModel()
        {
            model.clear()
            recCameraControls.capsCache = []
            let ncaps = VideoCapture.listTracks().length
            let formats = []

            for (let i = 0; i < ncaps; i++) {
                let caps = VideoCapture.rawCaps(i)
                let format = 0
                let resolution = Qt.size(0, 0)
                let fps = AkFrac.create()
                let description = ""

                switch (AkCaps.create(caps).type) {
                case AkCaps.CapsVideo: {
                    let videoCaps = AkVideoCaps.create(caps)
                    format = videoCaps.format
                    resolution = Qt.size(videoCaps.width, videoCaps.height)
                    fps = AkFrac.create(videoCaps.fps)
                    description = AkVideoCaps.pixelFormatToString(videoCaps.format).toUpperCase()

                    break
                }

                case AkCaps.CapsVideoCompressed: {
                    let videoCaps = AkCompressedVideoCaps.create(caps)
                    format = videoCaps.codec
                    let rawCaps = AkVideoCaps.create(videoCaps.rawCaps);
                    resolution = Qt.size(rawCaps.width, rawCaps.height)
                    fps = AkFrac.create(rawCaps.fps)
                    description = AkCompressedVideoCaps.videoCodecIDToString(videoCaps.codec).toUpperCase()

                    break
                }

                default:
                    break
                }

                recCameraControls.capsCache.push({format: format,
                                                  resolution: resolution,
                                                  fps: fps})

                if (!format)
                    continue

                if (formats.indexOf(description) < 0) {
                    model.append({format: format,
                                  description: description})
                    formats.push(description)
                }
            }

            updateIndex()
        }

        onCurrentIndexChanged: {
            recCameraControls.paramLevel = 2
            cbxResolution.updateModel()
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
        textRole: "description"
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.description: lblResolution.text
        model: ListModel { }

        function updateIndex()
        {
            for (let i = 0; i < model.count; i++) {
                let resolution = model.get(i).resolution

                if (Qt.size(resolution.width, resolution.height) == recCameraControls.currentCaps.resolution) {
                    currentIndex = i;

                    return;
                }
            }

            currentIndex = model.count > 0? 0: -1;
        }

        function updateModel()
        {
            model.clear()

            if (cbxFormat.model.count < 1) {
                currentIndex = -1

                return
            }

            let formatIndex = Math.max(cbxFormat.currentIndex, 0)
            let curFormat = cbxFormat.model.get(formatIndex).format
            let resolutions = []

            for (let i in recCameraControls.capsCache) {
                let caps = recCameraControls.capsCache[i]
                let format = caps.format
                let resolution = caps.resolution
                let description = resolution.width + "x" + resolution.height

                if (!format || format != curFormat)
                    continue

                if (resolutions.indexOf(description) < 0) {
                    model.append({resolution: resolution,
                                  description: description})
                    resolutions.push(description)
                }
            }

            updateIndex()
        }

        onCurrentIndexChanged: {
            if (recCameraControls.paramLevel < 1)
                recCameraControls.paramLevel = 2

            cbxFps.updateModel()
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
        textRole: "description"
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.description: lblFps.text
        model: ListModel { }

        function updateIndex()
        {
            for (let i = 0; i < model.count; i++) {
                if (model.get(i).fps.value == recCameraControls.currentCaps.fps.value) {
                    currentIndex = i;

                    return;
                }
            }

            currentIndex = model.count > 0? 0: -1;
        }

        function updateModel()
        {
            model.clear()

            if (cbxFormat.model.count < 1 || cbxResolution.model.count < 1) {
                currentIndex = -1;

                return;
            }

            let formatIndex = Math.max(cbxFormat.currentIndex, 0)
            let curFormat = cbxFormat.model.get(formatIndex).format
            let resolutionIndex = Math.max(cbxResolution.currentIndex, 0)
            let curResolution = cbxResolution.model.get(resolutionIndex).resolution
            curResolution = Qt.size(curResolution.width, curResolution.height)
            let resolutions = []

            for (let i in recCameraControls.capsCache) {
                let caps = recCameraControls.capsCache[i]
                let format = caps.format
                let resolution = caps.resolution
                let fps = caps.fps
                let description = Math.round(caps.fps.value * 100) / 100

                if (!format
                    || format != curFormat
                    || resolution != curResolution) {
                    continue
                }

                if (resolutions.indexOf(description) < 0) {
                    model.append({streamIndex: i,
                                  fps: fps,
                                  description: description})
                    resolutions.push(description)
                }
            }

            updateIndex()
        }

        onCurrentIndexChanged: {
            if (recCameraControls.paramLevel < 1)
                recCameraControls.paramLevel = 1

            recCameraControls.updateStream()
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
            updateFormatControls()
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

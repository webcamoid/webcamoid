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
import Ak 1.0

GridLayout {
    id: recCameraControls
    columns: 3

    function filterBy(caps, prop, filters)
    {
        let vals = []

        for (let i in caps) {
            let videoCaps = caps[i]
            let filterCaps = {format: videoCaps.format,
                              size: Qt.size(videoCaps.width, videoCaps.height),
                              fps: AkFrac.create(videoCaps.fps).string}
            let pass = false

            for (let filterProp in filters)
                if (filterCaps[filterProp] != filters[filterProp]) {
                    pass = true

                    break
                }

            if (pass)
                continue

            let val = filterCaps[prop]

            if (vals.indexOf(val) < 0)
                vals.push(val)
        }

        return vals
    }

    function filterCaps(caps, filters)
    {
        for (let i in caps) {
            let videoCaps = caps[i]
            let filterCaps = {format: videoCaps.format,
                              size: Qt.size(videoCaps.width, videoCaps.height),
                              fps: AkFrac.create(videoCaps.fps).string}
            let pass = false

            for (let filterProp in filters)
                if (filterCaps[filterProp] != filters[filterProp]) {
                    pass = true

                    break
                }

            if (pass)
                continue

            return i
        }

        return -1
    }

    function indexOf(capsList, caps)
    {
        for (let i in capsList) {
            let videoCaps = capsList[i]
            let size = Qt.size(videoCaps.width, videoCaps.height)
            let fps = AkFrac.create(caps.fps).string

            if (videoCaps.format == caps.format
                && size == caps.size
                && videoCaps.fps == caps.fps)
                return i
        }

        return -1
    }

    function indexBy(model, value)
    {
        return model.map(function (obj) {
                            return obj.value
                         }).indexOf(value)
    }

    function createModel(list, prop)
    {
        let maps = {
            format: function (value) {
                return {description: typeof value === 'number'?
                                         AkVideoCaps.pixelFormatToString(value).toUpperCase():
                                         value.toUpperCase(),
                        value: value}
            },
            size: function (value) {
                return {description: value.width + "x" + value.height,
                        value: value}
            },
            fps: function (value) {
                return {description: Number(AkFrac.create(value).value.toFixed(2)),
                        value: AkFrac.create(value).string}
            }
        }

        return list.map(maps[prop])
    }

    function updateFormatControls(mediaChanged)
    {
        cbxFormat.onCurrentIndexChanged.disconnect(cbxFormat.update)
        cbxResolution.onCurrentIndexChanged.disconnect(cbxResolution.update)
        cbxFps.onCurrentIndexChanged.disconnect(cbxFps.update)

        let ncaps = VideoCapture.listTracks().length
        let rawCaps = []

        for (let i = 0; i < ncaps; i++) {
            let caps = VideoCapture.rawCaps(i)

            switch (AkCaps.create(caps).type) {
            case AkCaps.CapsVideo:
                rawCaps.push(AkVideoCaps.create(caps))
                break
            case AkCaps.CapsVideoCompressed:
                rawCaps.push(AkCompressedVideoCaps.create(caps))
                break
            default:
                break
            }
        }

        let index = mediaChanged || VideoCapture.streams.length < 1?
                    0: VideoCapture.streams[0]

        if (index >= ncaps)
            index = 0;

        let caps = VideoCapture.rawCaps(index)
        let currentCaps = AkCaps.create(caps).type == AkCaps.CapsVideo?
                            AkVideoCaps.create(caps):
                            AkCompressedVideoCaps.create(caps)

        let filters = {}
        cbxFormat.model = createModel(filterBy(rawCaps, "format", filters),
                                      "format")
        filters.format = currentCaps.format
        cbxResolution.model = createModel(filterBy(rawCaps, "size", filters),
                                          "size")
        filters.size = Qt.size(currentCaps.width, currentCaps.height)
        cbxFps.model = createModel(filterBy(rawCaps, "fps", filters), "fps")

        cbxFormat.currentIndex = indexBy(cbxFormat.model, currentCaps.format)
        cbxResolution.currentIndex = indexBy(cbxResolution.model,
                                             Qt.size(currentCaps.width,
                                                     currentCaps.height))
        cbxFps.currentIndex = indexBy(cbxFps.model, currentCaps.fps)

        cbxFormat.currentIndex = indexBy(cbxFormat.model, currentCaps.format)
        cbxResolution.currentIndex = indexBy(cbxResolution.model,
                                             Qt.size(currentCaps.width,
                                                     currentCaps.height))
        cbxFps.currentIndex = indexBy(cbxFps.model,
                                      AkFrac.create(currentCaps.fps).string)

        cbxFormat.onCurrentIndexChanged.connect(cbxFormat.update)
        cbxResolution.onCurrentIndexChanged.connect(cbxResolution.update)
        cbxFps.onCurrentIndexChanged.connect(cbxFps.update)
    }

    function updateStreams(filters)
    {
        let ncaps = VideoCapture.listTracks().length
        let rawCaps = []

        for (let i = 0; i < ncaps; i++) {
            let caps = VideoCapture.rawCaps(i)

            switch (AkCaps.create(caps).type) {
            case AkCaps.CapsVideo:
                rawCaps.push(AkVideoCaps.create(caps))
                break
            case AkCaps.CapsVideoCompressed:
                rawCaps.push(AkCompressedVideoCaps.create(caps))
                break
            default:
                break
            }
        }

        let maps = {
            format: cbxFormat.model[cbxFormat.currentIndex]?
                    cbxFormat.model[cbxFormat.currentIndex].value:
                    cbxFormat.model[0].value,
            size: cbxResolution.model[cbxResolution.currentIndex]?
                    cbxResolution.model[cbxResolution.currentIndex].value:
                    cbxResolution.model[0].value,
            fps: cbxFps.model[cbxFps.currentIndex]?
                    cbxFps.model[cbxFps.currentIndex].value:
                    cbxFps.model[0].value
        }

        let capsFilters = {}

        for (let i in filters)
            capsFilters[filters[i]] = maps[filters[i]]

        let index = filterCaps(rawCaps, capsFilters);

        if (index < 0)
            return

        VideoCapture.streams = [index]
        updateFormatControls(false)
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
        updateFormatControls(false)
    }

    Connections {
        target: VideoCapture

        function onMediaChanged()
        {
            recCameraControls.createCameraControls()
            recCameraControls.updateFormatControls(true)
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
        model: []
        textRole: "description"
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.description: lblFormat.text

        function update()
        {
            recCameraControls.updateStreams(["format"])
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
        model: []
        textRole: "description"
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.description: lblResolution.text

        function update()
        {
            recCameraControls.updateStreams(["format", "size"])
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
        model: []
        textRole: "description"
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.description: lblFps.text

        function update()
        {
            recCameraControls.updateStreams(["format", "size", "fps"])
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
            updateFormatControls(false)
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

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
        var vals = []

        for (var i in caps) {
            var videoCaps = caps[i]
            var filterCaps = {fourcc: videoCaps.fourcc,
                              size: Qt.size(videoCaps.width, videoCaps.height),
                              fps: videoCaps.fps}
            var pass = false

            for (var filterProp in filters)
                if (filterCaps[filterProp] != filters[filterProp]) {
                    pass = true

                    break
                }

            if (pass)
                continue

            var val = filterCaps[prop]

            if (vals.indexOf(val) < 0)
                vals.push(val)
        }

        return vals
    }

    function filterCaps(caps, filters)
    {
        for (var i in caps) {
            var videoCaps = caps[i]
            var filterCaps = {fourcc: videoCaps.fourcc,
                              size: Qt.size(videoCaps.width, videoCaps.height),
                              fps: videoCaps.fps}
            var pass = false

            for (var filterProp in filters)
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
        for (var i in capsList) {
            var videoCaps = capsList[i]
            var size = Qt.size(videoCaps.width, videoCaps.height)

            if (videoCaps.fourcc == caps.fourcc
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
        var maps = {
            fourcc: function (value) {
                return {description: value,
                        value: value}
            },
            size: function (value) {
                return {description: value.width + "x" + value.height,
                        value: value}
            },
            fps: function (value) {
                return {description: Number(AkFrac.create(value).value.toFixed(2)),
                        value: value}
            }
        }

        return list.map(maps[prop])
    }

    function updateFormatControls(mediaChanged)
    {
        cbxFormat.onCurrentIndexChanged.disconnect(cbxFormat.update)
        cbxResolution.onCurrentIndexChanged.disconnect(cbxResolution.update)
        cbxFps.onCurrentIndexChanged.disconnect(cbxFps.update)

        var ncaps = VideoCapture.listTracks().length
        var rawCaps = []

        for (var i = 0; i < ncaps; i++)
            rawCaps.push(AkCaps.create(VideoCapture.rawCaps(i)).toMap())

        var index = mediaChanged || VideoCapture.streams.length < 1?
                    0: VideoCapture.streams[0]

        if (index >= ncaps)
            index = 0;

        var currentCaps = AkCaps.create(VideoCapture.rawCaps(index)).toMap()

        var filters = {}
        cbxFormat.model = createModel(filterBy(rawCaps, "fourcc", filters),
                                      "fourcc")
        filters.fourcc = currentCaps.fourcc
        cbxResolution.model = createModel(filterBy(rawCaps, "size", filters),
                                          "size")
        filters.size = Qt.size(currentCaps.width, currentCaps.height)
        cbxFps.model = createModel(filterBy(rawCaps, "fps", filters), "fps")

        cbxFormat.currentIndex = indexBy(cbxFormat.model, currentCaps.fourcc)
        cbxResolution.currentIndex = indexBy(cbxResolution.model,
                                             Qt.size(currentCaps.width,
                                                     currentCaps.height))
        cbxFps.currentIndex = indexBy(cbxFps.model, currentCaps.fps)

        cbxFormat.onCurrentIndexChanged.connect(cbxFormat.update)
        cbxResolution.onCurrentIndexChanged.connect(cbxResolution.update)
        cbxFps.onCurrentIndexChanged.connect(cbxFps.update)
    }

    function updateStreams(filters)
    {
        var ncaps = VideoCapture.listTracks().length
        var rawCaps = []

        for (var i = 0; i < ncaps; i++)
            rawCaps.push(AkCaps.create(VideoCapture.rawCaps(i)).toMap())

        var maps = {
            fourcc: cbxFormat.model[cbxFormat.currentIndex]?
                    cbxFormat.model[cbxFormat.currentIndex].value:
                    cbxFormat.model[0].value,
            size: cbxResolution.model[cbxResolution.currentIndex]?
                    cbxResolution.model[cbxResolution.currentIndex].value:
                    cbxResolution.model[0].value,
            fps: cbxFps.model[cbxFps.currentIndex]?
                    cbxFps.model[cbxFps.currentIndex].value:
                    cbxFps.model[0].value
        }

        var capsFilters = {}

        for (var i in filters)
            capsFilters[filters[i]] = maps[filters[i]]

        var index = filterCaps(rawCaps, capsFilters);

        if (index < 0)
            return

        VideoCapture.streams = [index]
        updateFormatControls(false)
    }

    function createControls(controls, where)
    {
        var minimumLeftWidth = lblFormat.width
        var minimumRightWidth = btnReset.width

        // Remove old controls.
        for(var i = where.children.length - 1; i >= 0 ; i--)
            where.children[i].destroy()

        // Create new ones.
        for (var control in controls) {
            var component = Qt.createComponent("CameraControl.qml")

            if (component.status !== Component.Ready)
                continue

            var obj = component.createObject(where)
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

    function createCameraControls()
    {
        var minimumImageWidth =
                recCameraControls.createControls(VideoCapture.imageControls(),
                                                 clyImageControls)
        var minimumCameraWidth =
                recCameraControls.createControls(VideoCapture.cameraControls(),
                                                 clyCameraControls)

        var minimumLeftWidth = Math.max(minimumImageWidth[0],
                                        minimumCameraWidth[0])
        var minimumRightWidth = Math.max(minimumImageWidth[1],
                                         minimumCameraWidth[1])

        var controls = [clyImageControls, clyCameraControls]

        for (var where in controls)
            for (var child in controls[where].children) {
                controls[where].children[child].minimumLeftWidth =
                        minimumLeftWidth
                controls[where].children[child].minimumRightWidth =
                        minimumRightWidth
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

        onMediaChanged: {
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

        function update()
        {
            recCameraControls.updateStreams(["fourcc"])
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

        function update()
        {
            recCameraControls.updateStreams(["fourcc", "size"])
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

        function update()
        {
            recCameraControls.updateStreams(["fourcc", "size", "fps"])
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

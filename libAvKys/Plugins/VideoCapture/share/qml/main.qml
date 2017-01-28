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
import AkQml 1.0

GridLayout {
    id: recCameraControls
    columns: 3

    property bool locked: false
    property var caps: []

    function filterBy(prop, filters)
    {
        var vals = []

        for (var i in recCameraControls.caps) {
            var videoCaps = recCameraControls.caps[i]

            var caps = {fourcc: videoCaps.fourcc,
                        size: Qt.size(videoCaps.width, videoCaps.height),
                        fps: videoCaps.fps}

            var pass = false

            for (var filterProp in filters)
                if (caps[filterProp] != filters[filterProp]) {
                    pass = true

                    break
                }

            if (pass)
                continue

            var val = caps[prop]

            if (vals.indexOf(val) >= 0)
                continue

            vals.push(val)
        }

        return vals
    }

    function indexOf(caps)
    {
        for (var i in recCameraControls.caps) {
            var videoCaps = recCameraControls.caps[i]
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
        var maps = {fourcc: function (value) {
                        return {description: value,
                                value: value}
                    },
                    size: function (value) {
                        return {description: value.width + "x" + value.height,
                                value: value}
                    },
                    fps: function (value) {
                        return {description: Number(Ak.newFrac(value).value.toFixed(2)),
                                value: value}
                    }}

        return list.map(maps[prop])
    }

    function updateStreams()
    {
        VideoCapture.streams = [indexOf({fourcc: cbxFormat.model[cbxFormat.currentIndex]?
                                                 cbxFormat.model[cbxFormat.currentIndex].value: "",
                                         size: cbxResolution.model[cbxResolution.currentIndex]?
                                               cbxResolution.model[cbxResolution.currentIndex].value: Qt.size(-1, -1),
                                         fps: cbxFps.model[cbxFps.currentIndex]?
                                              cbxFps.model[cbxFps.currentIndex].value: Ak.newFrac()})]
    }

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

        var ncaps = VideoCapture.listTracks().length
        var rawCaps = []

        for (var i = 0; i < ncaps; i++)
            rawCaps.push(Ak.newCaps(VideoCapture.rawCaps(i)).toMap())

        caps = rawCaps
    }

    function updateParams(streams)
    {
        if (streams.length > 0) {
            var videoCaps = recCameraControls.caps[streams[0] < 0? 0: streams[0]]

            if (typeof videoCaps == "undefined")
                return

            cbxFormat.currentIndex = indexBy(cbxFormat.model,
                                             videoCaps.fourcc)
            cbxResolution.currentIndex = indexBy(cbxResolution.model,
                                                 Qt.size(videoCaps.width,
                                                         videoCaps.height))
            cbxFps.currentIndex = indexBy(cbxFps.model,
                                          videoCaps.fps)
        } else {
            cbxFormat.currentIndex = -1
            cbxResolution.currentIndex = -1
            cbxFps.currentIndex = -1
        }
    }

    Component.onCompleted: createInterface()

    Connections {
        target: VideoCapture

        onImageControlsChanged: controlsUpdated(imageControls)
        onCameraControlsChanged: controlsUpdated(cameraControls)
        onMediaChanged: createInterface()
        onStreamsChanged: updateParams(streams)
    }

    Label {
        id: lblFormat
        text: qsTr("Video format")
        Layout.minimumWidth: minimumWidth

        property int minimumWidth: 0
    }
    ComboBox {
        id: cbxFormat
        model: createModel(filterBy("fourcc"), "fourcc")
        textRole: "description"
        Layout.fillWidth: true
        Layout.columnSpan: 2

        onCurrentIndexChanged: {
            if (locked)
                return

            locked = true

            cbxResolution.model = model.length < 1?
                        []: createModel(filterBy("size",
                                                 {fourcc: model[currentIndex < 0?
                                                             0: currentIndex].value}),
                                        "size")

            cbxFps.model = model.length < 1?
                        []: createModel(filterBy("fps",
                                                 {fourcc: model[currentIndex < 0?
                                                             0: currentIndex].value,
                                                  size: cbxResolution.model[0].value}),
                                        "fps")

            updateStreams()
            locked = false
        }
        onModelChanged: {
            cbxResolution.model = model.length < 1?
                        []: createModel(filterBy("size",
                                                 {fourcc: model[0].value}),
                                        "size")
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

        onCurrentIndexChanged: {
            if (locked)
                return

            locked = true

            cbxFps.model = model.length < 1?
                        []: createModel(filterBy("fps",
                                                 {fourcc: cbxFormat.model[cbxFormat.currentIndex < 0?
                                                                            0: cbxFormat.currentIndex].value,
                                                  size: model[currentIndex < 0?
                                                                0: currentIndex].value}),
                                        "fps")

            updateStreams()
            locked = false
        }
        onModelChanged: {
            cbxFps.model = model.length < 1?
                        []: createModel(filterBy("fps",
                                                 {fourcc: cbxFormat.model[cbxFormat.currentIndex < 0?
                                                                            0: cbxFormat.currentIndex].value,
                                                  size: model[0].value}),
                                        "fps")
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

        onCurrentIndexChanged: {
            if (locked)
                return

            locked = true
            updateStreams()
            locked = false
        }
    }
    Label {
        Layout.fillWidth: true
        Layout.columnSpan: 2
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

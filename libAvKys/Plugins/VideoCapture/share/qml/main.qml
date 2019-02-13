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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import AkQml 1.0
import AkQmlControls 1.0

GridLayout {
    id: recCameraControls
    columns: 3

    property int lock: 0
    property int lockStreams: 0
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

            if (vals.indexOf(val) < 0)
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
                return {description: Number(Ak.newFrac(value).value.toFixed(2)),
                        value: value}
            }
        }

        return list.map(maps[prop])
    }

    function updateStreams()
    {
        if (lockStreams > 0)
            return

        var index = indexOf({fourcc: cbxFormat.model[cbxFormat.currentIndex]?
                                    cbxFormat.model[cbxFormat.currentIndex].value: "",
                            size: cbxResolution.model[cbxResolution.currentIndex]?
                                  cbxResolution.model[cbxResolution.currentIndex].value: Qt.size(-1, -1),
                            fps: cbxFps.model[cbxFps.currentIndex]?
                                 cbxFps.model[cbxFps.currentIndex].value: Ak.newFrac()});

        if (index < 0)
            return

        VideoCapture.streams = [index]
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
        var index = VideoCapture.streams.length < 1? 0: VideoCapture.streams[0]
        var currentCaps = Ak.newCaps(VideoCapture.rawCaps(index)).toMap()

        lock++
        lockStreams++;
        cbxFormat.update()
        cbxFormat.currentIndex = indexBy(cbxFormat.model, currentCaps.fourcc)
        cbxResolution.update()
        cbxResolution.currentIndex = indexBy(cbxResolution.model,
                                             Qt.size(currentCaps.width,
                                                     currentCaps.height))
        cbxFps.update()
        cbxFps.currentIndex = indexBy(cbxFps.model, currentCaps.fps)
        lockStreams--;
        lock--;
    }

    Component.onCompleted: createInterface()

    Connections {
        target: VideoCapture

        onMediaChanged: createInterface()
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
            lock++;
            model = []
            model = createModel(filterBy("fourcc"), "fourcc")
            currentIndex = -1

            if (model.length > 0)
                currentIndex = 0

            lock--;
        }

        function lockedUpdate()
        {
            if (lock > 0)
                return

            update()
        }

        onCurrentIndexChanged: cbxResolution.lockedUpdate()
        onModelChanged: cbxResolution.update()
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
            lock++;
            model = []
            model = cbxFormat.model.length < 1?
                        []: createModel(filterBy("size",
                                                 {fourcc: cbxFormat.model[cbxFormat.currentIndex < 0?
                                                             0: cbxFormat.currentIndex].value}),
                                        "size")
            currentIndex = -1

            if (model.length > 0)
                currentIndex = 0

            lock--;
        }

        function lockedUpdate()
        {
            if (lock > 0)
                return

            update()
        }

        onCurrentIndexChanged: cbxFps.lockedUpdate()
        onModelChanged: cbxFps.update()
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
            lock++;
            model = []
            model = cbxResolution.model.length < 1?
                 []: createModel(filterBy("fps",
                                          {fourcc: cbxFormat.model[cbxFormat.currentIndex < 0?
                                                      0: cbxFormat.currentIndex].value,
                                           size: cbxResolution.model[cbxResolution.currentIndex < 0?
                                                      0: cbxResolution.currentIndex].value}),
                                 "fps")
            currentIndex = -1

            if (model.length > 0)
                currentIndex = 0

            lock--;

            updateStreams()
        }

        function lockedUpdate()
        {
            if (lock > 0)
                return

            update()
        }

        onCurrentIndexChanged: updateStreams()
    }
    Label {
        Layout.fillWidth: true
        Layout.columnSpan: 2
    }
    AkButton {
        id: btnReset
        label: qsTr("Reset")
        iconRc: "image://icons/reset"
        Layout.minimumWidth: minimumWidth

        property int minimumWidth: 75

        onClicked: {
            VideoCapture.reset()
            createInterface()
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

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

    property var controls: []

    function createControl(itemType, where)
    {
        return Qt.createQmlObject("import QtQuick.Controls 1.4; "
                                  + itemType
                                  + " {property string controlName: \"\";"
                                  + " property string controlType: \""
                                  + itemType
                                  + "\"}",
                                  where,
                                  "VideoCapture")
    }

    function connectControls()
    {
        for (var i = 0; i < recCameraControls.controls.length; i++) {
            var control = recCameraControls.controls[i]

            if (control.controlType === "Slider")
                control.onValueChanged.connect(sliderChanged)
            else if (control.controlType === "SpinBox")
                control.onValueChanged.connect(spinBoxChanged)
            else if (control.controlType === "CheckBox")
                control.onCheckedChanged.connect(updateControls)
            else if (control.controlType === "ComboBox")
                control.onCurrentIndexChanged.connect(updateControls)
        }

        VideoCapture.imageControlsChanged.connect(controlsUpdated)
        VideoCapture.cameraControlsChanged.connect(controlsUpdated)
        VideoCapture.sizeChanged.connect(resolutionChanged)
    }

    function disconnectControls()
    {
        for (var i = 0; i < recCameraControls.controls.length; i++) {
            var control = recCameraControls.controls[i]

            if (control.controlType === "Slider")
                control.onValueChanged.disconnect(sliderChanged)
            else if (control.controlType === "SpinBox")
                control.onValueChanged.disconnect(spinBoxChanged)
            else if (control.controlType === "CheckBox")
                control.onCheckedChanged.disconnect(updateControls)
            else if (control.controlType === "ComboBox")
                control.onCurrentIndexChanged.disconnect(updateControls)
        }

        VideoCapture.imageControlsChanged.disconnect(controlsUpdated)
        VideoCapture.cameraControlsChanged.disconnect(controlsUpdated)
        VideoCapture.sizeChanged.disconnect(resolutionChanged)
    }

    function spinBoxChanged()
    {
        disconnectControls()

        for (var i = 0; i < recCameraControls.controls.length; i++)
            if (recCameraControls.controls[i].controlType === "SpinBox")
                recCameraControls.controls[i - 1].value =
                        recCameraControls.controls[i].value

        connectControls()

        updateControls()
    }

    function sliderChanged()
    {
        disconnectControls()

        for (var i = 0; i < recCameraControls.controls.length; i++)
            if (recCameraControls.controls[i].controlType === "Slider")
                recCameraControls.controls[i + 1].value =
                        recCameraControls.controls[i].value

        connectControls()

        updateControls()
    }

    function updateControls()
    {
        var controls = {}

        for (var i = 0; i < recCameraControls.controls.length; i++) {
            var control = recCameraControls.controls[i]

            if (control.controlType === "Slider")
                controls[control.controlName] = control.value
            else if (control.controlType === "CheckBox")
                controls[control.controlName] = control.checked
            else if (control.controlType === "ComboBox")
                controls[control.controlName] = control.currentIndex
        }

        disconnectControls()

        VideoCapture.setImageControls(controlId, controls)
        VideoCapture.setCameraControls(controlId, controls)

        connectControls()
    }

    function controlsUpdated(camera, controls)
    {
        if (camera !== controlId)
            return

        disconnectControls()

        for (var i = 0; i < recCameraControls.controls.length; i++) {
            var control = recCameraControls.controls[i]

            if (!controls.hasOwnProperty(control.controlName))
                continue

            if (control.controlType === "Slider")
                control.value = controls[control.controlName]
            else if (control.controlType === "CheckBox")
                control.checked = controls[control.controlName]
            else if (control.controlType === "ComboBox")
                control.currentIndex = controls[control.controlName]
        }

        connectControls()
    }

    function resolutionChanged(camera, size)
    {
        if (camera !== controlId)
            return

        var resolutions = VideoCapture.availableSizes(controlId)
        cbxResolution.currentIndex = resolutions.indexOf(size)
    }

    function createControls(controls)
    {
        for (var control in controls) {
            var controlParams = controls[control]

            if (controlParams[1] === "integer" ||
                controlParams[1] === "integer64") {
                var labelInt = createControl("Label", recCameraControls)
                labelInt.text = controlParams[0]

                var slider = createControl("Slider", recCameraControls)
                slider.controlName = controlParams[0]
                slider.minimumValue = controlParams[2]
                slider.maximumValue = controlParams[3]
                slider.stepSize = controlParams[4]
                slider.value = controlParams[6]
                slider.Layout.fillWidth = true
                recCameraControls.controls.push(slider)

                var spinBox = createControl("SpinBox", recCameraControls)
                spinBox.controlName = controlParams[0]
                spinBox.minimumValue = controlParams[2]
                spinBox.maximumValue = controlParams[3]
                spinBox.stepSize = controlParams[4]
                spinBox.value = controlParams[6]
                spinBox.Layout.fillWidth = true
                spinBox.Layout.maximumWidth = btnReset.width
                spinBox.Layout.alignment = Qt.AlignHCenter
                recCameraControls.controls.push(spinBox)
            }
            else if (controlParams[1] === "boolean") {
                var checkBox = createControl("CheckBox", recCameraControls)
                checkBox.controlName = controlParams[0]
                checkBox.text = controlParams[0]
                checkBox.checked = controlParams[6] !== 0
                checkBox.Layout.columnSpan = 3
                recCameraControls.controls.push(checkBox)
            }
            else if (controlParams[1] === "menu") {
                var labelMenu = createControl("Label", recCameraControls)
                labelMenu.text = controlParams[0]

                var comboBox = createControl("ComboBox", recCameraControls)
                comboBox.controlName = controlParams[0]
                comboBox.model = controlParams[7]
                comboBox.currentIndex = controlParams[6]
                recCameraControls.controls.push(comboBox)

                var labelBlank = createControl("Label", recCameraControls)
            }
        }
    }

    Component.onCompleted: {
        var model = []
        var resolutions = VideoCapture.availableSizes(controlId)

        for (var resolution in resolutions) {
            var resolutionStr = resolutions[resolution].width
                                + "x"
                                + resolutions[resolution].height

            model.push(resolutionStr)
        }

        cbxResolution.model = model
        cbxResolution.currentIndex = resolutions.indexOf(VideoCapture.size(controlId))

        createControls(VideoCapture.imageControls(controlId))
        createControls(VideoCapture.cameraControls(controlId))

        var label = createControl("Label", recCameraControls)
        label.Layout.fillHeight = true

        connectControls()
    }

    Label {
        id: lblResolution
        text: qsTr("Video resolution")
    }

    ComboBox {
        id: cbxResolution

        onCurrentIndexChanged: {
            var resolutions = VideoCapture.availableSizes(controlId)
            VideoCapture.setSize(controlId, resolutions[currentIndex])
        }
    }

    Button {
        id: btnReset
        text: qsTr("Reset")
        iconName: "reset"
        Layout.alignment: Qt.AlignRight

        onClicked: VideoCapture.reset(controlId)
    }
}

/* Webcamoid, webcam capture application. Crop Plug-in.
 * Copyright (C) 2022  Tj <hacker@iam.tj>
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
import CropElement 1.0

GridLayout {
    property real aspectRatio: 0
    property bool firstMax: true
    property bool firstMin: true
    property int tipDelay: 1000
    property int tipTimeout: 5000
    property rect source: Qt.rect(0,0,0,0)
    property rect box: Qt.rect(0,0,0,0)

    columns: 4
    Layout.fillWidth: true
    Layout.rightMargin: 8

    Component.onCompleted: {
        Cropping.initLimits();
    }

    Connections {
        target: Cropping

        // provides QML type 'rect' (from QRectF)
        function onBoxChanged(newBox) {
            // trigger calls to C++ setters
            sliderWidth.bFromRemote = true;
            sliderHeight.bFromRemote = true;
            sliderLeft.bFromRemote = true;
            sliderTop.bFromRemote = true;

            sliderWidth.value = box.width = newBox.width;
            sliderHeight.value = box.height = newBox.height;
            sliderLeft.value = box.x = newBox.x;
            sliderTop.value = box.y = newBox.y;
        }
        function onMaximumChanged(newMax) {
            // don't trigger C++ calls whilst ensuring values are within new limits
            if (sliderWidth.value > newMax.width) {
                sliderWidth.bFromRemote = true;
                sliderWidth.value = newMax.width;
            }
            if (sliderHeight.value > newMax.height) {
                sliderHeight.bFromRemote = true;
                sliderHeight.value = newMax.height;
            }

            sliderWidth.to = newMax.width;
            sliderHeight.to = newMax.height;
            if (firstMax) {
                source.width = newMax.width;
                source.height = newMax.height;
                sliderLeft.to = source.width - sliderWidth.from;
                sliderTop.to = source.height - sliderHeight.from;
                firstMax = false;
            }
        }
        function onMinimumChanged(newMin) {
            // don't trigger C++ calls whilst ensuring values are within new limits
            if (sliderWidth.value < newMin.width) {
                sliderWidth.bFromRemote = true;
                sliderWidth.value = newMin.width;
            }
            if (sliderHeight.value < newMin.height) {
                sliderHeight.bFromRemote = true;
                sliderHeight.value = newMin.height;
            }

            sliderWidth.from = newMin.width;
            sliderHeight.from = newMin.height;
            if (firstMin) {
                sliderLeft.to = source.width - newMin.width;
                sliderTop.to = source.height - newMin.height;
                firstMin = false;
            }
        }
        function onAspectRatioChanged(newAspect) {
            aspectRatio = newAspect;
        }
    }

    function updateRemoteBox(x, y, w, h) {
        let changed = false;

        if (x !== -1) {
            if (x >= sliderLeft.from && x <= sliderLeft.to) {
                box.x = x;
                changed = true;
            }
        }
        if (y !== -1) {
            if (y >= sliderTop.from && y <= sliderTop.to) {
                box.y = y;
                changed = true;
            }
        }
        // need to abide by aspect ratio
        if (w !== -1) {
            if (w >= sliderWidth.from && w <= sliderWidth.to) {
                let newH = w / aspectRatio;
                box.width = w;
                box.height = Math.round(newH);
                changed = true;
            }
        }
        if (h !== -1) {
            if (h >= sliderHeight.from && h <= sliderHeight.to) {
                let newW = h * aspectRatio;
                box.height = h;
                box.width = Math.round(newW);
                changed = true;
            }
        }

        if (changed) {
            Cropping.setBox(box);
        }

        return changed;
    }

    function myLeftChanged(x) {
        // was called locally due to GUI change
        if (sliderLeft.bFromRemote === false)
            updateRemoteBox(x, -1, -1, -1);
        sliderLeft.bFromRemote = false;
    }
    function myTopChanged(y) {
        if (sliderTop.bFromRemote === false)
            updateRemoteBox(-1, y, -1, -1);
        sliderTop.bFromRemote = false;
    }
    function myWidthChanged(w) {
        if (sliderWidth.bFromRemote === false)
            updateRemoteBox(-1, -1, w, -1);
        sliderWidth.bFromRemote = false;
    }
    function myHeightChanged(h) {
        if (sliderHeight.bFromRemote === false)
            updateRemoteBox(-1, -1, -1, h);
        sliderHeight.bFromRemote = false;
    }

    Label {
        id: lAspectReminder
        text: qsTr("Aspect ratio will be maintained. Chain 'Scale' effect to change it.")
        wrapMode: Text.WordWrap
        Layout.columnSpan: 4
        Layout.fillWidth: true
    }

    Label {
        id: lSliderLeft
        text: qsTr("Left")
    }
    Label {
        id:valLeft
        text: sliderLeft.value
    }
    Slider {
        id: sliderLeft
        Layout.minimumWidth: 128
        Layout.fillWidth: true
        Accessible.name: qsTr("Left")
        Accessible.description: qsTr("Left (x) coordinate ( %1 to %2)").arg(from).arg(to)
        ToolTip.text: Accessible.description
        ToolTip.visible: hovered
        ToolTip.delay: tipDelay
        ToolTip.timeout: tipTimeout
        stepSize: 1
        from: 0
        to: 0
        value: 0
        onValueChanged: myLeftChanged(value)
        property bool iCompleted: false
        property bool bFromRemote: false

        Component.onCompleted: {
            console.debug("onCompleted() sliderLeft");
            iCompleted = true;
        }
    }
    Label {
        id: valMaxTo
        text: sliderLeft.to
    }

    Label {
        id: lSliderTop
        text: qsTr("Top")
    }
    Label {
        id: valTop
        text: sliderTop.value
    }
    Slider {
        id: sliderTop
        Layout.minimumWidth: 128
        Layout.fillWidth: true
        Accessible.name: qsTr("Top")
        Accessible.description: qsTr("Top (y) coordinate ( %1 to %2)").arg(from).arg(to)
        ToolTip.text: Accessible.description
        ToolTip.visible: hovered
        ToolTip.delay: tipDelay
        ToolTip.timeout: tipTimeout
        stepSize: 1
        from: 0
        to: 0
        value: 0
        onValueChanged: myTopChanged(value)
        property bool iCompleted: false
        property bool bFromRemote: false

        Component.onCompleted: {
            console.debug("onCompleted() sliderTop")
            iCompleted = true;
        }
    }
    Label {
        id: valMaxTop
        text: sliderTop.to
    }

    Label {
        id: lSliderWidth
        text: qsTr("Width")
    }
    Label {
        id: valWidth
        text: sliderWidth.value
    }
    Slider {
        id: sliderWidth
        Layout.minimumWidth: 128
        Layout.fillWidth: true
        Accessible.name: qsTr("Width")
        Accessible.description: qsTr("Width ( %1 to %2)").arg(from).arg(to)
        ToolTip.text: Accessible.description
        ToolTip.visible: hovered
        ToolTip.delay: tipDelay
        ToolTip.timeout: tipTimeout
        stepSize: 1
        from: 1
        to: 1
        value: 1
        onValueChanged: myWidthChanged(value)
        property bool iCompleted: false
        property bool bFromRemote: false

        Component.onCompleted: {
            console.debug("onCompleted() sliderWidth")
            iCompleted = true;
        }
    }
    Label {
        id: valMaxWidth
        text: sliderWidth.to
    }

    Label {
        id: lSliderHeight
        text: qsTr("Height")
    }
    Label {
        id: valHeight
        text: sliderHeight.value
    }
    Slider {
        id: sliderHeight
        Layout.minimumWidth: 128
        Layout.fillWidth: true
        Accessible.name: qsTr("Height")
        Accessible.description: qsTr("Height ( %1 TO %2)").arg(from).arg(to)
        ToolTip.text: Accessible.description
        ToolTip.visible: hovered
        ToolTip.delay: tipDelay
        ToolTip.timeout: tipTimeout
        stepSize: 1
        from: 1
        to: 1
        value: 1
        onValueChanged: myHeightChanged(value)
        property bool iCompleted: false
        property bool bFromRemote: false

        Component.onCompleted: {
            console.debug("onCompleted() sliderHeight")
            iCompleted = true;
        }
    }
    Label {
        id: valMaxHeight
        text: sliderHeight.to
    }
    Button {
        id: bReset
        text: qsTr("Reset")
        Accessible.name: text
        Accessible.description: qsTr("Reset to picture size");
        ToolTip.text: Accessible.description
        ToolTip.visible: hovered
        ToolTip.delay: tipDelay
        ToolTip.timeout: tipTimeout
        Layout.columnSpan: 4
        onPressed: Cropping.resetBox()
    }
}

/* Webcamoid, webcam capture application. Zoom Plug-in.
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
import ZoomElement 1.0

GridLayout {
    id: grid1
    property int tipDelay: 1000
    property int tipTimeout: 5000

    columns: 1
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.rightMargin: 8

    Connections {
        target: Zoom
        function zoomChanged(newZoom)
        {
            console.debug("zoomChanged()", newZoom);
        }
        function maxZoomChanged(newMaxZoom)
        {
            console.debug("maxZoomChanged()", newMaxZoom);
        }
    }

    Label {
        id: lSliderZoom
        text: qsTr("Zoom")
    }
    Label {
        id: valMaxTo
        text: { Number(sliderZoom.to).toFixed(2); }
    }
    Slider {
        id: sliderZoom
        orientation: Qt.Vertical
        Layout.minimumHeight: 128
        Layout.fillWidth: true
        Layout.fillHeight: true
        Accessible.name: qsTr("Zoom")
        Accessible.description: qsTr("Zoom (magnify) ( x%1 to x%2)").arg(from).arg(to)
        ToolTip.text: Accessible.description
        ToolTip.visible: hovered
        ToolTip.delay: tipDelay
        ToolTip.timeout: tipTimeout
        snapMode: Slider.SnapAlways
        stepSize: 0.01
        from: 1.0
        to: spSetMaximum.value
        value: 1.0
        onValueChanged: { Zoom.setZoom(Number(value).toFixed(2)); }
        onToChanged: Zoom.setMaxZoom(to)

        Component.onCompleted: {
            height: grid1.height * 0.75
            Zoom.setMaxZoom(to);
            Zoom.setZoom(value);
        }
    }
    Label {
        id:valZoom
        text: {Number(sliderZoom.value).toFixed(2); }
    }
    Label {
        id: lsetMax
        text: qsTr("Maximum")
    }
    SpinBox {
        id: spSetMaximum
        from: 1
        to: 100
        value:20
        stepSize: 1
    }
}

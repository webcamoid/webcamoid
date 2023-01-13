/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

GridLayout {
    columns: 3

    Connections {
        target: Zoom

        function onZoomChanged(zoom)
        {
            sldZoom.value = zoom
            spbZoom.value = zoom * spbZoom.multiplier
        }
    }

    Label {
        id: lblZoom
        text: qsTr("Zoom")
    }
    Slider {
        id: sldZoom
        value: Zoom.zoom
        stepSize: 0.1
        from: 1.0
        to: 10.0
        Layout.fillWidth: true
        Accessible.name: lblZoom.text

        onValueChanged: Zoom.zoom = value
    }
    SpinBox {
        id: spbZoom
        value: multiplier * Zoom.zoom
        from: multiplier * sldZoom.from
        to: multiplier * sldZoom.to
        stepSize: multiplier * sldZoom.stepSize
        editable: true
        Accessible.name: lblZoom.text

        readonly property int decimals: 1
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbZoom.from, spbZoom.to)
            top:  Math.max(spbZoom.from, spbZoom.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Zoom.zoom = value / multiplier
    }
}

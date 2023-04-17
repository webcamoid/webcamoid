/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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
import AkControls 1.0 as AK

GridLayout {
    columns: 3

    Connections {
        target: Crop

        function onTopChanged(crop)
        {
            sldTop.value = crop
            spbTop.value = crop * spbTop.multiplier
        }

        function onBottomChanged(crop)
        {
            sldBottom.value = crop
            spbBottom.value = crop * spbBottom.multiplier
        }

        function onLeftChanged(crop)
        {
            sldLeft.value = crop
            spbLeft.value = crop * spbLeft.multiplier
        }

        function onRightChanged(crop)
        {
            sldRight.value = crop
            spbRight.value = crop * spbRight.multiplier
        }
    }

    RowLayout {
        Layout.columnSpan: 3

        Button {
            text: qsTr("Edit")
            icon.source: "image://icons/edit"
            Accessible.description: qsTr("Enable edition mode")
            checked: Crop.editMode
            checkable: true

            onToggled: Crop.editMode = checked
        }
        Button {
            text: qsTr("Pixels/%")
            Accessible.description: qsTr("Select cropping unit")
            checked: Crop.relative
            checkable: true

            onToggled: Crop.relative = checked
        }
    }
    Label {
        id: txtKeepResolution
        text: qsTr("Keep resolution")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        Switch {
            checked: Crop.keepResolution
            Accessible.name: txtKeepResolution.text

            onCheckedChanged: Crop.keepResolution = checked
        }
    }
    Label {
        id: lblLeft
        text: qsTr("Left")
    }
    Slider {
        id: sldLeft
        value: Crop.left
        stepSize: 1
        from: 0.0
        to: Crop.relative? 100.0: Crop.frameWidth
        Layout.fillWidth: true
        Accessible.name: lblLeft.text

        onValueChanged: Crop.left = value
    }
    SpinBox {
        id: spbLeft
        value: multiplier * Crop.left
        from: multiplier * sldLeft.from
        to: multiplier * sldLeft.to
        stepSize: multiplier * sldLeft.stepSize
        editable: true
        Accessible.name: lblLeft.text

        readonly property int decimals: 1
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbLeft.from, spbLeft.to)
            top:  Math.max(spbLeft.from, spbLeft.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Crop.left = value / multiplier
    }
    Label {
        id: lblRight
        text: qsTr("Right")
    }
    Slider {
        id: sldRight
        value: Crop.right
        stepSize: 1
        from: 1.0
        to: Crop.relative? 100.0: Crop.frameWidth
        Layout.fillWidth: true
        Accessible.name: lblRight.text

        onValueChanged: Crop.right = value
    }
    SpinBox {
        id: spbRight
        value: multiplier * Crop.right
        from: multiplier * sldRight.from
        to: multiplier * sldRight.to
        stepSize: multiplier * sldRight.stepSize
        editable: true
        Accessible.name: lblRight.text

        readonly property int decimals: 1
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbRight.from, spbRight.to)
            top:  Math.max(spbRight.from, spbRight.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Crop.right = value / multiplier
    }
    Label {
        id: lblTop
        text: qsTr("Top")
    }
    Slider {
        id: sldTop
        value: Crop.top
        stepSize: 1
        from: 0.0
        to: Crop.relative? 100.0: Crop.frameHeight
        Layout.fillWidth: true
        Accessible.name: lblTop.text

        onValueChanged: Crop.top = value
    }
    SpinBox {
        id: spbTop
        value: multiplier * Crop.top
        from: multiplier * sldTop.from
        to: multiplier * sldTop.to
        stepSize: multiplier * sldTop.stepSize
        editable: true
        Accessible.name: lblTop.text

        readonly property int decimals: 1
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbTop.from, spbTop.to)
            top:  Math.max(spbTop.from, spbTop.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Crop.top = value / multiplier
    }
    Label {
        id: lblBottom
        text: qsTr("Bottom")
    }
    Slider {
        id: sldBottom
        value: Crop.bottom
        stepSize: 1
        from: 1.0
        to: Crop.relative? 100.0: Crop.frameHeight
        Layout.fillWidth: true
        Accessible.name: lblBottom.text

        onValueChanged: Crop.bottom = value
    }
    SpinBox {
        id: spbBottom
        value: multiplier * Crop.bottom
        from: multiplier * sldBottom.from
        to: multiplier * sldBottom.to
        stepSize: multiplier * sldBottom.stepSize
        editable: true
        Accessible.name: lblBottom.text

        readonly property int decimals: 1
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbBottom.from, spbBottom.to)
            top:  Math.max(spbBottom.from, spbBottom.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Crop.bottom = value / multiplier
    }
    Label {
        id: txtFillColor
        text: qsTr("Fill color")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Crop.fillColor)
            title: qsTr("Choose the filling color")
            showAlphaChannel: true
            Accessible.description: txtFillColor.text

            onCurrentColorChanged: Crop.fillColor = AkUtils.toRgba(currentColor)
        }
    }
    Button {
        text: qsTr("Reset")
        icon.source: "image://icons/reset"
        Accessible.description: qsTr("Reset parameters")

        onClicked: Crop.reset()
    }
}

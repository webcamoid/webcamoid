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
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
import AkQmlControls 1.0

GridLayout {
    columns: 2

    function modeIndex(mode)
    {
        var index = -1

        for (var i = 0; i < cbxMode.model.count; i++)
            if (cbxMode.model.get(i).mode === mode) {
                index = i
                break
            }

        return index
    }

    function fromRgba(rgba)
    {
        var a = ((rgba >> 24) & 0xff) / 255.0
        var r = ((rgba >> 16) & 0xff) / 255.0
        var g = ((rgba >> 8) & 0xff) / 255.0
        var b = (rgba & 0xff) / 255.0

        return Qt.rgba(r, g, b, a)
    }

    function toRgba(color)
    {
        var a = Math.round(255 * color.a) << 24
        var r = Math.round(255 * color.r) << 16
        var g = Math.round(255 * color.g) << 8
        var b = Math.round(255 * color.b)

        return a | r | g | b
    }

    Label {
        text: qsTr("Mode")
    }
    ComboBox {
        id: cbxMode
        textRole: "text"
        currentIndex: modeIndex(Radioactive.mode)
        Layout.fillWidth: true

        model: ListModel {
            ListElement {
                text: qsTr("Soft normal")
                mode: "softNormal"
            }
            ListElement {
                text: qsTr("Hard normal")
                mode: "hardNormal"
            }
            ListElement {
                text: qsTr("Soft color")
                mode: "softColor"
            }
            ListElement {
                text: qsTr("Hard color")
                mode: "hardColor"
            }
        }

        onCurrentIndexChanged: Radioactive.mode = cbxMode.model.get(currentIndex).mode
    }

    Label {
        text: qsTr("Blur")
    }
    TextField {
        text: Radioactive.blur
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Radioactive.blur = text
    }

    Label {
        text: qsTr("Zoom")
    }
    TextField {
        text: Radioactive.zoom
        validator: RegExpValidator {
            regExp: /\d+\.\d+|\d+\.|\.\d+|\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Radioactive.zoom = text
    }

    Label {
        text: qsTr("Threshold")
    }
    TextField {
        text: Radioactive.threshold
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Radioactive.threshold = text
    }

    Label {
        text: qsTr("Luma threshold")
    }
    TextField {
        text: Radioactive.lumaThreshold
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Radioactive.lumaThreshold = text
    }

    Label {
        text: qsTr("Alpha differential")
    }
    TextField {
        text: Radioactive.alphaDiff
        validator: RegExpValidator {
            regExp: /-?\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Radioactive.alphaDiff = text
    }

    Label {
        text: qsTr("Radiation color")
    }
    AkColorButton {
        currentColor: fromRgba(Radioactive.radColor)
        title: qsTr("Choose a color")
        showAlphaChannel: true

        onCurrentColorChanged: Radioactive.radColor = toRgba(currentColor)
    }
}

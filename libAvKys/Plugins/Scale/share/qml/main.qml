/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ScaleElement

GridLayout {
    columns: 2

    function optionIndex(cbx, option)
    {
        var index = -1

        for (var i = 0; i < cbx.model.count; i++)
            if (cbx.model.get(i).option == option) {
                index = i
                break
            }

        return index
    }

    Label {
        text: qsTr("Width")
    }
    TextField {
        text: Scaling.width
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /-?\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Scaling.width = Number(text)
    }
    Label {
        text: qsTr("Height")
    }
    TextField {
        text: Scaling.height
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /-?\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Scaling.height = Number(text)
    }
    Label {
        text: qsTr("Scaling mode")
    }
    ComboBox {
        textRole: "text"
        currentIndex: optionIndex(this, Scaling.scaling)
        Layout.fillWidth: true
        model: ListModel {
            ListElement {
                text: qsTr("Fast")
                option: ScaleElement.Fast
            }
            ListElement {
                text: qsTr("Linear")
                option: ScaleElement.Linear
            }
        }

        onCurrentIndexChanged: Scaling.scaling = model.get(currentIndex).option
    }
    Label {
        text: qsTr("Aspect ratio mode")
    }
    ComboBox {
        textRole: "text"
        currentIndex: optionIndex(this, Scaling.aspectRatio)
        Layout.fillWidth: true
        model: ListModel {
            ListElement {
                text: qsTr("Ignore")
                option: ScaleElement.Ignore
            }
            ListElement {
                text: qsTr("Keep")
                option: ScaleElement.Keep
            }
            ListElement {
                text: qsTr("Expanding")
                option: ScaleElement.Expanding
            }
        }

        onCurrentIndexChanged: Scaling.aspectRatio = model.get(currentIndex).option
    }
}

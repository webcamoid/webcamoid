/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import Qt.labs.platform as LABS
import QtQuick.Layouts
import Ak
import AkControls as AK

ColumnLayout {
    id: glyHalftone

    readonly property string filePrefix: Ak.platform() == "windows"?
                                             "file:///":
                                             "file://"

    function toQrc(uri)
    {
        if (uri.indexOf(":") == 0)
            return "qrc" + uri

        return "file:" + uri
    }

    function strToSize(str)
    {
        if (str.length < 1)
            return Qt.size()

        var size = str.split("x")

        if (size.length < 2)
            return Qt.size()

        return Qt.size(size[0], size[1])
    }

    AK.LabeledComboBox {
        id: cbxPattern
        label: qsTr("Pattern")
        textRole: "text"
        Layout.fillWidth: true
        Accessible.description: label

        model: ListModel {
            ListElement {
                text: qsTr("90° Halftone 6x6")
                pattern: ":/Halftone/share/patterns/dither90Halftone6x6Matrix.bmp"
            }
            ListElement {
                text: qsTr("Cluster 3")
                pattern: ":/Halftone/share/patterns/ditherCluster3Matrix.bmp"
            }
            ListElement {
                text: qsTr("Cluster 4")
                pattern: ":/Halftone/share/patterns/ditherCluster4Matrix.bmp"
            }
            ListElement {
                text: qsTr("Cluster 8")
                pattern: ":/Halftone/share/patterns/ditherCluster8Matrix.bmp"
            }
            ListElement {
                text: qsTr("Lines 4x4")
                pattern: ":/Halftone/share/patterns/ditherLines4x4Matrix.bmp"
            }
            ListElement {
                text: qsTr("Magic 2x2")
                pattern: ":/Halftone/share/patterns/ditherMagic2x2Matrix.bmp"
            }
            ListElement {
                text: qsTr("Magic 4x4")
                pattern: ":/Halftone/share/patterns/ditherMagic4x4Matrix.bmp"
            }
            ListElement {
                text: qsTr("Ordered 4x4")
                pattern: ":/Halftone/share/patterns/ditherOrdered4x4Matrix.bmp"
            }
            ListElement {
                text: qsTr("Ordered 6x6")
                pattern: ":/Halftone/share/patterns/ditherOrdered6x6Matrix.bmp"
            }
            ListElement {
                text: qsTr("Ordered 8x8")
                pattern: ":/Halftone/share/patterns/ditherOrdered8x8Matrix.bmp"
            }
            ListElement {
                text: qsTr("Custom")
                pattern: ""
            }
        }

        onCurrentIndexChanged: Halftone.pattern = cbxPattern.model.get(currentIndex).pattern
    }
    RowLayout {
        Image {
            width: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            height: width
            fillMode: Image.PreserveAspectFit
            sourceSize: Qt.size(width, height)
            source: toQrc(txtBitmapPattern.labelText)
        }
        AK.ActionTextField {
            id: txtBitmapPattern
            icon.source: "image://icons/search"
            labelText: Halftone.pattern
            placeholderText: qsTr("Bitmap pattern")
            buttonText: qsTr("Search the image to use as pattern")
            Layout.fillWidth: true

            onLabelTextChanged: {
                for (var i = 0; i < cbxPattern.model.count; i++) {
                    if (cbxPattern.model.get(i).pattern == Halftone.pattern) {
                        cbxPattern.currentIndex = i

                        break
                    }
                    else if (i == cbxPattern.model.count - 1) {
                        cbxPattern.model.get(i).pattern = Halftone.pattern
                        cbxPattern.currentIndex = i

                        break
                    }
                }
            }
            onButtonClicked: fileDialog.open()
        }
    }

    Label {
        id: txtPatternSize
        text: qsTr("Pattern size")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: Halftone.patternSize.width + "x" + Halftone.patternSize.height
        placeholderText: qsTr("Pattern size")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /-?\d+x-?\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtPatternSize.text

        onTextChanged: Halftone.patternSize = strToSize(text)
    }
    Label {
        id: txtLightning
        text: qsTr("Lightning")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldLightning
        value: Halftone.lightning
        stepSize: 0
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtLightning.text

        onValueChanged: Halftone.lightning = value
    }

    Label {
        id: txtSlope
        text: qsTr("Slope")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: Halftone.slope
        placeholderText: qsTr("Slope")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.name: txtSlope.text

        onTextChanged: Halftone.slope = Number(text)
    }
    Label {
        id: txtInterception
        text: qsTr("Interception")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: Halftone.interception
        placeholderText: qsTr("Intercept")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.name: txtInterception.text

        onTextChanged: Halftone.interception = Number(text)
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: glyHalftone.filePrefix + picturesPath

        onAccepted: Halftone.pattern =
                    String(file).replace(glyHalftone.filePrefix, "")
    }
}

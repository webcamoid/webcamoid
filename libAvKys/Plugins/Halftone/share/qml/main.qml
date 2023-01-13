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

import QtQuick 2.12
import QtQuick.Controls 2.5
import Qt.labs.platform 1.1 as LABS
import QtQuick.Layouts 1.3

GridLayout {
    columns: 3

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

    Connections {
        target: Halftone

        function onLightningChanged(lightning)
        {
            sldLightning.value = lightning
            spbLightning.value = lightning
        }
    }

    Label {
        id: txtPattern
        text: qsTr("Pattern")
    }
    ComboBox {
        id: cbxPattern
        textRole: "text"
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.description: txtPattern.text

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
    ColumnLayout {
        Layout.columnSpan: 3

        RowLayout {
            Image {
                width: 16
                height: 16
                fillMode: Image.PreserveAspectFit
                sourceSize.width: 16
                sourceSize.height: 16
                source: toQrc(txtBitmapPattern.text)
            }
            TextField {
                id: txtBitmapPattern
                text: Halftone.pattern
                placeholderText: qsTr("Bitmap pattern")
                selectByMouse: true
                Layout.fillWidth: true
                Accessible.name: qsTr("Image to use as pattern")

                onTextChanged: {
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
            }
            Button {
                text: qsTr("Search")
                icon.source: "image://icons/search"
                Accessible.description: qsTr("Search the image to use as pattern")

                onClicked: fileDialog.open()
            }
        }
    }

    Label {
        id: txtPatternSize
        text: qsTr("Pattern size")
    }
    TextField {
        text: Halftone.patternSize.width + "x" + Halftone.patternSize.height
        placeholderText: qsTr("Pattern size")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /-?\d+x-?\d+/
        }
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.name: txtPatternSize.text

        onTextChanged: Halftone.patternSize = strToSize(text)
    }
    Label {
        id: txtLightning
        text: qsTr("Lightning")
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
    SpinBox {
        id: spbLightning
        value: Halftone.lightning
        to: sldLightning.to
        stepSize: sldLightning.stepSize
        editable: true
        Accessible.name: txtLightning.text

        onValueChanged: Halftone.lightning = Number(value)
    }

    Label {
        id: txtSlope
        text: qsTr("Slope")
    }
    TextField {
        text: Halftone.slope
        placeholderText: qsTr("Slope")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.fillWidth: true
        Layout.columnSpan: 2
        Accessible.name: txtSlope.text

        onTextChanged: Halftone.slope = Number(text)
    }
    Label {
        id: txtInterception
        text: qsTr("Interception")
    }
    TextField {
        text: Halftone.interception
        placeholderText: qsTr("Intercept")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
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
        folder: "file://" + picturesPath

        onAccepted: Halftone.pattern = String(file).replace("file://", "")
    }
}

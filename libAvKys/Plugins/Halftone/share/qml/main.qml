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
    columns: 2

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

    Label {
        text: qsTr("Pattern")
    }
    ComboBox {
        id: cbxPattern
        textRole: "text"
        Layout.fillWidth: true

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
        Layout.columnSpan: 2

        RowLayout {
            Image {
                width: 16
                height: 16
                fillMode: Image.PreserveAspectFit
                sourceSize.width: 16
                sourceSize.height: 16
                source: toQrc(txtPattern.text)
            }
            TextField {
                id: txtPattern
                text: Halftone.pattern
                placeholderText: qsTr("Bitmap pattern")
                selectByMouse: true
                Layout.fillWidth: true

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

                onClicked: fileDialog.open()
            }
        }
    }

    Label {
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

        onTextChanged: Halftone.patternSize = strToSize(text)
    }
    Label {
        text: qsTr("Lightness")
    }
    TextField {
        text: Halftone.lightness
        placeholderText: qsTr("Lightness")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.fillWidth: true

        onTextChanged: Halftone.lightness = Number(text)
    }
    Label {
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

        onTextChanged: Halftone.slope = Number(text)
    }
    Label {
        text: qsTr("Intercept")
    }
    TextField {
        text: Halftone.intercept
        placeholderText: qsTr("Intercept")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.fillWidth: true

        onTextChanged: Halftone.intercept = Number(text)
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: "file://" + picturesPath

        onAccepted: Halftone.pattern = String(file).replace("file://", "")
    }
}

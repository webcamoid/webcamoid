/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.2

GridLayout {
    columns: 2

    function markerTypeIndex(markerType)
    {
        var index = -1

        for (var i = 0; i < cbxMarkerType.model.count; i++)
            if (cbxMarkerType.model.get(i).markerType === markerType) {
                index = i
                break
            }

        return index
    }

    function markerStyleIndex(markerStyle)
    {
        var index = -1

        for (var i = 0; i < cbxMarkerStyle.model.count; i++)
            if (cbxMarkerStyle.model.get(i).markerStyle === markerStyle) {
                index = i
                break
            }

        return index
    }

    function strToFloat(str)
    {
        return str.length < 1? 0: parseFloat(str)
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

    function invert(color) {
        return Qt.rgba(1.0 - color.r, 1.0 - color.g, 1.0 - color.b, 1)
    }

    function toQrc(uri)
    {
        if (uri.indexOf(":") === 0)
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

    // Haar file.
    Label {
        text: qsTr("Haar file")
    }
    RowLayout {
        TextField {
            text: FaceDetect.haarFile
            placeholderText: qsTr("XML haar file.")
            Layout.fillWidth: true
        }
        Button {
            text: qsTr("...")

            onClicked: fileDialog.open()
        }
    }

    // Scan block.
    Label {
        text: qsTr("Scan block")
    }
    TextField {
        text: FaceDetect.scanSize.width + "x" + FaceDetect.scanSize.height
        validator: RegExpValidator {
            regExp: /\d+x\d+/
        }

        onTextChanged: FaceDetect.scanSize = strToSize(text)
    }

    // Marker type.
    Label {
        text: qsTr("Marker type")
    }
    ComboBox {
        id: cbxMarkerType
        currentIndex: markerTypeIndex(FaceDetect.markerType)

        model: ListModel {
            ListElement {
                text: qsTr("Rectangle")
                markerType: "rectangle"
            }
            ListElement {
                text: qsTr("Ellipse")
                markerType: "ellipse"
            }
            ListElement {
                text: qsTr("Image")
                markerType: "image"
            }
            ListElement {
                text: qsTr("Pixelate")
                markerType: "pixelate"
            }
            ListElement {
                text: qsTr("Blur")
                markerType: "blur"
            }
        }

        onCurrentIndexChanged: FaceDetect.markerType = cbxMarkerType.model.get(currentIndex).markerType
    }

    // Marker style.
    Label {
        text: qsTr("Marker style")
    }
    ComboBox {
        id: cbxMarkerStyle
        currentIndex: markerStyleIndex(FaceDetect.markerStyle)

        model: ListModel {
            ListElement {
                text: qsTr("Solid")
                markerStyle: "solid"
            }
            ListElement {
                text: qsTr("Dash")
                markerStyle: "dash"
            }
            ListElement {
                text: qsTr("Dot")
                markerStyle: "dot"
            }
            ListElement {
                text: qsTr("Dash dot")
                markerStyle: "dashDot"
            }
            ListElement {
                text: qsTr("Dash dot dot")
                markerStyle: "dashDotDot"
            }
        }

        onCurrentIndexChanged: FaceDetect.markerStyle = cbxMarkerStyle.model.get(currentIndex).markerStyle
    }

    // Marker color.
    Label {
        text: qsTr("Marker color")
    }
    Button {
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32

        style: ButtonStyle {
            background: Rectangle {
                color: fromRgba(FaceDetect.markerColor)
                border.color: invert(color)
                border.width: 1
            }
        }

        onClicked: colorDialog.open()
    }

    // Marker width.
    Label {
        text: qsTr("Marker width")
    }
    TextField {
        text: FaceDetect.markerWidth
        validator: RegExpValidator {
            regExp: /\d+/
        }

        onTextChanged: FaceDetect.markerWidth = strToFloat(text)
    }

    // Marker picture.
    Label {
        text: qsTr("Marker picture")
    }
    RowLayout {
        Image {
            width: 16
            height: 16
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 16
            sourceSize.height: 16
            source: toQrc(txtTable.text)
        }
        TextField {
            id: txtTable
            text: FaceDetect.markerImage
            placeholderText: qsTr("Replace face with this picture.")
            Layout.fillWidth: true
        }
        Button {
            text: qsTr("...")

            onClicked: pictureDialog.open()
        }
    }

    // Pixel grid.
    Label {
        text: qsTr("Pixel grid size")
    }
    TextField {
        text: FaceDetect.pixelGridSize.width + "x" + FaceDetect.pixelGridSize.height
        validator: RegExpValidator {
            regExp: /\d+x\d+/
        }

        onTextChanged: FaceDetect.pixelGridSize = strToSize(text)
    }

    // Blur radius.
    Label {
        text: qsTr("Blur radius")
    }
    TextField {
        text: FaceDetect.blurRadius
        validator: RegExpValidator {
            regExp: /\d+/
        }

        onTextChanged: FaceDetect.blurRadius = strToFloat(text)
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Select marker color")
        currentColor: fromRgba(FaceDetect.markerColor)
        showAlphaChannel: true

        onAccepted: FaceDetect.markerColor = toRgba(color)
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Please choose a haar file")
        nameFilters: ["XML haar file (*.xml)"]
        folder: "file://" + picturesPath

        onAccepted: FaceDetect.haarFile = String(fileUrl).replace("file://", "")
    }

    FileDialog {
        id: pictureDialog
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: "file://" + picturesPath

        onAccepted: FaceDetect.markerImage = String(fileUrl).replace("file://", "")
    }
}

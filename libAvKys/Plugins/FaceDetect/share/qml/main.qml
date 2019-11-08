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
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.2
import AkQmlControls 1.0

GridLayout {
    columns: 2

    function haarFileIndex(haarFile)
    {
        var index = -1

        for (var i = 0; i < cbxHaarFile.model.count; i++)
            if (cbxHaarFile.model.get(i).haarFile === haarFile) {
                index = i
                break
            }

        return index
    }

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
    ComboBox {
        id: cbxHaarFile
        textRole: "text"
        currentIndex: haarFileIndex(FaceDetect.haarFile)
        Layout.fillWidth: true

        model: ListModel {
            ListElement {
                text: qsTr("Eye")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_eye.xml"
            }
            ListElement {
                text: qsTr("Eye glasses")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_eye_tree_eyeglasses.xml"
            }
            ListElement {
                text: qsTr("Frontal face alternative 1")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt.xml"
            }
            ListElement {
                text: qsTr("Frontal face alternative 2")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt2.xml"
            }
            ListElement {
                text: qsTr("Frontal face alternative 3")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt_tree.xml"
            }
            ListElement {
                text: qsTr("Frontal face default")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_frontalface_default.xml"
            }
            ListElement {
                text: qsTr("Full body")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_fullbody.xml"
            }
            ListElement {
                text: qsTr("Left Eye 1")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_lefteye_2splits.xml"
            }
            ListElement {
                text: qsTr("Lower body")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_lowerbody.xml"
            }
            ListElement {
                text: qsTr("Eye pair big")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_eyepair_big.xml"
            }
            ListElement {
                text: qsTr("Eye pair small")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_eyepair_small.xml"
            }
            ListElement {
                text: qsTr("Left ear")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_leftear.xml"
            }
            ListElement {
                text: qsTr("Left eye 2")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_lefteye.xml"
            }
            ListElement {
                text: qsTr("Mouth")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_mouth.xml"
            }
            ListElement {
                text: qsTr("Nose")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_nose.xml"
            }
            ListElement {
                text: qsTr("Right ear")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_rightear.xml"
            }
            ListElement {
                text: qsTr("Right Eye 1")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_righteye.xml"
            }
            ListElement {
                text: qsTr("Upper body 1")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_upperbody.xml"
            }
            ListElement {
                text: qsTr("Profile face")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_profileface.xml"
            }
            ListElement {
                text: qsTr("Right eye 2")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_righteye_2splits.xml"
            }
            ListElement {
                text: qsTr("Smile")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_smile.xml"
            }
            ListElement {
                text: qsTr("Upper body")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_upperbody.xml"
            }
        }

        onCurrentIndexChanged: FaceDetect.haarFile = cbxHaarFile.model.get(currentIndex).haarFile
    }

    // Scan block.
    TextField {
        text: FaceDetect.scanSize.width + "x" + FaceDetect.scanSize.height
        placeholderText: qsTr("Scan block")
        validator: RegExpValidator {
            regExp: /\d+x\d+/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true

        onTextChanged: FaceDetect.scanSize = strToSize(text)
    }

    // Marker type.
    Label {
        text: qsTr("Marker type")
    }
    ComboBox {
        id: cbxMarkerType
        textRole: "text"
        currentIndex: markerTypeIndex(FaceDetect.markerType)
        Layout.fillWidth: true

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
        textRole: "text"
        currentIndex: markerStyleIndex(FaceDetect.markerStyle)
        Layout.fillWidth: true

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
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        AkColorButton {
            currentColor: fromRgba(FaceDetect.markerColor)
            title: qsTr("Select marker color")
            showAlphaChannel: true

            onCurrentColorChanged: FaceDetect.markerColor = toRgba(currentColor)
        }
    }

    // Marker width.
    TextField {
        text: FaceDetect.markerWidth
        placeholderText: qsTr("Marker width")
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true

        onTextChanged: FaceDetect.markerWidth = text
    }

    // Marker picture.
    Label {
        text: qsTr("Masks")
    }
    ComboBox {
        id: cbxMasks
        textRole: "text"
        Layout.fillWidth: true

        model: ListModel {
            ListElement {
                text: qsTr("Angel")
                mask: ":/FaceDetect/share/masks/angel.png"
            }
            ListElement {
                text: qsTr("Bear")
                mask: ":/FaceDetect/share/masks/bear.png"
            }
            ListElement {
                text: qsTr("Beaver")
                mask: ":/FaceDetect/share/masks/beaver.png"
            }
            ListElement {
                text: qsTr("Cat")
                mask: ":/FaceDetect/share/masks/cat.png"
            }
            ListElement {
                text: qsTr("Chicken")
                mask: ":/FaceDetect/share/masks/chicken.png"
            }
            ListElement {
                text: qsTr("Cow")
                mask: ":/FaceDetect/share/masks/cow.png"
            }
            ListElement {
                text: qsTr("Devil")
                mask: ":/FaceDetect/share/masks/devil.png"
            }
            ListElement {
                text: qsTr("Dog")
                mask: ":/FaceDetect/share/masks/dog.png"
            }
            ListElement {
                text: qsTr("Dalmatian dog")
                mask: ":/FaceDetect/share/masks/dog-dalmatian.png"
            }
            ListElement {
                text: qsTr("Happy dog")
                mask: ":/FaceDetect/share/masks/dog-happy.png"
            }
            ListElement {
                text: qsTr("Dragon")
                mask: ":/FaceDetect/share/masks/dragon.png"
            }
            ListElement {
                text: qsTr("Elephant 1")
                mask: ":/FaceDetect/share/masks/elephant1.png"
            }
            ListElement {
                text: qsTr("Elephant 2")
                mask: ":/FaceDetect/share/masks/elephant2.png"
            }
            ListElement {
                text: qsTr("Elk")
                mask: ":/FaceDetect/share/masks/elk.png"
            }
            ListElement {
                text: qsTr("Frog")
                mask: ":/FaceDetect/share/masks/frog.png"
            }
            ListElement {
                text: qsTr("Ghost")
                mask: ":/FaceDetect/share/masks/ghost.png"
            }
            ListElement {
                text: qsTr("Giraffe")
                mask: ":/FaceDetect/share/masks/giraffe.png"
            }
            ListElement {
                text: qsTr("Gnu")
                mask: ":/FaceDetect/share/masks/gnu.png"
            }
            ListElement {
                text: qsTr("Goat")
                mask: ":/FaceDetect/share/masks/goat.png"
            }
            ListElement {
                text: qsTr("Hippo")
                mask: ":/FaceDetect/share/masks/hippo.png"
            }
            ListElement {
                text: qsTr("Horse")
                mask: ":/FaceDetect/share/masks/horse.png"
            }
            ListElement {
                text: qsTr("Gray horse")
                mask: ":/FaceDetect/share/masks/horse-gray.png"
            }
            ListElement {
                text: qsTr("Koala")
                mask: ":/FaceDetect/share/masks/koala.png"
            }
            ListElement {
                text: qsTr("Monkey")
                mask: ":/FaceDetect/share/masks/monkey.png"
            }
            ListElement {
                text: qsTr("Gray mouse")
                mask: ":/FaceDetect/share/masks/mouse-gray.png"
            }
            ListElement {
                text: qsTr("White mouse")
                mask: ":/FaceDetect/share/masks/mouse-white.png"
            }
            ListElement {
                text: qsTr("Panda")
                mask: ":/FaceDetect/share/masks/panda.png"
            }
            ListElement {
                text: qsTr("Penguin")
                mask: ":/FaceDetect/share/masks/penguin.png"
            }
            ListElement {
                text: qsTr("Pumpkin 1")
                mask: ":/FaceDetect/share/masks/pumpkin1.png"
            }
            ListElement {
                text: qsTr("Pumpkin 2")
                mask: ":/FaceDetect/share/masks/pumpkin2.png"
            }
            ListElement {
                text: qsTr("Raccoon")
                mask: ":/FaceDetect/share/masks/raccoon.png"
            }
            ListElement {
                text: qsTr("Rhino")
                mask: ":/FaceDetect/share/masks/rhino.png"
            }
            ListElement {
                text: qsTr("Sheep")
                mask: ":/FaceDetect/share/masks/sheep.png"
            }
            ListElement {
                text: qsTr("Skull 1")
                mask: ":/FaceDetect/share/masks/skull1.png"
            }
            ListElement {
                text: qsTr("Skull 2")
                mask: ":/FaceDetect/share/masks/skull2.png"
            }
            ListElement {
                text: qsTr("Triceratops")
                mask: ":/FaceDetect/share/masks/triceratops.png"
            }
            ListElement {
                text: qsTr("Zebra")
                mask: ":/FaceDetect/share/masks/zebra.png"
            }
            ListElement {
                text: qsTr("Custom")
                mask: ""
            }
        }

        onCurrentIndexChanged: FaceDetect.markerImage = cbxMasks.model.get(currentIndex).mask
    }

    RowLayout {
        Layout.columnSpan: 2

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
            placeholderText: qsTr("Marker picture")
            Layout.fillWidth: true

            onTextChanged: {
                for (var i = 0; i < cbxMasks.model.count; i++) {
                    if (cbxMasks.model.get(i).mask === FaceDetect.markerImage) {
                        cbxMasks.currentIndex = i

                        break
                    } else if (i == cbxMasks.model.count - 1) {
                        cbxMasks.model.get(i).mask = FaceDetect.markerImage
                        cbxMasks.currentIndex = i

                        break
                    }
                }
            }
        }
        Button {
            text: qsTr("Search")
            icon.source: "image://icons/edit-find"

            onClicked: pictureDialog.open()
        }
    }

    // Pixel grid.
    TextField {
        text: FaceDetect.pixelGridSize.width + "x" + FaceDetect.pixelGridSize.height
        placeholderText: qsTr("Pixel grid size")
        validator: RegExpValidator {
            regExp: /\d+x\d+/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true

        onTextChanged: FaceDetect.pixelGridSize = strToSize(text)
    }

    // Blur radius.
    TextField {
        text: FaceDetect.blurRadius
        placeholderText: qsTr("Blur radius")
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true

        onTextChanged: FaceDetect.blurRadius = text
    }

    FileDialog {
        id: pictureDialog
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: "file://" + picturesPath

        onAccepted: FaceDetect.markerImage = String(fileUrl).replace("file://", "")
    }
}

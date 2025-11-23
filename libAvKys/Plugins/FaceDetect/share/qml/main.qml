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
import QtQuick.Layouts
import Qt.labs.platform as LABS
import Ak
import AkControls as AK
import FaceDetectElement

ColumnLayout {
    id: clyFaceDetect

    readonly property string filePrefix: Ak.platform() == "windows"?
                                             "file:///":
                                             "file://"

    function haarFileIndex(haarFile)
    {
        var index = -1

        for (var i = 0; i < cbxHaarFile.model.count; i++)
            if (cbxHaarFile.model.get(i).haarFile == haarFile) {
                index = i
                break
            }

        return index
    }

    function markerTypeIndex(markerType)
    {
        var index = -1

        for (var i = 0; i < cbxMarkerType.model.count; i++)
            if (cbxMarkerType.model.get(i).markerType == markerType) {
                index = i
                break
            }

        return index
    }

    function markerStyleIndex(markerStyle)
    {
        var index = -1

        for (var i = 0; i < cbxMarkerStyle.model.count; i++)
            if (cbxMarkerStyle.model.get(i).markerStyle == markerStyle) {
                index = i
                break
            }

        return index
    }

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

    // Haar file.
    Label {
        id: txtHaarFile
        //: https://en.wikipedia.org/wiki/Haar-like_feature
        text: qsTr("Haar file")
        font.bold: true
        Layout.fillWidth: true
    }
    ComboBox {
        id: cbxHaarFile
        textRole: "text"
        currentIndex: haarFileIndex(FaceDetect.haarFile)
        Layout.fillWidth: true
        Accessible.description: txtHaarFile.text

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
    Label {
        id: txtScanBlock
        text: qsTr("Scan block")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: FaceDetect.scanSize.width + "x" + FaceDetect.scanSize.height
        placeholderText: qsTr("Scan block")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /\d+x\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtScanBlock.text

        onTextChanged: FaceDetect.scanSize = strToSize(text)
    }

    // Marker type.
    Label {
        id: txtMarkerType
        text: qsTr("Marker type")
        font.bold: true
        Layout.fillWidth: true
    }
    ComboBox {
        id: cbxMarkerType
        textRole: "text"
        currentIndex: markerTypeIndex(FaceDetect.markerType)
        Layout.fillWidth: true
        Accessible.description: txtMarkerType.text

        model: ListModel {
            ListElement {
                text: qsTr("Rectangle")
                markerType: FaceDetectElement.MarkerTypeRectangle
            }
            ListElement {
                text: qsTr("Ellipse")
                markerType: FaceDetectElement.MarkerTypeEllipse
            }
            ListElement {
                text: qsTr("Image")
                markerType: FaceDetectElement.MarkerTypeImage
            }
            ListElement {
                text: qsTr("Pixelate")
                markerType: FaceDetectElement.MarkerTypePixelate
            }
            ListElement {
                text: qsTr("Blur")
                markerType: FaceDetectElement.MarkerTypeBlur
            }
            ListElement {
                text: qsTr("Blur Outer")
                markerType: FaceDetectElement.MarkerTypeBlurOuter
            }
            ListElement {
                text: qsTr("Background Image")
                markerType: FaceDetectElement.MarkerTypeImageOuter
            }
        }

        onCurrentIndexChanged: FaceDetect.markerType = cbxMarkerType.model.get(currentIndex).markerType
    }

    // Marker style.
    Label {
        id: txtMarkerStyle
        text: qsTr("Marker style")
        font.bold: true
        Layout.fillWidth: true
    }
    ComboBox {
        id: cbxMarkerStyle
        textRole: "text"
        currentIndex: markerStyleIndex(FaceDetect.markerStyle)
        Layout.fillWidth: true
        Accessible.description: txtMarkerStyle.text

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
    RowLayout {
        Label {
            id: txtMarkerColor
            text: qsTr("Marker color")
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(FaceDetect.markerColor)
            title: qsTr("Select marker color")
            showAlphaChannel: true
            Accessible.description: txtMarkerColor.text

            onCurrentColorChanged: FaceDetect.markerColor = AkUtils.toRgba(currentColor)
        }
    }

    // Marker width.
    Label {
        id: txtMarkerWidth
        text: qsTr("Marker width")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: FaceDetect.markerWidth
        placeholderText: qsTr("Marker width")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtMarkerWidth.text

        onTextChanged: FaceDetect.markerWidth = Number(text)
    }

    // Marker picture.
    Label {
        id: txtMasks
        text: qsTr("Masks")
        font.bold: true
        Layout.fillWidth: true
    }
    ComboBox {
        id: cbxMasks
        textRole: "text"
        Layout.fillWidth: true
        Accessible.description: txtMasks.text

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

    Label {
        id: txtMarkerPicture
        text: qsTr("Marker picture")
        font.bold: true
        Layout.fillWidth: true
    }
    RowLayout {
        Image {
            width: 16
            height: 16
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 16
            sourceSize.height: 16
            source: toQrc(txtTable.labelText)
        }
        AK.ActionTextField {
            id: txtTable
            icon.source: "image://icons/search"
            labelText: FaceDetect.markerImage
            placeholderText: qsTr("Replace face with this picture")
            buttonText: qsTr("Search the image to put into the detected rectangle")
            Layout.fillWidth: true

            onLabelTextChanged: {
                for (var i = 0; i < cbxMasks.model.count; i++) {
                    if (cbxMasks.model.get(i).mask == FaceDetect.markerImage) {
                        cbxMasks.currentIndex = i

                        break
                    } else if (i == cbxMasks.model.count - 1) {
                        cbxMasks.model.get(i).mask = FaceDetect.markerImage
                        cbxMasks.currentIndex = i

                        break
                    }
                }
            }
            onButtonClicked: fileDialog.open()
        }
    }

    // Background picture.
    Label {
        id: txtBackground
        text: qsTr("Background")
        font.bold: true
        Layout.fillWidth: true
    }
    ComboBox {
        id: cbxBackgrounds
        textRole: "text"
        Layout.fillWidth: true
        Accessible.description: txtBackground.text

        model: ListModel {
            ListElement {
                text: qsTr("Black Square")
                background: ":/FaceDetect/share/background/black_square.png"
            }
            ListElement {
                text: qsTr("Custom")
                background: ""
            }
        }

        onCurrentIndexChanged: FaceDetect.backgroundImage = cbxBackgrounds.model.get(currentIndex).background
    }

    Label {
        id: txtBackgroundPicture
        text: qsTr("Background picture")
        font.bold: true
        Layout.fillWidth: true
    }
    RowLayout {
        Image {
            width: 16
            height: 16
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 16
            sourceSize.height: 16
            source: toQrc(txtBackgroundImage.labelText)
        }
        AK.ActionTextField {
            id: txtBackgroundImage
            icon.source: "image://icons/search"
            labelText: FaceDetect.backgroundImage
            placeholderText: qsTr("Replace background with this picture")
            buttonText: qsTr("Search the image to use as background")
            Layout.fillWidth: true

            onLabelTextChanged: {
                for (var i = 0; i < cbxBackgrounds.model.count; i++) {
                    if (cbxBackgrounds.model.get(i).background == FaceDetect.backgroundImage) {
                        cbxBackgrounds.currentIndex = i

                        break
                    } else if (i == cbxBackgrounds.model.count - 1) {
                        cbxBackgrounds.model.get(i).background = FaceDetect.backgroundImage
                        cbxBackgrounds.currentIndex = i

                        break
                    }
                }
            }
            onButtonClicked: fileDialogBGImage.open()
        }
    }

    // Pixel grid.
    Label {
        id: txtPixelGridSize
        text: qsTr("Pixel grid size")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: FaceDetect.pixelGridSize.width + "x" + FaceDetect.pixelGridSize.height
        placeholderText: qsTr("Pixel grid size")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /\d+x\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtPixelGridSize.text

        onTextChanged: FaceDetect.pixelGridSize = strToSize(text)
    }

    // Blur radius.
    Label {
        id: txtBlurRadius
        text: qsTr("Blur radius")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: FaceDetect.blurRadius
        placeholderText: qsTr("Blur radius")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtBlurRadius.text

        onTextChanged: FaceDetect.blurRadius = Number(text)
    }

    Label {
        text: qsTr("Face Area Settings")
        font: AkTheme.fontSettings.h6
        Layout.fillWidth: true
    }
    Label {
        text: qsTr("Advanced face area settings for background blur or image below.")
        wrapMode: Text.WordWrap
        elide: Text.ElideNone
        Layout.fillWidth: true
    }

    // Face area size scale.
    Label {
        id: txtScale
        text: qsTr("Scale")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldScale
        value: FaceDetect.scale
        stepSize: 0.05
        from: 0.5
        to: 2
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: txtScale.text

        onValueChanged: FaceDetect.scale = value
    }

    // Configure face area offsets.
    Label {
        id: txtHorizontalOffset
        text: qsTr("Horizontal Offset")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldHOffset
        value: FaceDetect.hOffset
        from: -150
        to: 150
        stepSize: 1
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: txtHorizontalOffset.text

        onValueChanged: FaceDetect.hOffset = value
    }

    Label {
        id: txtVerticalOffset
        text: qsTr("Vertical Offset")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldVOffset
        value: FaceDetect.vOffset
        from: -150
        to: 150
        stepSize: 1
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: txtVerticalOffset.text

        onValueChanged: FaceDetect.vOffset = value
    }

    // Configure face area width/height.
    Label {
        id: txtWidthAdjustPercent
        text: qsTr("Width Adjust %")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldWAdjust
        value: FaceDetect.wAdjust
        from: 1
        to: 200
        stepSize: 1
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: txtWidthAdjustPercent.text

        onValueChanged: FaceDetect.wAdjust = value
    }

    Label {
        id: txtHeightAdjustPercent
        text: qsTr("Height Adjust %")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldHAdjust
        value: FaceDetect.hAdjust
        from: 1
        to: 200
        stepSize: 1
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: txtHeightAdjustPercent.text

        onValueChanged: FaceDetect.hAdjust = value
    }

    // Round face area overlay.
    Switch {
        id: chkSmotheEdges
        text: qsTr("Round Area")
        checked: FaceDetect.smootheEdges
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: FaceDetect.smootheEdges = checked
    }

    // Edge smothing size scale.
    Label {
        id: txtEdgeSmothingSizeScale
        text: qsTr("Scale")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldRScale
        value: FaceDetect.rScale
        stepSize: 0.05
        from: 0.5
        to: 2
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: txtEdgeSmothingSizeScale.text

        onValueChanged: FaceDetect.rScale = value
    }

    // Configure rounded face area width/height.
    Label {
        id: txtRfWidthAdjustPercent
        text: qsTr("Width Adjust %")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldRWAdjust
        value: FaceDetect.rWAdjust
        from: 1
        to: 200
        stepSize: 1
        Layout.fillWidth: true
        Accessible.name: txtRfWidthAdjustPercent.text

        onValueChanged: FaceDetect.rWAdjust = value
    }

    Label {
        id: txtRfHeightAdjustPercent
        text: qsTr("Height Adjust %")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldRHAdjust
        value: FaceDetect.rHAdjust
        from: 1
        to: 200
        stepSize: 1
        Layout.fillWidth: true
        Accessible.name: txtRfHeightAdjustPercent.text

        onValueChanged: FaceDetect.rHAdjust = value
    }

    // Configure rounded face area radius
    Label {
        id: txtHorizontalRadiusPercent
        text: qsTr("Horizontal Radius %")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldHRad
        value: FaceDetect.rHRadius
        to: 100
        stepSize: 1
        Layout.fillWidth: true
        Accessible.name: txtHorizontalRadiusPercent.text

        onValueChanged: FaceDetect.rHRadius = value
    }

    Label {
        id: txtVerticalRadiusPercent
        text: qsTr("Vertical Radius %")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldVRad
        value: FaceDetect.rVRadius
        to: 100
        stepSize: 1
        Layout.fillWidth: true
        Accessible.name: txtVerticalRadiusPercent.text

        onValueChanged: FaceDetect.rVRadius = value
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: clyFaceDetect.filePrefix + picturesPath

        onAccepted: FaceDetect.markerImage =
                    String(file).replace(clyFaceDetect.filePrefix, "")
    }

    LABS.FileDialog {
        id: fileDialogBGImage
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: clyFaceDetect.filePrefix + picturesPath

        onAccepted: FaceDetect.backgroundImage =
                    String(file).replace(clyFaceDetect.filePrefix, "")
    }
}

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
import QtQuick.Layouts 1.3
import Qt.labs.platform 1.1 as LABS
import Ak 1.0
import AkControls 1.0 as AK
import FaceDetectElement 1.0

GridLayout {
    columns: 2

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

    Connections {
        target: FaceDetect

        function onScale(scale)
        {
            sldScale.value = scale
            spbScale.value = scale * spbScale.multiplier
        }

        function onRScaleChanged(rScale)
        {
            sldRScale.value = rScale
            spbRScale.value = rScale * spbRScale.multiplier
        }
    }

    // Haar file.
    Label {
        id: txtHaarFile
        //: https://en.wikipedia.org/wiki/Haar-like_feature
        text: qsTr("Haar file")
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
    }
    TextField {
        text: FaceDetect.scanSize.width + "x" + FaceDetect.scanSize.height
        placeholderText: qsTr("Scan block")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+x\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtScanBlock.text

        onTextChanged: FaceDetect.scanSize = strToSize(text)
    }

    // Marker type.
    Label {
        id: txtMarkerType
        text: qsTr("Marker type")
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
    Label {
        id: txtMarkerColor
        text: qsTr("Marker color")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
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
    }
    TextField {
        text: FaceDetect.markerWidth
        placeholderText: qsTr("Marker width")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtMarkerWidth.text

        onTextChanged: FaceDetect.markerWidth = Number(text)
    }

    // Marker picture.
    Label {
        id: txtMasks
        text: qsTr("Masks")
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
            placeholderText: qsTr("Replace face with this picture")
            selectByMouse: true
            Layout.fillWidth: true
            Accessible.name: txtMarkerPicture.text

            onTextChanged: {
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
        }
        Button {
            text: qsTr("Search")
            icon.source: "image://icons/search"
            Accessible.description: qsTr("Search the image to put into the detected rectangle")

            onClicked: fileDialog.open()
        }
    }

    // Background picture.
    Label {
        id: txtBackground
        text: qsTr("Background")
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
    }
    RowLayout {
        Image {
            width: 16
            height: 16
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 16
            sourceSize.height: 16
            source: toQrc(txtBackgroundImage.text)
        }
        TextField {
            id: txtBackgroundImage
            text: FaceDetect.backgroundImage
            placeholderText: qsTr("Replace background with this picture")
            selectByMouse: true
            Layout.fillWidth: true
            Accessible.name: txtBackgroundPicture.text

            onTextChanged: {
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
        }
        Button {
            text: qsTr("Search")
            icon.source: "image://icons/search"
            Accessible.description: qsTr("Search the image to use as background")

            onClicked: fileDialogBGImage.open()
        }
    }

    // Pixel grid.
    Label {
        id: txtPixelGridSize
        text: qsTr("Pixel grid size")
    }
    TextField {
        text: FaceDetect.pixelGridSize.width + "x" + FaceDetect.pixelGridSize.height
        placeholderText: qsTr("Pixel grid size")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+x\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtPixelGridSize.text

        onTextChanged: FaceDetect.pixelGridSize = strToSize(text)
    }

    // Blur radius.
    Label {
        id: txtBlurRadius
        text: qsTr("Blur radius")
    }
    TextField {
        text: FaceDetect.blurRadius
        placeholderText: qsTr("Blur radius")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtBlurRadius.text

        onTextChanged: FaceDetect.blurRadius = Number(text)
    }

    Label {
        id: txtFaceAreaSettings
        text: qsTr("Face Area Settings")
    }
    RowLayout {
        Item {
            Layout.fillWidth: true
        }
        Label {
            text: qsTr("Advanced face area settings for \nbackground blur or image below.")
        }
        Item {
            Layout.fillWidth: true
        }
    }

    // Face area size scale.
    Label {
        id: txtScale
        text: qsTr("Scale")
    }
    RowLayout {
        Slider {
            id: sldScale
            value: FaceDetect.scale
            stepSize: 0.05
            from: 0.5
            to: 2
            Layout.fillWidth: true
            Accessible.name: txtScale.text

            onValueChanged: FaceDetect.scale = value
        }
        SpinBox {
            id: spbScale
            value: multiplier * FaceDetect.scale
            from: multiplier * sldScale.from
            to: multiplier * sldScale.to
            stepSize: sldScale.stepSize * multiplier
            editable: true
            Accessible.name: txtScale.text

            property int decimals: 2
            property real multiplier: Math.pow(10, decimals);

            validator: DoubleValidator {
                bottom: Math.min(spbScale.from, spbScale.to)
                top:  Math.max(spbScale.from, spbScale.to)
            }
            textFromValue: function(value, locale) {
                return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
            }
            valueFromText: function(text, locale) {
                return Number.fromLocaleString(locale, text) * multiplier
            }
            onValueChanged: FaceDetect.scale = value / multiplier
        }
    }

    // Configure face area offsets.
    Label {
        id: txtHorizontalOffset
        text: qsTr("Horizontal Offset")
    }
    RowLayout {
        Slider {
            id: sldHOffset
            value: FaceDetect.hOffset
            from: -150
            to: 150
            stepSize: 1
            Layout.fillWidth: true
            Accessible.name: txtHorizontalOffset.text

            onValueChanged: FaceDetect.hOffset = value
        }
        SpinBox {
            id: spbHOffset
            value: FaceDetect.hOffset
            from: sldHOffset.from
            to: sldHOffset.to
            stepSize: sldHOffset.stepSize
            editable: true
            Accessible.name: txtHorizontalOffset.text

            onValueChanged: FaceDetect.hOffset = Number(value)
        }
    }

    Label {
        id: txtVerticalOffset
        text: qsTr("Vertical Offset")
    }
    RowLayout {
        Slider {
            id: sldVOffset
            value: FaceDetect.vOffset
            from: -150
            to: 150
            stepSize: 1
            Layout.fillWidth: true
            Accessible.name: txtVerticalOffset.text

            onValueChanged: FaceDetect.vOffset = value
        }
        SpinBox {
            id: spbVOffset
            value: FaceDetect.vOffset
            from: sldVOffset.from
            to: sldVOffset.to
            stepSize: sldVOffset.stepSize
            editable: true
            Accessible.name: txtVerticalOffset.text

            onValueChanged: FaceDetect.vOffset = Number(value)
        }
    }

    // Configure face area width/height.
    Label {
        id: txtWidthAdjustPercent
        text: qsTr("Width Adjust %")
    }
    RowLayout {
        Slider {
            id: sldWAdjust
            value: FaceDetect.wAdjust
            from: 1
            to: 200
            stepSize: 1
            Layout.fillWidth: true
            Accessible.name: txtWidthAdjustPercent.text

            onValueChanged: FaceDetect.wAdjust = value
        }
        SpinBox {
            id: spbWAdjust
            value: FaceDetect.wAdjust
            from: sldWAdjust.from
            to: sldWAdjust.to
            stepSize: sldWAdjust.stepSize
            editable: true
            Accessible.name: txtWidthAdjustPercent.text

            onValueChanged: FaceDetect.wAdjust = Number(value)
        }
    }

    Label {
        id: txtHeightAdjustPercent
        text: qsTr("Height Adjust %")
    }
    RowLayout {
        Slider {
            id: sldHAdjust
            value: FaceDetect.hAdjust
            from: 1
            to: 200
            stepSize: 1
            Layout.fillWidth: true
            Accessible.name: txtHeightAdjustPercent.text

            onValueChanged: FaceDetect.hAdjust = value
        }
        SpinBox {
            id: spbHAdjust
            value: FaceDetect.hAdjust
            from: sldHAdjust.from
            to: sldHAdjust.to
            stepSize: sldHAdjust.stepSize
            editable: true
            Accessible.name: txtHeightAdjustPercent.text

            onValueChanged: FaceDetect.hAdjust = Number(value)
        }
    }

    // Round face area overlay.
    Label {
        id: txtRoundArea
        text: qsTr("Round Area")
    }
    Switch {
        id: chkSmotheEdges
        checked: FaceDetect.smootheEdges
        Accessible.name: txtRoundArea.text

        onCheckedChanged: FaceDetect.smootheEdges = checked
    }

    // Edge smothing size scale.
    Label {
        id: txtEdgeSmothingSizeScale
        text: qsTr("Scale")
    }
    RowLayout {
        Slider {
            id: sldRScale
            value: FaceDetect.rScale
            stepSize: 0.05
            from: 0.5
            to: 2
            Layout.fillWidth: true
            Accessible.name: txtEdgeSmothingSizeScale.text

            onValueChanged: FaceDetect.rScale = value
        }
        SpinBox {
            id: spbRScale
            value: multiplier * FaceDetect.rScale
            from: multiplier * sldRScale.from
            to: multiplier * sldRScale.to
            stepSize: multiplier * sldRScale.stepSize
            editable: true
            Accessible.name: txtEdgeSmothingSizeScale.text

            property int decimals: 2
            property real multiplier: Math.pow(10, decimals);

            validator: DoubleValidator {
                bottom: Math.min(spbRScale.from, spbRScale.to)
                top:  Math.max(spbRScale.from, spbRScale.to)
            }
            textFromValue: function(value, locale) {
                return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
            }
            valueFromText: function(text, locale) {
                return Number.fromLocaleString(locale, text) * multiplier
            }
            onValueModified: FaceDetect.rScale = value / multiplier
        }
    }

    // Configure rounded face area width/height.
    Label {
        id: txtRfWidthAdjustPercent
        text: qsTr("Width Adjust %")
    }
    RowLayout {
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
        SpinBox {
            id: spbRWAdjust
            value: FaceDetect.rWAdjust
            from: sldRWAdjust.from
            to: sldRWAdjust.to
            stepSize: sldRWAdjust.stepSize
            editable: true
            Accessible.name: txtRfWidthAdjustPercent.text

            onValueChanged: FaceDetect.rWAdjust = Number(value)
        }
    }

    Label {
        id: txtRfHeightAdjustPercent
        text: qsTr("Height Adjust %")
    }
    RowLayout {
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
        SpinBox {
            id: spbRHAdjust
            value: FaceDetect.rHAdjust
            from: sldRHAdjust.from
            to: sldRHAdjust.to
            stepSize: sldRHAdjust.stepSize
            editable: true
            Accessible.name: txtRfHeightAdjustPercent.text

            onValueChanged: FaceDetect.rHAdjust = Number(value)
        }
    }

    // Configure rounded face area radius
    Label {
        id: txtHorizontalRadiusPercent
        text: qsTr("Horizontal Radius %")
    }
    RowLayout {
        Slider {
            id: sldHRad
            value: FaceDetect.rHRadius
            to: 100
            stepSize: 1
            Layout.fillWidth: true
            Accessible.name: txtHorizontalRadiusPercent.text

            onValueChanged: FaceDetect.rHRadius = value
        }
        SpinBox {
            id: spbHRad
            value: FaceDetect.rHRadius
            to: sldHRad.to
            stepSize: sldHRad.stepSize
            editable: true
            Accessible.name: txtHorizontalRadiusPercent.text

            onValueChanged: FaceDetect.rHRadius = Number(value)
        }
    }

    Label {
        id: txtVerticalRadiusPercent
        text: qsTr("Vertical Radius %")
    }
    RowLayout {
        Slider {
            id: sldVRad
            value: FaceDetect.rVRadius
            to: 100
            stepSize: 1
            Layout.fillWidth: true
            Accessible.name: txtVerticalRadiusPercent.text

            onValueChanged: FaceDetect.rVRadius = value
        }
        SpinBox {
            id: spbVRad
            value: FaceDetect.rVRadius
            to: sldVRad.to
            stepSize: sldVRad.stepSize
            editable: true
            Accessible.name: txtVerticalRadiusPercent.text

            onValueChanged: FaceDetect.rVRadius = Number(value)
        }
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: "file://" + picturesPath

        onAccepted: {
            var curFile = String(file)
            if (curFile.match("file:\/\/\/[A-Za-z]{1,2}:")) {
                FaceDetect.markerImage = curFile.replace("file:///", "")
            } else {
                FaceDetect.markerImage = curFile.replace("file://", "")
            }
        }
    }

    LABS.FileDialog {
        id: fileDialogBGImage
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: "file://" + picturesPath

        onAccepted: {
            var curFile = String(file)
            if (curFile.match("file:\/\/\/[A-Za-z]{1,2}:")) {
                FaceDetect.backgroundImage = curFile.replace("file:///", "")
            } else {
                FaceDetect.backgroundImage = curFile.replace("file://", "")
            }
        }
    }
}

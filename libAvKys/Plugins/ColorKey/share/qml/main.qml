/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as LABS
import Ak
import AkControls as AK
import ColorKeyElement

ColumnLayout {
    id: root
    layoutDirection: rtl? Qt.RightToLeft: Qt.LeftToRight

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft
    readonly property string filePrefix: Ak.platform() == "windows"?
                                             "file:///":
                                             "file://"

    function toQrc(uri)
    {
        if (uri.indexOf(":") == 0)
            return "qrc" + uri

        return "file:" + uri
    }

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

    AK.ColorButton {
        text: qsTr("Color")
        currentColor: AkUtils.fromRgba(ColorKey.colorKey)
        title: qsTr("Choose the color to filter")
        modality: Qt.NonModal
        horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
        Layout.fillWidth: true

        onCurrentColorChanged: ColorKey.colorKey = AkUtils.toRgba(currentColor)
    }

    Label {
        id: lblColorDiff
        text: qsTr("Color Difference")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldColorDiff
        value: ColorKey.colorDiff
        stepSize: 1
        to: 512
        Layout.fillWidth: true
        Accessible.name: lblColorDiff.text

        onValueChanged: ColorKey.colorDiff = value
    }

    Label {
        id: lblSmoothness
        text: qsTr("Smoothness")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldSmoothness
        value: ColorKey.smoothness
        stepSize: 1
        to: 128
        Layout.fillWidth: true
        Accessible.name: lblSmoothness.text

        onValueChanged: ColorKey.smoothness = value
    }

    Switch {
        text: qsTr("Normalize")
        checked: ColorKey.normalize
        Accessible.name: text
        Layout.fillWidth: true

        onCheckedChanged: ColorKey.normalize = checked
    }

    AK.LabeledComboBox {
        id: cbxBackgroundType
        label: qsTr("Background type")
        textRole: "text"
        currentIndex: optionIndex(cbxBackgroundType, ColorKey.backgroundType)
        Layout.fillWidth: true
        Accessible.description: label
        model: ListModel {
            ListElement {
                text: qsTr("No background")
                option: ColorKeyElement.BackgroundTypeNoBackground
            }
            ListElement {
                text: qsTr("Color")
                option: ColorKeyElement.BackgroundTypeColor
            }
            ListElement {
                text: qsTr("Image")
                option: ColorKeyElement.BackgroundTypeImage
            }
        }

        onCurrentIndexChanged: ColorKey.backgroundType = cbxBackgroundType.model.get(currentIndex).option
    }
    AK.ColorButton {
        text: qsTr("Background color")
        currentColor: AkUtils.fromRgba(ColorKey.backgroundColor)
        title: qsTr("Choose the background color")
        showAlphaChannel: true
        horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
        visible: cbxBackgroundType.currentIndex == 1
        Layout.fillWidth: true

        onCurrentColorChanged: ColorKey.backgroundColor = AkUtils.toRgba(currentColor)
    }

    RowLayout {
        layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
        visible: cbxBackgroundType.currentIndex == 2

        Image {
            width: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            height: width
            fillMode: Image.PreserveAspectFit
            sourceSize: Qt.size(width, height)
            source: toQrc(txtTable.labelText)
        }
        AK.ActionTextField {
            id: txtTable
            icon.source: "image://icons/search"
            labelText: ColorKey.background
            placeholderText: qsTr("Source palette")
            buttonText: qsTr("Search the image file to use as palette")
            Layout.fillWidth: true

            onLabelTextChanged: ColorKey.background = text
            onButtonClicked: fileDialog.open()
        }
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: root.filePrefix + picturesPath

        onAccepted: ColorKey.background =
                    String(file).replace(root.filePrefix, "")
    }
}

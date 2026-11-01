/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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
import TextSrcElement

ColumnLayout {
    id: root
    layoutDirection: rtl? Qt.RightToLeft: Qt.LeftToRight

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft
    readonly property string filePrefix: Ak.platform() == "windows"?
                                             "file:///":
                                             "file://"

    // A label with a spin box at its side.
    component NumberOption: RowLayout {
        id: option

        property alias label: lblOption.text
        property alias from: spbOption.from
        property alias to: spbOption.to
        property alias stepSize: spbOption.stepSize
        property alias value: spbOption.value

        signal modified(int newValue)

        layoutDirection: root.layoutDirection
        Layout.fillWidth: true

        Label {
            id: lblOption
            elide: Label.ElideRight
            Layout.fillWidth: true
        }
        SpinBox {
            id: spbOption
            editable: true
            Accessible.name: lblOption.text

            onValueModified: option.modified(value)
        }
    }

    component SectionTitle: Label {
        font: AkTheme.fontSettings.h6
        Layout.fillWidth: true
        Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
    }

    // Text

    SectionTitle {
        text: qsTr("Text")
    }
    AK.LabeledComboBox {
        id: cbxTextSource
        label: qsTr("Text from")
        currentIndex: TextSrc.textSource
        model: [qsTr("Text box"), qsTr("File")]
        Accessible.description: label
        Layout.fillWidth: true

        onActivated: TextSrc.textSource = currentIndex
    }
    ScrollView {
        visible: TextSrc.textSource == TextSrcElement.TextSourceInline
        clip: true
        Layout.fillWidth: true
        Layout.preferredHeight:
            AkUnit.create(140 * AkTheme.controlScale, "dp").pixels

        TextArea {
            id: txtText
            text: TextSrc.text
            placeholderText: qsTr("Type the text here")
            horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
            wrapMode: TextArea.Wrap
            selectByMouse: true
            persistentSelection: true
            Accessible.name: qsTr("Text")

            onTextChanged: {
                if (text !== TextSrc.text)
                    TextSrc.text = text
            }
        }
    }
    AK.ActionTextField {
        id: txtFile
        icon.source: "image://icons/file"
        labelText: TextSrc.textFile
        placeholderText: qsTr("Text file path")
        buttonText: qsTr("Search the text file to use")
        visible: TextSrc.textSource == TextSrcElement.TextSourceFile
        Layout.fillWidth: true

        onLabelTextChanged: {
            if (labelText !== TextSrc.textFile)
                TextSrc.textFile = labelText
        }
        onButtonClicked: fileDialog.open()
    }
    Label {
        text: TextSrc.fileError
        color: "red"
        wrapMode: Text.WordWrap
        visible: TextSrc.textSource == TextSrcElement.TextSourceFile
                 && TextSrc.fileError.length > 0
        Layout.fillWidth: true
    }
    Label {
        text: qsTr("The text updates automatically when the file changes.")
        wrapMode: Text.WordWrap
        opacity: 0.6
        font: AkTheme.fontSettings.subtitle2
        visible: TextSrc.textSource == TextSrcElement.TextSourceFile
        Layout.fillWidth: true
    }

    // Font

    SectionTitle {
        text: qsTr("Font")
    }
    AK.LabeledComboBox {
        label: qsTr("Family")
        model: TextSrc.fontFamilies
        currentIndex: TextSrc.fontFamilies.indexOf(TextSrc.fontFamily)
        Accessible.description: label
        Layout.fillWidth: true

        onActivated: TextSrc.fontFamily = TextSrc.fontFamilies[currentIndex]
    }
    NumberOption {
        label: qsTr("Size (pixels)")
        from: 1
        to: 2048
        value: TextSrc.fontSize

        onModified: newValue => TextSrc.fontSize = newValue
    }
    Switch {
        text: qsTr("Bold")
        checked: TextSrc.bold
        Accessible.name: text
        Layout.fillWidth: true

        onToggled: TextSrc.bold = checked
    }
    Switch {
        text: qsTr("Italic")
        checked: TextSrc.italic
        Accessible.name: text
        Layout.fillWidth: true

        onToggled: TextSrc.italic = checked
    }
    Switch {
        text: qsTr("Underline")
        checked: TextSrc.underline
        Accessible.name: text
        Layout.fillWidth: true

        onToggled: TextSrc.underline = checked
    }
    Switch {
        text: qsTr("Strikethrough")
        checked: TextSrc.strikeout
        Accessible.name: text
        Layout.fillWidth: true

        onToggled: TextSrc.strikeout = checked
    }
    NumberOption {
        label: qsTr("Letter spacing (pixels)")
        from: -100
        to: 500
        value: Math.round(TextSrc.letterSpacing)

        onModified: newValue => TextSrc.letterSpacing = newValue
    }
    NumberOption {
        label: qsTr("Line spacing (%)")
        from: 10
        to: 1000
        stepSize: 5
        value: Math.round(TextSrc.lineSpacing)

        onModified: newValue => TextSrc.lineSpacing = newValue
    }
    Switch {
        text: qsTr("Antialiasing")
        checked: TextSrc.antialiasing
        Accessible.name: text
        Layout.fillWidth: true

        onToggled: TextSrc.antialiasing = checked
    }

    // Layout

    SectionTitle {
        text: qsTr("Layout")
    }
    NumberOption {
        label: qsTr("Frame width")
        from: 16
        to: 4096
        stepSize: 10
        value: TextSrc.frameWidth

        onModified: newValue => TextSrc.frameWidth = newValue
    }
    NumberOption {
        label: qsTr("Frame height")
        from: 16
        to: 4096
        stepSize: 10
        value: TextSrc.frameHeight

        onModified: newValue => TextSrc.frameHeight = newValue
    }
    AK.LabeledComboBox {
        label: qsTr("Horizontal alignment")
        currentIndex: TextSrc.horizontalAlignment
        model: [qsTr("Left"),
                qsTr("Center"),
                qsTr("Right"),
                qsTr("Justify")]
        Accessible.description: label
        Layout.fillWidth: true

        onActivated: TextSrc.horizontalAlignment = currentIndex
    }
    AK.LabeledComboBox {
        label: qsTr("Vertical alignment")
        currentIndex: TextSrc.verticalAlignment
        model: [qsTr("Top"),
                qsTr("Middle"),
                qsTr("Bottom")]
        Accessible.description: label
        Layout.fillWidth: true

        onActivated: TextSrc.verticalAlignment = currentIndex
    }
    Switch {
        text: qsTr("Word wrap")
        checked: TextSrc.wordWrap
        Accessible.name: text
        Layout.fillWidth: true

        onToggled: TextSrc.wordWrap = checked
    }
    NumberOption {
        label: qsTr("Padding (pixels)")
        from: 0
        to: 2048
        value: TextSrc.padding

        onModified: newValue => TextSrc.padding = newValue
    }

    // Colors

    SectionTitle {
        text: qsTr("Colors")
    }
    AK.ColorButton {
        text: qsTr("Text color")
        currentColor: AkUtils.fromRgba(TextSrc.foregroundColor)
        title: qsTr("Choose the text color")
        showAlphaChannel: true
        horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
        Layout.fillWidth: true

        onCurrentColorChanged:
            TextSrc.foregroundColor = AkUtils.toRgba(currentColor)
    }

    // Background

    SectionTitle {
        text: qsTr("Background")
    }
    AK.ColorButton {
        text: qsTr("Background color")
        currentColor: AkUtils.fromRgba(TextSrc.backgroundColor)
        title: qsTr("Choose the background color")
        showAlphaChannel: true
        horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
        Layout.fillWidth: true

        onCurrentColorChanged:
            TextSrc.backgroundColor = AkUtils.toRgba(currentColor)
    }
    Switch {
        text: qsTr("Background fits the text")
        checked: TextSrc.fitBackground
        Accessible.name: text
        Layout.fillWidth: true

        onToggled: TextSrc.fitBackground = checked
    }
    AK.LabeledComboBox {
        label: qsTr("Shape")
        currentIndex: TextSrc.backgroundShape
        model: [qsTr("Rectangle"),
                qsTr("Ellipse"),
                qsTr("Cloud (thought)"),
                qsTr("Burst (shout)")]
        Accessible.description: label
        Layout.fillWidth: true

        onActivated: TextSrc.backgroundShape = currentIndex
    }
    NumberOption {
        label: qsTr("Corner radius")
        from: 0
        to: 2048
        value: TextSrc.backgroundRadius
        visible: TextSrc.backgroundShape == TextSrcElement.BackgroundShapeRectangle

        onModified: newValue => TextSrc.backgroundRadius = newValue
    }
    Switch {
        text: qsTr("Enable border")
        checked: TextSrc.backgroundBorderEnabled
        Accessible.name: text
        Layout.fillWidth: true

        onToggled: TextSrc.backgroundBorderEnabled = checked
    }
    AK.ColorButton {
        text: qsTr("Border color")
        currentColor: AkUtils.fromRgba(TextSrc.backgroundBorderColor)
        title: qsTr("Choose the border color")
        showAlphaChannel: true
        horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
        visible: TextSrc.backgroundBorderEnabled
        Layout.fillWidth: true

        onCurrentColorChanged:
            TextSrc.backgroundBorderColor = AkUtils.toRgba(currentColor)
    }
    NumberOption {
        label: qsTr("Border width (pixels)")
        from: 0
        to: 256
        value: TextSrc.backgroundBorderWidth
        visible: TextSrc.backgroundBorderEnabled

        onModified: newValue => TextSrc.backgroundBorderWidth = newValue
    }
    Switch {
        text: qsTr("Enable background shadow")
        checked: TextSrc.backgroundShadowEnabled
        Accessible.name: text
        Layout.fillWidth: true

        onToggled: TextSrc.backgroundShadowEnabled = checked
    }
    AK.ColorButton {
        text: qsTr("Background shadow color")
        currentColor: AkUtils.fromRgba(TextSrc.backgroundShadowColor)
        title: qsTr("Choose the background shadow color")
        showAlphaChannel: true
        horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
        visible: TextSrc.backgroundShadowEnabled
        Layout.fillWidth: true

        onCurrentColorChanged:
            TextSrc.backgroundShadowColor = AkUtils.toRgba(currentColor)
    }
    NumberOption {
        label: qsTr("Background shadow horizontal offset (pixels)")
        from: -512
        to: 512
        value: TextSrc.backgroundShadowOffsetX
        visible: TextSrc.backgroundShadowEnabled

        onModified: newValue => TextSrc.backgroundShadowOffsetX = newValue
    }
    NumberOption {
        label: qsTr("Background shadow vertical offset (pixels)")
        from: -512
        to: 512
        value: TextSrc.backgroundShadowOffsetY
        visible: TextSrc.backgroundShadowEnabled

        onModified: newValue => TextSrc.backgroundShadowOffsetY = newValue
    }
    NumberOption {
        label: qsTr("Background shadow blur (pixels)")
        from: 0
        to: 256
        value: TextSrc.backgroundShadowBlur
        visible: TextSrc.backgroundShadowEnabled

        onModified: newValue => TextSrc.backgroundShadowBlur = newValue
    }

    // Balloon tail

    SectionTitle {
        text: qsTr("Balloon tail")
    }
    AK.LabeledComboBox {
        label: qsTr("Style")
        currentIndex: TextSrc.tailStyle
        model: [qsTr("None"),
                qsTr("Pointer (speech)"),
                qsTr("Circles (thought)"),
                qsTr("Lightning")]
        Accessible.description: label
        Layout.fillWidth: true

        onActivated: TextSrc.tailStyle = currentIndex
    }
    NumberOption {
        label: qsTr("Direction (degrees, 90 = down)")
        from: 0
        to: 360
        stepSize: 5
        value: TextSrc.tailAngle
        visible: TextSrc.tailStyle != TextSrcElement.TailStyleNone

        onModified: newValue => TextSrc.tailAngle = newValue
    }
    NumberOption {
        label: qsTr("Length (pixels)")
        from: 0
        to: 1024
        stepSize: 5
        value: TextSrc.tailLength
        visible: TextSrc.tailStyle != TextSrcElement.TailStyleNone

        onModified: newValue => TextSrc.tailLength = newValue
    }
    NumberOption {
        label: qsTr("Width (pixels)")
        from: 1
        to: 1024
        stepSize: 5
        value: TextSrc.tailWidth
        visible: TextSrc.tailStyle != TextSrcElement.TailStyleNone

        onModified: newValue => TextSrc.tailWidth = newValue
    }

    // Outline

    SectionTitle {
        text: qsTr("Outline")
    }
    Switch {
        text: qsTr("Enable outline")
        checked: TextSrc.outlineEnabled
        Accessible.name: text
        Layout.fillWidth: true

        onToggled: TextSrc.outlineEnabled = checked
    }
    AK.ColorButton {
        text: qsTr("Outline color")
        currentColor: AkUtils.fromRgba(TextSrc.outlineColor)
        title: qsTr("Choose the outline color")
        showAlphaChannel: true
        horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
        visible: TextSrc.outlineEnabled
        Layout.fillWidth: true

        onCurrentColorChanged:
            TextSrc.outlineColor = AkUtils.toRgba(currentColor)
    }
    NumberOption {
        label: qsTr("Outline width (pixels)")
        from: 0
        to: 256
        value: TextSrc.outlineWidth
        visible: TextSrc.outlineEnabled

        onModified: newValue => TextSrc.outlineWidth = newValue
    }

    // Shadow

    SectionTitle {
        text: qsTr("Shadow")
    }
    Switch {
        text: qsTr("Enable shadow")
        checked: TextSrc.shadowEnabled
        Accessible.name: text
        Layout.fillWidth: true

        onToggled: TextSrc.shadowEnabled = checked
    }
    AK.ColorButton {
        text: qsTr("Shadow color")
        currentColor: AkUtils.fromRgba(TextSrc.shadowColor)
        title: qsTr("Choose the shadow color")
        showAlphaChannel: true
        horizontalAlignment: root.rtl? Text.AlignRight: Text.AlignLeft
        visible: TextSrc.shadowEnabled
        Layout.fillWidth: true

        onCurrentColorChanged:
            TextSrc.shadowColor = AkUtils.toRgba(currentColor)
    }
    NumberOption {
        label: qsTr("Horizontal offset (pixels)")
        from: -512
        to: 512
        value: TextSrc.shadowOffsetX
        visible: TextSrc.shadowEnabled

        onModified: newValue => TextSrc.shadowOffsetX = newValue
    }
    NumberOption {
        label: qsTr("Vertical offset (pixels)")
        from: -512
        to: 512
        value: TextSrc.shadowOffsetY
        visible: TextSrc.shadowEnabled

        onModified: newValue => TextSrc.shadowOffsetY = newValue
    }
    NumberOption {
        label: qsTr("Blur (pixels)")
        from: 0
        to: 256
        value: TextSrc.shadowBlur
        visible: TextSrc.shadowEnabled

        onModified: newValue => TextSrc.shadowBlur = newValue
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Choose the text file")
        fileMode: LABS.FileDialog.OpenFile
        nameFilters: [qsTr("Text files") + " (*.txt *.md *.log *.srt *.csv)",
                      qsTr("All Files") + " (*)"]

        onAccepted: TextSrc.textFile =
                        String(file).replace(root.filePrefix, "")
    }
}

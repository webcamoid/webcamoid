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
import Ak

Dialog {
    id: videoCodecOptions
    title: qsTr("Video Codec Options")
    standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Reset
    width: AkUnit.create(420 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(320 * AkTheme.controlScale, "dp").pixels
    modal: true

    property string currentCodec: ""
    property variant controlValues: ({})
    property int startChildren: 7

    function updateValues(key, value) {
        controlValues[key] = value
    }

    function updateOptions() {
        for (let i = mainLayout.children.length - 1; i >= startChildren; i--)
            mainLayout.children[i].destroy()

        let codecOptions = recording.codecOptions(AkCaps.CapsVideo);

        for (let i in codecOptions) {
            let option = AkPropertyOption.create(codecOptions[i])

            if (option.type != AkPropertyOption.OptionType_Flags) {
                let cLabel = controlLabel.createObject(mainLayout);
                cLabel.text = option.description
            }

            let value =
                recording.codecOptionValue(AkCaps.CapsVideo, option.name)

            switch (option.type) {
            case AkPropertyOption.OptionType_String:
                if (option.menu.length < 1) {
                    let cString = controlString.createObject(mainLayout)
                    cString.key = option.name
                    cString.defaultValue = option.defaultValue
                    cString.text = value
                    cString.onControlChanged.connect(updateValues)
                } else {
                    let cMenu = controlMenu.createObject(mainLayout)
                    cMenu.key = option.name
                    cMenu.defaultValue = option.defaultValue
                    cMenu.update(option)
                    cMenu.onControlChanged.connect(updateValues)
                }

                break

            case AkPropertyOption.OptionType_Number:
                if (option.menu.length < 1) {
                    let minimumValue = option.min
                    let maximumValue = option.max
                    let stepSize = option.step
                    let maxSteps = 4096

                    if ((maximumValue - minimumValue) <= maxSteps * stepSize) {
                        let cRangeDiscrete = controlRangeDiscrete.createObject(mainLayout)
                        cRangeDiscrete.key = option.name
                        cRangeDiscrete.defaultValue = option.defaultValue
                        cRangeDiscrete.from = minimumValue
                        cRangeDiscrete.to = maximumValue
                        cRangeDiscrete.stepSize = stepSize
                        cRangeDiscrete.value = value
                        cRangeDiscrete.onControlChanged.connect(updateValues)
                    } else {
                        let cRange = controlRange.createObject(mainLayout)
                        cRange.key = option.name
                        cRange.defaultValue = option.defaultValue
                        cRange.text = value
                        cRange.onControlChanged.connect(updateValues)
                    }
                } else {
                    let cMenu = controlMenu.createObject(mainLayout)
                    cMenu.key = option.name
                    cMenu.defaultValue = option.defaultValue
                    cMenu.update(option)
                    cMenu.onControlChanged.connect(updateValues)
                }

                break

            case AkPropertyOption.OptionType_Boolean:
                let cBoolean = controlBoolean.createObject(mainLayout)
                cBoolean.key = option.name
                cBoolean.defaultValue = option.defaultValue
                cBoolean.checked = value
                cBoolean.onControlChanged.connect(updateValues)

                break

            case AkPropertyOption.OptionType_Flags:
                let cFlags = controlFlags.createObject(mainLayout)
                cFlags.key = option.name
                cFlags.defaultValue = option.defaultValue
                cFlags.title = option.description
                cFlags.update(option)
                cFlags.onControlChanged.connect(updateValues)

                break

            case AkPropertyOption.OptionType_Frac:
                let cFrac = controlFrac.createObject(mainLayout)
                cFrac.key = option.name
                cFrac.defaultValue = option.defaultValue
                cFrac.text = value
                cFrac.onControlChanged.connect(updateValues)

                break

            case AkPropertyOption.OptionType_ImageSize:
                let cImageSize = controlImageSize.createObject(mainLayout)
                cImageSize.key = option.name
                cImageSize.defaultValue = option.defaultValue
                cImageSize.text = value
                cImageSize.onControlChanged.connect(updateValues)

                break

            default:
                break
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            bitrate.text = recording.bitrate(AkCaps.CapsVideo)
            videoGOP.text = recording.videoGOP
            audioCodecOptions.currentCodec = recording.codec(AkCaps.CapsVideo)
            cbxVideoCodec.model.clear()
            let codecs =
                recording.supportedCodecs(recording.videoFormat,
                                          AkCaps.CapsVideo);

            for (let i in codecs) {
                let cdc = codecs[i]

                cbxVideoCodec.model.append({
                    codec: cdc,
                    description: recording.codecDescription(cdc)
                })
            }

            let index = codecs.indexOf(audioCodecOptions.currentCodec)

            if (index < 0) {
                index =
                    codecs.indexOf(recording.defaultCodec(recording.videoFormat,
                                                          AkCaps.CapsVideo))

                if (index < 0 && codecs.length > 0)
                    index = 0
            }

            cbxVideoCodec.currentIndex = index
            videoCodecOptions.updateOptions()
            cbxVideoCodec.forceActiveFocus()
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: mainLayout.height
        clip: true

        GridLayout {
            id: mainLayout
            columns: 2
            width: scrollView.width

            Label {
                id: txtVideoCodec
                text: qsTr("Video codec")
            }
            ComboBox {
                id: cbxVideoCodec
                Accessible.description: txtVideoCodec.text
                textRole: "description"
                Layout.fillWidth: true
                model: ListModel {
                }

                onCurrentIndexChanged: {
                    if (currentIndex >= 0) {
                        recording.setCodec(AkCaps.CapsVideo,
                                           model.get(currentIndex).codec)
                        videoCodecOptions.updateOptions()
                    }
                }
            }
            Label {
                id: txtBitrate
                text: qsTr("Bitrate")
            }
            TextField {
                id: bitrate
                placeholderText: qsTr("Bitrate (bits/secs)")
                Accessible.name: txtBitrate.text
                selectByMouse: true
                validator: RegularExpressionValidator {
                    regularExpression: /\d+/
                }
                Layout.fillWidth: true
            }
            Label {
                id: txtGOP
                text: qsTr("Keyframes stride (ms)")
            }
            TextField {
                id: videoGOP
                placeholderText: qsTr("1000")
                Accessible.name: txtGOP.text
                selectByMouse: true
                validator: RegularExpressionValidator {
                    regularExpression: /\d+/
                }
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Advanced options")
                font: AkTheme.fontSettings.h6
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.columnSpan: 2
            }
        }
    }

    onAccepted: {
        recording.setBitrate(AkCaps.CapsVideo,
                             Number.fromLocaleString(locale, bitrate.text))
        recording.videoGOP = Number.fromLocaleString(locale, videoGOP.text)

        for (let key in controlValues)
            recording.setCodecOptionValue(AkCaps.CapsVideo,
                                          key,
                                          controlValues[key]);
    }
    onRejected: {
        recording.setCodec(AkCaps.CapsVideo, videoCodecOptions.currentCodec)
    }
    onReset: {
        let codecs =
            recording.supportedCodecs(recording.videoFormat, AkCaps.CapsVideo)
        cbxVideoCodec.currentIndex =
            codecs.indexOf(recording.defaultCodec(recording.videoFormat,
                                                  AkCaps.CapsVideo))
        bitrate.text = recording.defaultBitrate(AkCaps.CapsAudio)
        videoGOP.text = recording.defaultVideoGOP

        for (let i in mainLayout.children)
            if (mainLayout.children[i].reset)
                mainLayout.children[i].reset()
    }

    Component {
        id: controlLabel

        Label {
        }
    }
    Component {
        id: controlString

        TextField {
            selectByMouse: true
            Layout.fillWidth: true
            Accessible.name: key

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                text = recording.codecOptionValue(AkCaps.CapsVideo, key)
            }

            function reset() {
                text = defaultValue
            }

            onTextChanged: controlChanged(key, text)
        }
    }
    Component {
        id: controlFrac

        TextField {
            selectByMouse: true
            validator: RegularExpressionValidator {
                regularExpression: /-?\d+\/\d+/
            }
            Layout.fillWidth: true
            Accessible.name: key

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                text = recording.codecOptionValue(AkCaps.CapsVideo, key)
            }

            function reset() {
                text = defaultValue
            }

            onTextChanged: controlChanged(key, text)
        }
    }
    Component {
        id: controlRangeDiscrete

        GridLayout {
            id: rangeLayout
            columns: 2

            property string key: ""
            property variant defaultValue: null
            property real value: 0
            property real from: 0
            property real to: 1
            property real stepSize: 1

            signal controlChanged(string key, variant value)

            function restore() {
                sldRange.value =
                    recording.codecOptionValue(AkCaps.CapsVideo, key)
            }

            function reset() {
                sldRange.value = defaultValue
            }

            Slider {
                id: sldRange
                value: parent.value
                from: parent.from
                to: parent.to
                stepSize: parent.stepSize
                Layout.fillWidth: true
                Accessible.name: rangeLayout.key

                onValueChanged: {
                    spbRange.value = spbRange.multiplier * value
                    rangeLayout.controlChanged(rangeLayout.key, value)
                }
            }
            SpinBox {
                id: spbRange
                value: multiplier * sldRange.value
                from: multiplier * parent.from
                to: multiplier * parent.to
                stepSize: multiplier * parent.stepSize
                editable: true
                validator: DoubleValidator {
                    bottom: Math.min(spbRange.from, spbRange.to)
                    top:  Math.max(spbRange.from, spbRange.to)
                }
                Accessible.name: rangeLayout.key

                readonly property int decimals: parent.stepSize < 1? 2: 0
                readonly property int multiplier: Math.pow(10, decimals)

                textFromValue: function(value, locale) {
                    return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
                }
                valueFromText: function(text, locale) {
                    return Number.fromLocaleString(locale, text) * multiplier
                }
                onValueModified: sldRange.value = value / multiplier
            }
        }
    }
    Component {
        id: controlRange

        TextField {
            selectByMouse: true
            validator: RegularExpressionValidator {
                regularExpression: /[-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?/
            }
            Layout.fillWidth: true
            Accessible.name: key

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                text = recording.codecOptionValue(AkCaps.CapsVideo, key)
            }

            function reset() {
                text = defaultValue
            }

            onTextChanged: controlChanged(key, Number(text))
        }
    }
    Component {
        id: controlBoolean

        Switch {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            Accessible.name: key

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                checked = recording.codecOptionValue(AkCaps.CapsVideo, key)
            }

            function reset() {
                checked = defaultValue
            }

            onCheckedChanged: controlChanged(key, checked)
        }
    }
    Component {
        id: controlMenu

        ComboBox {
            model: ListModel {
            }
            textRole: "description"
            Layout.fillWidth: true
            Accessible.description: key

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                let value = recording.codecOptionValue(AkCaps.CapsVideo, key)

                for (let i = 0; i < model.count; i++)
                    if (model.get(i).value == value) {
                        currentIndex = i

                        return
                    }

                currentIndex = model.count > 0? 0: -1
            }

            function reset() {
                for (let i = 0; i < model.count; i++)
                    if (model.get(i).value == defaultValue) {
                        currentIndex = i

                        return
                    }

                currentIndex = model.count > 0? 0: -1
            }

            function update(option)
            {
                model.clear()
                let menu = option.menu

                for (let i in menu) {
                    let menuOption = AkMenuOption.create(menu[i])

                    model.append({
                        value: menuOption.value,
                        description: menuOption.description
                    })
                }

                currentIndex = currentMenuIndex(option)
            }

            function currentMenuIndex(option)
            {
                let value = recording.codecOptionValue(AkCaps.CapsVideo,
                                                       option.name)
                let menu = option.menu

                for (let i in menu) {
                    let menuOption = AkMenuOption.create(menu[i])

                    if (menuOption.value == value)
                        return i
                }

                return menu.length > 0? 0: -1
            }

            onCurrentIndexChanged: {
                if (currentIndex >= 0)
                    controlChanged(key, model.get(currentIndex).value);
            }
        }
    }
    Component {
        id: controlFlags

        GroupBox {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Accessible.name: key

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            ColumnLayout {
                id: flagsLayout
                anchors.fill: parent
            }

            Component {
                id: classFlag

                CheckBox {
                    Layout.fillWidth: true

                    property int flagValue: 0
                }
            }

            function restore() {
                let value = recording.codecOptionValue(AkCaps.CapsVideo, key)

                for (let i in flagsLayout.children) {
                    flagsLayout.children[i].checked =
                            value & flagsLayout.children[i].flagValue
                }
            }

            function reset() {
                for (let i in flagsLayout.children) {
                    flagsLayout.children[i].checked =
                            defaultValue & flagsLayout.children[i].flagValue
                }
            }

            function update(option)
            {
                // Remove old controls.
                for (let i = flagsLayout.children.length - 1; i >= 0; i--)
                    flagsLayout.children[i].destroy()

                let value = recording.codecOptionValue(AkCaps.CapsVideo,
                                                       option.name)
                let menu = option.menu

                // Create new ones.
                for (let i in menu) {
                    let menuOption = AkMenuOption.create(menu[i])
                    let flag = classFlag.createObject(flagsLayout)
                    flag.text = menuOption.description
                    flag.flagValue = menuOption.value
                    flag.checked = value & menuOption.value

                    flag.onCheckedChanged.connect(function (checked)
                    {
                        let flags = 0

                        for (var i in flagsLayout.children) {
                            if (flagsLayout.children[i].checked)
                                flags |= flagsLayout.children[i].flagValue
                        }

                        controlChanged(key, flags)
                    })
                }
            }
        }
    }
    Component {
        id: controlImageSize

        TextField {
            selectByMouse: true
            validator: RegularExpressionValidator {
                regularExpression: /\d+x\d+/
            }
            Layout.fillWidth: true
            Accessible.name: key

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                let value = recording.codecOptionValue(AkCaps.CapsVideo, key)

                text = value.width + "x" + value.height
            }

            function reset() {
                text = defaultValue
            }

            onTextChanged: {
                let sizeParts = text.split("x")
                let width = Number.fromLocaleString(locale, sizeParts[0])
                let height = Number.fromLocaleString(locale, sizeParts[1])
                controlChanged(key, Qt.size(width, height))
            }
        }
    }
}

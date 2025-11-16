/* Webcamoid, camera capture application.
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
import AkControls as AK

Dialog {
    id: audioCodecOptions
    title: qsTr("Audio Codec Options")
    standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Reset
    width: AkUnit.create(420 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(320 * AkTheme.controlScale, "dp").pixels
    modal: true

    property string currentCodec: ""
    property variant controlValues: ({})
    property int startChildren: 4
    property real maxSpinLabelWidth: 0

    function updateValues(key, value) {
        controlValues[key] = value
    }

    function updateOptions() {
        for (let i = mainLayout.children.length - 1; i >= startChildren; i--)
            mainLayout.children[i].destroy()

        let codecOptions = recording.codecOptions(AkCaps.CapsAudio);

        // Calculate the maximum width for the SpinBox labels

        maxSpinLabelWidth = 0

        let metrics = textMetricsComponent.createObject(null)

        for (let i in codecOptions) {
            let option = AkPropertyOption.create(codecOptions[i])

            if (option.type === AkPropertyOption.OptionType_Number && option.menu.length < 1) {
                let minimumValue = option.min
                let maximumValue = option.max
                let stepSize = option.step
                let maxSteps = 4096

                if ((maximumValue - minimumValue) <= maxSteps * stepSize) {
                    metrics.text = option.description
                    let bounding = AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                    maxSpinLabelWidth = Math.max(maxSpinLabelWidth, metrics.boundingRect.width + bounding)
                }
            }
        }

        metrics.destroy()

        // Create the controls

        for (let i in codecOptions) {
            let option = AkPropertyOption.create(codecOptions[i])
            let value =
                recording.codecOptionValue(AkCaps.CapsAudio, option.name)

            switch (option.type) {
            case AkPropertyOption.OptionType_String:
                if (option.menu.length < 1) {
                    let cString = controlString.createObject(mainLayout)
                    cString.key = option.name
                    cString.description = option.description
                    cString.value = value
                    cString.defaultValue = option.defaultValue
                    cString.onControlChanged.connect(updateValues)
                } else {
                    let cMenu = controlMenu.createObject(mainLayout)
                    cMenu.key = option.name
                    cMenu.defaultValue = option.defaultValue
                    cMenu.label = option.description
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
                        cRangeDiscrete.description = option.description
                        cRangeDiscrete.defaultValue = option.defaultValue
                        cRangeDiscrete.controlMinimumValue = minimumValue
                        cRangeDiscrete.controlMaximumValue = maximumValue
                        cRangeDiscrete.controlStepValue = stepSize
                        cRangeDiscrete.controlValue = value
                        cRangeDiscrete.onControlChanged.connect(updateValues)
                    } else {
                        let cRange = controlRange.createObject(mainLayout)
                        cRange.key = option.name
                        cRange.description = option.description
                        cRange.value = value
                        cRange.defaultValue = option.defaultValue
                        cRange.onControlChanged.connect(updateValues)
                    }
                } else {
                    let cMenu = controlMenu.createObject(mainLayout)
                    cMenu.key = option.name
                    cMenu.defaultValue = option.defaultValue
                    cMenu.label = option.description
                    cMenu.update(option)
                    cMenu.onControlChanged.connect(updateValues)
                }

                break

            case AkPropertyOption.OptionType_Boolean:
                let cBoolean = controlBoolean.createObject(mainLayout)
                cBoolean.key = option.name
                cBoolean.defaultValue = option.defaultValue
                cBoolean.text = option.description
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
                cFrac.description = option.description
                cFrac.value = value
                cFrac.defaultValue = option.defaultValue
                cFrac.onControlChanged.connect(updateValues)

                break

            default:
                break
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            bitrate.text = recording.bitrate(AkCaps.CapsAudio)
            audioCodecOptions.currentCodec = recording.codec(AkCaps.CapsAudio)
            cbxAudioCodec.model.clear()
            let codecs =
                recording.supportedCodecs(recording.videoFormat,
                                          AkCaps.CapsAudio);

            for (let i in codecs) {
                let cdc = codecs[i]

                cbxAudioCodec.model.append({
                    codec: cdc,
                    description: recording.codecDescription(cdc)
                })
            }

            let index = codecs.indexOf(audioCodecOptions.currentCodec)

            if (index < 0) {
                index =
                    codecs.indexOf(recording.defaultCodec(recording.videoFormat,
                                                          AkCaps.CapsAudio))

                if (index < 0 && codecs.length > 0)
                    index = 0
            }

            cbxAudioCodec.currentIndex = index
            audioCodecOptions.updateOptions()
            cbxAudioCodec.forceActiveFocus()
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: mainLayout.height
        clip: true

        ColumnLayout {
            id: mainLayout
            width: scrollView.width

            AK.LabeledComboBox {
                id: cbxAudioCodec
                label: qsTr("Audio codec")
                Accessible.description: label
                textRole: "description"
                Layout.fillWidth: true
                model: ListModel {
                }

                onCurrentIndexChanged: {
                    if (currentIndex >= 0) {
                        recording.setCodec(AkCaps.CapsAudio,
                                           model.get(currentIndex).codec)
                        audioCodecOptions.updateOptions()
                    }
                }
            }
            Label {
                id: txtBitrate
                text: qsTr("Bitrate")
                font.bold: true
                Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
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
                text: qsTr("Advanced options")
                font: AkTheme.fontSettings.h6
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }
        }
    }

    onAccepted: {
        recording.setBitrate(AkCaps.CapsAudio,
                             Number.fromLocaleString(locale, bitrate.text))

        for (let key in controlValues)
            recording.setCodecOptionValue(AkCaps.CapsAudio,
                                          key,
                                          controlValues[key]);
    }
    onRejected: {
        recording.setCodec(AkCaps.CapsAudio, audioCodecOptions.currentCodec)
    }
    onReset: {
        let codecs =
            recording.supportedCodecs(recording.videoFormat, AkCaps.CapsAudio)
        cbxAudioCodec.currentIndex =
            codecs.indexOf(recording.defaultCodec(recording.videoFormat,
                                                  AkCaps.CapsAudio))
        bitrate.text = recording.defaultBitrate(AkCaps.CapsAudio)

        for (let i in mainLayout.children)
            if (mainLayout.children[i].reset)
                mainLayout.children[i].reset()
    }

    Component {
        id: textMetricsComponent

        TextMetrics {
            font: AkTheme.fontSettings.body1
        }
    }
    Component {
        id: controlString

        ColumnLayout {
            Layout.fillWidth: true

            property string key: ""
            property string description: ""
            property string value: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                csTextField.text = recording.codecOptionValue(AkCaps.CapsAudio, key)
            }

            function reset() {
                csTextField.text = defaultValue
            }

            Label {
                text: parent.description
                font.bold: true
                Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            }
            TextField {
                id: csTextField
                text: parent.value
                selectByMouse: true
                Layout.fillWidth: true
                Accessible.name: parent.key

                onTextChanged: parent.controlChanged(parent.key, text)
            }
        }
    }
    Component {
        id: controlFrac

        ColumnLayout {
            Layout.fillWidth: true

            property string key: ""
            property string description: ""
            property string value: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                cfTextField.text = recording.codecOptionValue(AkCaps.CapsAudio, key)
            }

            function reset() {
                cfTextField.text = defaultValue
            }

            Label {
                text: parent.description
                font.bold: true
                Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            }
            TextField {
                id: cfTextField
                text: parent.value
                selectByMouse: true
                validator: RegularExpressionValidator {
                    regularExpression: /-?\d+\/\d+/
                }
                Layout.fillWidth: true
                Accessible.name: key

                onTextChanged: parent.controlChanged(parent.key, text)
            }
        }
    }
    Component {
        id: controlRangeDiscrete

        RowLayout {
            Layout.fillWidth: true

            property string key: ""
            property string description: ""
            property real defaultValue: 0
            property real controlValue: 0
            property real controlMinimumValue: 0
            property real controlMaximumValue: 1
            property real controlStepValue: 1

            signal controlChanged(string key, variant value)

            function restore() {
                value = multiplier * recording.codecOptionValue(AkCaps.CapsAudio, key)
            }

            function reset() {
                spbRange.value = multiplier * defaultValue
            }

            Label {
                text: parent.description
                Layout.minimumWidth: audioCodecOptions.maxSpinLabelWidth
            }
            SpinBox {
                id: spbRange
                value: multiplier * parent.controlValue
                from: multiplier * parent.controlMinimumValue
                to: multiplier * parent.controlMaximumValue
                stepSize: multiplier * parent.controlStepValue
                editable: true
                validator: DoubleValidator {
                    bottom: Math.min(spbRange.from, spbRange.to)
                    top:  Math.max(spbRange.from, spbRange.to)
                }
                Accessible.name: key

                readonly property int decimals: parent.controlStepValue < 1? 2: 0
                readonly property int multiplier: Math.pow(10, decimals)

                textFromValue: function(value, locale) {
                    return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
                }
                valueFromText: function(text, locale) {
                    return Number.fromLocaleString(locale, text) * multiplier
                }
                onValueModified: {
                    parent.controlChanged(parent.key, value / multiplier)
                }
            }
        }
    }
    Component {
        id: controlRange

        ColumnLayout {
            Layout.fillWidth: true

            property string key: ""
            property string description: ""
            property string value: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                crTextField.text = recording.codecOptionValue(AkCaps.CapsAudio, key)
            }

            function reset() {
                crTextField.text = defaultValue
            }

            Label {
                text: parent.description
                font.bold: true
                Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            }
            TextField {
                id: crTextField
                text: parent.value
                selectByMouse: true
                validator: RegularExpressionValidator {
                    regularExpression: /[-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?/
                }
                Layout.fillWidth: true
                Accessible.name: key

                onTextChanged: parent.controlChanged(parent.key, Number(text))
            }
        }
    }
    Component {
        id: controlBoolean

        Switch {
            Layout.fillWidth: true
            Accessible.name: key

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                checked = recording.codecOptionValue(AkCaps.CapsAudio, key)
            }

            function reset() {
                checked = defaultValue
            }

            onCheckedChanged: controlChanged(key, checked)
        }
    }
    Component {
        id: controlMenu

        AK.LabeledComboBox {
            model: ListModel {
            }
            textRole: "description"
            Layout.fillWidth: true
            Accessible.description: key

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                let value = recording.codecOptionValue(AkCaps.CapsAudio, key)

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
                let value = recording.codecOptionValue(AkCaps.CapsAudio,
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
                let value = recording.codecOptionValue(AkCaps.CapsAudio, key)

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

                let value = recording.codecOptionValue(AkCaps.CapsAudio,
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
}

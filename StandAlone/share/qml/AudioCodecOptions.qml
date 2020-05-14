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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Ak 1.0

Dialog {
    id: audioCodecOptions
    title: qsTr("Audio Codec Options")
    standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Reset
    width: AkUnit.create(420 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(320 * AkTheme.controlScale, "dp").pixels
    modal: true

    property variant controlValues: ({})
    property int startChildren: 2

    function updateOptions() {
        for (let i = mainLayout.children.length - 1; i >= startChildren; i--)
            mainLayout.children[i].destroy()

        let options = recording.availableAudioCodecOptions

        for (let i in options) {
            if (options[i][2] != "flags") {
                let cLabel = controlLabel.createObject(mainLayout);
                cLabel.text = options[i][0]
            }

            let value = recording.audioCodecOptions[options[i][0]]

            if (!value)
                value = options[i][7]

            switch (options[i][2]) {
            case "string":
                let cString = controlString.createObject(mainLayout)
                cString.key = options[i][0]
                cString.defaultValue = options[i][6]
                cString.text = value
                cString.onControlChanged.connect(updateValues)

                break

            case "number":
                let minimumValue = options[i][3]
                let maximumValue = options[i][4]
                let stepSize = options[i][5]
                let maxSteps = 4096

                if ((maximumValue - minimumValue) <= maxSteps * stepSize) {
                    let cRangeDiscrete = controlRangeDiscrete.createObject(mainLayout)
                    cRangeDiscrete.key = options[i][0]
                    cRangeDiscrete.defaultValue = options[i][6]
                    cRangeDiscrete.from = minimumValue
                    cRangeDiscrete.to = maximumValue
                    cRangeDiscrete.stepSize = stepSize
                    cRangeDiscrete.value = value
                    cRangeDiscrete.onControlChanged.connect(updateValues)
                } else {
                    let cRange = controlRange.createObject(mainLayout)
                    cRange.key = options[i][0]
                    cRange.defaultValue = options[i][6]
                    cRange.text = value
                    cRange.onControlChanged.connect(updateValues)
                }

                break

            case "boolean":
                let cBoolean = controlBoolean.createObject(mainLayout)
                cBoolean.key = options[i][0]
                cBoolean.defaultValue = options[i][6]
                cBoolean.checked = value
                cBoolean.onControlChanged.connect(updateValues)

                break

            case "menu":
                let cMenu = controlMenu.createObject(mainLayout)
                cMenu.key = options[i][0]
                cMenu.defaultValue = options[i][6]
                cMenu.update(options[i])
                cMenu.onControlChanged.connect(updateValues)

                break

            case "flags":
                let cFlags = controlFlags.createObject(mainLayout)
                cFlags.key = options[i][0]
                cFlags.defaultValue = options[i][6]
                cFlags.title = options[i][0]
                cFlags.update(options[i])
                cFlags.onControlChanged.connect(updateValues)

                break

            case "frac":
                let cFrac = controlFrac.createObject(mainLayout)
                cFrac.key = options[i][0]
                cFrac.defaultValue = options[i][6]
                cFrac.text = value
                cFrac.onControlChanged.connect(updateValues)

                break

            default:
                break
            }
        }
    }

    function updateValues(key, value) {
        controlValues[key] = value
    }

    Connections {
        target: recording

        onAudioCodecParamsChanged: {
            if (audioCodecParams.codec)
                bitrate.text = audioCodecParams.bitrate
            else
                bitrate.text = ""
        }
        onAvailableAudioCodecOptionsChanged: audioCodecOptions.updateOptions()
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
                text: qsTr("Bitrate")
            }
            TextField {
                id: bitrate
                placeholderText: qsTr("Bitrate (bits/secs)")
                validator: RegExpValidator {
                    regExp: /\d+/
                }
                Layout.fillWidth: true

                Component.onCompleted:
                    text = recording.audioCodecParams.bitrate
            }

            Component.onCompleted: audioCodecOptions.updateOptions()
        }
    }

    onAccepted: {
        let params = recording.audioCodecParams
        params.bitrate = Number.fromLocaleString(locale, bitrate.text)
        recording.audioCodecParams = params
        let options = recording.audioCodecOptions

        for (let key in controlValues)
            options[key] = controlValues[key]

        recording.audioCodecOptions = options
    }
    onRejected: {
        if (recording.audioCodecParams.codec)
            bitrate.text = recording.audioCodecParams.bitrate
        else
            bitrate.text = ""

        for (let i in mainLayout.children)
            if (mainLayout.children[i].restore)
                mainLayout.children[i].restore()
    }
    onReset: {
        if (recording.audioCodecParams.codec)
            bitrate.text = recording.audioCodecParams.defaultBitrate
        else
            bitrate.text = ""

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

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                let value = recording.audioCodecOptions[key]

                if (!value)
                    value = defaultValue

                text = value
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
            Layout.fillWidth: true
            validator: RegExpValidator {
                regExp: /-?\d+\/\d+/
            }

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                let value = recording.audioCodecOptions[key]

                if (!value)
                    value = defaultValue

                text = value
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
                let value = recording.audioCodecOptions[key]

                if (!value)
                    value = defaultValue

                sldRange.value = value
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
            Layout.fillWidth: true
            validator: RegExpValidator {
                regExp: /[-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?/
            }

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                let value = recording.audioCodecOptions[key]

                if (!value)
                    value = defaultValue

                text = value
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

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                let value = recording.audioCodecOptions[key]

                if (!value)
                    value = defaultValue

                checked = value
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

            property string key: ""
            property variant defaultValue: null

            signal controlChanged(string key, variant value)

            function restore() {
                let value = recording.audioCodecOptions[key]

                if (!value)
                    value = defaultValue

                for (let i = 0; i < model.count; i++)
                    if (model.get(i).value == value) {
                        currentIndex = i

                        return
                    }

                currentIndex = -1
            }

            function reset() {
                for (let i = 0; i < model.count; i++)
                    if (model.get(i).value == defaultValue) {
                        currentIndex = i

                        return
                    }

                currentIndex = -1
            }

            function update(options)
            {
                model.clear()

                for (let i in options[8]) {
                    let description = options[8][i][0]

                    if (options[8][i][1].length > 0)
                        description += " - " + options[8][i][1]

                    model.append({
                        value: options[8][i][0],
                        description: description
                    })
                }

                currentIndex = currentMenuIndex(options)
            }

            function currentMenuIndex(options)
            {
                let value = recording.audioCodecOptions[options[0]]

                if (!value)
                    value = options[7]

                for (let i in options[8])
                    if (options[8][i][0] == value)
                        return i

                return -1
            }

            onCurrentIndexChanged: {
                var value = model.get(currentIndex).value;
                controlChanged(key, value);
            }
        }
    }
    Component {
        id: controlFlags

        GroupBox {
            Layout.columnSpan: 2
            Layout.fillWidth: true

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
                }
            }

            function restore() {
                let value = recording.audioCodecOptions[key]

                if (!value)
                    value = defaultValue

                for (let i in flagsLayout.children) {
                    flagsLayout.children[i].checked =
                            value.includes(flagsLayout.children[i].text)
                }
            }

            function reset() {
                for (let i in flagsLayout.children) {
                    flagsLayout.children[i].checked =
                            defaultValue.includes(flagsLayout.children[i].text)
                }
            }

            function update(options)
            {
                // Remove old controls.
                for (let i = flagsLayout.children.length - 1; i >= 0; i--)
                    flagsLayout.children[i].destroy()

                let value = recording.audioCodecOptions[options[0]]

                if (!value)
                    value = options[7]

                // Create new ones.
                for (let i in options[8]) {
                    let flag = classFlag.createObject(flagsLayout)
                    flag.text = options[8][i][0]
                    flag.checked = value.indexOf(flag.text) >= 0

                    flag.onCheckedChanged.connect(function (checked)
                    {
                        var flags = []

                        for (var i in flagsLayout.children) {
                            if (flagsLayout.children[i].checked)
                                flags += flagsLayout.children[i].text
                        }

                        controlChanged(key, flags)
                    })
                }
            }
        }
    }
}

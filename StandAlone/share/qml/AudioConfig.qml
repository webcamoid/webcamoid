/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1

ColumnLayout {
    function updateOptions()
    {
        if (Webcamoid.recordAudioFrom === "source")
            optSource.checked = true
        else if (Webcamoid.recordAudioFrom === "mic")
            optMic.checked = true
        else
            optNone.checked = true
    }

    Component.onCompleted: updateOptions()

    GroupBox {
        title: qsTr("Record audio from")

        ColumnLayout {
            ExclusiveGroup {
                id: captureFrom
            }
            RadioButton {
                id: optSource
                text: qsTr("Source")
                exclusiveGroup: captureFrom

                onCheckedChanged: {
                    if (checked)
                        Webcamoid.recordAudioFrom = "source"
                }
            }
            RadioButton {
                id: optMic
                text: qsTr("Mic")
                exclusiveGroup: captureFrom

                onCheckedChanged: {
                    if (checked)
                        Webcamoid.recordAudioFrom = "mic"
                }
            }
            RadioButton {
                id: optNone
                text: qsTr("None")
                exclusiveGroup: captureFrom

                onCheckedChanged: {
                    if (checked)
                        Webcamoid.recordAudioFrom = "none"
                }
            }
        }
    }
    CheckBox {
        text: qsTr("Play audio from source")
        checked: Webcamoid.playAudioFromSource

        onCheckedChanged: Webcamoid.playAudioFromSource = checked
    }
    Label {
        Layout.fillHeight: true
    }
}

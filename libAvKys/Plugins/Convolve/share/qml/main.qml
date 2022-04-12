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
import Ak 1.0

ColumnLayout {
    id: configs
    property int cellSize: 50

    function updateKernel(index, value)
    {
        var kernel = Convolve.kernel
        kernel[index] = value
        Convolve.kernel = kernel
    }

    Connections {
        target: Convolve

        function onBiasChanged(bias)
        {
            sldBias.value = bias
            spbBias.value = bias
        }
    }

    Label {
        //: https://en.wikipedia.org/wiki/Kernel_(image_processing)
        text: qsTr("Convolve matrix")
    }
    GridLayout {
        columns: 3

        // Row 0
        TextField {
            id: k00
            text: Convolve.kernel[0]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?\d+/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 0, Row 0")

            onTextChanged: updateKernel(0, text)
        }
        TextField {
            id: k01
            text: Convolve.kernel[1]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?\d+/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 1, Row 0")

            onTextChanged: updateKernel(1, text)
        }
        TextField {
            id: k02
            text: Convolve.kernel[2]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?\d+/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 2, Row 0")

            onTextChanged: updateKernel(2, text)
        }

        // Row 1
        TextField {
            id: k10
            text: Convolve.kernel[3]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?\d+/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 0, Row 1")

            onTextChanged: updateKernel(3, text)
        }
        TextField {
            id: k11
            text: Convolve.kernel[4]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?\d+/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 1, Row 1")

            onTextChanged: updateKernel(4, text)
        }
        TextField {
            id: k12
            text: Convolve.kernel[5]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?\d+/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 2, Row 1")

            onTextChanged: updateKernel(5, text)
        }

        // Row 2
        TextField {
            id: k20
            text: Convolve.kernel[6]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?\d+/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 0, Row 2")

            onTextChanged: updateKernel(6, text)
        }
        TextField {
            id: k21
            text: Convolve.kernel[7]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?\d+/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 1, Row 2")

            onTextChanged: updateKernel(7, text)
        }
        TextField {
            id: k22
            text: Convolve.kernel[8]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?\d+/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 2, Row 2")

            onTextChanged: updateKernel(8, text)
        }
    }

    GridLayout {
        columns: 3

        Label {
            id: txtFactor
            text: qsTr("Factor")
        }
        TextField {
            text: AkFrac.create(Convolve.factor).string
            placeholderText: qsTr("Factor")
            validator: RegExpValidator {
                regExp: /-?\d+\/\d+/
            }
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Accessible.name: txtFactor.text

            onTextChanged: Convolve.factor = AkFrac.create(text).toVariant()
        }

        Label {
            id: txtBias
            text: qsTr("Bias")
        }
        Slider {
            id: sldBias
            value: Convolve.bias
            stepSize: 1
            from: -255
            to: 255
            Layout.fillWidth: true
            Accessible.name: txtBias.text

            onValueChanged: Convolve.bias = value
        }
        SpinBox {
            id: spbBias
            value: Convolve.bias
            stepSize: sldBias.stepSize
            from: sldBias.from
            to: sldBias.to
            editable: true
            Accessible.name: txtBias.text

            onValueChanged: Convolve.bias = Number(value)
        }
    }
}

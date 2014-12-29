/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import QbQml 1.0

ColumnLayout {
    id: configs
    property int cellSize: 50

    function updateKernel(index, value)
    {
        var kernel = Convolve.kernel
        kernel[index] = value
        Convolve.kernel = kernel
    }

    function strToFloat(str)
    {
        return str.length < 1? 0: parseFloat(str)
    }

    Label {
        text: qsTr("Convolve matrix")
    }
    GridLayout {
        columns: 3

        // Row 0
        TextField {
            id: k00
            Layout.preferredWidth: cellSize
            text: Convolve.kernel[0]
            validator: RegExpValidator {
                regExp: /-?\d+/
            }

            onTextChanged: updateKernel(0, strToFloat(text))
        }
        TextField {
            id: k01
            Layout.preferredWidth: cellSize
            text: Convolve.kernel[1]
            validator: RegExpValidator {
                regExp: /-?\d+/
            }

            onTextChanged: updateKernel(1, strToFloat(text))
        }
        TextField {
            id: k02
            Layout.preferredWidth: cellSize
            text: Convolve.kernel[2]
            validator: RegExpValidator {
                regExp: /-?\d+/
            }

            onTextChanged: updateKernel(2, strToFloat(text))
        }

        // Row 1
        TextField {
            id: k10
            Layout.preferredWidth: cellSize
            text: Convolve.kernel[3]
            validator: RegExpValidator {
                regExp: /-?\d+/
            }

            onTextChanged: updateKernel(3, strToFloat(text))
        }
        TextField {
            id: k11
            Layout.preferredWidth: cellSize
            text: Convolve.kernel[4]
            validator: RegExpValidator {
                regExp: /-?\d+/
            }

            onTextChanged: updateKernel(4, strToFloat(text))
        }
        TextField {
            id: k12
            Layout.preferredWidth: cellSize
            text: Convolve.kernel[5]
            validator: RegExpValidator {
                regExp: /-?\d+/
            }

            onTextChanged: updateKernel(5, strToFloat(text))
        }

        // Row 2
        TextField {
            id: k20
            Layout.preferredWidth: cellSize
            text: Convolve.kernel[6]
            validator: RegExpValidator {
                regExp: /-?\d+/
            }

            onTextChanged: updateKernel(6, strToFloat(text))
        }
        TextField {
            id: k21
            Layout.preferredWidth: cellSize
            text: Convolve.kernel[7]
            validator: RegExpValidator {
                regExp: /-?\d+/
            }

            onTextChanged: updateKernel(7, strToFloat(text))
        }
        TextField {
            id: k22
            Layout.preferredWidth: cellSize
            text: Convolve.kernel[8]
            validator: RegExpValidator {
                regExp: /-?\d+/
            }

            onTextChanged: updateKernel(8, strToFloat(text))
        }
    }

    GridLayout {
        columns: 2

        Label {
            text: qsTr("Factor")
        }
        TextField {
            text: Qb.newFrac(Convolve.factor).string
            validator: RegExpValidator {
                regExp: /-?\d+\/\d+/
            }

            onTextChanged: Convolve.factor = Qb.toVar(Qb.newFrac(text))
        }

        Label {
            text: qsTr("Bias")
        }
        SpinBox {
            stepSize: 1
            maximumValue: 255
            minimumValue: -255
            value: Convolve.bias

            onValueChanged: Convolve.bias = value
        }
    }
}

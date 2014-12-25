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

ColumnLayout {
    id: configs
    property double stepSize: 0.01
    property double maxMultiplier: 10
    property double maxDisplacement: 255
    property double decimals: 2

    function updateKernel(index, value)
    {
        var kernel = Convolve.kernel
        kernel[index] = value
        Convolve.kernel = kernel
    }

    Component.onCompleted: {
    }

    Label {
        text: qsTr("Convolve matrix")
    }
    GridLayout {
        columns: 3

        // Row 0
        SpinBox {
            id: k00
            stepSize: configs.stepSize
            maximumValue: configs.maxMultiplier
            minimumValue: -configs.maxMultiplier
            decimals: configs.decimals
            value: Convolve.kernel[0]

            onValueChanged: updateKernel(0, value)
        }
        SpinBox {
            id: k01
            stepSize: configs.stepSize
            maximumValue: configs.maxMultiplier
            minimumValue: -configs.maxMultiplier
            decimals: configs.decimals
            value: Convolve.kernel[1]

            onValueChanged: updateKernel(1, value)
        }
        SpinBox {
            id: k02
            stepSize: configs.stepSize
            maximumValue: configs.maxMultiplier
            minimumValue: -configs.maxMultiplier
            decimals: configs.decimals
            value: Convolve.kernel[2]

            onValueChanged: updateKernel(2, value)
        }

        // Row 1
        SpinBox {
            id: k10
            stepSize: configs.stepSize
            maximumValue: configs.maxMultiplier
            minimumValue: -configs.maxMultiplier
            decimals: configs.decimals
            value: Convolve.kernel[3]

            onValueChanged: updateKernel(3, value)
        }
        SpinBox {
            id: k11
            stepSize: configs.stepSize
            maximumValue: configs.maxMultiplier
            minimumValue: -configs.maxMultiplier
            decimals: configs.decimals
            value: Convolve.kernel[4]

            onValueChanged: updateKernel(4, value)
        }
        SpinBox {
            id: k12
            stepSize: configs.stepSize
            maximumValue: configs.maxMultiplier
            minimumValue: -configs.maxMultiplier
            decimals: configs.decimals
            value: Convolve.kernel[5]

            onValueChanged: updateKernel(5, value)
        }

        // Row 2
        SpinBox {
            id: k20
            stepSize: configs.stepSize
            maximumValue: configs.maxMultiplier
            minimumValue: -configs.maxMultiplier
            decimals: configs.decimals
            value: Convolve.kernel[6]

            onValueChanged: updateKernel(6, value)
        }
        SpinBox {
            id: k21
            stepSize: configs.stepSize
            maximumValue: configs.maxMultiplier
            minimumValue: -configs.maxMultiplier
            decimals: configs.decimals
            value: Convolve.kernel[7]

            onValueChanged: updateKernel(7, value)
        }
        SpinBox {
            id: k22
            stepSize: configs.stepSize
            maximumValue: configs.maxMultiplier
            minimumValue: -configs.maxMultiplier
            decimals: configs.decimals
            value: Convolve.kernel[8]

            onValueChanged: updateKernel(8, value)
        }
    }

    GridLayout {
        columns: 2

        Label {
            text: qsTr("Factor")
        }
        TextField {
            text: Qb.copy(Convolve.factor).string
            validator: RegExpValidator {
                regExp: /-?\d+\/\d+/
            }
        }

        Label {
            text: qsTr("Bias")
        }
        SpinBox {
            stepSize: 1
            maximumValue: configs.maxDisplacement
            minimumValue: -configs.maxDisplacement
            value: Convolve.bias

            onValueChanged: Convolve.bias = value
        }
    }
}

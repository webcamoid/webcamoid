/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1

ColumnLayout {
    property int cellSize: 50

    function updateKernel(index, value)
    {
        var kernel = MatrixTransform.kernel
        kernel[index] = value
        MatrixTransform.kernel = kernel
    }

    function strToFloat(str)
    {
        return str.length < 1? 0: parseFloat(str)
    }

    Label {
        text: qsTr("Transform matrix")
    }
    GridLayout {
        columns: 3

        // X axis
        TextField {
            id: xx
            Layout.preferredWidth: cellSize
            text: MatrixTransform.kernel[0]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(0, strToFloat(text))
        }
        TextField {
            id: xy
            Layout.preferredWidth: cellSize
            text: MatrixTransform.kernel[1]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(1, strToFloat(text))
        }
        TextField {
            id: x0
            Layout.preferredWidth: cellSize
            text: MatrixTransform.kernel[2]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(2, strToFloat(text))
        }

        // Y axis
        TextField {
            id: yx
            Layout.preferredWidth: cellSize
            text: MatrixTransform.kernel[3]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(3, strToFloat(text))
        }
        TextField {
            id: yy
            Layout.preferredWidth: cellSize
            text: MatrixTransform.kernel[4]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(4, strToFloat(text))
        }
        TextField {
            id: y0
            Layout.preferredWidth: cellSize
            text: MatrixTransform.kernel[5]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(5, strToFloat(text))
        }
    }
}

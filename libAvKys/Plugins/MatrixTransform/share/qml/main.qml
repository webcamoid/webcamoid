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

ColumnLayout {
    property int cellSize: 50

    function updateKernel(index, value)
    {
        var kernel = MatrixTransform.kernel
        kernel[index] = value
        MatrixTransform.kernel = kernel
    }

    Label {
        //: https://en.wikipedia.org/wiki/Transformation_matrix
        text: qsTr("Transform matrix")
    }
    GridLayout {
        columns: 3

        // X axis
        TextField {
            id: xx
            Layout.preferredWidth: cellSize
            text: MatrixTransform.kernel[0]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Accessible.name: qsTr("Column 0, Row 0")

            onTextChanged: updateKernel(0, text)
        }
        TextField {
            id: xy
            Layout.preferredWidth: cellSize
            text: MatrixTransform.kernel[1]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Accessible.name: qsTr("Column 1, Row 0")

            onTextChanged: updateKernel(1, text)
        }
        TextField {
            id: x0
            Layout.preferredWidth: cellSize
            text: MatrixTransform.kernel[2]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Accessible.name: qsTr("Column 2, Row 0")

            onTextChanged: updateKernel(2, text)
        }

        // Y axis
        TextField {
            id: yx
            Layout.preferredWidth: cellSize
            text: MatrixTransform.kernel[3]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Accessible.name: qsTr("Column 0, Row 1")

            onTextChanged: updateKernel(3, text)
        }
        TextField {
            id: yy
            Layout.preferredWidth: cellSize
            text: MatrixTransform.kernel[4]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Accessible.name: qsTr("Column 1, Row 1")

            onTextChanged: updateKernel(4, text)
        }
        TextField {
            id: y0
            Layout.preferredWidth: cellSize
            text: MatrixTransform.kernel[5]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Accessible.name: qsTr("Column 2, Row 1")

            onTextChanged: updateKernel(5, text)
        }
    }
}

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
    id: configs
    property int cellSize: 50

    function updateKernel(index, value)
    {
        var kernel = ChangeHSL.kernel
        kernel[index] = value
        ChangeHSL.kernel = kernel
    }

    Label {
        id: txtTransformMatrix
        //: https://en.wikipedia.org/wiki/Transformation_matrix
        text: qsTr("Transform matrix")
    }
    GridLayout {
        columns: 4

        // Red channel
        TextField {
            id: hh
            text: ChangeHSL.kernel[0]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 0, Row 0")

            onTextChanged: updateKernel(0, text)
        }
        TextField {
            id: hs
            text: ChangeHSL.kernel[1]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 1, Row 0")

            onTextChanged: updateKernel(1, text)
        }
        TextField {
            id: hl
            text: ChangeHSL.kernel[2]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 2, Row 0")

            onTextChanged: updateKernel(2, text)
        }
        TextField {
            id: h0
            text: ChangeHSL.kernel[3]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 3, Row 0")

            onTextChanged: updateKernel(3, text)
        }

        // Green channel
        TextField {
            id: sh
            text: ChangeHSL.kernel[4]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 0, Row 1")

            onTextChanged: updateKernel(4, text)
        }
        TextField {
            id: ss
            text: ChangeHSL.kernel[5]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 1, Row 1")

            onTextChanged: updateKernel(5, text)
        }
        TextField {
            id: sl
            text: ChangeHSL.kernel[6]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 2, Row 1")

            onTextChanged: updateKernel(6, text)
        }
        TextField {
            id: s0
            text: ChangeHSL.kernel[7]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 3, Row 1")

            onTextChanged: updateKernel(7, text)
        }

        // Blue channel
        TextField {
            id: lh
            text: ChangeHSL.kernel[8]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 0, Row 2")

            onTextChanged: updateKernel(8, text)
        }
        TextField {
            id: ls
            text: ChangeHSL.kernel[9]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 1, Row 2")

            onTextChanged: updateKernel(9, text)
        }
        TextField {
            id: ll
            text: ChangeHSL.kernel[10]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 2, Row 2")

            onTextChanged: updateKernel(10, text)
        }
        TextField {
            id: l0
            text: ChangeHSL.kernel[11]
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }
            Layout.preferredWidth: cellSize
            Accessible.name: qsTr("Column 3, Row 2")

            onTextChanged: updateKernel(11, text)
        }
    }
}

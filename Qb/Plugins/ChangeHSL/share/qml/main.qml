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

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

ColumnLayout {
    id: configs
    property int cellSize: 50

    function updateKernel(index, value)
    {
        var kernel = ChangeHSL.kernel
        kernel[index] = value
        ChangeHSL.kernel = kernel
    }

    function strToFloat(str)
    {
        return str.length < 1? 0: parseFloat(str)
    }

    Label {
        text: qsTr("Transform matrix")
    }
    GridLayout {
        columns: 4

        // Red channel
        TextField {
            id: hh
            Layout.preferredWidth: cellSize
            text: ChangeHSL.kernel[0]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(0, strToFloat(text))
        }
        TextField {
            id: hs
            Layout.preferredWidth: cellSize
            text: ChangeHSL.kernel[1]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(1, strToFloat(text))
        }
        TextField {
            id: hl
            Layout.preferredWidth: cellSize
            text: ChangeHSL.kernel[2]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(2, strToFloat(text))
        }
        TextField {
            id: h0
            Layout.preferredWidth: cellSize
            text: ChangeHSL.kernel[3]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(3, strToFloat(text))
        }

        // Green channel
        TextField {
            id: sh
            Layout.preferredWidth: cellSize
            text: ChangeHSL.kernel[4]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(4, strToFloat(text))
        }
        TextField {
            id: ss
            Layout.preferredWidth: cellSize
            text: ChangeHSL.kernel[5]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(5, strToFloat(text))
        }
        TextField {
            id: sl
            Layout.preferredWidth: cellSize
            text: ChangeHSL.kernel[6]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(6, strToFloat(text))
        }
        TextField {
            id: s0
            Layout.preferredWidth: cellSize
            text: ChangeHSL.kernel[7]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(7, strToFloat(text))
        }

        // Blue channel
        TextField {
            id: lh
            Layout.preferredWidth: cellSize
            text: ChangeHSL.kernel[8]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(8, strToFloat(text))
        }
        TextField {
            id: ls
            Layout.preferredWidth: cellSize
            text: ChangeHSL.kernel[9]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(9, strToFloat(text))
        }
        TextField {
            id: ll
            Layout.preferredWidth: cellSize
            text: ChangeHSL.kernel[10]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(10, strToFloat(text))
        }
        TextField {
            id: l0
            Layout.preferredWidth: cellSize
            text: ChangeHSL.kernel[11]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(11, strToFloat(text))
        }
    }
}

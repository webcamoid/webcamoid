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
    property int cellSize: 50

    function updateKernel(index, value)
    {
        var kernel = ColorTransform.kernel
        kernel[index] = value
        ColorTransform.kernel = kernel
    }

    Label {
        text: qsTr("Transform matrix")
    }
    GridLayout {
        columns: 4

        // Red channel
        TextField {
            id: rr
            Layout.preferredWidth: cellSize
            text: ColorTransform.kernel[0]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(0, parseFloat(text))
        }
        TextField {
            id: rg
            Layout.preferredWidth: cellSize
            text: ColorTransform.kernel[1]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(1, parseFloat(text))
        }
        TextField {
            id: rb
            Layout.preferredWidth: cellSize
            text: ColorTransform.kernel[2]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(2, parseFloat(text))
        }
        TextField {
            id: r0
            Layout.preferredWidth: cellSize
            text: ColorTransform.kernel[3]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(3, parseFloat(text))
        }

        // Green channel
        TextField {
            id: gr
            Layout.preferredWidth: cellSize
            text: ColorTransform.kernel[4]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(4, parseFloat(text))
        }
        TextField {
            id: gg
            Layout.preferredWidth: cellSize
            text: ColorTransform.kernel[5]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(5, parseFloat(text))
        }
        TextField {
            id: gb
            Layout.preferredWidth: cellSize
            text: ColorTransform.kernel[6]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(6, parseFloat(text))
        }
        TextField {
            id: g0
            Layout.preferredWidth: cellSize
            text: ColorTransform.kernel[7]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(7, parseFloat(text))
        }

        // Blue channel
        TextField {
            id: br
            Layout.preferredWidth: cellSize
            text: ColorTransform.kernel[8]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(8, parseFloat(text))
        }
        TextField {
            id: bg
            Layout.preferredWidth: cellSize
            text: ColorTransform.kernel[9]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(9, parseFloat(text))
        }
        TextField {
            id: bb
            Layout.preferredWidth: cellSize
            text: ColorTransform.kernel[10]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(10, parseFloat(text))
        }
        TextField {
            id: b0
            Layout.preferredWidth: cellSize
            text: ColorTransform.kernel[11]
            validator: RegExpValidator {
                regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
            }

            onTextChanged: updateKernel(11, parseFloat(text))
        }
    }
}

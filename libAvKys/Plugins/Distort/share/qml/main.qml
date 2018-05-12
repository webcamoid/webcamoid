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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

GridLayout {
    columns: 2

    // Configure amplitude.
    Label {
        text: qsTr("Amplitude")
    }
    TextField {
        text: Distort.amplitude
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.fillWidth: true

        onTextChanged: Distort.amplitude = text
    }

    // Configure frequency.
    Label {
        text: qsTr("Frequency")
    }
    TextField {
        text: Distort.frequency
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.fillWidth: true

        onTextChanged: Distort.frequency = text
    }

    // Configure grid size.
    Label {
        text: qsTr("Grid size")
    }
    TextField {
        text: Distort.gridSizeLog
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true

        onTextChanged: Distort.gridSizeLog = text
    }
}

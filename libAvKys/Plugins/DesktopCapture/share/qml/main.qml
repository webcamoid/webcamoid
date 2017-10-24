/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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
import AkQml 1.0

GridLayout {
    columns: 2

    Component.onCompleted: {
        var fps = Ak.newFrac(DesktopCapture.fps).value;
        var q = Infinity;
        var index = -1;

        for (var i in cbxFps.model) {
            var diff = Math.abs(cbxFps.model[i] - fps);

            if (diff < q) {
                index = i;
                q = diff;
            }
        }

        cbxFps.currentIndex = index;
        DesktopCapture.fps = Ak.varFrac(cbxFps.model[index], 1);
    }

    Label {
        id: lblFps
        text: qsTr("Frame rate")
    }
    ComboBox {
        id: cbxFps
        Layout.fillWidth: true
        model: [
            300,
            240,
            144,
            120,
            100,
            90,
            72,
            60,
            50,
            48,
            30,
            25,
            24,
            20,
            15,
            10,
            5,
            2,
            1]

        onCurrentIndexChanged: {
            if (currentIndex > -1)
                DesktopCapture.fps = Ak.varFrac(model[currentIndex], 1);
        }
    }
}

/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

GridLayout {
    columns: 2

    Component.onCompleted: {
        var fps = AkFrac.create(ImageSrc.fps).value;
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
        ImageSrc.fps = AkFrac.createVariant(cbxFps.model[index], 1);
    }

    Label {
        id: txtForceFrameRate
        text: qsTr("Force frame rate")
        visible: ImageSrc.isAnimated
    }
    RowLayout {
        visible: ImageSrc.isAnimated

        Label {
            Layout.fillWidth: true
        }
        Switch {
            checked: ImageSrc.forceFps
            Accessible.name: txtForceFrameRate.text

            onCheckedChanged: ImageSrc.forceFps = checked
        }
    }
    Label {
        id: lblFps
        text: qsTr("Frame rate")
        enabled: !ImageSrc.isAnimated
                 || (ImageSrc.isAnimated && ImageSrc.forceFps)
    }
    ComboBox {
        id: cbxFps
        Accessible.description: lblFps.text
        currentIndex: 10
        Layout.fillWidth: true
        enabled: !ImageSrc.isAnimated
                 || (ImageSrc.isAnimated && ImageSrc.forceFps)
        model: [300,
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
                ImageSrc.fps = AkFrac.createVariant(model[currentIndex], 1);
        }
    }
}

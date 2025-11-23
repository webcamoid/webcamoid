/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ak
import AkControls as AK

ColumnLayout {
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

    Switch {
        text: qsTr("Force frame rate")
        visible: ImageSrc.isAnimated
        checked: ImageSrc.forceFps
        Accessible.name: txtForceFrameRate.text
        Layout.fillWidth: true

        onCheckedChanged: ImageSrc.forceFps = checked
    }
    AK.LabeledComboBox {
        id: cbxFps
        label: qsTr("Frame rate")
        currentIndex: 10
        Accessible.description: lblFps.text
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

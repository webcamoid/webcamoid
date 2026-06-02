/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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
import Webcamoid

ScrollView {
    id: view

    property string videoOutput: ""
    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    signal videoOutputRemoved()

    function setOutput(videoOutput)
    {
        view.videoOutput = videoOutput
        let url = streaming.platformWebsite(videoOutput)
        deviceInfo.text =
                "<b>" + videoOutput + "</b><br/>"
                + "<i>" + url + "</i>"
    }

    ColumnLayout {
        width: view.width
        spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight

        Label {
            id: deviceInfo
            elide: Label.ElideRight
            Layout.fillWidth: true
            Layout.leftMargin:
                AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.rightMargin:
                AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.bottomMargin:
                AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        }
        Button {
            text: qsTr("Remove")
            icon.source: "image://icons/no"
            flat: true
            Layout.leftMargin:
                AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.rightMargin:
                AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

            onClicked: {
                streaming.platforms =
                    streaming.platforms.filter(p => p !== view.videoOutput)
                view.videoOutputRemoved()
            }
        }
    }
}

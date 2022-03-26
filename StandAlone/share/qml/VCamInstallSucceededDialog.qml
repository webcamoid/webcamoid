/* Webcamoid, webcam capture application.
 * Copyright (C) 2021  Gonzalo Exequiel Pedone
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
import Webcamoid 1.0

Dialog {
    standardButtons: Dialog.Yes | Dialog.No
    width: AkUnit.create(320 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(200 * AkTheme.controlScale, "dp").pixels
    modal: true
    title: qsTr("Installation succeeded")

    onVisibleChanged: forceActiveFocus()

    ScrollView {
        id: view
        anchors.fill: parent

        Label {
            text: qsTr("Restart Webcamoid?")
            Layout.fillWidth: true
        }
    }

    onAccepted: mediaTools.restartApp()
}

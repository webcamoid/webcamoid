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
import Webcamoid 1.0

Dialog {
    id: aboutDialog
    standardButtons: Dialog.Close
    width: AkUnit.create(640 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(400 * AkTheme.controlScale, "dp").pixels
    modal: true
    title: qsTr("About %1").arg(mediaTools.applicationName)

    ColumnLayout {
        anchors.fill: parent

        TabBar {
            id: tabBar
            Layout.fillWidth: true

            TabButton {
                /*: Information of the program, like name, description, vesion,
                    etc..
                 */
                text: qsTr("About")
            }
            TabButton {
                /*: List of people contributing to the project: software
                    developers, translators, designers, etc..
                 */
                text: qsTr("Contributors")
            }
            TabButton {
                //: Program license.
                text: qsTr("License")
            }
            TabButton {
                /*: License for 3rd party components used in Webcamoid, like
                    libraries and code snippets.
                 */
                text: qsTr("3rd Party Licenses")
            }
        }
        StackLayout {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex
            clip: true

            About { }
            Contributors { }
            License { }
            ThirdPartyLicenses { }
        }
    }
}

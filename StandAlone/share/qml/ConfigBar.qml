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
    id: recConfigBar

    property string option: ""

    OptionList {
        id: optConfigs
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: parent.top
        textRole: "description"
        model: ListModel {
            id: lstConfigs

            ListElement {
                option: "output"
                /*: Configuration for video output, like virtual camera or
                    streaming.
                 */
                description: qsTr("Output")
            }
            ListElement {
                option: "general"
                description: qsTr("General Options")
            }
            ListElement {
                option: "plugins"
                description: qsTr("Plugins Settings")
            }
            ListElement {
                option: "updates"
                description: qsTr("Updates")
            }
            ListElement {
                option: "about"
                /*: Information of the program, like name, description, vesion,
                    etc..
                 */
                description: qsTr("About")
            }
            ListElement {
                option: "contributors"
                /*: List of people contributing to the project: software
                    developers, translators, designers, etc..
                 */
                description: qsTr("Contributors")
            }
            ListElement {
                option: "license"
                //: Program license.
                description: qsTr("License")
            }
            ListElement {
                option: "3rdpartylicenses"
                /*: License for 3rd party components used in Webcamoid, like
                    libraries and code snippets.
                 */
                description: qsTr("3rd Party Licenses")
            }
        }

        onCurrentIndexChanged: recConfigBar.option = lstConfigs.get(optConfigs.currentIndex).option
    }
}

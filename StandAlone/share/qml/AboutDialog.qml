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
import QtQuick.Window 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Ak 1.0
import Webcamoid 1.0

Dialog {
    id: aboutDialog
    standardButtons: Dialog.Close
    width: physicalWidth <= 100 || physicalHeight <= 100?
               wdgMainWidget.width: wdgMainWidget.width * 0.75
    height: physicalWidth <= 100 || physicalHeight <= 100?
                wdgMainWidget.height: wdgMainWidget.height * 0.75
    modal: true
    title: qsTr("About %1").arg(mediaTools.applicationName)
    leftPadding: 0

    property real physicalWidth: wdgMainWidget.width / Screen.pixelDensity
    property real physicalHeight: wdgMainWidget.height / Screen.pixelDensity
    property int panelBorder: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
    property int dragBorder: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    property int minimumWidth: AkUnit.create(100 * AkTheme.controlScale, "dp").pixels
    property int maximumWidth:
        width - AkUnit.create(64 * AkTheme.controlScale, "dp").pixels

    onWidthChanged: {
        if (aboutDialog.visible)
            optionsItem.implicitWidth =
                    Math.min(Math.max(aboutDialog.minimumWidth,
                                      optionsItem.implicitWidth),
                             aboutDialog.maximumWidth)
    }
    onVisibleChanged: options.forceActiveFocus()

    RowLayout {
        anchors.fill: parent

        Item {
            id: optionsItem
            Layout.fillHeight: true
            implicitWidth:
                AkUnit.create(150 * AkTheme.controlScale, "dp").pixels

            ScrollView {
                id: optionsView
                anchors.fill: parent
                contentHeight: options.height
                clip: true

                OptionList {
                    id: options
                    width: optionsView.width

                    ItemDelegate {
                        /*: Information of the program, like name, description, version,
                            etc..
                         */
                        text: qsTr("About")
                    }
                    ItemDelegate {
                        /*: List of people contributing to the project: software
                            developers, translators, designers, etc..
                         */
                        text: qsTr("Contributors")
                    }
                    ItemDelegate {
                        //: Program license.
                        text: qsTr("License")
                    }
                    ItemDelegate {
                        /*: License for 3rd party components used in Webcamoid, like
                            libraries and code snippets.
                         */
                        text: qsTr("3rd Party Licenses")
                    }
                }
            }
            Rectangle {
                id: rectangleRight
                width: aboutDialog.panelBorder
                color: aboutDialog.activeDark
                anchors.leftMargin: -width / 2
                anchors.left: optionsView.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
            }
            MouseArea {
                cursorShape: Qt.SizeHorCursor
                drag.axis: Drag.XAxis
                width: aboutDialog.panelBorder + 2 * aboutDialog.dragBorder
                anchors.leftMargin: -width / 2
                anchors.left: optionsView.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                onPositionChanged: {
                    optionsItem.implicitWidth =
                            Math.min(Math.max(aboutDialog.minimumWidth,
                                              optionsItem.implicitWidth
                                              + mouse.x),
                                     aboutDialog.maximumWidth)
                }
            }
        }
        StackLayout {
            id: stack
            currentIndex: options.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true
            //clip: true

            About { }
            Contributors { }
            License { }
            ThirdPartyLicenses { }
        }
    }

    header: Item {
        id: rectangle
        clip: true
        visible: aboutDialog.title
        height: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels

        Label {
            text: aboutDialog.title
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin:
                AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
            elide: Label.ElideRight
            font.bold: true
            font.pointSize: 16
            enabled: aboutDialog.enabled
        }

        Rectangle {
            color: aboutDialog.enabled?
                       aboutDialog.activeDark:
                       aboutDialog.disabledDark
            height: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
            anchors.left: rectangle.left
            anchors.right: rectangle.right
            anchors.bottom: rectangle.bottom
        }
    }
}

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
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
import "qrc:/Ak/share/qml/AkQmlControls"

ColumnLayout {
    function toQrc(uri)
    {
        if (uri.indexOf(":") === 0)
            return "qrc" + uri

        return "file:" + uri
    }

    RowLayout {
        Label {
            text: qsTr("Color table")
        }
        ComboBox {
            id: cbxTable
            textRole: "text"
            Layout.fillWidth: true

            model: ListModel {
                ListElement {
                    text: qsTr("Base")
                    table: ":/ColorTap/share/tables/base.bmp"
                }
                ListElement {
                    text: qsTr("Esses")
                    table: ":/ColorTap/share/tables/esses.bmp"
                }
                ListElement {
                    text: qsTr("Heat")
                    table: ":/ColorTap/share/tables/heat.bmp"
                }
                ListElement {
                    text: qsTr("Old Photo")
                    table: ":/ColorTap/share/tables/oldphoto.bmp"
                }
                ListElement {
                    text: qsTr("Red & Green")
                    table: ":/ColorTap/share/tables/redgreen.bmp"
                }
                ListElement {
                    text: qsTr("Sepia")
                    table: ":/ColorTap/share/tables/sepia.bmp"
                }
                ListElement {
                    text: qsTr("X-Pro")
                    table: ":/ColorTap/share/tables/xpro.bmp"
                }
                ListElement {
                    text: qsTr("X-Ray")
                    table: ":/ColorTap/share/tables/xray.bmp"
                }
                ListElement {
                    text: qsTr("Yellow & Blue")
                    table: ":/ColorTap/share/tables/yellowblue.bmp"
                }
                ListElement {
                    text: qsTr("Custom")
                    table: ""
                }
            }

            onCurrentIndexChanged: ColorTap.table = cbxTable.model.get(currentIndex).table
        }
    }

    RowLayout {
        Image {
            width: 16
            height: 16
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 16
            sourceSize.height: 16
            source: toQrc(txtTable.text)
        }
        TextField {
            id: txtTable
            text: ColorTap.table
            placeholderText: qsTr("16x16 bitmap...")
            Layout.fillWidth: true

            onTextChanged: {
                for (var i = 0; i < cbxTable.model.count; i++) {
                    if (cbxTable.model.get(i).table === ColorTap.table) {
                        cbxTable.currentIndex = i

                        break
                    } else if (i == cbxTable.model.count - 1) {
                        cbxTable.model.get(i).table = ColorTap.table
                        cbxTable.currentIndex = i

                        break
                    }
                }
            }
        }
        AkButton {
            label: qsTr("Search")
            iconRc: "image://icons/edit-find"

            onClicked: fileDialog.open()
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: "file://" + picturesPath

        onAccepted: ColorTap.table = String(fileUrl).replace("file://", "")
    }
}

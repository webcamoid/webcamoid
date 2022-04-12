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
import Qt.labs.platform 1.1 as LABS

ColumnLayout {
    function toQrc(uri)
    {
        if (uri.indexOf(":") == 0)
            return "qrc" + uri

        return "file:" + uri
    }

    RowLayout {
        Label {
            id: txtColorTable
            text: qsTr("Color table")
        }
        ComboBox {
            id: cbxTable
            textRole: "text"
            Layout.fillWidth: true
            Accessible.description: txtColorTable.text

            model: ListModel {
                ListElement {
                    //: Base color, show the image without modifications
                    text: qsTr("Base")
                    table: ":/ColorTap/share/tables/base.bmp"
                }
                ListElement {
                    text: qsTr("Metal")
                    table: ":/ColorTap/share/tables/metal.bmp"
                }
                ListElement {
                    //: https://en.wikipedia.org/wiki/Heat_map
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
                    //: https://en.wikipedia.org/wiki/Sepia_(color)
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
            placeholderText: qsTr("Source palette")
            selectByMouse: true
            Layout.fillWidth: true
            Accessible.name: qsTr("Image file to use as palette")

            onTextChanged: {
                for (var i = 0; i < cbxTable.model.count; i++) {
                    if (cbxTable.model.get(i).table == ColorTap.table) {
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
        Button {
            text: qsTr("Search")
            icon.source: "image://icons/search"
            Accessible.description: qsTr("Search the image file to use as palette")

            onClicked: fileDialog.open()
        }
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: "file://" + picturesPath

        onAccepted: ColorTap.table = String(file).replace("file://", "")
    }
}

/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as LABS
import Ak
import AkControls as AK

ColumnLayout {
    id: clyColorTap

    readonly property string filePrefix: Ak.platform() == "windows"?
                                             "file:///":
                                             "file://"

    function toQrc(uri)
    {
        if (uri.indexOf(":") == 0)
            return "qrc" + uri

        return "file:" + uri
    }

    AK.LabeledComboBox {
        id: cbxTable
        label: qsTr("Color table")
        textRole: "text"
        Layout.fillWidth: true
        Accessible.description: label

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

    RowLayout {
        Image {
            width: 16
            height: 16
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 16
            sourceSize.height: 16
            source: toQrc(txtTable.labelText)
        }
        AK.ActionTextField {
            id: txtTable
            icon.source: "image://icons/search"
            labelText: ColorTap.table
            placeholderText:qsTr("Source palette")
            buttonText: qsTr("Search the image file to use as palette")
            Layout.fillWidth: true

            onLabelTextChanged: {
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
            onButtonClicked: fileDialog.open()
        }
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: clyColorTap.filePrefix + picturesPath

        onAccepted: ColorTap.table =
                    String(file).replace(clyColorTap.filePrefix, "")
    }
}

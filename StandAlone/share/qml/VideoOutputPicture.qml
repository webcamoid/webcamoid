/* Webcamoid, camera capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import Ak
import AkControls as AK
import Qt.labs.platform as LABS

Dialog {
    id: outputPictureDialog
    title: qsTr("Virtual camera output picture")
    standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Reset
    width: physicalWidth <= 100 || physicalHeight <= 100?
               wdgMainWidget.width: wdgMainWidget.width * 0.75
    height: physicalWidth <= 100 || physicalHeight <= 100?
                wdgMainWidget.height: wdgMainWidget.height * 0.75
    modal: true

    signal openErrorDialog(string title, string message)

    property real physicalWidth: wdgMainWidget.width / Screen.pixelDensity
    property real physicalHeight: wdgMainWidget.height / Screen.pixelDensity
    readonly property string filePrefix: Ak.platform() == "windows"?
                                             "file:///":
                                             "file://"

    function toQrc(uri)
    {
        if (uri.indexOf(":") == 0)
            return "qrc" + uri

        return "file:" + uri
    }

    onVisibleChanged: {
        if (visible)
            txtTable.labelText = videoLayer.picture

        txtTable.forceActiveFocus()
    }

    ScrollView {
        id: pictureView
        anchors.fill: parent
        contentHeight: pictureControls.height
        clip: true

        ColumnLayout {
            id: pictureControls
            width: pictureView.width

            AK.ActionTextField {
                id: txtTable
                icon.source: "image://icons/search"
                labelText: videoLayer.picture
                placeholderText: qsTr("Virtual camera default output picture")
                buttonText: qsTr("Search image to use as default output picture")
                Layout.fillWidth: true

                onButtonClicked: fileDialog.open()
            }
            Image {
                width: 160
                height: 120
                fillMode: Image.PreserveAspectFit
                sourceSize.width: 160
                sourceSize.height: 120
                source: toQrc(txtTable.labelText)
                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            }
        }
    }

    onAccepted: {
        videoLayer.picture = txtTable.labelText

        if (videoLayer.clientsPids.length < 1) {
            if (!videoLayer.applyPicture()) {
                let title = qsTr("Can't set virtual camera picture")
                outputPictureDialog.openErrorDialog(title, videoLayer.outputError)
            }
        } else {
            let title = qsTr("Error Removing Virtual Cameras")
            let message = Commons.vcamDriverBusyMessage()
            outputPictureDialog.openErrorDialog(title, message)
        }
    }
    onReset: {
        videoLayer.picture = undefined
        txtTable.labelText = videoLayer.picture
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: outputPictureDialog.filePrefix + recording.imagesDirectory

        onAccepted: {
            txtTable.labelText =
                    String(file).replace(outputPictureDialog.filePrefix, "")
        }
    }
}

/* Webcamoid, webcam capture application.
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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Ak 1.0
import Qt.labs.platform 1.1 as LABS

Dialog {
    id: outputPictureDialog
    title: qsTr("Virtual camera output picture")
    standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Reset
    width: AkUnit.create(450 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(350 * AkTheme.controlScale, "dp").pixels
    modal: true

    signal openErrorDialog(string title, string message)

    function toQrc(uri)
    {
        if (uri.indexOf(":") == 0)
            return "qrc" + uri

        return "file:" + uri
    }

    onVisibleChanged: {
        if (visible)
            txtTable.text = videoLayer.picture

        btnSearch.forceActiveFocus()
    }

    ScrollView {
        id: pictureView
        anchors.fill: parent
        contentHeight: pictureControls.height
        clip: true

        GridLayout {
            id: pictureControls
            width: pictureView.width
            columns: 2

            TextField {
                id: txtTable
                text: videoLayer.picture
                placeholderText: qsTr("Virtual camera default output picture")
                selectByMouse: true
                Layout.fillWidth: true
            }
            Button {
                id: btnSearch
                text: qsTr("Search")
                Accessible.description: qsTr("Search image to use as default output picture")
                icon.source: "image://icons/search"

                onClicked: fileDialog.open()
            }
            Image {
                width: 160
                height: 120
                fillMode: Image.PreserveAspectFit
                sourceSize.width: 160
                sourceSize.height: 120
                source: toQrc(txtTable.text)
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            }
        }
    }

    onAccepted: {
        videoLayer.picture = txtTable.text

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
        txtTable.text = videoLayer.picture
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Please choose an image file")
        nameFilters: ["Image files (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm *.xbm *.xpm)"]
        folder: "file://" + recording.imagesDirectory

        onAccepted: txtTable.text = String(file).replace("file://", "")
    }
}

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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: videoOptions

    signal openErrorDialog(string title, string message)
    signal openVideoInputAddEditDialog(string videoInput)
    signal openVideoOutputAddEditDialog(string videoOutput)
    signal openVideoInputOptions(string videoInput)
    signal openVideoOutputOptions(string videoOutput)
    signal openVideoOutputPictureDialog()
    signal openVCamDownloadDialog()
    signal openVCamManualDownloadDialog()

    TabBar {
        id: tabBar
        Layout.fillWidth: true

        TabButton {
            text: qsTr("Sources")
        }
        TabButton {
            text: qsTr("Outputs")
            visible: videoLayer.isVCamSupported
            width: videoLayer.isVCamSupported? undefined: 0
        }
    }
    StackLayout {
        id: stack
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: tabBar.currentIndex
        clip: true

        VideoInputs {
            onOpenVideoInputAddEditDialog: videoInput =>
                videoOptions.openVideoInputAddEditDialog(videoInput)
            onOpenVideoInputOptions: videoInput =>
                videoOptions.openVideoInputOptions(videoInput)
        }
        VideoOutputs {
            onOpenErrorDialog: (title, message) =>
                videoOptions.openErrorDialog(title, message)
            onOpenVideoOutputAddEditDialog: videoOutput =>
                videoOptions.openVideoOutputAddEditDialog(videoOutput)
            onOpenVideoOutputOptions: videoOutput =>
                videoOptions.openVideoOutputOptions(videoOutput)
            onOpenVideoOutputPictureDialog: videoOptions.openVideoOutputPictureDialog()
            onOpenVCamDownloadDialog: videoOptions.openVCamDownloadDialog()
            onOpenVCamManualDownloadDialog: videoOptions.openVCamManualDownloadDialog()
        }
    }
}

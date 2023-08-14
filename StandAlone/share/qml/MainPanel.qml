/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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

OptionsPanel {
    id: panel
    title: layout.currentIndex < 1?
               qsTr("Audio"):
           layout.currentIndex < 2?
               qsTr("Video"):
           layout.currentIndex < 3?
               qsTr("Effects"):
           layout.currentIndex < 4?
               qsTr("Video Source Options"):
           layout.currentIndex < 5?
               qsTr("Video Output Options"):
               qsTr("%1 options").arg(effectOptions.effectDescription)
    edge: Qt.RightEdge

    signal openErrorDialog(string title, string message)
    signal openVideoEffectsDialog()

    function previousPage()
    {
        let item = layout.children[layout.currentIndex]

        if (item.closeOption)
            item.closeOption()
        else
            close()
    }

    function openAudioSettings()
    {
        let co = layout.currentIndex != 0
        layout.currentIndex = 0

        if (co)
            closeAndOpen()
        else
            open()
    }

    function openVideoSettings()
    {
        let co = layout.currentIndex != 1
        layout.currentIndex = 1

        if (co)
            closeAndOpen()
        else
            open()
    }

    function openVideoEffects()
    {
        let co = layout.currentIndex != 2
        layout.currentIndex = 2

        if (co)
            closeAndOpen()
        else
            open()
    }

    Connections {
        target: videoLayer

        function onVcamInstallFinished(exitCode, error)
        {
            if (exitCode == 0)
                vcamInstallSucceeded.open()
            else
                vcamInstallFailed.openWithError(error)
        }
    }

    Keys.onEscapePressed: previousPage()
    onActionClicked: previousPage()

    contents: StackLayout {
        id: layout
        clip: true

        AudioOptions {
        }
        VideoOptions {
            onOpenErrorDialog: (title, message) =>
                panel.openErrorDialog(title, message)
            onOpenVideoInputAddEditDialog: videoInput =>
                videoInputAddEdit.openOptions(videoInput)
            onOpenVideoOutputAddEditDialog: videoOutput =>
                videoOutputAddEdit.openOptions(videoOutput)
            onOpenVideoInputOptions: function (videoInput) {
                closeAndOpen()
                layout.currentIndex = 3
                videoInputOptions.setInput(videoInput)
            }
            onOpenVideoOutputOptions: function (videoOutput) {
                closeAndOpen()
                layout.currentIndex = 4
                videoOutputOptions.setOutput(videoOutput)
            }
            onOpenVideoOutputPictureDialog: videoOutputPicture.open()
            onOpenVCamDownloadDialog: vcamDownload.openDownloads()
            onOpenVCamManualDownloadDialog: vcamManualDownload.open()
        }
        VideoEffectsList {
            onOpenVideoEffectsDialog: panel.openVideoEffectsDialog()
            onOpenVideoEffectOptions: function (effectIndex) {
                closeAndOpen()
                layout.currentIndex = 5
                effectOptions.effectIndex = effectIndex
            }
        }
        VideoInputOptions {
            id: videoInputOptions

            function closeOption()
            {
                closeAndOpen()
                layout.currentIndex = 1
            }

            onOpenVideoInputAddEditDialog: videoInput =>
                videoInputAddEdit.openOptions(videoInput)
            onVideoInputRemoved: closeOption()
        }
        VideoOutputOptions {
            id: videoOutputOptions

            function closeOption()
            {
                closeAndOpen()
                layout.currentIndex = 1
            }

            onOpenErrorDialog: (title, message) =>
                panel.openErrorDialog(title, message)
            onOpenVideoOutputAddEditDialog: videoOutput =>
                videoOutputAddEdit.openOptions(videoOutput)
            onVideoOutputRemoved: closeOption()
        }
        VideoEffectOptions {
            id: effectOptions

            function closeOption()
            {
                closeAndOpen()
                layout.currentIndex = 2
            }

            onEffectRemoved: closeOption()
        }
    }

    VideoInputAddEdit {
        id: videoInputAddEdit
        anchors.centerIn: Overlay.overlay

        onEdited: videoInputOptions.closeOption()
    }
    VideoOutputAddEdit {
        id: videoOutputAddEdit
        anchors.centerIn: Overlay.overlay

        onOpenErrorDialog: (title, message) =>
            panel.openErrorDialog(title, message)
        onOpenOutputFormatDialog: (index, caps) =>
            addVideoFormat.openOptions(index, caps)
        onEdited: videoOutputOptions.closeOption()
    }
    AddVideoFormat {
        id:  addVideoFormat
        anchors.centerIn: Overlay.overlay

        onAddFormat: caps =>
            videoOutputAddEdit.addFormat(caps)
        onChangeFormat: (index, caps) =>
            videoOutputAddEdit.changeFormat(index, caps)
        onRemoveFormat: index =>
            videoOutputAddEdit.removeFormat(index)
    }
    VideoOutputPicture {
        id: videoOutputPicture
        anchors.centerIn: Overlay.overlay

        onOpenErrorDialog: (title, message) =>
            panel.openErrorDialog(title, message)
    }
    DownloadDialog {
        id: vcamDownload
        anchors.centerIn: Overlay.overlay

        onDownloadSucceeded: installerFile =>
            vcamDownloadSucceeded.openWithInstaller(installerFile)
        onDownloadFailed: error =>
            vcamDownloadFailed.openWithError(error)
    }
    VCamManualDownloadDialog {
        id: vcamManualDownload
        anchors.centerIn: Overlay.overlay
    }
    DownloadSucceededDialog {
        id: vcamDownloadSucceeded
        anchors.centerIn: Overlay.overlay
    }
    DownloadFailedDialog {
        id: vcamDownloadFailed
        anchors.centerIn: Overlay.overlay
    }
    VCamInstallSucceededDialog {
        id: vcamInstallSucceeded
        anchors.centerIn: Overlay.overlay
    }
    VCamInstallFailedDialog {
        id: vcamInstallFailed
        anchors.centerIn: Overlay.overlay
    }
}

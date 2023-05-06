/* Webcamoid, webcam capture application.
 * Copyright (C) 2021  Gonzalo Exequiel Pedone
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
    id: downloadDialog
    standardButtons: Dialog.Cancel
    width: physicalWidth <= 100 || physicalHeight <= 100?
               wdgMainWidget.width: wdgMainWidget.width * 0.75
    height: physicalWidth <= 100 || physicalHeight <= 100?
                wdgMainWidget.height: wdgMainWidget.height * 0.75
    modal: true
    title: qsTr("Downloading %1").arg(downloadTitle)
    closePolicy: Popup.NoAutoClose

    property real physicalWidth: wdgMainWidget.width / Screen.pixelDensity
    property real physicalHeight: wdgMainWidget.height / Screen.pixelDensity
    property string downloadTitle: ""
    property string downloadUrl: ""
    property string downloadFile: ""
    property int downloadSize: 0
    property int downloadedBytes: 0
    property int downloadStatus: DownloadManager.DownloadStatusFinished
    property int downloadTimeElapsed: 0
    property double downloadSpeed:
        downloadTimeElapsed > 0?
            1000 * downloadedBytes / downloadTimeElapsed:
            0
    property double timeRemaining:
        downloadTimeElapsed > 0?
            (downloadSize - downloadedBytes) / downloadSpeed:
            0
    property bool downloadRejected: false

    signal downloadSucceeded(string installerFile)
    signal downloadFailed(string error)

    onVisibleChanged: showNextTime.forceActiveFocus()

    function unit(value)
    {
        let units = ["", "K", "M", "G", "T", "P", "E", "Z", "Y"]

        for (let i in units) {
            let val = value / 1024

            if (val < 1) {
                if (units[i] == "")
                    return units[i]
                else
                    return units[i] + "i"
            }

            value = val
        }

        return ''
    }

    function unitValue(value)
    {
        for (;;) {
            let val = value / 1024

            if (val < 1)
                break

            value = val
        }

        return value.toFixed(1)
    }

    function readableTime(secs)
    {
        secs = Math.round(secs)
        let h = Math.floor(secs / 3600)
        let m = Math.floor((secs - 3600 * h) / 60)
        let s = secs - 3600 * h - 60 * m

        return h.toString().padStart(2, "0")
               + ":"
               + m.toString().padStart(2, "0")
               + ":"
               + s.toString().padStart(2, "0")
    }

    function openDownloads()
    {
        downloadRejected = false
        open()
    }

    Connections {
        target: downloadManager

        function onFinished(url)
        {
            close()

            if (downloadDialog.downloadRejected)
                return

            let status = downloadManager.downloadStatus(url)

            if (status == DownloadManager.DownloadStatusFailed)
                downloadFailed(downloadManager.downloadErrorString(url))
            else
                downloadSucceeded(downloadManager.downloadFile(url))
        }

        function onDownloadChanged(url)
        {
            downloadTitle = downloadManager.downloadTitle(url)
            downloadUrl = url
            downloadFile = downloadManager.downloadFile(url)
            downloadSize = downloadManager.downloadSize(url)
            downloadedBytes = downloadManager.downloadedBytes(url)
            downloadStatus = downloadManager.downloadStatus(url)
            downloadTimeElapsed = downloadManager.downloadTimeElapsed(url)
        }
    }

    ScrollView {
        id: view
        anchors.fill: parent

        ColumnLayout {
            width: view.width
            clip: true

            Label {
                text: qsTr("<b>From:</b> %1").arg(downloadUrl)
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("<b>To:</b> %1").arg(downloadFile)
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("<b>Size:</b> %1 %2B / %3 %4B")
                        .arg(unitValue(downloadedBytes))
                        .arg(unit(downloadedBytes))
                        .arg(unitValue(downloadSize))
                        .arg(unit(downloadSize))
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("<b>Speed:</b> %1 %2B/s")
                        .arg(unitValue(downloadSpeed))
                        .arg(unit(downloadSpeed))
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("<b>Time remaining:</b> %1")
                        .arg(readableTime(timeRemaining))
                Layout.fillWidth: true
            }
            ProgressBar {
                id: showNextTime
                Layout.fillWidth: true
                Layout.topMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                from: 0
                to: downloadSize
                value: downloadedBytes
            }
        }
    }

    onRejected: {
        downloadRejected = true
        downloadManager.cancel()
        close()
    }
}

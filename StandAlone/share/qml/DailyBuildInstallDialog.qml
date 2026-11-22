/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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
import Ak
import Webcamoid

// Asks for confirmation before closing Webcamoid to run the new build: the
// user may be streaming or recording.
Dialog {
    id: root
    title: usesInstaller?
               qsTr("Install the new version?"):
               qsTr("Run the new version?")
    standardButtons: Dialog.Yes | Dialog.No
    width: AkUnit.create(400 * AkTheme.controlScale, "dp").pixels
    width: AkUnit.create(300 * AkTheme.controlScale, "dp").pixels
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay

    readonly property bool usesInstaller:
        dailyBuilds.installKind == DailyBuilds.InstallRunsInstaller
        || dailyBuilds.installKind == DailyBuilds.InstallPackage
        || dailyBuilds.installKind == DailyBuilds.InstallBuildsFromSource

    contentItem: ScrollView {
        id: scrollView
        clip: true
        contentWidth: availableWidth

        Label {
            width: scrollView.availableWidth
            text: {
                switch (dailyBuilds.installKind) {
                case DailyBuilds.InstallPackage:
                    return qsTr("Android will ask you to confirm the installation, and Webcamoid will close when it starts. If you are streaming, recording or doing something important, finish it first.<br /><br />Do you want to install the new version now?")
                case DailyBuilds.InstallRunsInstaller:
                    return qsTr("Webcamoid will close and the installer will start. If you are streaming, recording or doing something important, finish it first.<br /><br />Do you want to install the new version now?")
                case DailyBuilds.InstallBuildsFromSource:
                    return qsTr("A Terminal window will open to compile and install the new version on your Mac. This can take a long time and may ask for your administrator password. You can keep using Webcamoid meanwhile: when the compilation ends, the installer will ask you to close it to finish the installation.<br /><br />Do you want to start now?")
                default:
                    return qsTr("Webcamoid will close and the new version will start. If you are streaming, recording or doing something important, finish it first.<br /><br />Do you want to run the new version now?")
                }
            }
            textFormat: Text.StyledText
            wrapMode: Text.WordWrap
        }
    }

    onAccepted: dailyBuilds.install()

    Connections {
        target: dailyBuilds

        function onInstallerScheduled()
        {
            // The installer needs Webcamoid closed to replace its files, and
            // the new AppImage can only start after this instance is gone.
            Qt.quit()
        }
    }
}

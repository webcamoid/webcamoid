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
import QtQuick.Layouts
import Ak
import Webcamoid

// Daily builds channel: channel selector, GitHub account and build card.
// Everything is driven by the "dailyBuilds" controller.
ColumnLayout {
    id: root
    visible: dailyBuilds.available

    // Set by the parent.
    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int sectionSpacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int remainingSecs: 0

    readonly property bool daily: dailyBuilds.channel == DailyBuilds.Daily
    readonly property bool signedIn: dailyBuilds.state == DailyBuilds.SignedIn
                                     || dailyBuilds.state == DailyBuilds.Checking
                                     || dailyBuilds.state == DailyBuilds.Downloading
                                     || dailyBuilds.state == DailyBuilds.Downloaded
    readonly property bool installs:
        dailyBuilds.installKind != DailyBuilds.InstallUnsupported
    // An installer is run, instead of just running the new version.
    readonly property bool usesInstaller:
        dailyBuilds.installKind == DailyBuilds.InstallRunsInstaller
        || dailyBuilds.installKind == DailyBuilds.InstallPackage
        || dailyBuilds.installKind == DailyBuilds.InstallBuildsFromSource
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property int cardRadius:
        AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    readonly property int cardPadding:
        AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
    readonly property url sponsorUrl: "https://github.com/sponsors/hipersayanX"

    component SectionTitle: Label {
        font: AkTheme.fontSettings.h6
        Layout.topMargin: root.sectionSpacing
        Layout.leftMargin: root.leftMargin
        Layout.rightMargin: root.rightMargin
        Layout.fillWidth: true
    }
    component Description: Label {
        wrapMode: Text.WordWrap
        Layout.rightMargin: root.rightMargin
        Layout.fillWidth: true

        // Show the pointing hand over the links.
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            cursorShape: parent.hoveredLink != ""?
                             Qt.PointingHandCursor:
                             Qt.ArrowCursor
        }
    }
    component ChannelCard: Rectangle {
        id: card

        property bool selected: false
        // Set it to true while the pointer is over a link in the card, so
        // that clicking the link does not select the channel.
        property bool linkHovered: false
        default property alias content: cardLayout.data

        signal activated()

        radius: root.cardRadius
        color: "transparent"
        border.width: selected?
                          AkUnit.create(2 * AkTheme.controlScale, "dp").pixels:
                          AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
        border.color: selected?
                          root.activeHighlight:
                          AkTheme.shade(root.activeWindow, -0.25)
        implicitHeight: cardLayout.implicitHeight + 2 * root.cardPadding
        Layout.leftMargin: root.leftMargin
        Layout.rightMargin: root.rightMargin
        Layout.fillWidth: true

        ColumnLayout {
            id: cardLayout
            anchors.fill: parent
            anchors.margins: root.cardPadding
        }
        TapHandler {
            onTapped: {
                if (!card.linkHovered)
                    card.activated()
            }
        }
    }

    function sizeText()
    {
        return qsTr("%1 MB").arg(Math.round(dailyBuilds.buildSize / 1048576))
    }

    function buildDetails()
    {
        var parts = [qsTr("Built on %1")
                     .arg(dailyBuilds.buildDate.toLocaleDateString())]

        if (dailyBuilds.buildCommit != "")
            parts.push(qsTr("commit %1").arg(dailyBuilds.buildCommit))

        parts.push(sizeText())

        return parts.join(" - ")
    }

    function escapeHtml(text)
    {
        return text.replace(/&/g, "&amp;")
                   .replace(/</g, "&lt;")
                   .replace(/>/g, "&gt;")
    }

    function buildTitle()
    {
        if (dailyBuilds.state == DailyBuilds.Downloading)
            return qsTr("Downloading...")

        if (dailyBuilds.state == DailyBuilds.Downloaded)
            return installs && usesInstaller?
                        qsTr("Update ready to install"):
                        qsTr("Download ready")

        if (dailyBuilds.buildStatus == DailyBuilds.UpdateAvailable)
            return dailyBuilds.buildSkipped?
                        qsTr("A newer daily build was skipped"):
                        qsTr("New daily build available")

        if (dailyBuilds.buildStatus == DailyBuilds.UpToDate)
            return qsTr("You have the latest daily build")

        if (dailyBuilds.buildStatus == DailyBuilds.RunningNewer)
            return qsTr("You are using a build newer than the daily build")

        return qsTr("Latest daily build")
    }

    function lastCheckText()
    {
        var date = dailyBuilds.lastCheck

        return isNaN(date.getTime())? qsTr("Never"): date.toLocaleString()
    }

    function remainingText()
    {
        var m = Math.floor(remainingSecs / 60)
        var s = remainingSecs % 60

        return m + ":" + (s < 10? "0": "") + s
    }

    DailyBuildInstallDialog {
        id: installDialog
    }

    ButtonGroup {
        id: channelGroup
    }

    Timer {
        interval: 1000
        repeat: true
        triggeredOnStart: true
        running: dailyBuilds.state == DailyBuilds.SigningIn
                 && dailyBuilds.userCode != ""

        onTriggered: root.remainingSecs =
            Math.max(0, Math.round((dailyBuilds.codeExpiresAt - new Date()) / 1000))
    }

    TextEdit {
        id: clipboard
        visible: false
    }

    // Channel

    SectionTitle {
        text: qsTr("Update channel")
    }
    ChannelCard {
        selected: !root.daily

        onActivated: dailyBuilds.channel = DailyBuilds.Stable

        RadioButton {
            id: stableChannel
            text: qsTr("Stable releases")
            checked: !root.daily
            ButtonGroup.group: channelGroup
            Layout.fillWidth: true

            onClicked: dailyBuilds.channel = DailyBuilds.Stable
        }
        Description {
            text: qsTr("Recommended for most people. Available to everyone.")
            Layout.leftMargin: stableChannel.implicitIndicatorWidth
                               + stableChannel.spacing
            Layout.rightMargin: 0
        }
    }
    ChannelCard {
        selected: root.daily
        linkHovered: sponsorsDescription.hoveredLink != ""

        onActivated: dailyBuilds.channel = DailyBuilds.Daily

        RowLayout {
            Layout.fillWidth: true

            RadioButton {
                id: dailyChannel
                text: qsTr("Daily builds")
                checked: root.daily
                ButtonGroup.group: channelGroup

                onClicked: dailyBuilds.channel = DailyBuilds.Daily
            }
            Rectangle {
                radius: height / 2
                color: Qt.rgba(AkTheme.palette.active.windowText.r,
                               AkTheme.palette.active.windowText.g,
                               AkTheme.palette.active.windowText.b,
                               0.12)
                implicitWidth: sponsorsContent.implicitWidth
                               + AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                implicitHeight: sponsorsContent.implicitHeight
                                + AkUnit.create(4 * AkTheme.controlScale, "dp").pixels

                RowLayout {
                    id: sponsorsContent
                    anchors.centerIn: parent
                    spacing: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels

                    AkColorizedImage {
                        color: AkTheme.palette.active.windowText
                        source: "image://icons/heart"
                        sourceSize: Qt.size(width, height)
                        implicitWidth: sponsorsLabel.implicitHeight
                        implicitHeight: implicitWidth
                        asynchronous: true
                        mipmap: true
                    }
                    Label {
                        id: sponsorsLabel
                        text: qsTr("Sponsors")
                    }
                }
            }
            Item {
                Layout.fillWidth: true
            }
        }
        Description {
            id: sponsorsDescription
            text: qsTr("Latest development builds, updated every day. Not a sponsor yet? <a href=\"sponsor\">Become a sponsor</a> to unlock them.")
            textFormat: Text.StyledText
            linkColor: palette.link
            Layout.leftMargin: dailyChannel.implicitIndicatorWidth
                               + dailyChannel.spacing
            Layout.rightMargin: 0

            onLinkActivated: Qt.openUrlExternally(root.sponsorUrl)
        }
    }

    // GitHub account

    SectionTitle {
        text: qsTr("GitHub account")
        visible: root.daily
    }
    ColumnLayout {
        visible: root.daily && dailyBuilds.state == DailyBuilds.SignedOut
        spacing: root.sectionSpacing / 2
        Layout.fillWidth: true

        Description {
            text: qsTr("Sign in with the GitHub account you sponsor with to download the daily builds.")
            Layout.leftMargin: root.leftMargin
        }
        Button {
            text: qsTr("Sign in with GitHub")
            highlighted: true
            Layout.leftMargin: root.leftMargin

            onClicked: dailyBuilds.signIn()
        }
    }
    ColumnLayout {
        visible: root.daily && dailyBuilds.state == DailyBuilds.SigningIn
        spacing: root.sectionSpacing / 2
        Layout.fillWidth: true

        Label {
            text: dailyBuilds.userCode == ""?
                      qsTr("Contacting GitHub..."):
                      qsTr("Open GitHub and enter this code to sign in:")
            wrapMode: Text.WordWrap
            Layout.leftMargin: root.leftMargin
            Layout.rightMargin: root.rightMargin
            Layout.fillWidth: true
        }
        Label {
            visible: dailyBuilds.userCode != ""
            text: dailyBuilds.userCode
            font.family: "monospace"
            font.bold: true
            font.pixelSize: AkUnit.create(28 * AkTheme.controlScale, "dp").pixels
            font.letterSpacing: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
            horizontalAlignment: Text.AlignHCenter
            Layout.leftMargin: root.leftMargin
            Layout.rightMargin: root.rightMargin
            Layout.fillWidth: true
        }
        RowLayout {
            visible: dailyBuilds.userCode != ""
            Layout.leftMargin: root.leftMargin
            Layout.rightMargin: root.rightMargin

            Button {
                text: qsTr("Copy code")

                onClicked: {
                    clipboard.text = dailyBuilds.userCode
                    clipboard.selectAll()
                    clipboard.copy()
                }
            }
            Button {
                text: qsTr("Open GitHub")
                highlighted: true
                icon.source: "image://icons/internet"

                onClicked: Qt.openUrlExternally(dailyBuilds.verificationUri)
            }
        }
        RowLayout {
            visible: dailyBuilds.userCode != ""
            Layout.leftMargin: root.leftMargin
            Layout.rightMargin: root.rightMargin
            Layout.fillWidth: true

            BusyIndicator {
                running: parent.visible
                implicitWidth: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
                implicitHeight: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
            }
            Description {
                text: qsTr("Waiting for authorization... the code expires in %1")
                        .arg(root.remainingText())
                Layout.rightMargin: 0
            }
        }
        Button {
            text: qsTr("Cancel")
            Layout.leftMargin: root.leftMargin

            onClicked: dailyBuilds.cancelSignIn()
        }
    }
    RowLayout {
        visible: root.daily && root.signedIn
        Layout.leftMargin: root.leftMargin
        Layout.rightMargin: root.rightMargin
        Layout.fillWidth: true

        Label {
            text: dailyBuilds.userName == ""?
                      qsTr("Signed in with GitHub"):
                      qsTr("Signed in as <b>%1</b>")
                        .arg(root.escapeHtml(dailyBuilds.userName))
            textFormat: Text.StyledText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Button {
            text: qsTr("Sign out")

            onClicked: dailyBuilds.signOut()
        }
    }
    Description {
        visible: root.daily && root.signedIn && !dailyBuilds.sessionSaved
        text: qsTr("This system has no secure storage for credentials, so your session will not be saved. You will have to sign in again the next time you check for updates after restarting Webcamoid.")
        Layout.leftMargin: root.leftMargin
    }
    ColumnLayout {
        visible: root.daily && dailyBuilds.error != DailyBuilds.NoError
        spacing: root.sectionSpacing / 4
        Layout.fillWidth: true

        Label {
            text: dailyBuilds.errorMessage
            color: "#d9534f"
            wrapMode: Text.WordWrap
            Layout.leftMargin: root.leftMargin
            Layout.rightMargin: root.rightMargin
            Layout.fillWidth: true
        }
        Description {
            visible: dailyBuilds.error == DailyBuilds.NoAccess
            text: qsTr("Make sure you are sponsoring the project and that you accepted the invitation to the private repository. <a href=\"sponsor\">Become a sponsor</a>")
            textFormat: Text.StyledText
            linkColor: palette.link
            Layout.leftMargin: root.leftMargin

            onLinkActivated: Qt.openUrlExternally(root.sponsorUrl)
        }
    }

    // Build

    SectionTitle {
        text: qsTr("Daily build")
        visible: root.daily && root.signedIn
    }
    RowLayout {
        visible: root.daily && dailyBuilds.state == DailyBuilds.Checking
        Layout.leftMargin: root.leftMargin
        Layout.rightMargin: root.rightMargin
        Layout.fillWidth: true

        BusyIndicator {
            running: parent.visible
            implicitWidth: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
            implicitHeight: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
        }
        Label {
            text: qsTr("Checking for the latest daily build...")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
    ColumnLayout {
        visible: root.daily
                 && root.signedIn
                 && dailyBuilds.hasBuild
                 && dailyBuilds.state != DailyBuilds.Checking
        spacing: root.sectionSpacing / 2
        Layout.fillWidth: true

        Label {
            text: root.buildTitle()
            font.bold: true
            wrapMode: Text.WordWrap
            Layout.leftMargin: root.leftMargin
            Layout.rightMargin: root.rightMargin
            Layout.fillWidth: true
        }
        Description {
            text: root.buildDetails()
                  + "<br />"
                  + root.escapeHtml(dailyBuilds.downloadPath)
            textFormat: Text.StyledText
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            Layout.leftMargin: root.leftMargin
        }
        RowLayout {
            visible: dailyBuilds.state == DailyBuilds.Downloading
            Layout.leftMargin: root.leftMargin
            Layout.rightMargin: root.rightMargin
            Layout.fillWidth: true

            ProgressBar {
                value: dailyBuilds.progress
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Cancel")

                onClicked: dailyBuilds.cancelDownload()
            }
        }
        Description {
            visible: dailyBuilds.state == DailyBuilds.Downloaded
            text: {
                switch (dailyBuilds.installKind) {
                case DailyBuilds.InstallPackage:
                    return qsTr("Download complete and verified. Android will ask you to confirm the installation.")
                case DailyBuilds.InstallRunsInstaller:
                    return qsTr("Download complete and verified. Webcamoid will close to run the installer.")
                case DailyBuilds.InstallBuildsFromSource:
                    return qsTr("Download complete and verified. The new version will be compiled and installed from a Terminal window, you can keep using Webcamoid meanwhile.")
                default:
                    return qsTr("Download complete and verified. Webcamoid will close to run the new version.")
                }
            }
            Layout.leftMargin: root.leftMargin
        }
        Description {
            visible: dailyBuilds.state == DailyBuilds.Downloaded
                     && dailyBuilds.installerLaunched
            text: qsTr("The installer is running in Terminal. When the compilation ends, it will ask you to close Webcamoid to finish the installation.")
            Layout.leftMargin: root.leftMargin
        }
        RowLayout {
            visible: dailyBuilds.state == DailyBuilds.Downloaded
            Layout.leftMargin: root.leftMargin
            Layout.rightMargin: root.rightMargin

            Button {
                text: dailyBuilds.installerLaunched?
                          qsTr("Run again"):
                          root.usesInstaller?
                              qsTr("Install"):
                              qsTr("Run")
                highlighted: true
                visible: root.installs

                onClicked: installDialog.open()
            }
            Button {
                text: qsTr("Show in folder")
                // The downloads are in a private folder of the app.
                visible: dailyBuilds.installKind != DailyBuilds.InstallPackage

                onClicked: Qt.openUrlExternally(dailyBuilds.downloadFolder)
            }
        }
        RowLayout {
            visible: dailyBuilds.state == DailyBuilds.SignedIn
            Layout.leftMargin: root.leftMargin
            Layout.rightMargin: root.rightMargin

            Button {
                text: qsTr("Download")
                highlighted: dailyBuilds.buildStatus == DailyBuilds.UpdateAvailable
                visible: root.installs
                         && dailyBuilds.buildStatus != DailyBuilds.UpToDate
                         && dailyBuilds.buildStatus != DailyBuilds.RunningNewer

                onClicked: dailyBuilds.download()
            }
            Button {
                text: qsTr("Open downloads page")
                icon.source: "image://icons/internet"
                visible: !root.installs

                onClicked: Qt.openUrlExternally(mediaTools.projectDownloadsUrl)
            }
            Button {
                text: qsTr("Skip this build")
                flat: true
                visible: dailyBuilds.buildStatus == DailyBuilds.UpdateAvailable
                         && !dailyBuilds.buildSkipped

                onClicked: dailyBuilds.skipBuild()
            }
        }
    }
    RowLayout {
        // Without a session, checking asks for signing in first.
        visible: root.daily
                 && (dailyBuilds.state == DailyBuilds.SignedIn
                     || dailyBuilds.state == DailyBuilds.SignedOut)
        Layout.leftMargin: root.leftMargin
        Layout.rightMargin: root.rightMargin
        Layout.fillWidth: true

        Label {
            text: qsTr("<b>Last checked</b>: %1").arg(root.lastCheckText())
            textFormat: Text.StyledText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Button {
            text: qsTr("Check now")

            onClicked: dailyBuilds.checkNow()
        }
    }
}

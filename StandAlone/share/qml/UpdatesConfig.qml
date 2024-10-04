/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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
import QtCore
import Qt.labs.settings 1.0
import Ak
import Webcamoid

Page {
    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        GridLayout {
            id: layout
            columns: 2
            width: scrollView.width

            property int webcamoidStatus: updates.status("Webcamoid")
            property string webcamoidLatestVersion: updates.latestVersion("Webcamoid")

            Component.onCompleted: {
                var ciIndex = 0
                var ciDiff = Number.POSITIVE_INFINITY

                for (var i = 0; i < cbxCheckInterval.model.count; i++) {
                    if (cbxCheckInterval.model.get(i).interval == updates.checkInterval) {
                        ciIndex = i

                        break
                    } else {
                        var diff =
                                Math.abs(cbxCheckInterval.model.get(i).interval
                                         - updates.checkInterval)

                        if (diff < ciDiff) {
                            ciIndex = i
                            ciDiff = diff
                        }
                    }
                }

                cbxCheckInterval.currentIndex = ciIndex;
            }

            Connections {
                target: updates

                function onNewVersionAvailable(component, latestVersion)
                {
                    if (component == "Webcamoid") {
                        layout.webcamoidStatus = updates.status("Webcamoid");
                        layout.webcamoidLatestVersion = latestVersion;
                        state = layout.webcamoidStatus == Updates.ComponentOutdated?
                                    "isOutdated":
                                layout.webcamoidStatus == Updates.ComponentNewer?
                                    "isDevelopment": ""
                    }
                }
            }

            Label {
                id: txtNotifyNewVersions
                text: qsTr("Notify about new versions")
            }
            Switch {
                id: newVersion
                Accessible.name: txtNotifyNewVersions.text
                checked: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                onCheckedChanged: updates.notifyNewVersion = checked
            }
            Label {
                id: txtShowUpdatesDialog
                text: qsTr("Show updates dialog")
            }
            Switch {
                id: showUpdatesDialog
                Accessible.name: txtShowUpdatesDialog.text
                checked: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            }
            Label {
                id: txtCheckNewVersions
                text: qsTr("Check new versions")
            }
            ComboBox {
                id: cbxCheckInterval
                Accessible.description: txtCheckNewVersions.text
                Layout.fillWidth: true
                textRole: "description"
                model: ListModel {
                    ListElement {
                        description: qsTr("Daily")
                        interval: 1
                    }
                    ListElement {
                        description: qsTr("Every two days")
                        interval: 2
                    }
                    ListElement {
                        description: qsTr("Weekly")
                        interval: 7
                    }
                    ListElement {
                        description: qsTr("Every two weeks")
                        interval: 14
                    }
                    ListElement {
                        description: qsTr("Monthly")
                        interval: 30
                    }
                    ListElement {
                        description: qsTr("Never")
                        interval: 0
                    }
                }
                currentIndex: 0
                enabled: newVersion.checked

                onCurrentIndexChanged:
                    if (currentIndex >= 0)
                        updates.checkInterval = model.get(currentIndex).interval
            }

            Label {
                text: qsTr("Last updated")
            }
            Label {
                text: updates.lastUpdate.toLocaleString()
                font.bold: true
                Layout.alignment: Qt.AlignRight
            }

            Item {
                height: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
                Layout.columnSpan: 2
            }
            ColumnLayout {
                id: isOutdated
                spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
                Layout.columnSpan: 2
                visible: !mediaTools.isDailyBuild && layout.webcamoidStatus == Updates.ComponentOutdated

                Label {
                    text: qsTr("Your version of %1 is outdated. Latest version is <b>%2</b>.")
                            .arg(mediaTools.applicationName).arg(layout.webcamoidLatestVersion)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Upgrade Now!")
                    icon.source: "image://icons/internet"
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    onClicked: Qt.openUrlExternally(mediaTools.projectDownloadsUrl)
                }
            }
            ColumnLayout {
                id: isDevelopment
                spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
                Layout.columnSpan: 2
                visible: mediaTools.isDailyBuild

                Label {
                    text: qsTr("Thanks for using a <b>development version</b>!<br />It will be very helpful if you can report any bug and suggestions you have.")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Report a Bug")
                    icon.source: "image://icons/bug"
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    onClicked: Qt.openUrlExternally(mediaTools.projectIssuesUrl)
                }
            }
        }
    }

    Settings {
        category: "Updates"

        property alias notify: newVersion.checked
        property alias showDialog: showUpdatesDialog.checked
    }
}

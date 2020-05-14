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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Ak 1.0
import Webcamoid 1.0

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

                onVersionTypeChanged: state = versionType == Updates.VersionTypeOld?
                                                  "isOutdated":
                                              versionType == Updates.VersionTypeDevelopment?
                                                  "isDevelopment": ""
            }

            Label {
                text: qsTr("Notify about new versions")
            }
            Switch {
                id: newVersion
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                onCheckedChanged: updates.notifyNewVersion = checked
            }

            Label {
                text: qsTr("Check new versions")
            }
            ComboBox {
                id: cbxCheckInterval
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
                visible: updates.versionType == Updates.VersionTypeOld

                Label {
                    text: qsTr("Your version of %1 is outdated. Latest version is <b>%2</b>.")
                            .arg(mediaTools.applicationName).arg(updates.latestVersion)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Upgrade Now!")
                    icon.source: "image://icons/internet"
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    onClicked: Qt.openUrlExternally(mediaTools.projectDownloadsUrl())
                }
            }
            ColumnLayout {
                id: isDevelopment
                spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
                Layout.columnSpan: 2
                visible: updates.versionType == Updates.VersionTypeDevelopment

                Label {
                    text: qsTr("Thanks for using a <b>development version</b>!<br />It will be very helpful if you can report any bug and suggestions you have.")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Report a Bug")
                    icon.source: "image://icons/bug"
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    onClicked: Qt.openUrlExternally(mediaTools.projectIssuesUrl())
                }
            }
        }
    }
}

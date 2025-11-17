/* Webcamoid, camera capture application.
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
import AkControls as AK
import Webcamoid

AK.MenuOption {
    id: root
    title: qsTr("Updates")
    subtitle: qsTr("Configure the update frequency.")
    icon: "image://icons/update"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        ColumnLayout {
            id: layout
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

            Switch {
                id: newVersion
                text: qsTr("Notify about new versions")
                checked: true
                Accessible.name: text
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true

                onCheckedChanged: updates.notifyNewVersion = checked
            }
            Switch {
                id: showUpdatesDialog
                text: qsTr("Show updates dialog")
                checked: true
                Accessible.name: text
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }
            AK.LabeledComboBox {
                id: cbxCheckInterval
                label: qsTr("Check new versions")
                Accessible.description: label
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
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
                text: qsTr("<b>Last updated</b>: %1").arg(updates.lastUpdate.toLocaleString())
                wrapMode: Text.WordWrap
                elide: Text.ElideNone
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }

            Item {
                height: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }
            ColumnLayout {
                id: isOutdated
                spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
                visible: !mediaTools.isDailyBuild && layout.webcamoidStatus == Updates.ComponentOutdated

                Label {
                    text: qsTr("Your version of %1 is outdated. Latest version is <b>%2</b>.")
                            .arg(mediaTools.applicationName).arg(layout.webcamoidLatestVersion)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                }
                Button {
                    text: qsTr("Upgrade Now!")
                    icon.source: "image://icons/internet"
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin

                    onClicked: Qt.openUrlExternally(mediaTools.projectDownloadsUrl)
                }
            }
            ColumnLayout {
                id: isDevelopment
                spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
                visible: mediaTools.isDailyBuild

                Label {
                    text: qsTr("Thanks for using a <b>development version</b>!<br />It will be very helpful if you can report any bug and suggestions you have.")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                }
                Button {
                    text: qsTr("Report a Bug")
                    icon.source: "image://icons/bug"
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin

                    onClicked: Qt.openUrlExternally(mediaTools.projectIssuesUrl)
                }
            }
        }
        Settings {
            category: "Updates"

            property alias notify: newVersion.checked
            property alias showDialog: showUpdatesDialog.checked
        }
    }
}

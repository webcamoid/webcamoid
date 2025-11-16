/* Webcamoid, camera capture application.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
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
import AkControls as AK

AK.MenuOption {
    id: root
    title: qsTr("General Options")
    subtitle: qsTr("Configure capture, recording frameworks, and others.")
    icon: "image://icons/settings"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: generalConfigs.height
        clip: true

        ColumnLayout {
            id: generalConfigs
            width: scrollView.width

            function fillControl(control, pluginId, interfaces)
            {
                control.model.clear()
                let plugins =
                    AkPluginManager.listPlugins(pluginId,
                                                interfaces,
                                                AkPluginManager.FilterEnabled)

                plugins.sort(function(a, b) {
                    a = AkPluginInfo.create(AkPluginManager.pluginInfo(a)).name
                    b = AkPluginInfo.create(AkPluginManager.pluginInfo(b)).name

                    return a.localeCompare(b)
                })

                for (let i in plugins) {
                    let plugin = plugins[i]
                    let info = AkPluginInfo.create(AkPluginManager.pluginInfo(plugin))

                    control.model.append({
                        plugin: plugin,
                        description: info.name
                    })
                }

                let defaultPlugin =
                    AkPluginManager.defaultPlugin(pluginId, interfaces)
                let info = AkPluginInfo.create(defaultPlugin)
                control.currentIndex = plugins.indexOf(info.id)
            }

            Switch {
                text: qsTr("Play sources on start")
                checked: videoLayer.playOnStart
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                Accessible.name: text

                onCheckedChanged: videoLayer.playOnStart = checked
            }

            Label {
                text: qsTr("Frameworks and libraries")
                font: AkTheme.fontSettings.h6
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }

            AK.LabeledComboBox {
                label: qsTr("Video capture")
                Accessible.description: label
                Layout.fillWidth: true
                Layout.rightMargin: root.rightMargin
                textRole: "description"
                model: ListModel {
                }

                Component.onCompleted:
                    generalConfigs.fillControl(this,
                                               "VideoSource/CameraCapture/Impl/*",
                                               ["CameraCaptureImpl"])
                onCurrentIndexChanged:
                    AkPluginManager.link("VideoSource/CameraCapture/Impl/*",
                                         model.get(currentIndex).plugin)
            }
            AK.LabeledComboBox {
                label: qsTr("Screen capture")
                Accessible.description: label
                Layout.fillWidth: true
                Layout.rightMargin: root.rightMargin
                textRole: "description"
                model: ListModel {
                }

                Component.onCompleted:
                    generalConfigs.fillControl(this,
                                               "VideoSource/DesktopCapture/Impl/*",
                                               ["DesktopCaptureImpl"])
                onCurrentIndexChanged:
                    AkPluginManager.link("VideoSource/DesktopCapture/Impl/*",
                                         model.get(currentIndex).plugin)
            }
            AK.LabeledComboBox {
                label: qsTr("Audio capture/playback")
                Accessible.description: label
                Layout.fillWidth: true
                Layout.rightMargin: root.rightMargin
                textRole: "description"
                model: ListModel {
                }

                Component.onCompleted:
                    generalConfigs.fillControl(this,
                                               "AudioSource/AudioDevice/Impl/*",
                                               ["AudioDeviceImpl"])
                onCurrentIndexChanged:
                    AkPluginManager.link("AudioSource/AudioDevice/Impl/*",
                                         model.get(currentIndex).plugin)
            }
            AK.LabeledComboBox {
                label: qsTr("Video convert")
                Accessible.description: label
                Layout.fillWidth: true
                Layout.rightMargin: root.rightMargin
                textRole: "description"
                model: ListModel {
                }

                Component.onCompleted:
                    generalConfigs.fillControl(this,
                                               "VideoSource/CameraCapture/Convert/*",
                                               ["CameraCaptureConvert"])
                onCurrentIndexChanged:
                    AkPluginManager.link("VideoSource/CameraCapture/Convert/*",
                                         model.get(currentIndex).plugin)
            }
            AK.LabeledComboBox {
                label: qsTr("Video playback")
                Accessible.description: label
                Layout.fillWidth: true
                Layout.rightMargin: root.rightMargin
                textRole: "description"
                model: ListModel {
                }

                Component.onCompleted:
                    generalConfigs.fillControl(this,
                                               "MultimediaSource/MultiSrc/Impl/*",
                                               ["MultiSrcImpl"])
                onCurrentIndexChanged:
                    AkPluginManager.link("MultimediaSource/MultiSrc/Impl/*",
                                         model.get(currentIndex).plugin)
            }
            AK.LabeledComboBox {
                label: qsTr("Virtual camera driver")
                Accessible.description: label
                Layout.fillWidth: true
                Layout.rightMargin: root.rightMargin
                textRole: "description"
                model: ListModel {
                }
                visible: videoLayer.isVCamSupported
                enabled: visible

                Component.onCompleted:
                    generalConfigs.fillControl(this,
                                               "VideoSink/VirtualCamera/Impl/*",
                                               ["VirtualCameraImpl"])
                onCurrentIndexChanged:
                    AkPluginManager.link("VideoSink/VirtualCamera/Impl/*",
                                         model.get(currentIndex).plugin)
            }
            AK.LabeledComboBox {
                /*: The preferred method for executing commands with elevated
                    privileges in the system.
                 */
                label: qsTr("Root method")
                model: videoLayer.availableRootMethods
                currentIndex: model.indexOf(videoLayer.rootMethod)
                visible: videoLayer.isVCamSupported
                enabled: visible
                Layout.fillWidth: true
                Layout.rightMargin: root.rightMargin
                Accessible.description: label

                onCurrentIndexChanged: videoLayer.rootMethod = model[currentIndex]
            }
        }
    }
}

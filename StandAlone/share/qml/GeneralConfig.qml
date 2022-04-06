/* Webcamoid, webcam capture application.
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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Ak 1.0

Page {
    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: generalConfigs.height
        clip: true

        GridLayout {
            id: generalConfigs
            columns: 2
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

            Label {
                id: txtPlaySources
                /*: Start playing the webcam and other sources right after
                 *  opening Webcamoid.
                 */
                text: qsTr("Play sources on start")
            }
            Switch {
                Accessible.name: txtPlaySources.text
                checked: videoLayer.playOnStart
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                onCheckedChanged: videoLayer.playOnStart = checked
            }

            Label {
                text: qsTr("Frameworks and libraries")
                font.pointSize: 12
                font.bold: true
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.columnSpan: 2
            }

            Label {
                id: txtVideoCacture
                text: qsTr("Video capture")
            }
            ComboBox {
                Accessible.description: txtVideoCacture.text
                Layout.fillWidth: true
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
            Label {
                id: txtScreenSources
                text: qsTr("Screen capture")
            }
            ComboBox {
                Accessible.description: txtScreenSources.text
                Layout.fillWidth: true
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
            Label {
                id: txtAudioCapturePlayback
                text: qsTr("Audio capture/playback")
            }
            ComboBox {
                Accessible.description: txtAudioCapturePlayback.text
                Layout.fillWidth: true
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
            Label {
                id: txtVideoConvert
                text: qsTr("Video convert")
            }
            ComboBox {
                Accessible.description: txtVideoConvert.text
                Layout.fillWidth: true
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
            Label {
                id: txtVideoPlayback
                text: qsTr("Video playback")
            }
            ComboBox {
                Accessible.description: txtVideoPlayback.text
                Layout.fillWidth: true
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
            Label {
                id: txtVideoRecording
                text: qsTr("Video record")
            }
            ComboBox {
                Accessible.description: txtVideoRecording.text
                Layout.fillWidth: true
                textRole: "description"
                model: ListModel {
                }

                Component.onCompleted:
                    generalConfigs.fillControl(this,
                                               "MultimediaSink/MultiSink/Impl/*",
                                               ["MultiSinkImpl"])
                onCurrentIndexChanged:
                    AkPluginManager.link("MultimediaSink/MultiSink/Impl/*",
                                         model.get(currentIndex).plugin)
            }
            Label {
                id: txtVcamDriver
                text: qsTr("Virtual camera driver")
            }
            ComboBox {
                Accessible.description: txtVcamDriver.text
                Layout.fillWidth: true
                textRole: "description"
                model: ListModel {
                }

                Component.onCompleted:
                    generalConfigs.fillControl(this,
                                               "VideoSink/VirtualCamera/Impl/*",
                                               ["VirtualCameraImpl"])
                onCurrentIndexChanged:
                    AkPluginManager.link("VideoSink/VirtualCamera/Impl/*",
                                         model.get(currentIndex).plugin)
            }
            Label {
                id: txtRootMethod
                /*: The preferred method for executing commands with elevated
                    privileges in the system.
                 */
                text: qsTr("Root method")
            }
            ComboBox {
                Layout.fillWidth: true
                model: videoLayer.availableRootMethods
                currentIndex: model.indexOf(videoLayer.rootMethod)
                Accessible.description: txtRootMethod.text

                onCurrentIndexChanged: videoLayer.rootMethod = model[currentIndex]
            }
        }
    }
}

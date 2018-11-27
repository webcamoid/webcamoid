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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import AkQml 1.0
import AkQmlControls 1.0

AkScrollView {
    id: scrollView
    clip: true
    contentHeight: generalConfigs.height

    property variant videoCapture: Ak.newElement("VideoCapture")
    property variant desktopCapture: Ak.newElement("DesktopCapture")
    property variant audioDevice: Ak.newElement("AudioDevice")
    property variant audioConvert: Ak.newElement("ACapsConvert")
    property variant virtualCamera: Ak.newElement("VirtualCamera")
    property variant multiSrc: Ak.newElement("MultiSrc")
    property variant multiSink: Ak.newElement("MultiSink")

    ColumnLayout {
        id: generalConfigs
        width: scrollView.width
               - (scrollView.ScrollBar.vertical.visible?
                      scrollView.ScrollBar.vertical.width: 0)

        CheckBox {
            text: qsTr("Play webcam on start")
            checked: MediaSource.playOnStart

            onCheckedChanged: MediaSource.playOnStart = checked
        }
        CheckBox {
            text: qsTr("Enable advanced effects mode")
            checked: VideoEffects.advancedMode

            onCheckedChanged: VideoEffects.advancedMode = checked
        }
        Label {
            text: qsTr("Frameworks & libraries")
            font.pointSize: 1.25 * font.pointSize
            font.bold: true
            Layout.topMargin: 10
            Layout.bottomMargin: 10
        }
        GridLayout {
            columns: 2

            Label {
                text: qsTr("Video capture")
            }
            ComboBox {
                Layout.fillWidth: true
                model: videoCapture.listSubModules(["capture"])
                currentIndex: model.indexOf(videoCapture.captureLib)

                onCurrentIndexChanged: videoCapture.captureLib = model[currentIndex]
            }
            Label {
                text: qsTr("Desktop capture")
            }
            ComboBox {
                Layout.fillWidth: true
                model: desktopCapture.listSubModules()
                currentIndex: model.indexOf(desktopCapture.captureLib)

                onCurrentIndexChanged: desktopCapture.captureLib = model[currentIndex]
            }
            Label {
                text: qsTr("Audio capture/play")
            }
            ComboBox {
                Layout.fillWidth: true
                model: audioDevice.listSubModules()
                currentIndex: model.indexOf(audioDevice.audioLib)

                onCurrentIndexChanged: audioDevice.audioLib = model[currentIndex]
            }
            Label {
                text: qsTr("Video convert")
            }
            ComboBox {
                Layout.fillWidth: true
                model: videoCapture.listSubModules(["convert"])
                currentIndex: model.indexOf(videoCapture.codecLib)

                onCurrentIndexChanged: videoCapture.codecLib = model[currentIndex]
            }
            Label {
                text: qsTr("Audio convert")
            }
            ComboBox {
                Layout.fillWidth: true
                model: audioConvert.listSubModules()
                currentIndex: model.indexOf(audioConvert.convertLib)

                onCurrentIndexChanged: audioConvert.convertLib = model[currentIndex]
            }
            Label {
                text: qsTr("Video output")
            }
            ComboBox {
                Layout.fillWidth: true
                model: virtualCamera.listSubModules(["output"])
                currentIndex: model.indexOf(virtualCamera.outputLib)

                onCurrentIndexChanged: virtualCamera.outputLib = model[currentIndex]
            }
            Label {
                text: qsTr("Video playback")
            }
            ComboBox {
                Layout.fillWidth: true
                model: multiSrc.listSubModules()
                currentIndex: model.indexOf(multiSrc.codecLib)

                onCurrentIndexChanged: multiSrc.codecLib = model[currentIndex]
            }
            Label {
                text: qsTr("Video record")
            }
            ComboBox {
                Layout.fillWidth: true
                model: multiSink.listSubModules()
                currentIndex: model.indexOf(multiSink.codecLib)

                onCurrentIndexChanged: multiSink.codecLib = model[currentIndex]
            }
            Label {
                text: qsTr("Root method")
                visible: virtualCamera.availableMethods.length > 0
            }
            ComboBox {
                Layout.fillWidth: true
                model: virtualCamera.availableMethods
                currentIndex: virtualCamera.availableMethods.length > 0?
                                  model.indexOf(virtualCamera.rootMethod): -1
                visible: virtualCamera.availableMethods.length > 0

                onCurrentIndexChanged: virtualCamera.rootMethod = model[currentIndex]
            }
        }
    }
}

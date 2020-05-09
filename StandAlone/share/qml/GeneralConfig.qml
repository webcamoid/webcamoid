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

            property variant videoCapture:
                AkElement.createPtr("VideoCapture", "Ak.Element.Settings")
            property variant desktopCapture:
                AkElement.createPtr("DesktopCapture", "Ak.Element.Settings")
            property variant audioDevice:
                AkElement.createPtr("AudioDevice", "Ak.Element.Settings")
            property variant audioConvert:
                AkElement.createPtr("ACapsConvert", "Ak.Element.Settings")
            property variant virtualCamera:
                AkElement.createPtr("VirtualCamera")
            property variant multiSrc:
                AkElement.createPtr("MultiSrc", "Ak.Element.Settings")
            property variant multiSink:
                AkElement.createPtr("MultiSink", "Ak.Element.Settings")

            Label {
                /*: Start playing the webcam and other sources right after
                 *  opening Webcamoid.
                 */
                text: qsTr("Play sources on start")
            }
            Switch {
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
                text: qsTr("Video capture")
            }
            ComboBox {
                Layout.fillWidth: true
                model: generalConfigs.videoCapture.captureSubModules
                currentIndex: model.indexOf(generalConfigs.videoCapture.captureLib)

                onCurrentIndexChanged: generalConfigs.videoCapture.captureLib = model[currentIndex]
            }
            Label {
                text: qsTr("Desktop capture")
            }
            ComboBox {
                Layout.fillWidth: true
                model: generalConfigs.desktopCapture.subModules
                currentIndex: model.indexOf(generalConfigs.desktopCapture.captureLib)

                onCurrentIndexChanged: generalConfigs.desktopCapture.captureLib = model[currentIndex]
            }
            Label {
                text: qsTr("Audio capture/play")
            }
            ComboBox {
                Layout.fillWidth: true
                model: generalConfigs.audioDevice.subModules
                currentIndex: model.indexOf(generalConfigs.audioDevice.audioLib)

                onCurrentIndexChanged: generalConfigs.audioDevice.audioLib = model[currentIndex]
            }
            Label {
                text: qsTr("Video convert")
            }
            ComboBox {
                Layout.fillWidth: true
                model: generalConfigs.videoCapture.codecSubModules
                currentIndex: model.indexOf(generalConfigs.videoCapture.codecLib)

                onCurrentIndexChanged: generalConfigs.videoCapture.codecLib = model[currentIndex]
            }
            Label {
                text: qsTr("Audio convert")
            }
            ComboBox {
                Layout.fillWidth: true
                model: generalConfigs.audioConvert.subModules
                currentIndex: model.indexOf(generalConfigs.audioConvert.convertLib)

                onCurrentIndexChanged: generalConfigs.audioConvert.convertLib = model[currentIndex]
            }
            Label {
                text: qsTr("Video playback")
            }
            ComboBox {
                Layout.fillWidth: true
                model: generalConfigs.multiSrc.subModules
                currentIndex: model.indexOf(generalConfigs.multiSrc.codecLib)

                onCurrentIndexChanged: generalConfigs.multiSrc.codecLib = model[currentIndex]
            }
            Label {
                text: qsTr("Video record")
            }
            ComboBox {
                Layout.fillWidth: true
                model: generalConfigs.multiSink.subModules
                currentIndex: model.indexOf(generalConfigs.multiSink.codecLib)

                onCurrentIndexChanged: generalConfigs.multiSink.codecLib = model[currentIndex]
            }
            Label {
                text: qsTr("Root method")
                visible: generalConfigs.virtualCamera
            }
            ComboBox {
                Layout.fillWidth: true
                model: generalConfigs.virtualCamera.availableMethods
                currentIndex: generalConfigs.virtualCamera.availableMethods.length > 0?
                                  model.indexOf(generalConfigs.virtualCamera.rootMethod): -1
                visible: generalConfigs.virtualCamera

                onCurrentIndexChanged: generalConfigs.virtualCamera.rootMethod = model[currentIndex]
            }
            Label {
                text: qsTr("Virtual camera driver")
                visible: generalConfigs.virtualCamera
            }
            ComboBox {
                Layout.fillWidth: true
                model: generalConfigs.virtualCamera?
                           generalConfigs.virtualCamera.availableDrivers:
                           []
                currentIndex: !generalConfigs.virtualCamera?
                                  -1:
                              generalConfigs.virtualCamera.availableDrivers.length > 0?
                                  model.indexOf(generalConfigs.virtualCamera.driver):
                                  -1
                visible: generalConfigs.virtualCamera

                onCurrentIndexChanged: generalConfigs.virtualCamera.driver = model[currentIndex]
            }
        }
    }
}

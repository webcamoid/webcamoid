/* Webcamoid, camera capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

ScrollView {
    id: view

    // The source ID being edited
    property int sourceId: -1

    readonly property var inputType: sourceId >= 0?
                                        videoLayer.deviceType(sourceId):
                                        VideoLayer.InputUnknown
    readonly property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    signal openVideoInputAddEditDialog(int sourceId)
    signal videoInputRemoved()
    signal openSourceVideoEffectsDialog(int sourceId)

    function setInput(sourceId)
    {
        view.sourceId = sourceId
        deviceInfo.text =
                "<b>" + videoLayer.sourceLabel(sourceId) + "</b>"
                + "<br/><i>" + videoLayer.sourceDevice(sourceId) + "</i>"
        videoLayer.removeInterface("itmVideoInputOptions")
        videoLayer.embedControls("itmVideoInputOptions", sourceId, "")
    }

    ColumnLayout {
        width: view.width
        layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight

        Label {
            id: deviceInfo
            elide: Label.ElideRight
            Layout.fillWidth: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.bottomMargin:
                AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        }

        Button {
            text: qsTr("Edit")
            icon.source: "image://icons/edit"
            flat: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            visible: view.inputType == VideoLayer.InputStream
                     || view.inputType == VideoLayer.InputImage

            onClicked: view.openVideoInputAddEditDialog(view.sourceId)
        }

        Button {
            text: qsTr("Remove")
            icon.source: "image://icons/no"
            flat: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin

            onClicked: {
                videoLayer.removeInterface("itmVideoInputOptions")
                videoLayer.removeSource(view.sourceId)
                videoEffects.removeSource(view.sourceId)
                view.videoInputRemoved()
            }
        }

        Button {
            text: qsTr("Manage source effects")
            icon.source: "image://icons/video-effects"
            flat: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin

            onClicked:
                view.openSourceVideoEffectsDialog(view.sourceId)
        }

        // Opacity slider
        Label {
            text: qsTr("Opacity")
            font.bold: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true
        }

        Slider {
            id: opacitySlider
            from: 0.0
            to: 1.0
            stepSize: 0.01
            value: view.sourceId >= 0?
                        videoEffects.sourceOpacity(view.sourceId):
                        1.0
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true
            Accessible.name: qsTr("Source opacity")

            onMoved: {
                if (view.sourceId >= 0)
                    videoEffects.setSourceOpacity(view.sourceId, value)
            }
        }

        ColumnLayout {
            id: itmVideoInputOptions
            objectName: "itmVideoInputOptions"
            width: view.width
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
        }
    }
}

/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Chris Barth
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
import AkControls 1.0 as AK

GridLayout {
    columns: 2

    function haarFileIndex(haarFile)
    {
        let index = -1

        for (let i = 0; i < cbxHaarFile.model.count; i++)
            if (cbxHaarFile.model.get(i).haarFile == haarFile) {
                index = i

                break
            }

        return index
    }

    function strToSize(str)
    {
        if (str.length < 1)
            return Qt.size()

        let size = str.split(RegExp(/x|:/))

        if (size.length < 2)
            return Qt.size()

        return Qt.size(size[0], size[1])
    }

    // Haar file.
    Label {
        id: txtHaarFile
        //: https://en.wikipedia.org/wiki/Haar-like_feature
        text: qsTr("Haar file")
    }
    ComboBox {
        id: cbxHaarFile
        textRole: "text"
        currentIndex: haarFileIndex(FaceTrack.haarFile)
        Layout.fillWidth: true
        Accessible.description: txtHaarFile.text

        model: ListModel {
            ListElement {
                text: qsTr("Eye")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_eye.xml"
            }
            ListElement {
                text: qsTr("Eye glasses")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_eye_tree_eyeglasses.xml"
            }
            ListElement {
                text: qsTr("Frontal face alternative 1")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt.xml"
            }
            ListElement {
                text: qsTr("Frontal face alternative 2")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt2.xml"
            }
            ListElement {
                text: qsTr("Frontal face alternative 3")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt_tree.xml"
            }
            ListElement {
                text: qsTr("Frontal face default")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_frontalface_default.xml"
            }
            ListElement {
                text: qsTr("Full body")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_fullbody.xml"
            }
            ListElement {
                text: qsTr("Left Eye 1")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_lefteye_2splits.xml"
            }
            ListElement {
                text: qsTr("Lower body")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_lowerbody.xml"
            }
            ListElement {
                text: qsTr("Eye pair big")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_eyepair_big.xml"
            }
            ListElement {
                text: qsTr("Eye pair small")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_eyepair_small.xml"
            }
            ListElement {
                text: qsTr("Left ear")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_leftear.xml"
            }
            ListElement {
                text: qsTr("Left eye 2")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_lefteye.xml"
            }
            ListElement {
                text: qsTr("Mouth")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_mouth.xml"
            }
            ListElement {
                text: qsTr("Nose")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_nose.xml"
            }
            ListElement {
                text: qsTr("Right ear")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_rightear.xml"
            }
            ListElement {
                text: qsTr("Right Eye 1")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_righteye.xml"
            }
            ListElement {
                text: qsTr("Upper body 1")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_mcs_upperbody.xml"
            }
            ListElement {
                text: qsTr("Profile face")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_profileface.xml"
            }
            ListElement {
                text: qsTr("Right eye 2")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_righteye_2splits.xml"
            }
            ListElement {
                text: qsTr("Smile")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_smile.xml"
            }
            ListElement {
                text: qsTr("Upper body")
                haarFile: ":/FaceDetect/share/haarcascades/haarcascade_upperbody.xml"
            }
        }

        onCurrentIndexChanged:
            FaceTrack.haarFile = cbxHaarFile.model.get(currentIndex).haarFile
    }

    // Scan block.
    Label {
        id: txtScanBlock
        text: qsTr("Scan block")
    }
    TextField {
        text: FaceTrack.scanSize.width + "x" + FaceTrack.scanSize.height
        placeholderText: qsTr("Scan block")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+x\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtScanBlock.text

        onAccepted: FaceTrack.scanSize = strToSize(text)
    }

    // Face bucket size
    Label {
        id: txtFaceBracketingDuration
        text: qsTr("Face bracketing\nduration (seconds)")
    }
    SpinBox {
        value: FaceTrack.faceBucketSize
        from: 1
        to: 600
        stepSize: 1
        editable: true
        Layout.fillWidth: true
        Accessible.name: txtFaceBracketingDuration.text

        onValueChanged: FaceTrack.faceBucketSize = Number(value)
    }

    // Face bucket count
    Label {
        id: txtFaceBracketCount
        text: qsTr("Face bracket count")
    }
    SpinBox {
        value: FaceTrack.faceBucketCount
        from: 1
        to: 120
        stepSize: 1
        editable: true
        Layout.fillWidth: true
        Accessible.name: txtFaceBracketCount.text

        onValueChanged: FaceTrack.faceBucketCount = Number(value)
    }

    // Expand rate
    Label {
        id: txtZoomOutRate
        text: qsTr("Zoom out rate")
    }
    SpinBox {
        value: FaceTrack.expandRate
        from: 1
        to: 100
        stepSize: 1
        editable: true
        Layout.fillWidth: true
        Accessible.name: txtZoomOutRate.text

        onValueChanged: FaceTrack.expandRate = Number(value)
    }

    // Contract rate
    Label {
        id: txtZoomInRate
        text: qsTr("Zoom in rate")
    }
    SpinBox {
        value: FaceTrack.contractRate
        from: 1
        to: 100
        stepSize: 1
        editable: true
        Layout.fillWidth: true
        Accessible.name: txtZoomInRate.text

        onValueChanged: FaceTrack.contractRate = Number(value)
    }

    // Face padding
    GroupBox {
        title: qsTr("Face padding (% of face size)")
        Layout.columnSpan: 2
        Layout.fillWidth: true
        clip: true
        Accessible.name: title

        GridLayout {
            columns: 2
            x: (parent.width - width) / 2

            SpinBox {
                value: FaceTrack.facePadding.top
                from: 0
                to: 1000
                stepSize: 1
                editable: true
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignHCenter
                Accessible.name: qsTr("Padding top")

                onValueChanged: {
                    FaceTrack.facePadding = Qt.rect(FaceTrack.facePadding.x,
                                                    Number(value),
                                                    FaceTrack.facePadding.width,
                                                    FaceTrack.facePadding.height
                                                    + FaceTrack.facePadding.y
                                                    - Number(value))
                }
            }
            SpinBox {
                value: FaceTrack.facePadding.left
                from: 0
                to: 1000
                stepSize: 1
                editable: true
                Layout.columnSpan: 1
                Accessible.name: qsTr("Padding left")

                onValueChanged: {
                    FaceTrack.facePadding = Qt.rect(Number(value),
                                                    FaceTrack.facePadding.y,
                                                    FaceTrack.facePadding.width
                                                    + FaceTrack.facePadding.x
                                                    - Number(value),
                                                    FaceTrack.facePadding.height)
                }
            }
            SpinBox {
                value: FaceTrack.facePadding.right
                from: 0
                to: 1000
                stepSize: 1
                editable: true
                Layout.columnSpan: 1
                Accessible.name: qsTr("Padding right")

                onValueChanged: {
                    FaceTrack.facePadding = Qt.rect(FaceTrack.facePadding.x,
                                                    FaceTrack.facePadding.y,
                                                    Number(value) - FaceTrack.facePadding.x + 1,
                                                    FaceTrack.facePadding.height)
                }
            }
            SpinBox {
                value: FaceTrack.facePadding.bottom
                from: 0
                to: 1000
                stepSize: 1
                editable: true
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignHCenter
                Accessible.name: qsTr("Padding bottom")

                onValueChanged: {
                    FaceTrack.facePadding = Qt.rect(FaceTrack.facePadding.x,
                                                    FaceTrack.facePadding.y,
                                                    FaceTrack.facePadding.width,
                                                    Number(value) - FaceTrack.facePadding.y + 1)
                }
            }
        }
    }

    // Face margin
    GroupBox {
        title: qsTr("Face margin (% of face size)")
        Layout.columnSpan: 2
        Layout.fillWidth: true
        clip: true
        Accessible.name: title

        GridLayout {
            columns: 2
            x: (parent.width - width) / 2

            SpinBox {
                value: FaceTrack.faceMargin.top
                from: 0
                to: 1000
                stepSize: 1
                editable: true
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignHCenter
                Accessible.name: qsTr("Margin top")

                onValueChanged: {
                    FaceTrack.faceMargin = Qt.rect(FaceTrack.faceMargin.x,
                                                   Number(value),
                                                   FaceTrack.faceMargin.width,
                                                   FaceTrack.faceMargin.height
                                                   + FaceTrack.faceMargin.y
                                                   - Number(value))
                }
            }
            SpinBox {
                value: FaceTrack.faceMargin.left
                from: 0
                to: 1000
                stepSize: 1
                editable: true
                Layout.columnSpan: 1
                Layout.fillWidth: true
                Accessible.name: qsTr("Margin left")

                onValueChanged: {
                    FaceTrack.faceMargin = Qt.rect(Number(value),
                                                   FaceTrack.faceMargin.y,
                                                   FaceTrack.faceMargin.width
                                                   + FaceTrack.faceMargin.x
                                                   - Number(value),
                                                   FaceTrack.faceMargin.height)
                }
            }
            SpinBox {
                value: FaceTrack.faceMargin.right
                from: 0
                to: 1000
                stepSize: 1
                editable: true
                Layout.columnSpan: 1
                Layout.fillWidth: true
                Accessible.name: qsTr("Margin right")

                onValueChanged: {
                    FaceTrack.faceMargin = Qt.rect(FaceTrack.faceMargin.x,
                                                   FaceTrack.faceMargin.y,
                                                   Number(value) - FaceTrack.faceMargin.x + 1,
                                                   FaceTrack.faceMargin.height)
                }
            }
            SpinBox {
                value: FaceTrack.faceMargin.bottom
                from: 0
                to: 1000
                stepSize: 1
                editable: true
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignHCenter
                Accessible.name: qsTr("Margin bottom")

                onValueChanged: {
                    FaceTrack.faceMargin = Qt.rect(FaceTrack.faceMargin.x,
                                                   FaceTrack.faceMargin.y,
                                                   FaceTrack.faceMargin.width,
                                                   Number(value) - FaceTrack.faceMargin.y + 1)
                }
            }
        }
    }

    // Aspect ratio
    Label {
        id: txtAspectRatio
        text: qsTr("Aspect ratio")
    }
    GridLayout {
        columns: 2
        Layout.fillWidth: true

        TextField {
            text: AkFrac.create(FaceTrack.aspectRatio).string.replace("/", ":")
            placeholderText: qsTr("e.g. 16:9, 4:3")
            selectByMouse: true
            validator: RegExpValidator {
                regExp: /\d+:\d+/
            }
            enabled: FaceTrack.overrideAspectRatio
            Layout.fillWidth: true
            Accessible.name: txtAspectRatio.text

            onTextChanged: {
                FaceTrack.aspectRatio =
                        AkFrac.create(text.replace(":", "/")).toVariant()
            }
        }

        Switch {
            checked: FaceTrack.overrideAspectRatio
            Layout.alignment: Qt.AlignRight
            Accessible.name: txtAspectRatio.text

            onCheckedChanged: FaceTrack.overrideAspectRatio = checked
        }
    }

    // Lock viewport
    Label {
        id: txtLockViewport
        text: qsTr("Lock viewport")
    }
    Switch {
        checked: FaceTrack.lockedViewport
        Layout.alignment: Qt.AlignRight
        Accessible.name: txtLockViewport.text

        onCheckedChanged: FaceTrack.lockedViewport = checked
    }

    // Debug label
    Label {
        id: txtDebugMode
        text: qsTr("Debug mode")
    }
    Switch {
        checked: FaceTrack.debugModeEnabled
        Layout.alignment: Qt.AlignRight
        Accessible.name: txtDebugMode.text

        onCheckedChanged: FaceTrack.debugModeEnabled = checked
    }
}

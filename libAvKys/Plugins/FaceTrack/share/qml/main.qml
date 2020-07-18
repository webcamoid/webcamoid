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
import Qt.labs.platform 1.1 as LABS
import AkControls 1.0 as AK

GridLayout {
    columns: 2

    function haarFileIndex(haarFile)
    {
        var index = -1

        for (var i = 0; i < cbxHaarFile.model.count; i++)
            if (cbxHaarFile.model.get(i).haarFile === haarFile) {
                index = i
                break
            }

        return index
    }

    function strToSize(str)
    {
        if (str.length < 1)
            return Qt.size()

        var size = str.split(RegExp(/x|:/))

        if (size.length < 2)
            return Qt.size()

        return Qt.size(size[0], size[1])
    }

    // Haar file.
    Label {
        //: https://en.wikipedia.org/wiki/Haar-like_feature
        text: qsTr("Haar file")
    }
    ComboBox {
        Layout.fillWidth: true

        id: cbxHaarFile
        textRole: "text"
        currentIndex: haarFileIndex(FaceTrack.haarFile)

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

        onCurrentIndexChanged: FaceTrack.haarFile = cbxHaarFile.model.get(currentIndex).haarFile
    }

    // Scan block.
    Label {
        text: qsTr("Scan block")
    }
    TextField {
        Layout.fillWidth: true

        text: FaceTrack.scanSize.width + "x" + FaceTrack.scanSize.height
        placeholderText: qsTr("Scan block")
        validator: RegExpValidator {
            regExp: /\d+x\d+/
        }

        onAccepted: FaceTrack.scanSize = strToSize(text)
    }

    // Face bucket size
    Label {
        text: qsTr("Face bracketing\nduration (seconds)")
    }
    SpinBox {
        Layout.fillWidth: true

        value: FaceTrack.faceBucketSize
        from: 1
        to: 600
        stepSize: 1
        editable: true

        onValueChanged: FaceTrack.faceBucketSize = Number(value)
    }

    // Face bucket count
    Label {
        text: qsTr("Face bracket count")
    }
    SpinBox {
        Layout.fillWidth: true

        value: FaceTrack.faceBucketCount
        from: 1
        to: 120
        stepSize: 1
        editable: true

        onValueChanged: FaceTrack.faceBucketCount = Number(value)
    }

    // Expand rate
    Label {
        text: qsTr("Zoom out rate")
    }
    SpinBox {
        Layout.fillWidth: true

        value: FaceTrack.expandRate
        from: 1
        to: 100
        stepSize: 1
        editable: true

        onValueChanged: FaceTrack.expandRate = Number(value)
    }

    // Contract rate
    Label {
        text: qsTr("Zoom in rate")
    }
    SpinBox {
        Layout.fillWidth: true

        value: FaceTrack.contractRate
        from: 1
        to: 100
        stepSize: 1
        editable: true

        onValueChanged: FaceTrack.contractRate = Number(value)
    }

    // Face padding
    Label {
        text: qsTr("Face padding\n(% of face size)")
    }
    GridLayout {
        Layout.fillWidth: true

        columns: 2

        SpinBox {
            Layout.columnSpan: 2
            Layout.alignment: Qt.AlignHCenter

            value: FaceTrack.facePadding.top
            from: 0
            to: 1000
            stepSize: 1
            editable: true

            onValueChanged: FaceTrack.facePadding = Qt.rect(-1, Number(value), 1, -Number(value))
        }
        SpinBox {
            Layout.columnSpan: 1
            Layout.fillWidth: true

            value: FaceTrack.facePadding.left
            from: 0
            to: 1000
            stepSize: 1
            editable: true

            onValueChanged: FaceTrack.facePadding = Qt.rect(Number(value), -1, -Number(value), 1)
        }
        SpinBox {
            Layout.columnSpan: 1
            Layout.fillWidth: true

            value: FaceTrack.facePadding.right
            from: 0
            to: 1000
            stepSize: 1
            editable: true

            onValueChanged: FaceTrack.facePadding = Qt.rect(-1, -1, Number(value) + 2, 1)
        }
        SpinBox {
            Layout.columnSpan: 2
            Layout.alignment: Qt.AlignHCenter

            value: FaceTrack.facePadding.bottom
            from: 0
            to: 1000
            stepSize: 1
            editable: true

            onValueChanged: FaceTrack.facePadding = Qt.rect(-1, -1, 1, Number(value) + 2)
        }
    }

    // Face margin
    Label {
        text: qsTr("Face margin\n(% of face size)")
    }
    GridLayout {
        Layout.fillWidth: true

        columns: 2

        SpinBox {
            Layout.columnSpan: 2
            Layout.alignment: Qt.AlignHCenter

            value: FaceTrack.faceMargin.top
            from: 0
            to: 1000
            stepSize: 1
            editable: true

            onValueChanged: FaceTrack.faceMargin = Qt.rect(-1, Number(value), 1, -Number(value))
        }
        SpinBox {
            Layout.columnSpan: 1
            Layout.fillWidth: true

            value: FaceTrack.faceMargin.left
            from: 0
            to: 1000
            stepSize: 1
            editable: true

            onValueChanged: FaceTrack.faceMargin = Qt.rect(Number(value), -1, -Number(value), 1)
        }
        SpinBox {
            Layout.columnSpan: 1
            Layout.fillWidth: true

            value: FaceTrack.faceMargin.right
            from: 0
            to: 1000
            stepSize: 1
            editable: true

            onValueChanged: FaceTrack.faceMargin = Qt.rect(-1, -1, Number(value) + 2, 1)
        }
        SpinBox {
            Layout.columnSpan: 2
            Layout.alignment: Qt.AlignHCenter

            value: FaceTrack.faceMargin.bottom
            from: 0
            to: 1000
            stepSize: 1
            editable: true

            onValueChanged: FaceTrack.faceMargin = Qt.rect(-1, -1, 1, Number(value) + 2)
        }
    }

    // Aspect ratio
    Label {
        text: qsTr("Aspect ratio")
    }
    GridLayout {
        Layout.fillWidth: true

        columns: 2

        TextField {
            Layout.fillWidth: true

            text: FaceTrack.aspectRatio.width + ":" + FaceTrack.aspectRatio.height
            placeholderText: qsTr("e.g. 16:9, 4:3")
            validator: RegExpValidator {
                regExp: /\d+:\d+/
            }
            enabled: FaceTrack.overrideAspectRatio

            onAccepted: FaceTrack.aspectRatio = strToSize(text)
        }

        CheckBox {
            Layout.alignment: Qt.AlignRight

            checked: FaceTrack.overrideAspectRatio

            onCheckedChanged: FaceTrack.overrideAspectRatio = checked
        }
    }

    // Lock viewport
    Label {
        text: qsTr("Lock viewport")
    }
    CheckBox {
        Layout.alignment: Qt.AlignRight

        checked: FaceTrack.lockedViewport

        onCheckedChanged: FaceTrack.lockedViewport = checked
    }

    // Debug label
    Label {
        text: qsTr("Debug mode")
    }
    CheckBox {
        Layout.alignment: Qt.AlignRight

        checked: FaceTrack.debugModeEnabled

        onCheckedChanged: FaceTrack.debugModeEnabled = checked
    }
}

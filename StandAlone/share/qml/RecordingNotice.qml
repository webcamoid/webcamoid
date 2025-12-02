/* Webcamoid, camera capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

Rectangle {
    id: recordingNotice
    color: AkTheme.palette.active.toolTipBase
    border.color: AkTheme.palette.active.dark
    border.width: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
    width: AkUnit.create(128 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
    radius: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    onVisibleChanged: {
        if (visible) {
            recordingTimer.startTime = new Date().getTime()
            recordingTimer.start()
        } else {
            recordingTimer.stop()
        }
    }

    ColumnLayout {
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter

        RowLayout {
            layoutDirection: recordingNotice.rtl? Qt.RightToLeft: Qt.LeftToRight
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            AkColorizedImage {
                id: recordingIcon
                color: AkTheme.palette.active.toolTipText
                source: "image://icons/recording"
                sourceSize: Qt.size(width, height)
                implicitWidth: AkUnit.create(32 * AkTheme.controlScale, "dp").pixels
                implicitHeight: implicitWidth
                asynchronous: true
                mipmap: true
            }
            Label {
                text: qsTr("Recording")
                color: AkTheme.palette.active.toolTipText
                font.bold: true
            }
        }
        Label {
            id: recordingTime
            color: AkTheme.palette.active.toolTipText
            font.bold: true
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        }
    }

    Timer {
        id: recordingTimer
        interval: 500
        repeat: true

        property double startTime: new Date().getTime()

        function pad(x)
        {
            return x < 10? "0" + x: x
        }

        onTriggered: {
            var diffTime = (new Date().getTime() - startTime) / 1000
            diffTime = parseInt(diffTime, 10)

            var ss = diffTime % 60;
            diffTime = (diffTime - ss) / 60;
            var mm = diffTime % 60;
            var hh = (diffTime - mm) / 60;

            recordingTime.text = pad(hh) + ":" + pad(mm) + ":" + pad(ss)
        }
    }

    SequentialAnimation on opacity {
        loops: Animation.Infinite
        running: recordingNotice.visible

        PropertyAnimation {
            easing.type: Easing.OutSine
            to: 0
            duration: 1000
        }
        PropertyAnimation {
            easing.type: Easing.InSine
            to: 1
            duration: 1000
        }
    }
}

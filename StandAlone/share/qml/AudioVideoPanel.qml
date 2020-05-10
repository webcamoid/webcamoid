/* Webcamoid, webcam capture application.
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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

OptionsPanel {
    id: panel
    title: layout.currentIndex < 1?
               qsTr("Audio"):
           layout.currentIndex < 2?
               qsTr("Video"):
           layout.currentIndex < 3?
               "":
               qsTr("Video Source Options")
    edge: Qt.RightEdge

    function previousPage()
    {
        if (layout.currentIndex < 2) {
            close()
        } else {
            closeAndOpen()
            layout.currentIndex = Math.max(-1, layout.currentIndex - 2)
        }
    }

    function openAudioSettings()
    {
        layout.currentIndex = 0
        open()
    }

    function openVideoSettings()
    {
        layout.currentIndex = 1
        open()
    }

    Keys.onEscapePressed: previousPage()
    onActionClicked: previousPage()

    contents: StackLayout {
        id: layout
        clip: true

        AudioOptions {
        }
        VideoOptions {
            onOpenVideoInputOptions: {
                closeAndOpen()
                layout.currentIndex = 3
                videoInputOptions.videoInput = videoInput
            }
        }
        Item {
        }
        VideoInputOptions {
            id: videoInputOptions

            onVideoInputRemoved: {
                closeAndOpen()
                layout.currentIndex -= 2
            }
        }

        onCurrentIndexChanged: {
        }
    }
}

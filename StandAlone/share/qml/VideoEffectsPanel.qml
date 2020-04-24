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
               qsTr("Effects"):
               qsTr("%1 options").arg(effectOptions.effectDescription)

    signal openVideoEffectsDialog()

    function previousPage()
    {
        if (layout.currentIndex < 1) {
            close()
        } else {
            closeAndOpen()
            layout.currentIndex--
        }
    }

    Keys.onEscapePressed: previousPage()
    onActionClicked: previousPage()

    contents: StackLayout {
        id: layout

        VideoEffectsList {
            onOpenVideoEffectsDialog: panel.openVideoEffectsDialog()
            onOpenVideoEffectOptions: {
                closeAndOpen()
                layout.currentIndex = 1
                effectOptions.effectIndex = effectIndex
            }
        }
        VideoEffectOptions {
            id: effectOptions

            onEffectRemoved: {
                closeAndOpen()
                layout.currentIndex--
            }
        }

        onCurrentIndexChanged: {
            if (currentIndex < 1)
                effectOptions.effectIndex = -1
        }
    }
}

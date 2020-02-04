/* Webcamoid, webcam capture application.
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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Templates 2.5 as T
import QtQuick.Controls.impl 2.12
import Ak 1.0

T.Tumbler {
    id: tumbler
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    delegate: Text {
        color: ThemeSettings.colorWindowText
        font: tumbler.font
        opacity: (1.0 - Math.abs(Tumbler.displacement)
                  / (tumbler.visibleItemCount / 2))
                 * (tumbler.enabled? 1: 0.6)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    contentItem: TumblerView {
        id: tumblerView
        implicitWidth: AkUnit.create(60 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(200 * ThemeSettings.controlScale, "dp").pixels
        model: tumbler.model
        delegate: tumbler.delegate

        path: Path {
            startX: tumblerView.width / 2
            startY: -tumblerView.delegateHeight / 2

            PathLine {
                x: tumblerView.width / 2
                y: (tumbler.visibleItemCount + 1)
                   * tumblerView.delegateHeight - tumblerView.delegateHeight / 2
            }
        }

        property real delegateHeight: tumbler.availableHeight / tumbler.visibleItemCount
    }
}

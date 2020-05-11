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
import Ak 1.0

T.Tumbler {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    readonly property color activeLink: AkTheme.palette.active.link
    readonly property color activeWindowText: AkTheme.palette.active.windowText
    readonly property color disabledLink: AkTheme.palette.disabled.link
    readonly property color disabledWindowText: AkTheme.palette.disabled.windowText

    delegate: Text {
        text: modelData
        color: control.enabled?
                   control.activeWindowText:
                   control.disabledWindowText
        linkColor:
            control.enabled?
                control.activeLink:
                control.disabledLink
        font: control.font
        opacity: (1.0 - Math.abs(Tumbler.displacement)
                  / (control.visibleItemCount / 2))
                 * (control.enabled? 1: 0.6)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    contentItem: PathView {
        id: tumblerView
        implicitWidth:
            AkUnit.create(60 * AkTheme.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(200 * AkTheme.controlScale, "dp").pixels
        model: control.model
        delegate: control.delegate
        clip: true
        pathItemCount: control.visibleItemCount + 1
        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5
        dragMargin: width / 2

        path: Path {
            startX: tumblerView.width / 2
            startY: -tumblerView.delegateHeight / 2

            PathLine {
                x: tumblerView.width / 2
                y: (control.visibleItemCount + 1)
                   * tumblerView.delegateHeight - tumblerView.delegateHeight / 2
            }
        }

        property real delegateHeight:
            control.availableHeight / control.visibleItemCount
    }
}

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
import QtQuick.Templates 2.5 as T
import Ak 1.0

T.ProgressBar {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    hoverEnabled: true

    property int animationTime: 3000
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color disabledHighlight: AkTheme.palette.disabled.highlight

    contentItem: Item {
        Rectangle {
            id: progressRect
            color:
                !control.enabled?
                    control.disabledHighlight:
                control.hovered?
                    AkTheme.constShade(control.activeHighlight, 0.1):
                    control.activeHighlight
            implicitHeight: backgroundRect.implicitHeight
            x: control.indeterminate?
                   kx * backgroundRect.width:
                   0
            y: (control.height - height) / 2
            width: control.indeterminate?
                       kw * backgroundRect.width:
                       backgroundRect.width * control.position
            radius: height / 2

            property real kx: 0
            property real kw: 0

            SequentialAnimation {
                running: control.indeterminate && control.enabled
                loops: Animation.Infinite

                onStopped: {
                    progressRect.kx = 0
                    progressRect.kw = 0
                }

                // Advance

                NumberAnimation {
                    target: progressRect
                    property: "kw"
                    to: 1 / 4
                    duration: control.animationTime / 8
                }
                ParallelAnimation {
                    NumberAnimation {
                        target: progressRect
                        property: "kx"
                        to: 1 / 8
                        duration: control.animationTime / 8
                    }
                    NumberAnimation {
                        target: progressRect
                        property: "kw"
                        to: 1 / 2
                        duration: control.animationTime / 8
                    }
                }
                ParallelAnimation {
                    NumberAnimation {
                        target: progressRect
                        property: "kx"
                        to: 1 / 4
                        duration: control.animationTime / 8
                    }
                    NumberAnimation {
                        target: progressRect
                        property: "kw"
                        to: 3 / 4
                        duration: control.animationTime / 8
                    }
                }
                ParallelAnimation {
                    NumberAnimation {
                        target: progressRect
                        property: "kx"
                        to: 1
                        duration: control.animationTime / 8
                    }
                    NumberAnimation {
                        target: progressRect
                        property: "kw"
                        to: 0
                        duration: control.animationTime / 8
                    }
                }

                // Reset

                ParallelAnimation {
                    NumberAnimation {
                        target: progressRect
                        property: "kx"
                        to: 0
                        duration: 0
                    }
                    NumberAnimation {
                        target: progressRect
                        property: "kw"
                        to: 0
                        duration: 0
                    }
                }

                // Go Faster

                NumberAnimation {
                    target: progressRect
                    property: "kw"
                    to: 1 / 4
                    duration: control.animationTime / 16
                }
                ParallelAnimation {
                    NumberAnimation {
                        target: progressRect
                        property: "kx"
                        to: 1 / 8
                        duration: control.animationTime / 16
                    }
                    NumberAnimation {
                        target: progressRect
                        property: "kw"
                        to: 1 / 2
                        duration: control.animationTime / 16
                    }
                }
                ParallelAnimation {
                    NumberAnimation {
                        target: progressRect
                        property: "kx"
                        to: 1 / 4
                        duration: control.animationTime / 16
                    }
                    NumberAnimation {
                        target: progressRect
                        property: "kw"
                        to: 3 / 4
                        duration: control.animationTime / 16
                    }
                }
                ParallelAnimation {
                    NumberAnimation {
                        target: progressRect
                        property: "kx"
                        to: 1
                        duration: control.animationTime / 16
                    }
                    NumberAnimation {
                        target: progressRect
                        property: "kw"
                        to: 0
                        duration: control.animationTime / 16
                    }
                }

                // Reset

                ParallelAnimation {
                    NumberAnimation {
                        target: progressRect
                        property: "kx"
                        to: 0
                        duration: 0
                    }
                    NumberAnimation {
                        target: progressRect
                        property: "kw"
                        to: 0
                        duration: 0
                    }
                }

            }
        }
    }

    background: Rectangle {
        id: backgroundRect
        color: AkTheme.constShade(progressRect.color, 0.0, 0.5)
        implicitWidth:
            AkUnit.create(120 * AkTheme.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
        y: (control.height - height) / 2
        radius: height / 2
        visible: control.visible
    }
}

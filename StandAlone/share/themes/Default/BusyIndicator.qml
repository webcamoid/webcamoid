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

T.BusyIndicator {
    id: busyIndicator
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: AkUnit.create(6 * ThemeSettings.controlScale, "dp").pixels

    contentItem: Item {
        implicitWidth: AkUnit.create(64 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(64 * ThemeSettings.controlScale, "dp").pixels

        Item {
            id: item
            anchors.fill: parent
            opacity: busyIndicator.running? 1: 0

            Behavior on opacity {
                OpacityAnimator {
                    duration: 250
                }
            }

            RotationAnimator {
                target: item
                running: busyIndicator.visible && busyIndicator.running
                from: 0
                to: 360
                loops: Animation.Infinite
                duration: 1500
            }

            Repeater {
                id: repeater
                model: nBalls(ballRadiusFactor, 0.75)

                property real ballRadiusFactor: 0.2

                function nBalls(factor, k=1)
                {
                    let r = factor / (1 - factor)
                    let n = Math.PI / Math.asin(r)

                    return Math.floor(k * n)
                }

                Rectangle {
                    id: ball
                    x: (item.width - width) / 2
                    y: (item.height - height) / 2
                    width: repeater.ballRadiusFactor
                           * Math.min(busyIndicator.width, busyIndicator.height)
                    height: width
                    radius: width / 2
                    color: ThemeSettings.colorPrimary
                    opacity: 1 - index / repeater.count

                    transform: [
                        Translate {
                            x: (Math.min(item.width, item.height) - ball.width) / 2
                        },
                        Rotation {
                            angle: -360 * index / repeater.count
                            origin.x: ball.radius
                            origin.y: ball.radius
                        }
                    ]
                }
            }
        }
    }
}

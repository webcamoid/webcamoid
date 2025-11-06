/* Webcamoid, camera capture application.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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

Item {
    id: root

    property alias model: listModel
    default property alias pages: pagesList.children

    function goBack()
    {
        if (stackView.depth > 1)
            stackView.pop(StackView.Animated)
    }

    StackView {
        id: stackView
        anchors.fill: parent
        clip: true
        focus: true

        pushEnter: Transition {
            ParallelAnimation {
                NumberAnimation {
                    properties: "x"
                    from: stackView.width
                    to: 0
                    duration: 300
                    easing.type: Easing.OutQuad
                }
                NumberAnimation {
                    properties: "opacity"
                    from: 0
                    to: 1
                    duration: 300
                }
            }
        }
        pushExit: Transition {
            ParallelAnimation {
                NumberAnimation {
                    properties: "x"
                    from: 0
                    to: -stackView.width
                    duration: 300
                    easing.type: Easing.OutQuad
                }
                NumberAnimation {
                    properties: "opacity"
                    from: 1
                    to: 0
                    duration: 300
                }
            }
        }
        popEnter: Transition {
            ParallelAnimation {
                NumberAnimation {
                    properties: "x"
                    from: -stackView.width
                    to: 0
                    duration: 300
                    easing.type: Easing.OutQuad
                }
                NumberAnimation {
                    properties: "opacity"
                    from: 0
                    to: 1
                    duration: 300
                }
            }
        }
        popExit: Transition {
            ParallelAnimation {
                NumberAnimation {
                    properties: "x"
                    from: 0
                    to: stackView.width
                    duration: 300
                    easing.type: Easing.OutQuad
                }
                NumberAnimation {
                    properties: "opacity"
                    from: 1
                    to: 0
                    duration: 300
                }
            }
        }

        initialItem: menuComponent
    }

    ListModel {
        id: listModel
    }

    Item {
        id: pagesList
    }

    Component {
        id: menuComponent

        OptionsMenu {
            id: menuRect
            model: listModel

            onSelected: (index) => {
                let option = listModel.get(index)
                stackView.push(option.page, StackView.Animated)
                menuRect.currentIndex = -1
            }

            Component.onCompleted: menuRect.forceActiveFocus()
            onVisibleChanged: if (visible) menuRect.currentIndex = -1
        }
    }

    Component.onCompleted: {
        listModel.clear()

        for (var i = 0; i < pagesList.children.length; ++i) {
            var page = pagesList.children[i]

            if (page.title && page.subtitle && page.icon !== undefined) {
                listModel.append({
                    title: page.title,
                    subtitle: page.subtitle,
                    icon: page.icon,
                    page: page
                })

                if (page.onGoBack)
                    page.onGoBack.connect(root.goBack)

                // Hide the page at the beginning
                page.visible = false
            }
        }

        stackView.forceActiveFocus()
    }
}

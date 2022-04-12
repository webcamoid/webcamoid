/* Webcamoid, webcam capture application.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
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

Container {
    id: container
    implicitWidth: contentItem.childrenRect.width
    implicitHeight: contentItem.childrenRect.height
    focusPolicy: Qt.StrongFocus
    clip: true
    Accessible.name: currentItem? currentItem.text: ""
    Accessible.role: Accessible.MenuBar

    property bool enableHighlight: true

    function setupChildrens() {
        for (var i in contentChildren) {
            contentChildren[i].parent = container

            if (enableHighlight && contentChildren[i].highlighted != null)
                contentChildren[i].highlighted = i == currentIndex

            contentChildren[i].width = container.width

            if (contentChildren[i].onClicked != null)
                contentChildren[i].onClicked.connect((i => () => setCurrentIndex(i))(i))

            onCurrentIndexChanged.connect((i => function () {
                let item = itemAt(i)

                if (enableHighlight && item && item.highlighted != null)
                    item.highlighted = i == currentIndex
            })(i))
            container.onWidthChanged.connect((i => function () {
                var obj = itemAt(i)

                if (obj)
                    obj.width = container.width
            })(i))
        }
    }

    Keys.onUpPressed: {
        if (currentIndex <= 0)
            setCurrentIndex(count - 1)
        else
            decrementCurrentIndex()
    }
    Keys.onDownPressed: {
        if (currentIndex >= count - 1)
            setCurrentIndex(0)
        else
            incrementCurrentIndex()
    }

    Component.onCompleted: setupChildrens()
    onContentChildrenChanged: setupChildrens()
    onCurrentItemChanged:
        if (currentItem)
            currentItem.forceActiveFocus()

    contentItem: ListView {
        id: optionList
        model: container.contentModel
        snapMode: ListView.SnapOneItem
        currentIndex: container.currentIndex
    }
}

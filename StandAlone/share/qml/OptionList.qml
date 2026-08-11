/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls

Container {
    id: container
    implicitWidth: contentItem.childrenRect.width
    implicitHeight: contentItem.childrenRect.height
    focusPolicy: Qt.StrongFocus
    clip: true
    Accessible.name: currentItem? currentItem.text: ""
    Accessible.role: Accessible.MenuBar

    property bool enableHighlight: true
    property var clickHandlers: []

    function setupChildrens() {
        // Desconectamos todos los handlers que conectamos la vez anterior
        for (var j = 0; j < clickHandlers.length; j++) {
            var entry = clickHandlers[j]
            if (entry.item && entry.item.onClicked != null)
                entry.item.onClicked.disconnect(entry.handler)
        }
        clickHandlers = []

        for (var i = 0; i < contentChildren.length; i++) {
            contentChildren[i].parent = container

            if (enableHighlight && contentChildren[i].highlighted != null)
                contentChildren[i].highlighted = i == currentIndex

            contentChildren[i].width = container.width

            if (contentChildren[i].onClicked != null) {
                var handler = (idx => () => setCurrentIndex(idx))(i)
                contentChildren[i].onClicked.connect(handler)
                clickHandlers.push({ item: contentChildren[i], handler: handler })
            }
        }
    }

    // Un único handler para toda la lista
    function updateHighlights() {
        if (!enableHighlight)
            return
        for (var i = 0; i < count; i++) {
            var item = itemAt(i)
            if (item && item.highlighted != null)
                item.highlighted = (i == currentIndex)
        }
    }

    // Un único handler para toda la lista
    function updateWidths() {
        for (var i = 0; i < count; i++) {
            var item = itemAt(i)
            if (item)
                item.width = container.width
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

    Component.onCompleted: {
        setupChildrens()
        onCurrentIndexChanged.connect(updateHighlights)
        onWidthChanged.connect(updateWidths)
    }

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

/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

import QtQuick 2.3
import QtQuick.Controls 1.2

Rectangle
{
    id: recEffects
    width: 800
    height: 480
    color: Qt.rgba(0, 0, 0, 1)
    radius: 2 * recEffects.borderSize
    border.width: 1
    border.color: Qt.rgba(0.5, 0.5, 0.5, 1)

    property variant devices: []
    property variant activeDevices: []
    property variant effects: []
    property variant selected: []
    property real borderSize: 8
    property string currentDeviceId: ""
    property string preview: ""

    signal setEffect(string pluginId, string spaceId)
    signal unsetEffect(string pluginId, string spaceId)
    signal pluginMoved(string spaceId, int from, int to)
    signal pluginConfigureClicked(string pluginId)

    function sortAlphaNoCase(a, b)
    {
        if (a[0].toLowerCase() < b[0].toLowerCase())
            return -1

        if (a[0].toLowerCase() > b[0].toLowerCase())
            return 1

        return 0;
    }

    function listCategories()
    {
        var categories = []
        var cats = []

        for (var effect in recEffects.effects)
        {
            var category = recEffects.effects[effect].category

            if (cats.indexOf(category) < 0)
            {
                categories.push([category, category])
                cats.push(category)
            }
        }

        categories.sort(recEffects.sortAlphaNoCase)

        return [["All", "All"]].concat(categories)
    }

    onDevicesChanged:
    {
        var devices = []

        if (recEffects.activeDevices && recEffects.activeDevices.length > 0)
            recEffects.currentDeviceId = recEffects.activeDevices[0]

        for (var activeDevice in recEffects.activeDevices)
            for (var device in recEffects.devices)
                if (recEffects.activeDevices[activeDevice] == recEffects.devices[device].deviceId)
                    devices.push([recEffects.devices[device].summary, recEffects.devices[device].deviceId])

        devices.sort(recEffects.sortAlphaNoCase)
        cbxDevice.updateOptions(devices)

        if (devices.length > 0)
            recEffects.currentDeviceId = devices[0][1]
    }

    onEffectsChanged:
    {
        cbxEffectsCategory.updateOptions(recEffects.listCategories())
        lsmEffects.clear()
        lsmStack.clear()
        grvEffects.contentHeight = 0
        lsvStack.contentHeight = 0
        var stack = []
        var effects = recEffects.effects.slice()
        var devices = (recEffects.devices)? recEffects.devices.slice(): []
        var effect = 0
        var newEffect = {}
        var prop = 0

        for (var device in devices)
            if (devices[device].deviceId == cbxDevice.currentValue)
            {
                for (effect in effects)
                {
                    if (effects[effect].applyTo.indexOf(cbxDevice.currentValue) >= 0)
                        continue

                    newEffect = {}

                    for (prop in effects[effect])
                        newEffect["prop" + prop.charAt(0).toUpperCase() + prop.slice(1)] = effects[effect][prop]

                    if (cbxEffectsCategory.currentValue == "" ||
                         cbxEffectsCategory.currentValue == "All" ||
                         cbxEffectsCategory.currentValue == effects[effect].category)
                        lsmEffects.append(newEffect)
                }

                for (var devEffect in devices[device].effects)
                    for (effect in effects)
                        if (devices[device].effects[devEffect] == effects[effect].pluginId)
                        {
                            newEffect = {}

                            for (prop in effects[effect])
                                newEffect["prop" + prop.charAt(0).toUpperCase() + prop.slice(1)] = effects[effect][prop]

                            if (cbxEffectsCategory.currentValue == "" ||
                                 cbxEffectsCategory.currentValue == "All" ||
                                 cbxEffectsCategory.currentValue == effects[effect].category)
                                lsmStack.append(newEffect)

                            break
                        }

                break
            }

        grvEffects.currentIndex = 0
        lsvStack.currentIndex = 0

        sldEffects.updateValue()
        sldStack.updateValue()
    }

    Component
    {
        id: cmpEffectDelegate

        Item
        {
            width: 136
            height: 104

            Rectangle
            {
                x: 10
                y: 10
                width: 128
                height: 96
                color: Qt.rgba(0, 0, 0, 1)
                border.color: (recEffects.selected.indexOf(propPluginId) != -1)? Qt.rgba(0.5, 0.5, 1, 1): Qt.rgba(0, 0, 0, 0)
                border.width: 8

                Image
                {
                    id: imgStaticPreview
                    anchors.fill: parent
                    source: propThumbnail
                    opacity: 0.75

                    Rectangle
                    {
                        id: recIs3D
                        width: 20
                        height: 20
                        color: Qt.rgba(0, 0, 0, 1)
                        anchors.right: parent.right
                        anchors.top: parent.top

                        Text
                        {
                            id: txtIs3D
                            color: Qt.rgba(1, 1, 1, 1)
                            text: propIs3D? "3D": "2D"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            anchors.fill: parent
                            font.bold: true
                        }
                    }

                    MouseArea
                    {
                        id: msaEffect
                        anchors.fill: parent
                        hoverEnabled: true

                        onClicked:
                        {
                            var selected = recEffects.selected.slice()

                            if (selected.indexOf(propPluginId) == -1)
                                selected.push(propPluginId)
                            else
                                selected.splice(selected.indexOf(propPluginId), 1)

                            recEffects.selected = selected
                        }

                        onEntered:
                        {
                            imgStaticPreview.opacity = 1
                        }

                        onExited:
                        {
                            imgStaticPreview.opacity = 0.75
                        }

                        onPressed:
                        {
                            txtName.text = propName
                            txtVersion.text = "(" + propVersion + ")"
                            txtSummary.text = propSummary
                            txtCategory.text = propCategory
                            txtLicense.text = propLicense
                            txtAuthor.text = propAuthor

                            var showContact = false

                            for (var effect in recEffects.effects)
                                if (recEffects.effects[effect].pluginId == propPluginId &&
                                    recEffects.effects[effect].applyTo.indexOf(cbxDevice.currentValue) >= 0)
                                {
                                    showContact = true

                                    break
                                }

                            recContact.visible = showContact
                            btnConfigure.enabled = propIsConfigurable
                            btnConfigure.pluginId = propPluginId
                            btnWeb.url = propWebsite
                            btnMail.mail = propMail
                        }
                    }

                    Button
                    {
                        id: btnMove
                        icon: "qrc:/Webcamoid/share/icons/move.svg"
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: 0.01
                        visible: propApplyTo.count > 0

                        property bool moving: false
                        property real oldIndex: 0
                        property real oldPositionY: 0

                        onClicked:
                        {
                            var selected = recEffects.selected.slice()

                            if (selected.indexOf(propPluginId) == -1)
                                selected.push(propPluginId)
                            else
                                selected.splice(selected.indexOf(propPluginId), 1)

                            recEffects.selected = selected
                        }

                        onEntered:
                        {
                            imgStaticPreview.opacity = 1
                            btnMove.opacity = 1
                        }

                        onExited:
                        {
                            imgStaticPreview.opacity = 0.75
                            btnMove.opacity = 0.01
                        }

                        onPressed:
                        {
                            btnMove.oldIndex = lsvStack.pluginIndex(propPluginId)
                            btnMove.oldPositionY = mouseY
                            btnMove.moving = true
                        }

                        onPositionChanged:
                        {
                            if (!btnMove.moving)
                                return

                            var newIndex = Math.round((mouseY - btnMove.oldPositionY) *
                                                      (lsvStack.count / lsvStack.contentHeight) +
                                                      btnMove.oldIndex)

                            if (newIndex < 0)
                                newIndex = 0

                            if (newIndex >= lsvStack.count)
                                newIndex = lsvStack.count - 1

                            if (newIndex != btnMove.oldIndex)
                            {
                                lsmStack.move(btnMove.oldIndex, newIndex, 1)
                                recEffects.pluginMoved(cbxDevice.currentValue, btnMove.oldIndex, newIndex)
                                btnMove.oldIndex = newIndex
                            }
                        }

                        onReleased: btnMove.moving = false
                    }
                }
            }
        }
    }

    Rectangle
    {
        id: recEffectsContainer
        color: Qt.rgba(0, 0, 0, 0)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
        anchors.right: cbxDevice.left
        anchors.rightMargin: 8
        anchors.top: cbxEffectsCategory.bottom
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.leftMargin: 16

        Text
        {
            id: txtEffects
            x: 0
            y: -56
            color: Qt.rgba(1, 1, 1, 1)
            text: qsTr("Effects")
            anchors.horizontalCenter: recEffectsGrid.horizontalCenter
            anchors.top: parent.top
            font.bold: true
        }

        Rectangle
        {
            id: recEffectsGrid
            radius: 4
            anchors.topMargin: 8
            border.color: Qt.rgba(0.25, 0.25, 0.25, 1)
            anchors.right: recStack.left
            anchors.rightMargin: 48
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.top: txtEffects.bottom

            gradient: Gradient
            {
                GradientStop
                {
                    position: 0
                    color: Qt.rgba(0.12, 0.12, 0.12, 1)
                }

                GradientStop
                {
                    position: 1
                    color: Qt.rgba(0, 0, 0, 1)
                }
            }

            GridView
            {
                id: grvEffects
                interactive: false
                clip: true
                anchors.right: sldEffects.left
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.top: parent.top
                cellWidth: 136
                cellHeight: 104

                delegate: cmpEffectDelegate

                model: ListModel
                {
                    id: lsmEffects
                }
            }

            Slider
            {
                id: sldEffects
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.top: parent.top
                showUpDown: true

                function updVal()
                {
                    var tmpNewMaxValue = 0

                    if (grvEffects.count != 0 && grvEffects.contentHeight == 0)
                        grvEffects.contentHeight = grvEffects.count * 104

                    if (grvEffects.contentHeight == 0 && grvEffects.height == 0)
                        tmpNewMaxValue = 0
                    else
                        tmpNewMaxValue = Math.ceil(grvEffects.contentHeight / grvEffects.height - 1)

                    if (tmpNewMaxValue < sldEffects.minValue)
                        sldEffects.maxValue = sldEffects.minValue
                    else
                        sldEffects.maxValue = tmpNewMaxValue

                    var newValue = sldEffects.minValue

                    if (sldEffects.maxValue != sldEffects.minValue)
                    {
                        var k = (tmpNewMaxValue - sldEffects.minValue) / (sldEffects.maxValue - sldEffects.minValue)
                        newValue = Math.ceil(k * (sldEffects.value - sldEffects.minValue) + sldEffects.minValue)
                    }

                    sldEffects.setValue(newValue)
                    sldEffects.visible = sldEffects.maxValue > sldEffects.minValue? true: false
                    sldEffects.width = sldEffects.visible? 16: 0
                }

                onHeightChanged: sldEffects.updVal()

                Component.onCompleted: sldEffects.updateValue()

                onValueChanged:
                {
                    var index = Math.ceil((value - sldEffects.minValue) * (grvEffects.count - 1) /
                                          (sldEffects.maxValue - sldEffects.minValue))

                    grvEffects.positionViewAtIndex(index, ListView.Beginning)
                }
            }
        }

        Rectangle
        {
            id: recEffectsControls
            x: 202
            y: 43
            width: 32
            height: 80
            color: Qt.rgba(0, 0, 0, 0)
            anchors.right: recStack.left
            anchors.rightMargin: 8
            anchors.verticalCenter: recEffectsGrid.verticalCenter

            Button
            {
                id: btnAddEffect
                icon: "qrc:/Webcamoid/share/icons/arrow-right.svg"
                anchors.top: parent.top

                onClicked:
                {
                    var devices = recEffects.devices.slice()
                    var effects = recEffects.effects.slice()
                    var selected = recEffects.selected.slice()

                    for (var plugin in recEffects.selected)
                        for (var effect in effects)
                            if (recEffects.selected[plugin] == effects[effect].pluginId &&
                                effects[effect].applyTo.indexOf(cbxDevice.currentValue) < 0)
                            {
                                recEffects.setEffect(recEffects.selected[plugin], recEffects.currentDeviceId)
                                selected.splice(selected.indexOf(recEffects.selected[plugin]), 1)
                            }

                    recEffects.selected = selected

                    txtName.text = ""
                    txtVersion.text = ""
                    txtSummary.text = ""
                    txtCategory.text = ""
                    txtLicense.text = ""
                    txtAuthor.text = ""
                    recContact.visible = ""
                    btnConfigure.enabled = false
                    btnConfigure.pluginId = ""
                    btnWeb.url = ""
                    btnMail.mail = ""

                    sldEffects.updVal()
                    sldStack.updVal()
                }
            }

            Button
            {
                id: btnRemoveEffect
                icon: "qrc:/Webcamoid/share/icons/arrow-left.svg"
                anchors.bottom: parent.bottom

                onClicked:
                {
                    var devices = recEffects.devices.slice()
                    var effects = recEffects.effects.slice()
                    var selected = recEffects.selected.slice()

                    for (var plugin in recEffects.selected)
                        for (var effect in effects)
                            if (recEffects.selected[plugin] == effects[effect].pluginId &&
                                effects[effect].applyTo.indexOf(cbxDevice.currentValue) >= 0)
                            {
                                recEffects.unsetEffect(recEffects.selected[plugin], recEffects.currentDeviceId)
                                selected.splice(selected.indexOf(recEffects.selected[plugin]), 1)
                            }

                    recEffects.selected = selected

                    txtName.text = ""
                    txtVersion.text = ""
                    txtSummary.text = ""
                    txtCategory.text = ""
                    txtLicense.text = ""
                    txtAuthor.text = ""
                    recContact.visible = ""
                    btnConfigure.enabled = false
                    btnConfigure.pluginId = ""
                    btnWeb.url = ""
                    btnMail.mail = ""

                    sldEffects.updVal()
                    sldStack.updVal()
                }
            }
        }

        Text
        {
            id: txtStack
            color: Qt.rgba(1, 1, 1, 1)
            text: qsTr("Stack")
            anchors.horizontalCenter: recStack.horizontalCenter
            anchors.top: parent.top
            font.bold: true
        }

        Rectangle
        {
            id: recStack
            x: 360
            y: 22
            width: 158
            height: 386
            radius: 4
            anchors.topMargin: 8
            border.color: Qt.rgba(0.25, 0.25, 0.25, 1)
            anchors.bottom: parent.bottom
            anchors.top: txtStack.bottom
            anchors.right: parent.right

            gradient: Gradient
            {
                GradientStop
                {
                    position: 0
                    color: Qt.rgba(0.12, 0.12, 0.12, 1)
                }

                GradientStop
                {
                    position: 1
                    color: Qt.rgba(0, 0, 0, 1)
                }
            }

            ListView
            {
                id: lsvStack
                interactive: false
                clip: true
                anchors.right: sldStack.left
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.top: parent.top

                function pluginIndex(pluginId)
                {
                    var prevIndex = lsvStack.currentIndex

                    for (var effect = 0; effect < lsvStack.count; effect++)
                    {
                        lsvStack.currentIndex = effect

                        if (lsmStack.get(lsvStack.currentIndex).propPluginId == pluginId)
                        {
                            lsvStack.currentIndex = prevIndex

                            return effect
                        }
                    }

                    lsvStack.currentIndex = prevIndex

                    return -1
                }

                delegate: cmpEffectDelegate

                model: ListModel
                {
                    id: lsmStack
                }
            }

            Slider
            {
                id: sldStack
                anchors.bottom: parent.bottom
                anchors.top: parent.top
                anchors.right: parent.right
                showUpDown: true

                function updVal()
                {
                    var tmpNewMaxValue = 0

                    if (lsvStack.count != 0 && lsvStack.contentHeight == 0)
                        lsvStack.contentHeight = lsvStack.count * 104

                    if (lsvStack.contentHeight == 0 && lsvStack.height == 0)
                        tmpNewMaxValue = 0
                    else
                        tmpNewMaxValue = Math.ceil(lsvStack.contentHeight / lsvStack.height - 1)

                    if (tmpNewMaxValue < sldStack.minValue)
                        sldStack.maxValue = sldStack.minValue
                    else
                        sldStack.maxValue = tmpNewMaxValue

                    var newValue = sldStack.minValue

                    if (sldStack.maxValue != sldStack.minValue)
                    {
                        var k = (tmpNewMaxValue - sldStack.minValue) / (sldStack.maxValue - sldStack.minValue)
                        newValue = Math.ceil(k * (sldStack.value - sldStack.minValue) + sldStack.minValue)
                    }

                    sldStack.setValue(newValue)
                    sldStack.visible = sldStack.maxValue > sldStack.minValue? true: false
                    sldStack.width = sldStack.visible? 16: 0
                }

                onHeightChanged: sldStack.updVal()

                Component.onCompleted: sldStack.updateValue()

                onValueChanged:
                {
                    var index = Math.ceil((value - sldStack.minValue) * (lsvStack.count - 1) /
                                          (sldStack.maxValue - sldStack.minValue))

                    lsvStack.positionViewAtIndex(index, ListView.Beginning)
                }
            }
        }
    }

    ComboBox
    {
        id: cbxEffectsCategory
        height: 32
        anchors.top: parent.top
        anchors.topMargin: 16
        anchors.right: cbxDevice.left
        anchors.rightMargin: 8
        anchors.left: parent.left
        anchors.leftMargin: 16

        onItemSelected:
        {
            var effects = recEffects.effects.slice()
            recEffects.effects = []
            recEffects.effects = effects
        }
    }


    Text
    {
        id: txtPreview
        color: Qt.rgba(1, 1, 1, 1)
        text: qsTr("Preview")
        anchors.top: cbxDevice.bottom
        anchors.topMargin: 8
        anchors.horizontalCenter: cbxDevice.horizontalCenter
        font.bold: true
    }

    Image
    {
        id: imgLivePreview
        x: 528
        y: 82
        width: 256
        height: 175
        fillMode: Image.PreserveAspectFit
        anchors.top: txtPreview.bottom
        anchors.topMargin: 8
        anchors.horizontalCenter: txtPreview.horizontalCenter
        source: recEffects.preview
    }

    Rectangle
    {
        id: recEffectInfo
        visible: (txtName.text == "")? false: true
        x: 556
        width: 256
        color: Qt.rgba(0, 0, 0, 0)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
        anchors.top: imgLivePreview.bottom
        anchors.topMargin: 8
        anchors.horizontalCenter: cbxDevice.horizontalCenter

        Text
        {
            id: txtName
            color: Qt.rgba(1, 1, 1, 1)
            anchors.left: parent.left
            anchors.top: parent.top
            font.bold: true
        }

        Text
        {
            id: txtVersion
            color: Qt.rgba(1, 1, 1, 1)
            anchors.leftMargin: 8
            anchors.top: parent.top
            anchors.left: txtName.right
        }

        Text
        {
            id: txtSummary
            color: Qt.rgba(1, 1, 1, 1)
            anchors.topMargin: 16
            anchors.top: txtName.bottom
            anchors.right: parent.right
            anchors.left: parent.left
            wrapMode: Text.WordWrap
        }

        Image
        {
            id: imgCategory
            anchors.topMargin: 16
            anchors.top: txtSummary.bottom
            anchors.left: parent.left
            source: "qrc:/Webcamoid/share/icons/category.svg"
        }

        Text
        {
            id: txtCategory
            y: 71
            color: Qt.rgba(1, 1, 1, 1)
            anchors.left: imgCategory.right
            anchors.leftMargin: 8
            anchors.verticalCenter: imgCategory.verticalCenter
            verticalAlignment: Text.AlignVCenter
        }

        Image
        {
            id: imgLicense
            anchors.top: imgCategory.bottom
            anchors.topMargin: 8
            anchors.left: parent.left
            source: "qrc:/Webcamoid/share/icons/license.svg"
        }

        Text
        {
            id: txtLicense
            y: 98
            color: Qt.rgba(1, 1, 1, 1)
            anchors.left: imgLicense.right
            anchors.leftMargin: 8
            anchors.verticalCenter: imgLicense.verticalCenter
            verticalAlignment: Text.AlignVCenter
        }

        Image
        {
            id: imgAuthor
            anchors.top: imgLicense.bottom
            anchors.topMargin: 8
            anchors.left: parent.left
            source: "qrc:/Webcamoid/share/icons/author.svg"
        }

        Text
        {
            id: txtAuthor
            y: 129
            color: Qt.rgba(1, 1, 1, 1)
            anchors.left: imgAuthor.right
            anchors.leftMargin: 8
            anchors.verticalCenter: imgAuthor.verticalCenter
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle
        {
            id: recContact
            width: 128
            height: 32
            color: Qt.rgba(0, 0, 0, 0)
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter

            Button
            {
                id: btnConfigure
                anchors.left: parent.left
                icon: "qrc:/Webcamoid/share/icons/configure.svg"

                property string pluginId: ""

                onClicked: recEffects.pluginConfigureClicked(btnConfigure.pluginId)
            }

            Button
            {
                id: btnWeb
                anchors.horizontalCenter: parent.horizontalCenter
                icon: "qrc:/Webcamoid/share/icons/web.svg"

                property string url: ""

                onClicked: Qt.openUrlExternally(btnWeb.url)
            }

            Button
            {
                id: btnMail
                anchors.right: parent.right
                icon: "qrc:/Webcamoid/share/icons/mail.svg"

                property string mail: ""

                onClicked: Qt.openUrlExternally(btnMail.mail)
            }
        }
    }

    ComboBox
    {
        id: cbxDevice
        x: 556
        width: 256
        height: 32
        anchors.top: parent.top
        anchors.topMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 16

        onItemSelected:
        {
            recEffects.currentDeviceId = value

            var effects = recEffects.effects.slice()
            recEffects.effects = []
            recEffects.effects = effects
        }
    }
}

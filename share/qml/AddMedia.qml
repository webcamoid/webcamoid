import QtQuick 2.0
import QtQuick.Window 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.2

Window {
    id: recAddMedia
    x: (Screen.desktopAvailableWidth - width) / 2
    y: (Screen.desktopAvailableHeight - height) / 2
    title: qsTr("Add new media")
    color: pallete.window
    width: 200
    height: 150
    flags: Qt.Dialog
    modality: Qt.ApplicationModal

    property bool editMode: false

    function defaultDescription(url)
    {
        return Webcamoid.streams.indexOf(url) < 0?
                    Webcamoid.fileNameFromUri(url):
                    Webcamoid.streamDescription(url)
    }

    SystemPalette {
        id: pallete
    }

    Label {
        id: lblDescription
        color: Qt.rgba(1, 1, 1, 1)
        text: qsTr("Description")
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.topMargin: 8
        font.bold: true
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
    }

    TextField {
        id: txtDescription
        anchors.topMargin: 8
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.top: lblDescription.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        placeholderText: qsTr("Insert media description")
        text: recAddMedia.editMode? Webcamoid.streamDescription(Webcamoid.curStream): ""
    }

    Label {
        id: lblMedia
        color: Qt.rgba(1, 1, 1, 1)
        text: qsTr("Media file")
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.topMargin: 8
        font.bold: true
        anchors.top: txtDescription.bottom
        anchors.right: parent.right
        anchors.left: parent.left
    }

    TextField {
        id: txtMedia
        anchors.right: btnAddMedia.left
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.topMargin: 8
        anchors.top: lblMedia.bottom
        anchors.left: parent.left
        placeholderText: qsTr("Select media file")
        text: recAddMedia.editMode? Webcamoid.curStream: ""
    }

    Button {
        id: btnAddMedia
        width: 30
        text: qsTr("...")
        anchors.top: lblMedia.bottom
        anchors.topMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 8

        onClicked: fileDialog.open()
    }

    Row {
        id: rowControls
        layoutDirection: Qt.RightToLeft
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8

        Button {
            id: btnCancel
            iconName: "cancel"
            text: qsTr("Cancel")

            onClicked: recAddMedia.visible = false
        }

        Button {
            id: btnOk
            iconName: "ok"
            text: qsTr("Ok")

            onClicked: {
                if (txtMedia.text.length > 0) {
                    if (recAddMedia.editMode
                        && Webcamoid.curStream !== txtMedia.text.toString())
                        Webcamoid.removeStream(Webcamoid.curStream)

                    if (txtDescription.text.length < 1)
                        txtDescription.text = recAddMedia.defaultDescription(txtMedia.text)

                    Webcamoid.setStream(txtMedia.text, txtDescription.text)
                    Webcamoid.curStream = txtMedia.text
                }

                recAddMedia.visible = false
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Choose the file to add as media")
        selectExisting : true
        selectFolder : false
        selectMultiple : false
        selectedNameFilter: nameFilters[nameFilters.length - 2]
        nameFilters: ["3GP Video (*.3gp)",
                      "AVI Video (*.avi)",
                      "Flash Video (*.flv)",
                      "Animated GIF (*.gif)",
                      "MKV Video (*.mkv)",
                      "Animated PNG (*.mng)",
                      "QuickTime Video (*.mov)",
                      "MP4 Video (*.mp4 *.m4v)",
                      "MPEG Video (*.mpg *.mpeg)",
                      "Ogg Video (*.ogg)",
                      "RealMedia Video (*.rm)",
                      "DVD Video (*.vob)",
                      "WebM Video (*.webm)",
                      "Windows Media Video (*.wmv)",
                      "All Video Files (*.3gp *.avi *.flv *.gif *.mkv *.mng"
                      + " *.mov *.mp4 *.m4v *.mpg *.mpeg *.ogg *.rm *.vob"
                      + " *.webm *.wmv)",
                      "All Files (*)"]

        onAccepted: {
            txtMedia.text = fileDialog.fileUrl
            txtDescription.text = recAddMedia.defaultDescription(fileDialog.fileUrl.toString())
        }
    }
}

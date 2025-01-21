// DropArea.qml
import QtQuick
import QtQuick.Controls
import Qt.labs.platform
import gdl.sdk
import "../Utils/utils.js" as Utils
Item {
    id: control
    property string path: ""
    signal accepted()
    Rectangle {
        id: dropArea
        property bool hoverd: false
        anchors.fill: parent
        color: GTheme.dark ? "#282828" : "#ffffff"

        Canvas {
            id:canvas
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(1, 1, width-2, height-2, 4, 4)
                ctx.setLineDash([5, 5])
                ctx.strokeStyle =  (dropZone.containsDrag || dropArea.hoverd) ? "#5151f9" : (GTheme.dark ? "#555555" : "#d4d4d4")
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.roundedRect(1, 1, width-2, height-2, 4, 4)
                ctx.stroke()

            }
            Connections {
                target: dropZone
                function onContainsDragChanged() {

                    canvas.requestPaint()
                }
            }
        }

        Image {
            id: icon
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -10
            source: "/images/taskdialog/drop-image-icon.svg"
            width: 32
            height: 32
        }

        Text {
            anchors.top: icon.bottom
            anchors.topMargin: 10
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Drag torrent file here, or click to select"
            color: GTheme.dark ? "#939495" : "#666666"
        }

        DropArea {
            id: dropZone
            anchors.fill: parent

            onEntered: (drag) => {
                if (drag.hasUrls) {
                    drag.accept()
                }
            }

            onDropped: (drop) => {
                if (drop.hasUrls) {
                    let fileUrl = drop.urls[0]
                    console.log("Dropped file:", fileUrl)
                    handleFile(fileUrl)
                }
            }
        }

        MouseArea {
            id:mouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: fileDialog.open()
            onEntered: {
                dropArea.hoverd = true
                canvas.requestPaint()
            }
            onExited: {
                dropArea.hoverd = false
                canvas.requestPaint()
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Please choose a torrent file"
        nameFilters: ["Torrent files (*.torrent)","Metalink Files (*.metalink)","Meta4 Files (*.meta4)"]
        onAccepted: {
            handleFile(fileDialog.currentFile)
        }
    }

    function handleFile(fileUrl) {
        path = Utils.urlToLocalPath(fileUrl)
        console.log("Processing file:", path)
        control.accepted()
    }
}

import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk
Rectangle {
    id: titleBar
    width: parent.width
    color: GTheme.dark ? "#242424" : "#ffffff"
    height: 32
    visible: Qt.platform.os === "osx" ? (mainWindow.fullScreen ? false : true) : true
    RowLayout{
        spacing: 5
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 10
        anchors.topMargin: 10
        ImageButton{
            id:close
            width: 8
            height: 8
            backgroundColor:"transparent"
            normalImage: "/images/titlebar/macos-close.svg"
            hoverImage: "/images/titlebar/macos-clos-hover.svg"
            onClicked: {
                Qt.quit()
            }
        }

        ImageButton{
            id:minsize
            width: 8
            height: 8
            backgroundColor:"transparent"
            normalImage: "/images/titlebar/macos-minimize.svg"
            hoverImage: "/images/titlebar/macos-minimize-hover.svg"
            onClicked: {
                mainWindow.showMinimized2()
            }
        }

        ImageButton{
            id:maxsize
            width: 8
            height: 8
            backgroundColor:"transparent"
            normalImage: "/images/titlebar/macos-maximize.svg"
            hoverImage: "/images/titlebar/macos-maximize-hover.svg"
            onClicked: {
                mainWindow.toggleFullScreen()
            }
        }
    }

}

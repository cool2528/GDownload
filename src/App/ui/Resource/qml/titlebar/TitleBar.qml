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
    Component.onCompleted: {
       if(Qt.platform.os !== "osx"){
           helper.setHitTestVisible(win_close)
           helper.setHitTestVisible(win_minsize)
           helper.setHitTestVisible(win_maxsize)
           helper.setHitTestVisible(win_restore)
       }
    }

    RowLayout{
        id:macosTitleBar
        spacing: 5
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 10
        anchors.topMargin: 10
        visible: Qt.platform.os === "osx"
        ImageButton{
            id:close
            Layout.preferredHeight: 15
            Layout.preferredWidth: 15
            backgroundColor:"transparent"
            normalImage: "/images/titlebar/macos-close.svg"
            hoverImage: "/images/titlebar/macos-clos-hover.svg"
            onClicked: {
                Qt.quit()
            }
        }

        ImageButton{
            id:minsize
            Layout.preferredHeight: 15
            Layout.preferredWidth: 15
            backgroundColor:"transparent"
            normalImage: "/images/titlebar/macos-minimize.svg"
            hoverImage: "/images/titlebar/macos-minimize-hover.svg"
            onClicked: {
                mainWindow.showMinimized2()
            }
        }

        ImageButton{
            id:maxsize
            Layout.preferredHeight: 15
            Layout.preferredWidth: 15
            backgroundColor:"transparent"
            normalImage: "/images/titlebar/macos-maximize.svg"
            hoverImage: "/images/titlebar/macos-maximize-hover.svg"
            onClicked: {
                mainWindow.toggleFullScreen()
            }
        }
    }

    // windows title bar
    RowLayout{
        id:winTitleBar
        spacing: 0
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: Qt.platform.os === "windows"
        ImageButton{
            id:win_minsize
            Layout.preferredHeight: 32
            Layout.preferredWidth: 40
            imageSize:Qt.size(16,16)
            backgroundColor: hovered ? "#c0c0c0" : "transparent"
            normalImage: "/images/titlebar/windows-minimize.svg"
            hoverImage: "/images/titlebar/windows-minimize-hover.svg"
            onClicked: {
                mainWindow.showMinimized2()
            }
        }
        ImageButton{
            id:win_maxsize
            visible: !mainWindow.maximized
            Layout.preferredHeight: 32
            Layout.preferredWidth: 40
            imageSize:Qt.size(16,16)
            backgroundColor: hovered ? "#c0c0c0" : "transparent"
            normalImage: "/images/titlebar/windows-maximize.svg"
            hoverImage: "/images/titlebar/windows-maximize-hover.svg"
            onClicked: {
                mainWindow.toggleMaximized()
            }
        }
        //windows-restore

        ImageButton{
            id:win_restore
             visible: mainWindow.maximized
            Layout.preferredHeight: 32
            Layout.preferredWidth: 40
            imageSize:Qt.size(16,16)
            backgroundColor: hovered ? "#c0c0c0" : "transparent"
            normalImage: "/images/titlebar/windows-restore.svg"
            hoverImage: "/images/titlebar/windows-restore-hover.svg"
            onClicked: {
                mainWindow.toggleMaximized()
            }
        }
        ImageButton{
            id:win_close
            Layout.preferredHeight: 32
            Layout.preferredWidth: 40
            imageSize:Qt.size(16,16)
            backgroundColor: hovered ? "#e81123" :"transparent"
            normalImage: "/images/titlebar/windows-close.svg"
            hoverImage: "/images/titlebar/windows-close-hover.svg"
            onClicked: {
                Qt.quit()
            }
        }



    }

}

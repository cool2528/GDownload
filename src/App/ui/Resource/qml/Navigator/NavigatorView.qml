import QtQuick
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0
import QtQuick.Layouts

Item{
    id:navigator
    Rectangle{
        id:systemNavigator
        anchors.fill: parent
        gradient: Gradient{
            // 使用 ElementPlusColors 提供的 QML 方法（深色中性色）
            GradientStop{ position: 0.0;  color: ElementPlusColors.fillBase(true) }
            GradientStop{ position: 0.5;  color: ElementPlusColors.fillLight(true) }
            GradientStop{ position: 1.0;  color: ElementPlusColors.fillLighter(true) }
        }

        // 右侧分隔线，增强分区感
        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: GTheme.borderLight
        }

        implicitWidth: 74
        SplitView.minimumWidth: 74
        // topLayout
        ColumnLayout{
            id:topLayout
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.left: parent.left
            anchors.leftMargin: 20
            spacing: 30
            IconButton{
                id:home
                Layout.minimumHeight: 30
                Layout.maximumHeight: 30
                Layout.fillWidth: true
                iconSource: SegoeFluentIcons.HomeSolid
                iconSize: 30
                iconColor: hovered ? GTheme.primaryColor : ElementPlusColors.bgWhite(false)
                onClicked: {
                    Qt.openUrlExternally("https://github.com/cool2528/GDownload")

                }
            }
            IconButton{
                id:download
                Layout.minimumHeight: 30
                Layout.maximumHeight: 30
                Layout.fillWidth: true
                iconSource: SegoeFluentIcons.SubscriptionAdd
                iconSize: 30
                iconColor: hovered ? GTheme.primaryColor : ElementPlusColors.bgWhite(false)
                onClicked: {
                    console.debug("select download page")
                    brower_view.index = 0
                    //updateDialog.open()
                }
            }

            ImageButton{
                id:addTask
                Layout.minimumHeight: 30
                Layout.maximumHeight: 30
                Layout.fillWidth: true
                //backgroundColor:"red"
                hoverImage: "/images/navigator/menu-add_hover.svg"
                normalImage: "/images/navigator/menu-add.svg"
                imageSize: Qt.size(30,30)
                onClicked: {
                    console.debug("open add task dialog")
                    let task = addDownloadTask()
                    if(task){
                        task.open()
                    }
                }
            }
        }
        //bottomLayout
        ColumnLayout{
            id:bottomLayout
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
            anchors.left: parent.left
            anchors.leftMargin: 20
            spacing: 10
            IconButton{
                id:setting
                iconSource: SegoeFluentIcons.SettingsSolid
                iconSize: 30
                iconColor: hovered ? GTheme.primaryColor : ElementPlusColors.bgWhite(false)
                onClicked: {
                    console.debug("open settings dialog")
                    brower_view.index = 1
                }
            }
            IconButton{
                id:help
                Layout.topMargin: 40
                iconSource: SegoeFluentIcons.Info
                iconSize: 30
                iconColor: hovered ? GTheme.primaryColor : ElementPlusColors.bgWhite(false)
                onClicked: {
                    console.debug("open help dialog")
                    let about = showAboutDialog()
                    if(about){
                        about.open()
                    }
                }
            }
        }

    }

    function addDownloadTask(){
        let component = Qt.createComponent("qrc:/qml/CommonComponents/TaskDialogPage.qml")
        if(component.status === Component.Error){
            console.error("Error loading component:", component.errorString());
            return null;
        }
        let task = component.createObject(mainWindow)
        if(task === null){
            console.error("Error creating object")
            return null
        }
        return task
    }

    function showAboutDialog(){
        let component = Qt.createComponent("qrc:/qml/CommonComponents/HelpDialog.qml")
        if(component.status === Component.Error){
            console.error("Error loading component:", component.errorString());
            return null;
        }
        let about = component.createObject(mainWindow)
        if(about === null){
            console.error("Error creating object")
            return null
        }
        return about
    }



}

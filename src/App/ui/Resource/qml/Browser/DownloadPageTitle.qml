import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk 1.0

// Element Plus 风格下载页面标题栏
Rectangle {
    id: control
    property int type: 0
    implicitHeight: 64
    color: GTheme.bgWhite

    // Element Plus 设计标准
    readonly property int standardSpacing: 16
    readonly property int buttonSize: 32
    readonly property int iconSize: 16

    // 底部分隔线
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: GTheme.borderLight
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: control.standardSpacing + 8  // 额外间距与左侧导航对齐
        anchors.rightMargin: control.standardSpacing + 8
        spacing: control.standardSpacing

        // 标题区域
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: {
                    switch(control.type) {
                        case 0: return qsTr("Downloading")
                        case 1: return qsTr("Waiting")
                        case 2: return qsTr("Stopped")
                        default: return qsTr("Downloads")
                    }
                }
                font.pixelSize: 18
                font.weight: Font.DemiBold
                color: GTheme.textPrimary
            }

            Text {
                text: {
                    switch(control.type) {
                        case 0: return qsTr("Active download tasks")
                        case 1: return qsTr("Queued download tasks")
                        case 2: return qsTr("Completed or stopped tasks")
                        default: return qsTr("Manage your downloads")
                    }
                }
                font.pixelSize: 12
                color: GTheme.textSecondary
                visible: text.length > 0
            }
        }

        // 操作按钮区域 - 全部靠右
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8

            // 恢复/开始所有按钮
            IconButton {
                visible: control.type !== 2
                iconSource: SegoeFluentIcons.PlayBadge12
                iconSize: control.iconSize
                Layout.preferredWidth: control.buttonSize
                Layout.preferredHeight: control.buttonSize
                iconColor: hovered ? GTheme.primaryColor : (GTheme.dark ? GTheme.textPrimary : GTheme.textSecondary)
                backgroundColor: hovered ? GTheme.fillLight : "transparent"
                onClicked: {
                    BrowserManager.UnpauseAllTask(control.type)
                }
            }

            // 暂停所有按钮
            IconButton {
                visible: control.type !== 2
                iconSource: SegoeFluentIcons.PauseBadge12
                iconSize: control.iconSize
                Layout.preferredWidth: control.buttonSize
                Layout.preferredHeight: control.buttonSize
                iconColor: hovered ? GTheme.primaryColor : (GTheme.dark ? GTheme.textPrimary : GTheme.textSecondary)
                backgroundColor: hovered ? GTheme.fillLight : "transparent"
                onClicked: {
                    BrowserManager.PauseAllTask(control.type)
                }
            }

            // 分隔线
            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 20
                color: GTheme.borderLight
                Layout.alignment: Qt.AlignVCenter
            }

            // 刷新按钮
            IconButton {
                iconSource: SegoeFluentIcons.Refresh
                iconSize: control.iconSize
                Layout.preferredWidth: control.buttonSize
                Layout.preferredHeight: control.buttonSize
                iconColor: hovered ? GTheme.primaryColor : (GTheme.dark ? GTheme.textPrimary : GTheme.textSecondary)
                backgroundColor: hovered ? GTheme.fillLight : "transparent"
                onClicked: {
                    BrowserManager.RefreshTaskList(control.type)
                }
            }

            // 删除按钮
            IconButton {
                iconSource: SegoeFluentIcons.Delete
                iconSize: control.iconSize
                Layout.preferredWidth: control.buttonSize
                Layout.preferredHeight: control.buttonSize
                iconColor: hovered ? GTheme.dangerColor : (GTheme.dark ? GTheme.textPrimary : GTheme.textSecondary)
                backgroundColor: hovered ? GTheme.fillLight : "transparent"
                onClicked: {
                    BrowserManager.RemoveAllTask(control.type, true)
                }
            }
        }
    }
}

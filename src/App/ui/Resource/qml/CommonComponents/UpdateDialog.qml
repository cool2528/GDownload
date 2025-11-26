import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import gdl.sdk

// Element Plus 风格更新对话框
Popup {
    id: updateDialog
    width: 560
    height: Math.min(680, contentLayout.implicitHeight + standardPadding * 2)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    closePolicy: Popup.CloseOnEscape
    visible: false

    // Element Plus 设计标准
    readonly property int standardPadding: 24
    readonly property int standardSpacing: 16
    readonly property int buttonHeight: 32
    readonly property int headerHeight: 64

    // 属性定义
    property string versionNumber: ""
    property string releaseNotes: ""
    property bool updating: false

    // Element Plus 风格背景
    background: Rectangle {
        color: GTheme.bgWhite
        radius: 8
        border.width: 1
        border.color: GTheme.borderLight

        // Element Plus 风格阴影
        layer.enabled: true
        layer.effect: DropShadow {
            radius: 16
            samples: 33
            color: Qt.rgba(0, 0, 0, 0.1)
            horizontalOffset: 0
            verticalOffset: 4
        }
    }

    // 内容区域
    contentItem: ColumnLayout {
        id: contentLayout
        spacing: 0

        // 头部区域
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: updateDialog.headerHeight
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: updateDialog.standardPadding
                anchors.rightMargin: updateDialog.standardPadding
                spacing: updateDialog.standardSpacing

                // 图标区域
                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    Layout.alignment: Qt.AlignVCenter
                    color: GTheme.successLight(9)
                    radius: 8

                    FontIcon {
                        anchors.centerIn: parent
                        iconSource: SegoeFluentIcons.CloudDownload
                        iconSize: 20
                        color: GTheme.successColor
                    }
                }

                // 标题和版本信息
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: qsTr("New Version Available")
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        color: GTheme.textPrimary
                    }

                    Text {
                        text: versionNumber
                        font.pixelSize: 14
                        color: GTheme.textSecondary
                        visible: versionNumber.length > 0
                    }
                }

                // 关闭按钮
                IconButton {
                    iconSource: SegoeFluentIcons.ChromeClose
                    iconSize: 14
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    iconColor: hovered ? GTheme.textPrimary : GTheme.textSecondary
                    backgroundColor: hovered ? GTheme.fillLight : "transparent"
                    onClicked: updateDialog.close()
                }
            }
        }

        // 分隔线
        Divider {
            Layout.fillWidth: true
        }

        // 内容区域
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 300
            Layout.maximumHeight: 400
            Layout.topMargin: updateDialog.standardSpacing
            Layout.bottomMargin: updateDialog.standardSpacing
            Layout.leftMargin: updateDialog.standardPadding
            Layout.rightMargin: updateDialog.standardPadding

            ColumnLayout {
                anchors.fill: parent
                spacing: updateDialog.standardSpacing

                // 更新说明标题
                Text {
                    text: qsTr("Release Notes")
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: GTheme.textPrimary
                    Layout.fillWidth: true
                }

                // 发布说明内容
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    background: Rectangle {
                        color: GTheme.fillLighter
                        border.width: 1
                        border.color: GTheme.borderLight
                        radius: 6
                    }

                    ScrollText {
                        id: releaseNotesText
                        width: parent.width
                        text: releaseNotes
                        textFormat: TextEdit.MarkdownText
                        promptPage: true
                        padding: updateDialog.standardSpacing
                    }
                }

                // GitHub 加速开关
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    GButtonSwitch {
                        id: githubAccelerateSwitch
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        text: qsTr("Enable GitHub Accelerated Download")
                        checked: SettingsManager.qEnableGithubAccelerate
                        onClicked: SettingsManager.SetEnableGithubAccelerate(checked)
                    }

                    Text {
                        text: qsTr("Use a GitHub mirror (ghproxy) when fetching update packages.")
                        font.pixelSize: 12
                        color: GTheme.textSecondary
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                // 更新进度区域
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: updating

                    Text {
                        id: updateTip
                        text: qsTr("Updating...")
                        font.pixelSize: 14
                        color: GTheme.textSecondary
                        Layout.fillWidth: true
                    }

                    GProgressBar {
                        id: updateProgressBar
                        Layout.fillWidth: true
                        Layout.preferredHeight: 6
                        from: 0
                        to: 100
                        value: 0
                    }
                }
            }
        }

        // 分隔线
        Divider {
            Layout.fillWidth: true
        }

        // 底部按钮区域
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: updateDialog.headerHeight
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: updateDialog.standardPadding
                anchors.rightMargin: updateDialog.standardPadding
                spacing: updateDialog.standardSpacing

                // 跳转下载按钮
                RowLayout {
                    spacing: 8

                    IconButton {
                        iconSource: SegoeFluentIcons.Globe
                        iconSize: 16
                        Layout.preferredWidth: updateDialog.buttonHeight
                        Layout.preferredHeight: updateDialog.buttonHeight
                        iconColor: GTheme.primaryColor
                        backgroundColor: "transparent"
                        onClicked: {
                            Qt.openUrlExternally("https://github.com/cool2528/GDownload/releases")
                        }
                    }

                    GButton {
                        text: qsTr("Go to Download Page")
                        Layout.preferredHeight: updateDialog.buttonHeight
                        onClicked: {
                            Qt.openUrlExternally("https://github.com/cool2528/GDownload/releases")
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                // 取消按钮
                GButton {
                    text: qsTr("Cancel")
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: updateDialog.buttonHeight
                    visible: !updating
                    onClicked: updateDialog.close()
                }

                // 更新按钮
                GButton {
                    type: 1  // Primary
                    text: qsTr("Update Now")
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: updateDialog.buttonHeight
                    visible: !updating
                    onClicked: {
                        if (UpdateManager.StartUpdate()) {
                            updating = true
                        }
                    }
                }
            }
        }
    }

    // 打开动画
    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "scale"
                from: 0.9
                to: 1.0
                duration: 200
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
    }

    // 关闭动画
    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.9
                duration: 150
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 150
                easing.type: Easing.InCubic
            }
        }
    }

    // 连接更新管理器信号
    Connections {
        target: UpdateManager

        function onUpdateAvailable(info) {
            versionNumber = "v" + info.version
            releaseNotes = info.release_note
            updating = false
            updateDialog.open()
        }

        function onUpdateProgress(progress) {
            updateProgressBar.value = progress.percentage

            // stage 0 = 检查更新 1 = 下载更新 2 = 解压更新 3 = 验证更新 4 = 安装更新 5 = 下载完成 6 = 更新失败
            if (progress.stage === 5) {
                updating = false
                updateDialog.close()
            } else if (progress.stage === 6) {
                ToastManager.ShowError(progress.message)
            } else if (progress.stage === 4) {
                ToastManager.ShowInfo(progress.message)
            }

            updateTip.text = progress.message
        }

        function onUpdateFinished(success) {
            updating = false
            console.log("update stage", success)
        }
    }
}

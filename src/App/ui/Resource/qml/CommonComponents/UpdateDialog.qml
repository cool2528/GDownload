import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// 更新对话框:复用 GDialogShell 外壳(背景/阴影/进出动效/表头/分隔线),底部按钮走 footer 槽
GDialogShell {
    id: updateDialog
    width: 560
    height: 600

    title: qsTr("New Version Available")
    subtitle: versionNumber
    iconSource: SegoeFluentIcons.CloudDownload
    iconBgColor: GTheme.successLight(9)
    iconColor: GTheme.successColor

    // 属性定义
    property string versionNumber: ""
    property string releaseNotes: ""
    property bool updating: false

    // ===== body:发布说明 + GitHub 加速开关 + 进度 =====
    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: GTheme.space2XL
        anchors.rightMargin: GTheme.space2XL
        anchors.topMargin: GTheme.spaceLG
        anchors.bottomMargin: GTheme.spaceLG
        spacing: GTheme.spaceLG

        // 更新说明标题
        Text {
            text: qsTr("Release Notes")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
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
                radius: GTheme.radiusBase + GTheme.radiusSmall
            }

            ScrollText {
                id: releaseNotesText
                width: parent.width
                text: releaseNotes
                textFormat: TextEdit.MarkdownText
                promptPage: true
                padding: GTheme.spaceLG
            }
        }

        // GitHub 加速开关
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceXS

            GButtonSwitch {
                id: githubAccelerateSwitch
                Layout.fillWidth: true
                Layout.preferredHeight: GTheme.sizeDefault
                text: qsTr("Enable GitHub Accelerated Download")
                checked: SettingsManager.qEnableGithubAccelerate
                onClicked: SettingsManager.SetEnableGithubAccelerate(checked)
            }

            Text {
                text: qsTr("Use a GitHub mirror (ghproxy) when fetching update packages.")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        // 更新进度区域
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceSM
            visible: updateDialog.updating

            Text {
                id: updateTip
                text: qsTr("Updating...")
                font.pixelSize: GTheme.fontBody
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

    // ===== footer:跳转下载 / 取消 / 立即更新 =====
    footer: Component {
        Item {
            implicitHeight: updateDialog.barHeight

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: GTheme.space2XL
                anchors.rightMargin: GTheme.space2XL
                spacing: GTheme.spaceLG

                // 跳转下载按钮
                RowLayout {
                    spacing: GTheme.spaceSM

                    GButton {
                        iconSource: SegoeFluentIcons.Globe
                        iconSize: GTheme.fontBody
                        Layout.preferredWidth: GTheme.sizeDefault
                        Layout.preferredHeight: GTheme.sizeDefault
                        onClicked: {
                            Qt.openUrlExternally("https://github.com/cool2528/GDownload/releases")
                        }
                    }

                    GButton {
                        text: qsTr("Go to Download Page")
                        Layout.preferredHeight: GTheme.sizeDefault
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
                    Layout.preferredHeight: GTheme.sizeDefault
                    visible: !updateDialog.updating
                    onClicked: updateDialog.close()
                }

                // 更新按钮
                GButton {
                    buttonType: "primary"
                    text: qsTr("Update Now")
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: GTheme.sizeDefault
                    visible: !updateDialog.updating
                    onClicked: {
                        if (UpdateManager.StartUpdate()) {
                            updateDialog.updating = true
                        }
                    }
                }
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

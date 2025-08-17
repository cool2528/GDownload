import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk
Popup {
    id: updateDialog
    width: 500
    height: contentLayout.height + 20
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    closePolicy: Popup.CloseOnEscape
    visible: false
    // 属性定义
    property string versionNumber: ""
    property string releaseNotes: ""
    property bool updating: false
    // 背景设置
    background: Rectangle {
        color: GTheme.dark ? "#2e2e2e" : "#ffffff"
        radius: 5
    }
    
    // 内容布局
    contentItem: Item {
        width: parent.width
        ColumnLayout {
            id: contentLayout
            width: parent.width
            spacing: 10
            // 标题部分
            RowLayout {
                Layout.preferredWidth: updateDialog.width - 20
                Layout.preferredHeight: 100
                spacing: 10
                Text {
                    Layout.alignment: Qt.AlignLeft
                    text: qsTr("New Version") + " " + versionNumber
                    font.pixelSize: 18
                    font.bold: true
                    color: GTheme.dark ? "#ffffff" : "#3b3b3b"
                }
                
                Item { Layout.fillWidth: true }
                
                GButton {
                    id: installButton
                    type: 1 // Primary
                    text: qsTr("Go to Download")
                    Layout.alignment: Qt.AlignRight
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: textMetrics.width + 40
                    onClicked: {
                        Qt.openUrlExternally("https://github.com/cool2528/GDownload/releases")
                    }
                }
                TextMetrics{
                    id: textMetrics
                    text: installButton.text
                    font: installButton.font
                }
            }
            // 分隔线
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: GTheme.dark ? "#3c3c3c" : "#e5e5e5"
            }

            Rectangle{
                id: releaseNotesRect
                Layout.preferredWidth: updateDialog.width - 20
                Layout.preferredHeight: 300
                color: GTheme.dark ? "#2e2e2e" : "#ffffff"
                border.color: GTheme.dark ? "#3c3c3c" : "#e5e5e5"
                border.width: 1
                radius: 5
                ScrollText {
                    id: releaseNotesText
                    width: releaseNotesRect.width - 1
                    height: releaseNotesRect.height - 1
                    text: releaseNotes
                    textFormat: TextEdit.MarkdownText
                    promptPage: true
                }
            }


            // 底部按钮
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                spacing: 10

                // 添加进度条
                GProgressBar {
                    id: updateProgressBar
                    Layout.fillWidth: true
                    Layout.preferredHeight: 6
                    from: 0
                    to: 100
                    value: 50
                    visible: updating
                }

                Item {
                    Layout.fillWidth: true
                    visible: !updating
                }

                GButton {
                    id: cancelButton
                    type: 0 // Default
                    text: qsTr("Cancel")
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 80
                    visible: !updating
                    onClicked: updateDialog.close()
                }

                GButton {
                    id: updateButton
                    type: 1 // Primary
                    text: qsTr("update")
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 80
                    visible: !updating
                    onClicked: {
                        // 调用更新管理器开始更新
                        if (UpdateManager.StartUpdate()) {
                            updating = true
                        }
                    }
                }
            }
            Text {
                id: updateTip
                font.pixelSize: 14
                color: GTheme.dark ? "#ffffff" : "#3b3b3b"
            }
        }
    }

    // 连接更新管理器信号
    Connections {
        target: UpdateManager

        function onUpdateAvailable(info) {
            // 更新对话框内容
            versionNumber = "v" + info.version
            releaseNotes = info.release_note
            updating = false
            updateDialog.open()
        }

        function onUpdateProgress(progress) {
            // 更新进度条
            updateProgressBar.value = progress.percentage

            // 如果更新完成，关闭对话框
            // stage 0 = 检查更新 1 = 下载更新 2 = 解压更新 3 = 验证更新 4 = 安装更新 5 = 下载完成 6 = 更新失败
            if (progress.stage === 5) {
                updating = false
                updateDialog.close()
            }else if(progress.stage ===  6){
                ToastManager.ShowError(progress.message)
            }else if(progress.stage ===  4){
                ToastManager.ShowInfo(progress.message)
            }

            updateTip.text = progress.message
        }

        function onUpdateFinished(success) {
            updating = false
            console.log("update stage ",success)
        }
    }

}

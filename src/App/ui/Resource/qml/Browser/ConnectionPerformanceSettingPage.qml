import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格连接和性能参数设置卡片
GCard {
    id: connectionPerformanceSettingPage
    Layout.fillWidth: true
    Layout.preferredHeight: contentLayout.implicitHeight + 48
    outlined: true
    padding: 16  // Element Plus 标准内边距

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // 卡片标题和描述
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: qsTr("Connection & Performance")
                font.pixelSize: 16
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Configure connection parameters and download performance")
                font.pixelSize: 12
                color: GTheme.textSecondary
            }
        }

        // 并行下载数
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Max Concurrent Downloads")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: qsTr("Count:")
                    font.pixelSize: 14
                    color: GTheme.textRegular
                    Layout.preferredWidth: 60
                }

                GSpinBox {
                    id: maxConcurrentDownloadsInput
                    Layout.preferredWidth: 150
                    implicitHeight: 32
                    from: 1
                    to: 64
                    value: SettingsManager.qMaxConcurrentDownloads
                }

                Text {
                    text: qsTr("Number of downloads running at the same time")
                    font.pixelSize: 12
                    color: GTheme.textPlaceholder
                }
            }
        }

        Divider {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color:GTheme.borderLight
        }

        // 单服务器连接数
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Max Connections Per Server")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: qsTr("Count:")
                    font.pixelSize: 14
                    color: GTheme.textRegular
                    Layout.preferredWidth: 60
                }

                GSpinBox {
                    id: maxConnectionPerServerInput
                    Layout.preferredWidth: 150
                    implicitHeight: 32
                    from: 1
                    to: 64
                    value: SettingsManager.qMaxConnectionPerServer
                }

                Text {
                    text: qsTr("More connections = faster speed, but may be blocked by servers")
                    font.pixelSize: 12
                    color: GTheme.textPlaceholder
                }
            }
        }

        Divider {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color:GTheme.borderLight
        }

        // 分片数
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("File Segments (Split)")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: qsTr("Count:")
                    font.pixelSize: 14
                    color: GTheme.textRegular
                    Layout.preferredWidth: 60
                }

                GSpinBox {
                    id: splitInput
                    Layout.preferredWidth: 150
                    implicitHeight: 32
                    from: 1
                    to: 64
                    value: SettingsManager.qSplit
                }

                Text {
                    text: qsTr("Number of segments to split the file for downloading")
                    font.pixelSize: 12
                    color: GTheme.textPlaceholder
                }
            }
        }

        Divider {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color:GTheme.borderLight
        }

        // 最小分割大小
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Min Split Size")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: qsTr("Size:")
                    font.pixelSize: 14
                    color: GTheme.textRegular
                    Layout.preferredWidth: 60
                }

                GComBoBox {
                    id: minSplitSizeCombo
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 32
                    model: ["1M", "5M", "10M", "20M", "50M", "100M"]

                    Component.onCompleted: {
                        // 根据当前配置设置索引
                        let currentSize = SettingsManager.qMinSplitSize
                        let sizes = [1, 5, 10, 20, 50, 100]
                        let index = sizes.indexOf(currentSize)
                        currentIndex = index >= 0 ? index : 3  // 默认 20M
                    }
                }

                Text {
                    text: qsTr("Don't split files smaller than this size")
                    font.pixelSize: 12
                    color: GTheme.textPlaceholder
                }
            }
        }

        // 统一操作按钮区域
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Layout.topMargin: 16

            Item {
                Layout.fillWidth: true
            }

            GButton {
                text: qsTr("Reset to Default")
                type: 3  // Warning 样式
                Layout.preferredWidth: 120
                Layout.preferredHeight: 36
                onClicked: {
                    maxConcurrentDownloadsInput.value = 5
                    maxConnectionPerServerInput.value = 16
                    splitInput.value = 16
                    minSplitSizeCombo.currentIndex = 3  // 20M

                    statusText.text = qsTr("Input fields reset to default values (not saved yet)")
                    statusText.color = GTheme.textSecondary
                }
            }

            GButton {
                text: qsTr("Save Settings")
                type: 1  // 主要按钮样式
                Layout.preferredWidth: 120
                Layout.preferredHeight: 36
                onClicked: {
                    // 解析输入值
                    let concurrentDownloads = maxConcurrentDownloadsInput.value
                    let connectionPerServer = maxConnectionPerServerInput.value
                    let split = splitInput.value

                    let sizes = [1, 5, 10, 20, 50, 100]
                    let minSplitSize = sizes[minSplitSizeCombo.currentIndex]

                    // 检查是否有更改
                    let originalConcurrent = SettingsManager.qMaxConcurrentDownloads
                    let originalConnection = SettingsManager.qMaxConnectionPerServer
                    let originalSplit = SettingsManager.qSplit
                    let originalMinSize = SettingsManager.qMinSplitSize

                    let hasChanges = (concurrentDownloads !== originalConcurrent) ||
                                   (connectionPerServer !== originalConnection) ||
                                   (split !== originalSplit) ||
                                   (minSplitSize !== originalMinSize)

                    if (hasChanges) {
                        // 保存设置（会自动通过 RPC 更新 aria2c）
                        SettingsManager.SetAria2MaxConcurrentDownloads(concurrentDownloads)
                        SettingsManager.SetAria2MaxConnectionPerServer(connectionPerServer)
                        SettingsManager.SetAria2Split(split)
                        SettingsManager.SetAria2MinSplitSize(minSplitSize)

                        ToastManager.ShowSuccess(
                            qsTr("✓ Connection & Performance settings saved and applied successfully!"),
                            3000
                        )

                        statusText.text = qsTr("✓ Settings saved: Concurrent=%1, Connection=%2, Split=%3, MinSize=%4M")
                            .arg(concurrentDownloads).arg(connectionPerServer).arg(split).arg(minSplitSize)
                        statusText.color = GTheme.successColor
                    } else {
                        ToastManager.ShowInfo(qsTr("No changes detected."), 2000)
                        statusText.text = qsTr("Settings unchanged.")
                        statusText.color = GTheme.textSecondary
                    }
                }
            }
        }

        // 状态提示文本
        Text {
            id: statusText
            Layout.fillWidth: true
            Layout.topMargin: 8
            text: qsTr("Adjust the parameters above and click 'Save Settings' to apply.")
            font.pixelSize: 12
            color: GTheme.textSecondary
            wrapMode: Text.WordWrap
        }
    }
}

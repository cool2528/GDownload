import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格实验功能页面
Rectangle {
    id: labSetting
    color: GTheme.bgPage  // 使用 Element Plus 页面背景色
    clip: true

    // Element Plus 设计标准
    readonly property int standardSpacing: 16
    readonly property int cardSpacing: 12
    readonly property int contentMargin: 24

    ScrollView {
        id: scrollView
        anchors.fill: parent
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        clip: true
        contentWidth: availableWidth
        contentHeight: columnLayout.implicitHeight + 40  // 底部间距

        ColumnLayout {
            id: columnLayout
            width: scrollView.availableWidth
            spacing: labSetting.cardSpacing
            anchors.margins: 0
            anchors.topMargin: labSetting.standardSpacing  // 顶部间距

            // 实验功能说明卡片
            GCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                Layout.leftMargin: labSetting.contentMargin
                Layout.rightMargin: labSetting.contentMargin
                outlined: true
                padding: labSetting.standardSpacing

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: labSetting.standardSpacing
                    spacing: labSetting.standardSpacing

                    // 图标区域
                    Rectangle {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        Layout.alignment: Qt.AlignTop
                        color: GTheme.primaryLight(9)
                        radius: 8

                        FontIcon {
                            anchors.centerIn: parent
                            iconSource: SegoeFluentIcons.ExploitProtectionSettings
                            iconSize: 24
                            color: GTheme.primaryColor
                        }
                    }

                    // 文本区域
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: qsTr("Experimental Features")
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                            color: GTheme.textPrimary
                        }

                        Text {
                            text: qsTr("These features are experimental and may be unstable. Use with caution and report any issues you encounter.")
                            font.pixelSize: 14
                            color: GTheme.textSecondary
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Text {
                            text: qsTr("⚠️ Enable at your own risk")
                            font.pixelSize: 12
                            color: GTheme.warningColor
                            font.weight: Font.Medium
                        }
                    }
                }
            }

            // 下载性能优化卡片
            GCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                Layout.leftMargin: labSetting.contentMargin
                Layout.rightMargin: labSetting.contentMargin
                outlined: true
                padding: labSetting.standardSpacing

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: labSetting.standardSpacing
                    spacing: 12

                    // 卡片标题
                    Text {
                        text: qsTr("Download Performance")
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        color: GTheme.textPrimary
                        Layout.fillWidth: true
                    }

                    // 多线程下载
                    GButtonSwitch {
                        id: multiThreadDownload
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: qsTr("Enhanced Multi-threading")
                        checked: false  // 默认关闭实验功能
                        onClicked: {
                            // TODO: 实现多线程下载设置
                            console.log("Multi-threading download:", checked)
                        }
                    }

                    Text {
                        text: qsTr("Improves download speed by using advanced threading algorithms")
                        font.pixelSize: 12
                        color: GTheme.textSecondary
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                    }

                    Divider {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                    }

                    // 智能带宽分配
                    GButtonSwitch {
                        id: smartBandwidth
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: qsTr("Smart Bandwidth Allocation")
                        checked: false
                        onClicked: {
                            // TODO: 实现智能带宽分配
                            console.log("Smart bandwidth:", checked)
                        }
                    }

                    Text {
                        text: qsTr("Automatically adjusts bandwidth usage based on network conditions")
                        font.pixelSize: 12
                        color: GTheme.textSecondary
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                    }
                }
            }

            // 用户界面实验卡片
            GCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                Layout.leftMargin: labSetting.contentMargin
                Layout.rightMargin: labSetting.contentMargin
                outlined: true
                padding: labSetting.standardSpacing

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: labSetting.standardSpacing
                    spacing: 12

                    // 卡片标题
                    Text {
                        text: qsTr("User Interface")
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        color: GTheme.textPrimary
                        Layout.fillWidth: true
                    }

                    // 动画效果增强
                    GButtonSwitch {
                        id: enhancedAnimations
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: qsTr("Enhanced Animations")
                        checked: false
                        onClicked: {
                            // TODO: 实现增强动画效果
                            console.log("Enhanced animations:", checked)
                        }
                    }

                    Text {
                        text: qsTr("Enables smooth transitions and micro-interactions throughout the interface")
                        font.pixelSize: 12
                        color: GTheme.textSecondary
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                    }

                    Divider {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                    }

                    // 自适应布局
                    GButtonSwitch {
                        id: adaptiveLayout
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: qsTr("Adaptive Layout")
                        checked: false
                        onClicked: {
                            // TODO: 实现自适应布局
                            console.log("Adaptive layout:", checked)
                        }
                    }

                    Text {
                        text: qsTr("Automatically adjusts layout based on window size and DPI settings")
                        font.pixelSize: 12
                        color: GTheme.textSecondary
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                    }
                }
            }

            // 网络和协议实验卡片
            GCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                Layout.leftMargin: labSetting.contentMargin
                Layout.rightMargin: labSetting.contentMargin
                outlined: true
                padding: labSetting.standardSpacing

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: labSetting.standardSpacing
                    spacing: 12

                    // 卡片标题
                    Text {
                        text: qsTr("Network & Protocols")
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        color: GTheme.textPrimary
                        Layout.fillWidth: true
                    }

                    // HTTP/3 支持
                    GButtonSwitch {
                        id: http3Support
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: qsTr("HTTP/3 Protocol Support")
                        checked: false
                        onClicked: {
                            // TODO: 实现 HTTP/3 支持
                            console.log("HTTP/3 support:", checked)
                        }
                    }

                    Text {
                        text: qsTr("Enables next-generation HTTP/3 protocol for faster downloads")
                        font.pixelSize: 12
                        color: GTheme.textSecondary
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                    }

                    Divider {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                    }

                    // 智能重试机制
                    GButtonSwitch {
                        id: smartRetry
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: qsTr("Intelligent Retry Mechanism")
                        checked: false
                        onClicked: {
                            // TODO: 实现智能重试
                            console.log("Smart retry:", checked)
                        }
                    }

                    Text {
                        text: qsTr("Advanced retry strategies with exponential backoff and error analysis")
                        font.pixelSize: 12
                        color: GTheme.textSecondary
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                    }

                    Divider {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                    }

                    // P2P 加速
                    GButtonSwitch {
                        id: p2pAcceleration
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: qsTr("P2P Download Acceleration")
                        checked: false
                        onClicked: {
                            // TODO: 实现 P2P 加速
                            console.log("P2P acceleration:", checked)
                        }
                    }

                    Text {
                        text: qsTr("Uses peer-to-peer technology to accelerate popular file downloads")
                        font.pixelSize: 12
                        color: GTheme.textSecondary
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                    }
                }
            }

            // 重置按钮区域
            GCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                Layout.leftMargin: labSetting.contentMargin
                Layout.rightMargin: labSetting.contentMargin
                Layout.bottomMargin: labSetting.standardSpacing
                outlined: true
                padding: labSetting.standardSpacing

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: labSetting.standardSpacing
                    spacing: labSetting.standardSpacing

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: qsTr("Reset Experimental Settings")
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            color: GTheme.textPrimary
                        }

                        Text {
                            text: qsTr("Disable all experimental features and restore defaults")
                            font.pixelSize: 12
                            color: GTheme.textSecondary
                        }
                    }

                    GButton {
                        id: resetButton
                        text: qsTr("Reset All")
                        type: 2  // 危险按钮样式
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 36
                        onClicked: {
                            // 重置所有实验功能
                            multiThreadDownload.checked = false
                            smartBandwidth.checked = false
                            enhancedAnimations.checked = false
                            adaptiveLayout.checked = false
                            http3Support.checked = false
                            smartRetry.checked = false
                            p2pAcceleration.checked = false

                            ToastManager.ShowSuccess(qsTr("All experimental features have been reset"))
                        }
                    }
                }
            }
        }
    }
}

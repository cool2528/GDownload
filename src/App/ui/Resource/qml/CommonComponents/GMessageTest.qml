import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

/**
 * GMessage 测试页面
 * 用于快速测试消息提示组件的各种功能
 */
Item {
    id: root
    
    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        
        ColumnLayout {
            width: parent.width
            spacing: 20
            
            // 标题
            Label {
                text: "GMessage 消息提示组件测试"
                font.pixelSize: 24
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 20
            }
            
            // 基础类型测试
            GroupBox {
                title: "1. 基础消息类型"
                Layout.fillWidth: true
                Layout.margins: 20
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    RowLayout {
                        spacing: 10
                        
                        GButton {
                            text: "Primary"
                            onClicked: MessageManager.primary("这是 Primary 消息")
                        }
                        
                        GButton {
                            text: "Success"
                            onClicked: MessageManager.success("操作成功！")
                        }
                        
                        GButton {
                            text: "Warning"
                            onClicked: MessageManager.warning("请注意！")
                        }
                        
                        GButton {
                            text: "Info"
                            onClicked: MessageManager.info("这是提示信息")
                        }
                        
                        GButton {
                            text: "Error"
                            onClicked: MessageManager.error("操作失败！")
                        }
                    }
                }
            }
            
            // 位置测试
            GroupBox {
                title: "2. 位置选项测试"
                Layout.fillWidth: true
                Layout.margins: 20
                
                GridLayout {
                    anchors.fill: parent
                    columns: 3
                    rowSpacing: 10
                    columnSpacing: 10
                    
                    GButton {
                        text: "Top Left"
                        Layout.fillWidth: true
                        onClicked: MessageManager.show("左上角消息", {
                            placement: "top-left"
                        })
                    }
                    
                    GButton {
                        text: "Top Center"
                        Layout.fillWidth: true
                        onClicked: MessageManager.show("顶部居中消息", {
                            placement: "top"
                        })
                    }
                    
                    GButton {
                        text: "Top Right"
                        Layout.fillWidth: true
                        onClicked: MessageManager.show("右上角消息", {
                            placement: "top-right"
                        })
                    }
                    
                    GButton {
                        text: "Bottom Left"
                        Layout.fillWidth: true
                        onClicked: MessageManager.show("左下角消息", {
                            placement: "bottom-left"
                        })
                    }
                    
                    GButton {
                        text: "Bottom Center"
                        Layout.fillWidth: true
                        onClicked: MessageManager.show("底部居中消息", {
                            placement: "bottom"
                        })
                    }
                    
                    GButton {
                        text: "Bottom Right"
                        Layout.fillWidth: true
                        onClicked: MessageManager.show("右下角消息", {
                            placement: "bottom-right"
                        })
                    }
                }
            }
            
            // 关闭按钮测试
            GroupBox {
                title: "3. 关闭按钮和持续时间"
                Layout.fillWidth: true
                Layout.margins: 20
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    RowLayout {
                        spacing: 10
                        
                        GButton {
                            text: "自动关闭 (3秒)"
                            onClicked: MessageManager.success("3秒后自动关闭", 3000)
                        }
                        
                        GButton {
                            text: "显示关闭按钮"
                            onClicked: MessageManager.show("带关闭按钮的消息", {
                                type: "info",
                                showClose: true,
                                duration: 10000
                            })
                        }
                        
                        GButton {
                            text: "不自动关闭"
                            onClicked: MessageManager.show("需要手动关闭", {
                                type: "warning",
                                showClose: true,
                                duration: 0
                            })
                        }
                    }
                }
            }
            
            // Plain 样式测试
            GroupBox {
                title: "4. Plain 纯色样式"
                Layout.fillWidth: true
                Layout.margins: 20
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    RowLayout {
                        spacing: 10
                        
                        GButton {
                            text: "普通样式"
                            onClicked: MessageManager.success("普通样式消息")
                        }
                        
                        GButton {
                            text: "Plain 样式"
                            onClicked: MessageManager.show("Plain 样式消息", {
                                type: "success",
                                plain: true
                            })
                        }
                        
                        GButton {
                            text: "Plain Error"
                            onClicked: MessageManager.show("Plain 错误消息", {
                                type: "error",
                                plain: true
                            })
                        }
                    }
                }
            }
            
            // 多消息测试
            GroupBox {
                title: "5. 多消息同时显示"
                Layout.fillWidth: true
                Layout.margins: 20
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    RowLayout {
                        spacing: 10
                        
                        GButton {
                            text: "连续显示 3 条"
                            onClicked: {
                                MessageManager.info("第 1 条消息")
                                MessageManager.success("第 2 条消息")
                                MessageManager.warning("第 3 条消息")
                            }
                        }
                        
                        GButton {
                            text: "不同位置显示"
                            onClicked: {
                                MessageManager.show("左上角", { placement: "top-left" })
                                MessageManager.show("右上角", { placement: "top-right" })
                                MessageManager.show("底部", { placement: "bottom" })
                            }
                        }
                        
                        GButton {
                            text: "关闭所有消息"
                            onClicked: MessageManager.closeAll()
                        }
                    }
                }
            }
            
            // 手动关闭测试
            GroupBox {
                title: "6. 手动关闭消息"
                Layout.fillWidth: true
                Layout.margins: 20
                
                property int currentMessageId: -1
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    RowLayout {
                        spacing: 10
                        
                        GButton {
                            text: "显示长时消息"
                            onClicked: {
                                parent.parent.parent.currentMessageId = MessageManager.show("这条消息需要手动关闭", {
                                    type: "info",
                                    duration: 0,
                                    showClose: false
                                })
                            }
                        }
                        
                        GButton {
                            text: "手动关闭上一条"
                            onClicked: {
                                if (parent.parent.parent.currentMessageId !== -1) {
                                    MessageManager.close(parent.parent.parent.currentMessageId)
                                    MessageManager.success("已关闭")
                                } else {
                                    MessageManager.warning("没有消息可关闭")
                                }
                            }
                        }
                    }
                }
            }
            
            // 实际场景模拟
            GroupBox {
                title: "7. 实际使用场景"
                Layout.fillWidth: true
                Layout.margins: 20
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    RowLayout {
                        spacing: 10
                        
                        GButton {
                            text: "模拟文件上传"
                            onClicked: {
                                let msgId = MessageManager.show("正在上传文件...", {
                                    type: "info",
                                    duration: 0,
                                    showClose: false,
                                    placement: "top-right"
                                })
                                
                                // 模拟 3 秒后上传完成
                                Qt.callLater(() => {
                                    uploadTimer.msgId = msgId
                                    uploadTimer.start()
                                })
                            }
                        }
                        
                        GButton {
                            text: "模拟表单验证"
                            onClicked: {
                                MessageManager.error("请填写必填项", {
                                    placement: "top-right"
                                })
                            }
                        }
                        
                        GButton {
                            text: "模拟网络错误"
                            onClicked: {
                                MessageManager.error("网络连接失败，请检查网络设置", {
                                    duration: 5000,
                                    showClose: true
                                })
                            }
                        }
                    }
                }
            }
            
            Item {
                Layout.fillHeight: true
                Layout.minimumHeight: 50
            }
        }
    }
    
    // 模拟上传完成的定时器
    Timer {
        id: uploadTimer
        interval: 3000
        repeat: false
        property int msgId: -1
        onTriggered: {
            MessageManager.close(msgId)
            MessageManager.success("文件上传成功！", {
                placement: "top-right"
            })
        }
    }
}

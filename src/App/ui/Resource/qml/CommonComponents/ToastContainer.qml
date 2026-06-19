import QtQuick
import gdl.sdk

// 顶部消息堆叠容器:监听 ToastManager.messageRequested,用统一的 GMessage 模板渲染
// (System A 的 C++ 接口与所有调用点不变;此处仅把渲染模板从 MessageToast 切到 GMessage)
Item {
    id: root

    // 消息之间的垂直间距
    property int spacing: GTheme.spaceMD

    // 存储所有活动消息
    property var activeMessages: []

    // GMessage 模板组件
    Component {
        id: messageComponent
        GMessage {
            // GMessage 关闭时(定时器到点或手动)会发出 messageClosed,并在 onClosed 中自销毁
            onMessageClosed: {
                let index = root.activeMessages.indexOf(this)
                if (index !== -1) {
                    root.activeMessages.splice(index, 1)
                    root.repositionMessages()
                }
                // 不在此处 destroy:GMessage 自身 onClosed 已延迟销毁,避免双重释放
            }
        }
    }

    // 重新堆叠所有消息的纵向位置
    function repositionMessages() {
        let currentY = GTheme.spaceXL
        for (let i = 0; i < activeMessages.length; i++) {
            let msg = activeMessages[i]
            msg.y = currentY
            currentY += msg.height + spacing
        }
    }

    // 显示新消息;type 为 ToastManager 的整型(0=Success,1=Warning,2=Info,3=Error)
    // GMessage 枚举为 Primary=0/Success=1/Warning=2/Info=3/Error=4,故 +1 映射
    function showToast(message, type, duration, id) {
        let msg = messageComponent.createObject(root, {
            placement: GMessage.Top
        })
        if (msg === null) {
            console.error("ToastContainer: failed to create GMessage")
            return
        }

        activeMessages.push(msg)
        repositionMessages()
        msg.show(message, type + 1, duration)
    }

    Component.onCompleted: {
        ToastManager.messageRequested.connect(showToast)
    }
}

import QtQuick
import QtQuick.Layouts
import gdl.sdk

Item {
    id: root
    
    // 消息之间的间距
    property int spacing: 10
    
    // 存储所有活动的toast
    property var activeToasts: []
    
    // 用于创建新的toast的组件
    Component {
        id: toastComponent
        MessageToast {
            onClosed: {
                // 当toast关闭时从队列中移除
                let index = root.activeToasts.indexOf(this)
                if (index !== -1) {
                    root.activeToasts.splice(index, 1)
                    root.repositionToasts()
                }
                this.destroy()
            }
        }
    }
    
    // 重新定位所有toast的位置
    function repositionToasts() {
        let currentY = 20
        for (let i = 0; i < activeToasts.length; i++) {
            let toast = activeToasts[i]
            toast.y = currentY
            currentY += toast.height + spacing
        }
    }
    
    // 显示新的toast
    function showToast(message, type, duration, id) {
        let toast = toastComponent.createObject(root, {
            x: Math.round((root.width - 500) / 2), // 500是最大宽度
            y: 20
        })
        
        activeToasts.push(toast)
        repositionToasts()
        toast.show(message, type, duration)
    }
    
    Component.onCompleted: {
        ToastManager.messageRequested.connect(showToast)
    }
} 
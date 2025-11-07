import QtQuick
import QtQuick.Layouts
import gdl.sdk

/**
 * GMessageContainer - 增强版消息容器管理器
 * 支持多位置消息管理和堆叠显示
 * 
 * 主要功能：
 * - 支持 6 种位置的消息独立管理
 * - 自动计算消息堆叠位置
 * - 最大消息数量限制
 * - 消息分组（可选）
 */
Item {
    id: root
    
    // ========== 配置属性 ==========
    
    // 消息之间的间距
    property int spacing: 10
    
    // 每个位置最大消息数量
    property int maxMessagesPerPosition: 5
    
    // 是否启用消息分组
    property bool grouping: false
    
    // ========== 内部数据结构 ==========
    
    // 按位置分组的活动消息
    property var activeMessages: ({
        "top": [],
        "top-left": [],
        "top-right": [],
        "bottom": [],
        "bottom-left": [],
        "bottom-right": []
    })
    
    // 消息分组缓存（用于合并相同内容的消息）
    property var messageGroups: ({})
    
    // ========== 组件定义 ==========
    
    // 消息组件模板
    Component {
        id: messageComponent
        GMessage {
            // 关闭时从队列中移除
            onMessageClosed: function(msgId) {
                root.removeMessage(this)
            }
        }
    }
    
    // ========== 辅助函数 ==========
    
    /**
     * 将位置枚举转换为字符串键
     */
    function placementToKey(placement) {
        switch(placement) {
            case GMessage.Top: return "top"
            case GMessage.TopLeft: return "top-left"
            case GMessage.TopRight: return "top-right"
            case GMessage.Bottom: return "bottom"
            case GMessage.BottomLeft: return "bottom-left"
            case GMessage.BottomRight: return "bottom-right"
            default: return "top"
        }
    }
    
    /**
     * 判断是否为顶部位置
     */
    function isTopPlacement(placement) {
        return placement === GMessage.Top || 
               placement === GMessage.TopLeft || 
               placement === GMessage.TopRight
    }
    
    /**
     * 重新定位指定位置的所有消息
     */
    function repositionMessages(placementKey, placement) {
        let messages = activeMessages[placementKey]
        if (!messages || messages.length === 0) {
            return
        }
        
        let isTop = isTopPlacement(placement)
        let currentOffset = messages[0].offset
        
        for (let i = 0; i < messages.length; i++) {
            let msg = messages[i]
            if (!msg) continue
            
            if (isTop) {
                // 从上到下堆叠
                msg.y = currentOffset
                currentOffset += msg.height + spacing
            } else {
                // 从下到上堆叠
                msg.y = root.height - currentOffset - msg.height
                currentOffset += msg.height + spacing
            }
        }
    }
    
    /**
     * 从队列中移除消息
     */
    function removeMessage(message) {
        let placementKey = placementToKey(message.placement)
        let messages = activeMessages[placementKey]
        
        if (!messages) return
        
        let index = messages.indexOf(message)
        if (index !== -1) {
            messages.splice(index, 1)
            activeMessages[placementKey] = messages
            repositionMessages(placementKey, message.placement)
        }
    }
    
    /**
     * 显示新消息
     * @param options 消息配置选项
     */
    function showMessage(options) {
        // 解析选项
        let message = options.message || ""
        let type = options.type || "info"
        let duration = options.duration !== undefined ? options.duration : 3000
        let showClose = options.showClose || false
        let plain = options.plain || false
        let placement = options.placement || GMessage.Top
        let offset = options.offset !== undefined ? options.offset : 16
        let customIcon = options.customIcon || -1
        let messageId = options.id || -1
        
        // 将字符串类型转换为枚举
        let typeEnum = stringToMessageType(type)
        let placementEnum = stringToPlacement(placement)
        let placementKey = placementToKey(placementEnum)
        
        // 检查是否启用分组且存在相同消息
        if (grouping && messageGroups[message]) {
            let existingMsg = messageGroups[message]
            // TODO: 更新重复计数
            return
        }
        
        // 检查消息数量限制
        let messages = activeMessages[placementKey]
        if (messages.length >= maxMessagesPerPosition) {
            // 移除最旧的消息
            let oldestMsg = messages[0]
            if (oldestMsg) {
                oldestMsg.closeMessage()
            }
        }
        
        // 创建新消息
        let msg = messageComponent.createObject(root, {
            message: message,
            messageType: typeEnum,
            duration: duration,
            showClose: showClose,
            plain: plain,
            placement: placementEnum,
            offset: offset,
            customIcon: customIcon,
            messageId: messageId
        })
        
        if (!msg) {
            console.error("Failed to create message")
            return
        }
        
        // 添加到对应位置的队列
        messages.push(msg)
        activeMessages[placementKey] = messages
        
        // 如果启用分组，添加到分组缓存
        if (grouping) {
            messageGroups[message] = msg
        }
        
        // 重新定位所有消息
        repositionMessages(placementKey, placementEnum)
        
        // 显示消息
        msg.show(message, typeEnum, duration)
    }
    
    /**
     * 字符串类型转换为消息类型枚举
     */
    function stringToMessageType(typeStr) {
        switch(typeStr.toLowerCase()) {
            case "primary": return GMessage.Primary
            case "success": return GMessage.Success
            case "warning": return GMessage.Warning
            case "info": return GMessage.Info
            case "error": return GMessage.Error
            default: return GMessage.Info
        }
    }
    
    /**
     * 字符串位置转换为位置枚举
     */
    function stringToPlacement(placementStr) {
        if (typeof placementStr === "number") {
            return placementStr
        }
        
        switch(placementStr.toLowerCase()) {
            case "top": return GMessage.Top
            case "top-left": return GMessage.TopLeft
            case "top-right": return GMessage.TopRight
            case "bottom": return GMessage.Bottom
            case "bottom-left": return GMessage.BottomLeft
            case "bottom-right": return GMessage.BottomRight
            default: return GMessage.Top
        }
    }
    
    /**
     * 关闭指定 ID 的消息
     */
    function closeMessageById(messageId) {
        // 遍历所有位置查找消息
        for (let key in activeMessages) {
            let messages = activeMessages[key]
            for (let i = 0; i < messages.length; i++) {
                if (messages[i].messageId === messageId) {
                    messages[i].closeMessage()
                    return
                }
            }
        }
    }
    
    /**
     * 关闭所有消息
     */
    function closeAllMessages() {
        for (let key in activeMessages) {
            let messages = activeMessages[key]
            for (let i = messages.length - 1; i >= 0; i--) {
                if (messages[i]) {
                    messages[i].closeMessage()
                }
            }
        }
    }
}

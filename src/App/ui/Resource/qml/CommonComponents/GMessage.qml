import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import gdl.sdk

/**
 * GMessage - 增强版消息提示组件
 * 基于 Element Plus Message 组件规范
 * 
 * 主要增强：
 * - 支持 Primary 消息类型
 * - 可选的关闭按钮
 * - 6 种位置选项
 * - 可配置偏移量
 * - Plain 纯色样式
 * - 自定义图标
 * - 关闭回调
 */
Popup {
    id: root

    // ========== 公开属性 ==========
    
    // 消息内容
    property string message: ""
    
    // 显示持续时间（毫秒），0 表示不自动关闭
    property int duration: 3000
    
    // 消息 ID（用于手动关闭）
    property int messageId: -1
    
    // ========== 样式属性 ==========
    
    // 最大宽度
    property int maxWidth: 500
    
    // 最小宽度
    property int minWidth: 300
    
    // 最大文本行数
    property int maxTextLines: 3
    
    // 是否显示关闭按钮
    property bool showClose: false
    
    // 纯色背景样式
    property bool plain: false
    
    // 自定义图标（优先级高于类型默认图标）
    property int customIcon: -1
    
    // 与视口边缘的偏移量
    property int offset: 16
    
    // ========== Element Plus 设计标准 ==========
    
    readonly property int standardPadding: 16
    readonly property int iconSize: 16
    readonly property int closeButtonSize: 20
    
    // ========== 消息类型枚举 ==========    
    enum MessageType {
        Primary,
        Success,
        Warning,
        Info,
        Error
    }
    
    property int messageType: GMessage.Info
    
    // ========== 位置枚举 ==========
    
    enum Placement {
        Top,
        TopLeft,
        TopRight,
        Bottom,
        BottomLeft,
        BottomRight
    }
    
    property int placement: GMessage.Top
    
    // ========== 信号 ==========
    
    // 关闭回调
    signal messageClosed(int messageId)
    
    // ========== Popup 配置 ==========
    
    modal: false
    dim: false
    closePolicy: Popup.NoAutoClose
    padding: standardPadding
    
    // ========== 位置计算 ==========
    
    // 动态计算宽度（考虑关闭按钮）
    width: {
        let contentWidth = messageLayout.implicitWidth + 2 * padding
        if (showClose) {
            contentWidth += closeButtonSize + messageLayout.spacing
        }
        return Math.max(Math.min(contentWidth, maxWidth), minWidth)
    }
    
    // 根据 placement 计算 x 坐标
    x: {
        if (!parent) return 0
        
        switch(placement) {
            case GMessage.Top:
            case GMessage.Bottom:
                return Math.round((parent.width - width) / 2)
                
            case GMessage.TopLeft:
            case GMessage.BottomLeft:
                return offset
                
            case GMessage.TopRight:
            case GMessage.BottomRight:
                return parent.width - width - offset
                
            default:
                return Math.round((parent.width - width) / 2)
        }
    }
    
    // y 坐标由容器管理，这里只设置初始值
    y: offset
    
    // ========== 进入动画 ==========
    
    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 300
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: isTopPlacement() ? "y" : "y"
                from: isTopPlacement() ? root.y - 20 : root.y + 20
                to: root.y
                duration: 300
                easing.type: Easing.OutCubic
            }
        }
    }
    
    // ========== 退出动画 ==========
    
    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 200
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                property: isTopPlacement() ? "y" : "y"
                from: root.y
                to: isTopPlacement() ? root.y - 10 : root.y + 10
                duration: 200
                easing.type: Easing.InCubic
            }
        }
    }
    
    // ========== 辅助函数 ==========
    
    // 判断是否为顶部位置
    function isTopPlacement() {
        return placement === GMessage.Top || 
               placement === GMessage.TopLeft || 
               placement === GMessage.TopRight
    }
    
    // 获取背景颜色
    function getBackgroundColor() {
        if (plain) {
            return "transparent"
        }
        
        switch(messageType) {
            case GMessage.Primary:
                return GTheme.primaryLight(8)
            case GMessage.Success:
                return GTheme.successLight(8)
            case GMessage.Warning:
                return GTheme.warningLight(8)
            case GMessage.Info:
                return GTheme.infoLight(8)
            case GMessage.Error:
                return GTheme.dangerLight(8)
            default:
                return GTheme.bgWhite
        }
    }
    
    // 获取边框颜色
    function getBorderColor() {
        switch(messageType) {
            case GMessage.Primary:
                return GTheme.primaryColor
            case GMessage.Success:
                return GTheme.successColor
            case GMessage.Warning:
                return GTheme.warningColor
            case GMessage.Info:
                return GTheme.infoColor
            case GMessage.Error:
                return GTheme.dangerColor
            default:
                return GTheme.borderLight
        }
    }
    
    // 获取图标（优先使用自定义图标）
    function getIcon() {
        if (customIcon !== -1) {
            return customIcon
        }
        
        switch(messageType) {
            case GMessage.Primary:
                return SegoeFluentIcons.CheckmarkCircle
            case GMessage.Success:
                return SegoeFluentIcons.CheckmarkCircle
            case GMessage.Warning:
                return SegoeFluentIcons.Warning
            case GMessage.Info:
                return SegoeFluentIcons.Info
            case GMessage.Error:
                return SegoeFluentIcons.DismissCircle
            default:
                return SegoeFluentIcons.Info
        }
    }
    
    // 获取图标颜色
    function getIconColor() {
        switch(messageType) {
            case GMessage.Primary:
                return GTheme.primaryColor
            case GMessage.Success:
                return GTheme.successColor
            case GMessage.Warning:
                return GTheme.warningColor
            case GMessage.Info:
                return GTheme.infoColor
            case GMessage.Error:
                return GTheme.dangerColor
            default:
                return GTheme.textSecondary
        }
    }
    
    // 获取文本颜色（plain 模式使用类型颜色）
    function getTextColor() {
        if (plain) {
            return getIconColor()
        }
        return GTheme.textPrimary
    }
    
    // ========== 公共方法 ==========
    
    /**
     * 显示消息
     * @param msg 消息内容
     * @param type 消息类型
     * @param time 显示时长（可选）
     */
    function show(msg, type, time) {
        message = msg
        messageType = type
        
        if (time !== undefined) {
            duration = time
        }
        
        // 根据消息长度动态调整显示时间
        if (duration > 0 && msg.length > 50) {
            duration = Math.min(duration * 1.5, 8000) // 最长8秒
        }
        
        open()
        
        // 启动自动关闭定时器（如果 duration > 0）
        if (duration > 0) {
            closeTimer.restart()
        }
    }
    
    /**
     * 关闭消息
     */
    function closeMessage() {
        closeTimer.stop()
        messageClosed(messageId)
        close()
    }
    
    // ========== 背景样式 ==========
    
    background: Rectangle {
        color: getBackgroundColor()
        radius: 6
        border.width: plain ? 1 : 1
        border.color: getBorderColor()
        
        // Element Plus 风格阴影
        layer.enabled: true
        layer.effect: DropShadow {
            radius: 12
            samples: 25
            color: Qt.rgba(0, 0, 0, 0.12)
            horizontalOffset: 0
            verticalOffset: 2
        }
    }
    
    // ========== 内容布局 ==========
    
    contentItem: RowLayout {
        id: messageLayout
        spacing: 12
        
        // 消息图标
        FontIcon {
            id: icon
            iconSource: getIcon()
            iconSize: root.iconSize
            color: getIconColor()
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 2
        }
        
        // 消息文本
        Label {
            id: messageLabel
            text: message
            color: getTextColor()
            font.pixelSize: 14
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            
            // 文本换行设置
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            maximumLineCount: root.maxTextLines
            elide: Text.ElideRight
            
            // 工具提示（当文本被截断时）
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: messageLabel.truncated ? Qt.PointingHandCursor : Qt.ArrowCursor
                
                ToolTip.visible: messageLabel.truncated && containsMouse
                ToolTip.text: message
                ToolTip.delay: 500
            }
        }
        
        // 关闭按钮(GButton icon-only,自带悬停底)
        GButton {
            id: closeButton
            visible: root.showClose
            iconSource: SegoeFluentIcons.ChromeClose
            iconSize: GTheme.fontBody

            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 2
            Layout.preferredWidth: root.closeButtonSize
            Layout.preferredHeight: root.closeButtonSize

            onClicked: {
                root.closeMessage()
            }
        }
    }
    
    // ========== 自动关闭定时器 ==========
    
    Timer {
        id: closeTimer
        interval: duration
        repeat: false
        onTriggered: root.closeMessage()
    }
    
    // ========== 销毁处理 ==========
    
    onClosed: {
        // 延迟销毁，等待退出动画完成
        root.destroy(300)
    }
}

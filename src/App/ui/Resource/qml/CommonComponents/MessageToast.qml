import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import gdl.sdk

// Element Plus 风格消息提示组件
Popup {
    id: root

    // 公开属性
    property string message: ""
    property int duration: 3000  // 显示持续时间，默认3秒
    property int maxWidth: 500   // 最大宽度
    property int maxTextLines: 3 // 最大行数

    // Element Plus 设计标准
    readonly property int standardPadding: 16
    readonly property int iconSize: 16

    // 消息类型枚举
    enum MessageType {
        Success,
        Warning,
        Info,
        Error
    }

    property int messageType: MessageToast.Success

    // Popup 相关属性设置
    modal: false
    dim: false
    closePolicy: Popup.NoAutoClose
    padding: standardPadding
    
    // 位置设置 - 默认顶部居中
    x: Math.round((parent.width - width) / 2)
    y: 20
    
    // 限制最大宽度
    width: Math.min(messageLayout.implicitWidth + 2 * padding, maxWidth)
    
    // Element Plus 风格动画
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
                property: "y"
                from: root.y - 20
                to: root.y
                duration: 300
                easing.type: Easing.OutCubic
            }
        }
    }

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
                property: "y"
                from: root.y
                to: root.y - 10
                duration: 200
                easing.type: Easing.InCubic
            }
        }
    }

    // 根据消息类型返回对应的颜色
    function getBackgroundColor() {
        switch(messageType) {
            case MessageToast.Success:
                return GTheme.successLight(8)
            case MessageToast.Warning:
                return GTheme.warningLight(8)
            case MessageToast.Info:
                return GTheme.infoLight(8)
            case MessageToast.Error:
                return GTheme.dangerLight(8)
            default:
                return GTheme.bgWhite()
        }
    }
    
    // 根据消息类型返回对应的边框颜色
    function getBorderColor() {
        switch(messageType) {
            case MessageToast.Success:
                return GTheme.successColor
            case MessageToast.Warning:
                return GTheme.warningColor
            case MessageToast.Info:
                return GTheme.infoColor
            case MessageToast.Error:
                return GTheme.dangerColor
            default:
                return GTheme.borderLight
        }
    }

    // 根据消息类型返回对应的图标
    function getIcon() {
        switch(messageType) {
            case MessageToast.Success:
                return SegoeFluentIcons.CheckmarkCircle
            case MessageToast.Warning:
                return SegoeFluentIcons.Warning
            case MessageToast.Info:
                return SegoeFluentIcons.Info
            case MessageToast.Error:
                return SegoeFluentIcons.DismissCircle
            default:
                return SegoeFluentIcons.Info
        }
    }

    // 根据消息类型返回对应的图标颜色
    function getIconColor() {
        switch(messageType) {
            case MessageToast.Success:
                return GTheme.successColor
            case MessageToast.Warning:
                return GTheme.warningColor
            case MessageToast.Info:
                return GTheme.infoColor
            case MessageToast.Error:
                return GTheme.dangerColor
            default:
                return GTheme.textSecondary
        }
    }

    // 显示消息的方法
    function show(msg, type, time) {
        message = msg
        messageType = type
        if (time !== undefined) {
            duration = time
        }
        
        // 根据消息长度动态调整显示时间
        if (msg.length > 50) {
            duration = Math.min(duration * 1.5, 8000) // 最长8秒
        }
        
        open()
        closeTimer.restart()
    }
    
    // Element Plus 风格背景
    background: Rectangle {
        color: getBackgroundColor()
        radius: 6
        border.width: 1
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

    contentItem: RowLayout {
        id: messageLayout
        spacing: 12
        width: Math.min(implicitWidth, root.maxWidth - 2 * root.padding)

        FontIcon {
            id: icon
            iconSource: getIcon()
            iconSize: root.iconSize
            color: getIconColor()
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 2
        }

        Label {
            id: messageLabel
            text: message
            color: GTheme.textPrimary
            font.pixelSize: 14
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            
            // 文本换行设置
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            maximumLineCount: root.maxTextLines
            elide: Text.ElideRight  // 超出部分显示省略号
            
            // 如果文本被截断，显示完整文本的工具提示
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                ToolTip.visible: messageLabel.truncated && containsMouse
                ToolTip.text: message
                ToolTip.delay: 500
            }
        }
    }

    Timer {
        id: closeTimer
        interval: duration
        repeat: false
        onTriggered: root.close()
    }

} 

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

Popup {
    id: root
    
    // 公开属性
    property string message: ""
    property int duration: 3000  // 显示持续时间，默认3秒
    property int maxWidth: 500   // 最大宽度
    property int maxTextLines: 3 // 最大行数
    
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
    padding: 12  // 增加内边距使文本显示更美观
    
    // 位置设置 - 默认顶部居中
    x: Math.round((parent.width - width) / 2)
    y: 20
    
    // 限制最大宽度
    width: Math.min(messageLayout.implicitWidth + 2 * padding, maxWidth)
    
    // 动画效果
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 200 }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 200 }
    }

    // 根据消息类型返回对应的颜色
    function getBackgroundColor() {
        switch(messageType) {
            case MessageToast.Success:
                return "#E6F4EA"
            case MessageToast.Warning:
                return "#FFF3E0"
            case MessageToast.Info:
                return "#E8EAF6"
            case MessageToast.Error:
                return "#FDECEA"
            default:
                return "#FFFFFF"
        }
    }
    
    // 根据消息类型返回对应的图标
    function getIcon() {
        switch(messageType) {
            case MessageToast.Success:
                return "/images/toast/success.svg"
            case MessageToast.Warning:
                return "/images/toast/warning.svg"
            case MessageToast.Info:
                return "/images/toast/info.svg"
            case MessageToast.Error:
                return "/images/toast/error.svg"
            default:
                return ""
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
    
    background: Rectangle {
        color: getBackgroundColor()
        radius: 4
        border.width: 1
        border.color: Qt.darker(color, 1.1)
    }

    contentItem: RowLayout {
        id: messageLayout
        spacing: 12
        width: Math.min(implicitWidth, root.maxWidth - 2 * root.padding)

        Image {
            id: icon
            source: getIcon()
            width: 20
            height: 20
            Layout.alignment: Qt.AlignTop  // 改为顶部对齐
            Layout.topMargin: 4  // 微调顶部间距
        }

        Label {
            id: messageLabel
            text: message
            color: "#333333"
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

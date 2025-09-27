import QtQuick
import gdl.sdk 1.0
import QtQuick.Layouts
import "../CommonComponents"

// Element Plus 风格页面标题
Item {
    property int type: 0
    height: 56  // Element Plus 标准页面标题高度
    // Element Plus 风格标题文本
    Text {
        id: titleText
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: {
            if (type === 0) {
                return qsTr("Basic Settings")
            } else if (type === 1) {
                return qsTr("Advanced Settings")
            } else {
                return qsTr("Lab Settings")
            }
        }
        font.pixelSize: 20
        font.weight: Font.Medium
        color: GTheme.textPrimary
    }

    // 可选的页面描述
    Text {
        id: descriptionText
        anchors.left: parent.left
        anchors.top: titleText.bottom
        anchors.topMargin: 4
        text: {
            if (type === 0) {
                return qsTr("Configure basic download preferences")
            } else if (type === 1) {
                return qsTr("Advanced configuration options")
            } else {
                return qsTr("Experimental features and settings")
            }
        }
        font.pixelSize: 14
        color: GTheme.textSecondary
        visible: text.length > 0
    }
    // Element Plus 风格分隔线
    Divider {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: 8
    }
}

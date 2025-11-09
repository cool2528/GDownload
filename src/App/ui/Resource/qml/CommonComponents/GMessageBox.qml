import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import gdl.sdk

/**
 * GMessageBox - 通用消息框组件
 * 基于 Element Plus MessageBox 规范
 *
 * 功能特性：
 * - 支持多种类型（info, warning, error, success, question）
 * - 可配置 1-3 个自定义按钮
 * - 支持自定义内容组件
 * - 集成 GTheme 主题系统
 */
Dialog {
    id: root

    // ========== 公开属性 ==========

    // 消息内容
    property string message: ""

    // 消息类型枚举
    enum MessageType {
        Info,
        Warning,
        Error,
        Success,
        Question
    }

    property int messageType: GMessageBox.Info

    // 自定义图标（优先级高于类型默认图标）
    property int customIcon: -1

    // 自定义内容组件（在 message 之后显示）
    property Component customContent: null

    // 按钮配置（按钮数组，每个按钮包含 text, buttonType, action）
    property var buttons: []

    // 默认按钮索引（用于高亮显示）
    property int defaultButtonIndex: -1

    // 对话框宽度
    property int dialogWidth: 420
    property int standardHeight: 300


    // ========== 内部属性 ==========

    readonly property int standardPadding: 20
    readonly property int standardSpacing: 16

    // ========== 信号 ==========

    // 按钮点击信号（返回按钮索引）
    signal buttonClicked(int index, var buttonData)

    // ========== Dialog 配置 ==========

    modal: true
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: dialogWidth
    height: standardHeight

    // ========== 辅助函数 ==========

    // 获取类型对应的图标
    function getTypeIcon() {
        if (customIcon !== -1) {
            return customIcon
        }

        switch(messageType) {
            case GMessageBox.Info:
                return SegoeFluentIcons.Info
            case GMessageBox.Warning:
                return SegoeFluentIcons.Warning
            case GMessageBox.Error:
                return SegoeFluentIcons.ErrorBadge
            case GMessageBox.Success:
                return SegoeFluentIcons.CheckmarkCircle
            case GMessageBox.Question:
                return SegoeFluentIcons.Help
            default:
                return SegoeFluentIcons.Info
        }
    }

    // 获取类型对应的颜色
    function getTypeColor() {
        switch(messageType) {
            case GMessageBox.Info:
                return GTheme.infoColor
            case GMessageBox.Warning:
                return GTheme.warningColor
            case GMessageBox.Error:
                return GTheme.dangerColor
            case GMessageBox.Success:
                return GTheme.successColor
            case GMessageBox.Question:
                return GTheme.primaryColor
            default:
                return GTheme.infoColor
        }
    }

    // ========== 背景样式 ==========

    background: Rectangle {
        color: GTheme.bgWhite
        radius: 8
        border.width: 1
        border.color: GTheme.borderLight

        layer.enabled: true
        layer.effect: DropShadow {
            radius: 16
            samples: 33
            color: Qt.rgba(0, 0, 0, 0.15)
            horizontalOffset: 0
            verticalOffset: 4
        }
    }

    // ========== 标题栏样式 ==========

    header: Rectangle {
        height: 50
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: root.standardPadding
            anchors.rightMargin: root.standardPadding
            spacing: 12

            // 图标
            FontIcon {
                iconSource: getTypeIcon()
                iconSize: 20
                color: getTypeColor()
                Layout.alignment: Qt.AlignVCenter
            }

            // 标题文本
            Label {
                text: root.title
                font.pixelSize: 16
                font.bold: true
                color: GTheme.textPrimary
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }
        }

        // 底部分隔线
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: GTheme.borderLighter
        }
    }

    // ========== 内容区域 ==========

    contentItem: Item {
        implicitWidth: contentColumn.implicitWidth + root.standardPadding * 2
        implicitHeight: contentColumn.implicitHeight + 20

        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.leftMargin: root.standardPadding
            anchors.rightMargin: root.standardPadding
            anchors.topMargin: 10
            anchors.bottomMargin: 10
            spacing: root.standardSpacing

            // 消息文本
            Label {
                text: root.message
                font.pixelSize: 14
                color: GTheme.textRegular
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                visible: root.message !== ""
            }

            // 自定义内容区域
            Loader {
                id: customContentLoader
                sourceComponent: root.customContent
                Layout.fillWidth: true
                visible: root.customContent !== null
            }

            // 按钮区域
            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Layout.topMargin: 5
                visible: root.buttons.length > 0

                Item {
                    Layout.fillWidth: true
                }

                // 动态创建按钮
                Repeater {
                    model: root.buttons

                    GButton {
                        required property int index
                        required property var modelData

                        text: modelData.text || ""
                        implicitWidth: modelData.width || 90
                        implicitHeight: 32

                        // 使用 GButton 的 buttonType 属性
                        buttonType: {
                            const type = modelData.type || "default"
                            // 映射类型
                            if (type === "default" && index === root.defaultButtonIndex) {
                                return "primary"
                            }
                            return type
                        }

                        onClicked: {
                            root.buttonClicked(index, modelData)
                            root.close()
                        }
                    }
                }
            }
        }
    }
}

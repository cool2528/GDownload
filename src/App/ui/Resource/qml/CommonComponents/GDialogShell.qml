import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import gdl.sdk

// Aurora 对话框外壳：统一 scrim、elevated surface、焦点、动效、表头和底栏。
// body 走默认内容槽(需 anchors.fill: parent),footer 走可选 Component 槽(自动在其上加分隔线)
Popup {
    id: shell

    // ===== 表头配置 =====
    property string title: ""
    property string subtitle: ""
    // Aurora 语义图标优先；图片和字体入口保留用于旧调用。
    property string iconName: ""
    property int iconSource: 0
    property url iconImage: ""
    property color iconBgColor: GTheme.bgInfo
    property color iconColor: GTheme.primaryColor
    property bool showClose: true
    // 对话框可指定语义上的默认操作；未指定时回退到关闭按钮。
    property Item initialFocusItem: null
    readonly property bool hasSemanticIcon: iconName.length > 0
    readonly property bool hasImageIcon: iconImage.toString().length > 0
    readonly property bool hasFontIcon: iconSource > 0
    // 表头/底栏高度:titleBarHeight(40) + space2XL(24) = 64
    property int barHeight: GTheme.titleBarHeight + GTheme.space2XL

    // ===== 内容槽 =====
    // body:默认内容槽,放入的根元素需 anchors.fill: parent
    default property alias content: bodyContainer.data
    // footer:可选底部 Component,非空时自动渲染顶部分隔线 + Loader
    property Component footer: null

    // ===== Popup 基础配置 =====
    modal: true
    closePolicy: Popup.CloseOnEscape
    visible: false
    padding: 0
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0

    Overlay.modal: Rectangle {
        color: GTheme.overlayScrim
    }

    // ===== 背景 + 阴影(取自 elevation4 令牌)=====
    background: Rectangle {
        color: GTheme.surfaceElevated
        radius: GTheme.radiusLarge
        border.width: 1
        border.color: GTheme.borderBase

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: GTheme.elevation4.color
            shadowBlur: Math.min(1.0, GTheme.elevation4.blur / 32)
            shadowHorizontalOffset: GTheme.elevation4.offsetX
            shadowVerticalOffset: GTheme.elevation4.offsetY
            autoPaddingEnabled: true
        }
    }

    // ===== 进出动效(统一令牌)=====
    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "scale"
                from: 0.9
                to: 1.0
                duration: GTheme.durationBase
                easing.type: GTheme.easingStandard
            }
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: GTheme.durationBase
                easing.type: GTheme.easingStandard
            }
        }
    }
    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.9
                duration: GTheme.durationFast
                easing.type: GTheme.easingStandard
            }
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: GTheme.durationFast
                easing.type: GTheme.easingStandard
            }
        }
    }

    contentItem: ColumnLayout {
        Accessible.role: Accessible.Dialog
        Accessible.name: shell.title
        Accessible.description: shell.subtitle
        spacing: 0

        // ===== 表头 =====
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: shell.barHeight

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: GTheme.space2XL
                anchors.rightMargin: GTheme.space2XL
                spacing: GTheme.spaceLG

                // 图标盒(着色背景)
                Rectangle {
                    Layout.preferredWidth: GTheme.sizeLarge
                    Layout.preferredHeight: GTheme.sizeLarge
                    Layout.alignment: Qt.AlignVCenter
                    radius: GTheme.radiusBase * 2
                    color: shell.iconBgColor
                    visible: shell.hasSemanticIcon || shell.hasImageIcon || shell.hasFontIcon

                    AuroraIcon {
                        anchors.centerIn: parent
                        visible: shell.hasSemanticIcon
                        name: shell.hasSemanticIcon ? shell.iconName : "info"
                        iconSize: GTheme.fontTitle
                        color: shell.iconColor
                    }

                    FontIcon {
                        anchors.centerIn: parent
                        visible: !shell.hasSemanticIcon && !shell.hasImageIcon && shell.hasFontIcon
                        iconSource: shell.iconSource
                        iconSize: GTheme.fontTitle
                        color: shell.iconColor
                    }
                    Image {
                        anchors.centerIn: parent
                        visible: !shell.hasSemanticIcon && shell.hasImageIcon
                        source: shell.iconImage
                        sourceSize: Qt.size(GTheme.space2XL, GTheme.space2XL)
                    }
                }

                // 标题 + 副标题
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: GTheme.spaceXS

                    Text {
                        text: shell.title
                        textFormat: Text.PlainText
                        font.pixelSize: GTheme.fontTitle
                        font.weight: GTheme.weightDemiBold
                        color: GTheme.textPrimary
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Text {
                        text: shell.subtitle
                        textFormat: Text.PlainText
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textSecondary
                        visible: shell.subtitle.length > 0
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }

                // 关闭按钮(GButton icon-only)
                GButton {
                    id: closeButton
                    visible: shell.showClose
                    iconName: "close"
                    iconColor: GTheme.textSecondary
                    implicitWidth: GTheme.sizeSmall + GTheme.spaceXS
                    implicitHeight: GTheme.sizeSmall + GTheme.spaceXS
                    Layout.alignment: Qt.AlignVCenter
                    Accessible.name: qsTr("Close dialog")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Close")
                    onClicked: shell.close()
                    Keys.onReturnPressed: event => {
                        shell.close()
                        event.accepted = true
                    }
                    Keys.onEnterPressed: event => {
                        shell.close()
                        event.accepted = true
                    }
                }
            }
        }

        // 表头分隔线
        Divider {
            Layout.fillWidth: true
        }

        // ===== body(默认内容槽)=====
        Item {
            id: bodyContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // ===== 可选 footer =====
        Divider {
            Layout.fillWidth: true
            visible: shell.footer !== null
        }
        Loader {
            Layout.fillWidth: true
            active: shell.footer !== null
            sourceComponent: shell.footer
        }
    }

    onOpened: Qt.callLater(function() {
        if (shell.initialFocusItem
                && shell.initialFocusItem.visible
                && shell.initialFocusItem.enabled) {
            shell.initialFocusItem.forceActiveFocus()
        } else if (shell.showClose) {
            closeButton.forceActiveFocus()
        }
    })
}

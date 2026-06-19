import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import gdl.sdk

// 对话框外壳:统一 弹层背景 + 阴影(elevation4)+ 进出动效 + 表头(图标/标题/副标题/关闭)+ 分隔线
// body 走默认内容槽(需 anchors.fill: parent),footer 走可选 Component 槽(自动在其上加分隔线)
Popup {
    id: shell

    // ===== 表头配置 =====
    property string title: ""
    property string subtitle: ""
    // 字体图标(>0 生效)与图片图标二选一
    property int iconSource: 0
    property url iconImage: ""
    property color iconBgColor: GTheme.fillLight
    property color iconColor: GTheme.primaryColor
    property bool showClose: true
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

    // ===== 背景 + 阴影(取自 elevation4 令牌)=====
    background: Rectangle {
        color: GTheme.bgWhite
        radius: GTheme.radiusBase * 2
        border.width: 1
        border.color: GTheme.borderLight

        layer.enabled: true
        layer.effect: DropShadow {
            radius: GTheme.elevation4.blur
            samples: GTheme.elevation4.blur * 2 + 1
            color: GTheme.elevation4.color
            horizontalOffset: GTheme.elevation4.offsetX
            verticalOffset: GTheme.elevation4.offsetY
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
                    visible: shell.iconSource > 0 || String(shell.iconImage).length > 0

                    FontIcon {
                        anchors.centerIn: parent
                        visible: shell.iconSource > 0 && String(shell.iconImage).length === 0
                        iconSource: shell.iconSource
                        iconSize: GTheme.fontTitle
                        color: shell.iconColor
                    }
                    Image {
                        anchors.centerIn: parent
                        visible: String(shell.iconImage).length > 0
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
                        font.pixelSize: GTheme.fontTitle
                        font.weight: GTheme.weightDemiBold
                        color: GTheme.textPrimary
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Text {
                        text: shell.subtitle
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textSecondary
                        visible: shell.subtitle.length > 0
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }

                // 关闭按钮(GButton icon-only)
                GButton {
                    visible: shell.showClose
                    iconSource: SegoeFluentIcons.ChromeClose
                    iconSize: GTheme.fontBody
                    implicitWidth: GTheme.sizeSmall + GTheme.spaceXS
                    implicitHeight: GTheme.sizeSmall + GTheme.spaceXS
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: shell.close()
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
}

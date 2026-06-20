import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk 1.0

/**
 * AlertTip - 通用告警/提示条
 * 替换设置页内裸 Rectangle 告警框,统一告警语义配色:背景/边框/文字取 GTheme 告警令牌
 * (bgXxx/borderXxx/textXxx),修复阶段①前的幽灵令牌 bug。风格参照 GMessage:前导图标 + 文本。
 * 颜色/尺寸/间距/字号/动效一律取自 GTheme 令牌,零魔法数字。
 *
 * 用法:
 *   AlertTip {
 *       severity: "info"  // info | success | warning | danger
 *       text: qsTr("...")
 *   }
 */
Control {
    id: root

    // ========== 公开属性 ==========

    // 告警级别:info | success | warning | danger(未知值容错回退为 info)
    property string severity: "info"

    // 提示文本(调用方用 qsTr 包裹英文)
    property string text: ""

    // 是否显示前导图标(默认按 severity 自动选取)
    property bool showIcon: true

    // 自定义图标字形(>0 优先于 severity 默认图标,与 GButton 的 iconSource 约定一致)
    property int iconSource: 0

    // ========== 派生:按级别解析令牌 ==========

    // 归一化级别(小写化 + 容错回退)
    readonly property string resolvedSeverity: {
        const s = severity.toLowerCase()
        if (s === "success" || s === "warning" || s === "danger")
            return s
        return "info"
    }

    // 背景色(取 Section 1 告警令牌)
    readonly property color alertBg: {
        switch (resolvedSeverity) {
        case "success": return GTheme.bgSuccess
        case "warning": return GTheme.bgWarning
        case "danger": return GTheme.bgDanger
        default: return GTheme.bgInfo
        }
    }

    // 边框色(取 Section 1 告警令牌)
    readonly property color alertBorder: {
        switch (resolvedSeverity) {
        case "success": return GTheme.borderSuccess
        case "warning": return GTheme.borderWarning
        case "danger": return GTheme.borderDanger
        default: return GTheme.borderInfo
        }
    }

    // 文字色(取 Section 1 告警令牌,贴合原裸 Rectangle 告警框的 textInfo/textWarning 配色)
    readonly property color alertText: {
        switch (resolvedSeverity) {
        case "success": return GTheme.textSuccess
        case "warning": return GTheme.textWarning
        case "danger": return GTheme.textDanger
        default: return GTheme.textInfo
        }
    }

    // 图标色(取主状态色,更鲜亮,参照 GMessage 的图标配色)
    readonly property color iconTint: {
        switch (resolvedSeverity) {
        case "success": return GTheme.successColor
        case "warning": return GTheme.warningColor
        case "danger": return GTheme.dangerColor
        default: return GTheme.infoColor
        }
    }

    // 默认图标字形(按 severity 自动选取)
    readonly property int defaultIcon: {
        switch (resolvedSeverity) {
        case "success": return SegoeFluentIcons.Completed
        case "warning": return SegoeFluentIcons.Warning
        case "danger": return SegoeFluentIcons.Error
        default: return SegoeFluentIcons.Info
        }
    }

    // ========== 外观 ==========

    // 内边距取令牌(spec Section 2.4 约定 spaceMD)
    padding: GTheme.spaceMD

    background: Rectangle {
        radius: GTheme.radiusBase
        color: root.alertBg
        border.width: 1
        border.color: root.alertBorder

        // 主题切换 / 级别变更时背景、边框平滑过渡
        Behavior on color {
            ColorAnimation {
                duration: GTheme.durationBase
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: GTheme.durationBase
            }
        }
    }

    contentItem: RowLayout {
        spacing: GTheme.spaceSM

        FontIcon {
            visible: root.showIcon
            iconSource: root.iconSource > 0 ? root.iconSource : root.defaultIcon
            iconSize: GTheme.fontBody
            color: root.iconTint
            Layout.alignment: Qt.AlignTop
        }

        Text {
            text: root.text
            color: root.alertText
            font.pixelSize: GTheme.fontCaption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            verticalAlignment: Text.AlignVCenter

            Behavior on color {
                ColorAnimation {
                    duration: GTheme.durationBase
                }
            }
        }
    }
}

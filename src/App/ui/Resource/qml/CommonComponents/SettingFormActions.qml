import QtQuick
import QtQuick.Layouts
import gdl.sdk

// 设置表单操作区:统一 Reset + Save 按钮 + 状态提示文本
// 服务暂存-保存派设置页(本地暂存,Save 时比对旧值并批量提交)
// 颜色/尺寸/间距/字号/动效一律取自 GTheme 令牌;buttonWidth 为页面级布局常量(非设计令牌)
ColumnLayout {
    id: root

    // ========== 公开属性 ==========

    // 是否有未保存的改动,控制 Save 按钮启用状态
    property bool hasChanges: false

    // 状态提示文案(由 applySettings / reset 处理器写入,勿外部绑定)
    property string statusText: ""

    // 状态提示颜色(成功/错误/中性随状态切换)
    property color statusColor: GTheme.textSecondary

    // 默认提示文案(statusText 为空时展示)
    property string defaultStatusText: qsTr("Modify the values above and click 'Save Settings' to apply.")

    // 按钮宽度(页面级布局常量,非设计令牌,可由调用方覆盖)
    property int buttonWidth: 120

    // ========== 信号 ==========

    // Reset 按钮点击(由调用方在 onReset 中重置输入控件并更新 statusText)
    signal reset()

    // Save 按钮点击(由调用方在 onSave 中校验后调用 applySettings)
    signal save()

    // ========== 帮助函数 ==========

    // 批量应用暂存的改动:遍历 changes,值有变才调 setter 并累加摘要
    // changes: [{ val, old, setter, label }, ...]
    //   val    新值
    //   old    旧值(与 val 比对,不同才提交)
    //   setter 提交函数,签名 setter(newValue)
    //   label  摘要标签(用于拼接状态文案)
    // successMessage: 全部应用成功后的 Toast 文案
    // 返回: 用于状态提示的新 statusText 文案
    function applySettings(changes, successMessage) {
        var msg = (successMessage && successMessage.length > 0)
                  ? successMessage
                  : qsTr("Settings saved")
        var applied = []
        for (var i = 0; changes && i < changes.length; i++) {
            var c = changes[i]
            if (c.val !== c.old) {
                c.setter(c.val)
                applied.push(c.label + "=" + c.val)
            }
        }
        if (applied.length > 0) {
            ToastManager.ShowSuccess(msg, 3000)
            var summary = msg + ": " + applied.join(", ")
            root.statusText = summary
            root.statusColor = GTheme.successColor
            return summary
        }
        ToastManager.ShowInfo(qsTr("No changes detected."), 2000)
        root.statusText = qsTr("Settings unchanged.")
        root.statusColor = GTheme.textSecondary
        return root.statusText
    }

    // ========== 布局 ==========

    Layout.fillWidth: true
    spacing: GTheme.spaceSM

    // 按钮行:弹性占位把按钮推到右侧
    RowLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Item {
            Layout.fillWidth: true
        }

        // Reset:次要按钮(type:3 = default 样式),始终可用
        GButton {
            text: qsTr("Reset")
            type: 3
            Layout.preferredWidth: root.buttonWidth
            onClicked: root.reset()
        }

        // Save:主要按钮(type:1 = primary 样式),仅在有改动时启用
        GButton {
            text: qsTr("Save Settings")
            type: 1
            enabled: root.hasChanges
            Layout.preferredWidth: root.buttonWidth
            onClicked: root.save()
        }
    }

    // 状态提示文本
    Text {
        Layout.fillWidth: true
        text: root.statusText.length > 0 ? root.statusText : root.defaultStatusText
        font.pixelSize: GTheme.fontCaption
        color: root.statusColor
        wrapMode: Text.WordWrap

        Behavior on color {
            ColorAnimation {
                duration: GTheme.durationBase
            }
        }
    }
}

import QtQuick
import "qrc:/qml/CommonComponents"
import gdl.sdk

// GMessageBox 多类型视觉用例包装器(Task 8)
//
// 目标:在同一张截图中渲染 GMessageBox 的 4 种 messageType(Info / Success /
// Warning / Error),供 tst_dialogs.qml 经 PageHarness.Loader 加载后由
// Screenshot.captureWindow(harness, ...) 抓取窗口 contentItem(含 Overlay 层)。
//
// 实现要点:
// 1. GMessageBox 继承 Dialog(Popup 子类),其内容渲染在窗口 Overlay 上,
//    而非作为 harness 的可视子项;故必须用 captureWindow 抓 contentItem。
// 2. Popup.x / Popup.y 相对其 parent item(GMessageBoxMulti 根 Item,由 harness
//    Loader 填充)。4 个实例按列横向排开,各占 dialogWidth + spacing 宽度。
// 3. messageType 枚举:GMessageBox.Info=0 / Warning=1 / Error=2 / Success=3 /
//    Question=4(对应 Element Plus info/warning/danger/success 语义)。
// 4. open() 触发 Popup 显示;enter 动画 durationBase=150ms,调用方需等待 ~250ms。
Item {
    id: root

    // 4 种消息类型横向布局参数
    readonly property int boxWidth: 420
    readonly property int boxHeight: 280
    readonly property int boxSpacing: 24
    readonly property int boxY: 32

    Component.onCompleted: {
        var configs = [
            { type: GMessageBox.Info,    title: qsTr("Info"),    msg: qsTr("This is an informational message.") },
            { type: GMessageBox.Success, title: qsTr("Success"), msg: qsTr("Operation completed successfully.") },
            { type: GMessageBox.Warning, title: qsTr("Warning"), msg: qsTr("This action may have consequences.") },
            { type: GMessageBox.Error,   title: qsTr("Error"),   msg: qsTr("An error occurred during the operation.") }
        ]
        for (var i = 0; i < configs.length; ++i) {
            var box = msgBoxComponent.createObject(root)
            box.messageType = configs[i].type
            box.title = configs[i].title
            box.message = configs[i].msg
            // 覆盖 GMessageBox 默认居中绑定,改为横向排开
            box.x = i * (root.boxWidth + root.boxSpacing)
            box.y = root.boxY
            box.open()
        }
    }

    Component {
        id: msgBoxComponent
        GMessageBox {
            dialogWidth: root.boxWidth
            standardHeight: root.boxHeight
            buttons: [
                { text: qsTr("OK"), type: "primary", width: 90 }
            ]
            defaultButtonIndex: 0
        }
    }
}

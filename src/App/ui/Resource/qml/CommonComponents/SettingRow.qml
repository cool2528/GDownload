import QtQuick
import QtQuick.Layouts
import gdl.sdk

// 设置项行:统一 label + control(内容槽)+ hint 三段式布局
// 替代设置页中重复的 RowLayout{ Text(label) + Control + Text(hint) } 模板(spec 2.2)
// 颜色/尺寸/间距/字号一律取自 GTheme 令牌;labelWidth 为本地布局常量
// (spec 2.2 约定:GTheme 暂无 controlLabelWidth 令牌,页面可按需覆盖)
Item {
    id: root

    // 标签文本(行左侧)
    property string label: ""

    // 提示文本(控件右侧,如单位 "KB/s")
    property string hint: ""

    // 标签宽度:本地布局常量,默认 60,页面可按需覆盖
    property int labelWidth: 60

    // 控件内容槽:调用方以 control: <Control> {} 注入
    // 运行时重 parenting 到中间布局槽,以保留注入控件自身的 Layout.* 尺寸
    // 并保证三段顺序为 label / control / hint
    property Item control: null

    // 行隐式尺寸跟随内部布局,便于在 ColumnLayout 中按内容撑开
    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    RowLayout {
        id: row
        anchors.fill: parent
        spacing: GTheme.spaceMD

        // 标签:fontBody + textRegular
        Text {
            text: root.label
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightRegular
            color: GTheme.textRegular
            Layout.preferredWidth: root.labelWidth
            Layout.alignment: Qt.AlignVCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        // 控件槽:作为布局容器使注入控件的 Layout.* 尺寸生效,高度基线 sizeDefault
        RowLayout {
            id: controlSlot
            Layout.preferredHeight: GTheme.sizeDefault
            Layout.alignment: Qt.AlignVCenter
        }

        // 提示:fontCaption + textPlaceholder
        Text {
            text: root.hint
            font.pixelSize: GTheme.fontCaption
            color: GTheme.textPlaceholder
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
        }
    }

    // 控件注入后重 parenting 到中间槽,保留其 Layout.* 尺寸与三段顺序
    onControlChanged: {
        if (root.control) {
            root.control.parent = controlSlot
        }
    }
}

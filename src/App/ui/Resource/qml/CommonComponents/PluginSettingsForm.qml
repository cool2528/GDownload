import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// 按插件 manifest settings schema 动态渲染的通用配置表单(设计文档第 6 节)
// schema 项字段: key/type/label/hint/required/role/options/defaultValue(已由 PluginConfigManager 本地化)
ColumnLayout {
    id: form

    property string pluginName: ""
    property var schemaFields: []
    property var currentValues: ({})
    property bool hasMissingRequired: false

    spacing: GTheme.spaceMD

    function reload() {
        schemaFields = PluginConfigManager.schema(pluginName)
        currentValues = PluginConfigManager.values(pluginName)
        hasMissingRequired = false
    }

    // 收集编辑值; required 未填时置 hasMissingRequired 并返回 null
    function collect() {
        var out = {}
        var missing = false
        for (var i = 0; i < fieldsRepeater.count; ++i) {
            var row = fieldsRepeater.itemAt(i)
            if (!row)
                continue
            out[row.fieldKey] = row.fieldValue()
            if (row.isMissing())
                missing = true
        }
        hasMissingRequired = missing
        return missing ? null : out
    }

    Repeater {
        id: fieldsRepeater
        model: form.schemaFields

        delegate: ColumnLayout {
            id: fieldRow
            required property var modelData
            readonly property string fieldKey: modelData.key
            readonly property string fieldType: modelData.type
            readonly property var helpSteps: modelData.helpSteps || []
            readonly property string helpUrl: modelData.helpUrl || ""
            readonly property bool hasHelp: helpSteps.length > 0 || helpUrl !== ""
            // 必填且尚未填值时默认展开引导,首次配置直接可见步骤
            property bool helpExpanded: hasHelp && modelData.required === true
                                        && String(initialValue()) === ""

            Layout.fillWidth: true
            spacing: GTheme.spaceXS

            function initialValue() {
                if (form.currentValues[fieldKey] !== undefined)
                    return form.currentValues[fieldKey]
                if (modelData.defaultValue !== undefined && modelData.defaultValue !== null)
                    return modelData.defaultValue
                if (fieldType === "bool")
                    return false
                if (fieldType === "number")
                    return 0
                return ""
            }

            function fieldValue() {
                return editorLoader.item ? editorLoader.item.editorValue : initialValue()
            }

            // required 且文本为空视为未填(bool/number 恒有值)
            function isMissing() {
                if (modelData.required !== true)
                    return false
                var v = fieldValue()
                return v === undefined || v === null || v === ""
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceXS
                visible: fieldRow.fieldType !== "bool"
                Text {
                    text: fieldRow.modelData.required === true
                          ? fieldRow.modelData.label + " *"
                          : fieldRow.modelData.label
                    textFormat: Text.PlainText
                    color: GTheme.textPrimary
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightMedium
                }
                AuroraIcon {
                    visible: fieldRow.hasHelp
                    name: "help"
                    iconSize: GTheme.fontBody
                    color: helpToggleArea.containsMouse || fieldRow.helpExpanded
                           ? GTheme.primaryColor : GTheme.textSecondary
                    Accessible.name: qsTr("How to get this value?")

                    MouseArea {
                        id: helpToggleArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: fieldRow.helpExpanded = !fieldRow.helpExpanded
                    }
                }
                Item { Layout.fillWidth: true }
            }

            Loader {
                id: editorLoader
                Layout.fillWidth: true
                sourceComponent: {
                    if (fieldRow.fieldType === "bool")
                        return boolEditor
                    if (fieldRow.fieldType === "select")
                        return selectEditor
                    if (fieldRow.fieldType === "number")
                        return numberEditor
                    if (fieldRow.fieldType === "textarea")
                        return textareaEditor
                    return textEditor
                }
            }

            // 分步骤获取引导面板(manifest settings[].help 声明,可折叠)
            Rectangle {
                visible: fieldRow.helpExpanded && fieldRow.hasHelp
                Layout.fillWidth: true
                implicitHeight: helpColumn.implicitHeight + GTheme.spaceMD * 2
                color: GTheme.fillLighter
                radius: GTheme.radiusMedium

                ColumnLayout {
                    id: helpColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: GTheme.spaceMD
                    spacing: GTheme.spaceXS

                    Text {
                        text: qsTr("How to get this value?")
                        textFormat: Text.PlainText
                        color: GTheme.textPrimary
                        font.pixelSize: GTheme.fontBody
                        font.weight: GTheme.weightMedium
                    }

                    Repeater {
                        model: fieldRow.helpSteps
                        delegate: Text {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            text: (index + 1) + ". " + modelData
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            color: GTheme.textSecondary
                            font.pixelSize: GTheme.fontBody
                        }
                    }

                    GButton {
                        visible: fieldRow.helpUrl !== ""
                        text: qsTr("View full tutorial")
                        onClicked: Qt.openUrlExternally(fieldRow.helpUrl)
                    }
                }
            }

            Component {
                id: textEditor
                GTextField {
                    readonly property var editorValue: text
                    text: String(fieldRow.initialValue())
                    echoMode: fieldRow.fieldType === "password" ? TextInput.Password : TextInput.Normal
                    placeholderText: fieldRow.modelData.hint || ""
                    Accessible.name: fieldRow.modelData.label
                }
            }

            Component {
                id: textareaEditor
                TextArea {
                    readonly property var editorValue: text
                    text: String(fieldRow.initialValue())
                    wrapMode: TextEdit.WrapAnywhere
                    selectByMouse: true
                    implicitHeight: 96
                    font.pixelSize: GTheme.fontBody
                    color: GTheme.textPrimary
                    placeholderText: fieldRow.modelData.hint || ""
                    placeholderTextColor: GTheme.textPlaceholder
                    Accessible.name: fieldRow.modelData.label
                    background: Rectangle {
                        color: GTheme.fillLighter
                        radius: GTheme.radiusMedium
                        border.width: 1
                        border.color: parent.activeFocus ? GTheme.primaryColor : GTheme.borderLight
                    }
                }
            }

            Component {
                id: boolEditor
                RowLayout {
                    readonly property var editorValue: switchControl.checked
                    spacing: GTheme.spaceSM
                    GButtonSwitch {
                        id: switchControl
                        checked: fieldRow.initialValue() === true
                        Accessible.name: fieldRow.modelData.label
                    }
                    Text {
                        text: fieldRow.modelData.label
                        textFormat: Text.PlainText
                        color: GTheme.textPrimary
                        font.pixelSize: GTheme.fontBody
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }
                }
            }

            Component {
                id: selectEditor
                GComBoBox {
                    readonly property var options: fieldRow.modelData.options || []
                    readonly property var editorValue: currentIndex >= 0 && currentIndex < options.length
                                                       ? options[currentIndex] : ""
                    model: options
                    implicitHeight: GTheme.sizeDefault
                    Accessible.name: fieldRow.modelData.label
                    Component.onCompleted: {
                        var idx = options.indexOf(String(fieldRow.initialValue()))
                        currentIndex = idx >= 0 ? idx : 0
                    }
                }
            }

            Component {
                id: numberEditor
                GSpinBox {
                    readonly property var editorValue: value
                    // 业务值:通用数字字段的宽松边界(具体插件语义由 schema 决定)
                    from: -999999999
                    to: 999999999
                    Accessible.name: fieldRow.modelData.label
                    Component.onCompleted: value = Number(fieldRow.initialValue()) || 0
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// 网盘插件验证输入对话框:提取码/验证码统一入口
// 由 VerificationBridge.verificationRequested 触发,提交/取消回写桥接层
GDialogShell {
    id: dialog

    property string message: ""
    property string imageBase64: ""
    property bool submitted: false

    readonly property real outerMargin: GTheme.spaceLG

    width: Math.min(420, parent ? Math.max(0, parent.width - outerMargin * 2) : 420)
    height: Math.min(imageBase64.length > 0 ? 420 : 300,
                     parent ? Math.max(0, parent.height - outerMargin * 2) : 420)

    title: qsTr("Verification required")
    subtitle: qsTr("The cloud share needs extra input to continue")
    iconName: "lock"
    closePolicy: Popup.CloseOnEscape
    initialFocusItem: codeInput

    function openFor(msg, image) {
        message = msg || ""
        imageBase64 = image || ""
        submitted = false
        codeInput.text = ""
        open()
    }

    function submit() {
        var text = codeInput.text.trim()
        if (text.length === 0)
            return
        submitted = true
        VerificationBridge.Submit(text)
        close()
    }

    onClosed: {
        if (!submitted)
            VerificationBridge.Cancel()
    }

    Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: GTheme.spaceLG
            spacing: GTheme.spaceMD

            Text {
                text: dialog.message.length > 0 ? dialog.message
                                                : qsTr("This share link requires an extraction code.")
                // 该文本来源于网盘服务端返回的验证提示，禁用富文本解析以防内容伪装
                textFormat: Text.PlainText
                color: GTheme.textPrimary
                font.pixelSize: GTheme.fontBody
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Image {
                visible: dialog.imageBase64.length > 0
                source: dialog.imageBase64.length > 0 ? "data:image;base64," + dialog.imageBase64 : ""
                fillMode: Image.PreserveAspectFit
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 96 : 0
            }

            GTextField {
                id: codeInput
                objectName: "verificationCodeInput"
                Layout.fillWidth: true
                placeholderText: qsTr("Enter the code here")
                Accessible.name: qsTr("Verification code")
                onAccepted: dialog.submit()
            }

            Item { Layout.fillHeight: true }
        }
    }

    footer: Item {
        implicitHeight: footerRow.implicitHeight + GTheme.spaceMD * 2

        RowLayout {
            id: footerRow
            anchors.fill: parent
            anchors.leftMargin: GTheme.spaceLG
            anchors.rightMargin: GTheme.spaceLG
            anchors.topMargin: GTheme.spaceMD
            anchors.bottomMargin: GTheme.spaceMD
            spacing: GTheme.spaceSM

            Item { Layout.fillWidth: true }

            GButton {
                text: qsTr("Cancel")
                onClicked: dialog.close()
            }

            GButton {
                type: 1
                text: qsTr("Confirm")
                enabled: codeInput.text.trim().length > 0
                onClicked: dialog.submit()
            }
        }
    }
}
